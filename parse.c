/* parse.c - grammar definition parser
 *
 * Copyright (C) 2005 Momchil Velikov
 *
 * This file is part of XG.
 *
 * XG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XG; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "symtab.h"
#include "grammar.h"

/* Parser data.  */
struct parse_ctx
{
  /* Input file name.  */
  const char *name;

  /* Input file stream.  */
  FILE *in;

  /* Current line number.  */
  unsigned int lineno;

  /* Current token.  */
  int token;

  /* Current token semantic value.  */
  union
  {
    int chr;
    char *word;
  } value;

  /* Main symbol table. */
  xg_symtab symtab;
};
typedef struct parse_ctx parse_ctx;

/* Error reporting.  */
static void
error (parse_ctx *ctx, const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s:%d: ERROR: ", ctx->name, ctx->lineno);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
}

/* Out of memory error.  */
static void
error_oom ()
{
  fprintf (stderr, "xg: Out of memory\n");
}

/* Token encoding.  */
#define TOKEN_WORD 257
#define TOKEN_LITERAL 258

/* Lexical analyzer.  */
static int
scan_escape (parse_ctx *ctx)
{
  int ch;

  ch = getc (ctx->in);
  switch (ch)
    {
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case '\\':
      return '\\';

    default:
      error (ctx, "Invalid escape sequence");
      return -1;
    }
}

/* Scan a token literal: '<char>'.  */
static int
scan_token_literal (parse_ctx *ctx)
{
  int ch, ch1;

  ch = getc (ctx->in);
  if (ch == EOF)
    {
      error (ctx, "Invalid token literal");
      return -1;
    }

  if (ch == '\\' && (ch = scan_escape (ctx)) == -1)
    return -1;

  ch1 = getc (ctx->in);
  if (ch1 != '\'')
    {
      error (ctx, "Invalid token literal");
      return -1;
    }

  ctx->value.chr = ch;
  ctx->token = TOKEN_LITERAL;
  return 0;
}

/* Scan a word.  */
static void
scan_word (parse_ctx *ctx, int ch)
{
  char *word;
  unsigned int cnt, n;

  cnt = n = 0;
  word = 0;
  while (ch != EOF && !isspace (ch))
    {
      if (cnt == n)
        {
          n += 10;
          word = realloc (word, n);
        }
      word [cnt++] = ch;
      ch = getc (ctx->in);
    }
  ungetc (ch, ctx->in);

  ctx->value.word = word;
  ctx->token = TOKEN_WORD;
}

static int
getlex (parse_ctx *ctx)
{
  int ch;

  /* Skip whitespace.  */
  do
    {
      ch = getc (ctx->in);
      if (ch == '\n')
        ++ctx->lineno;
    }
  while (isspace (ch));

  /* Check for single-character tokens and EOF.  */
  switch (ch)
    {
    case EOF:
      ctx->token = 0;
      return 0;

    case ':':
    case '|':
    case ';':
      ctx->token = ch;
      return 0;
    }

  if (ch == '\'')
    return scan_token_literal (ctx);
  else if (isalpha (ch))
    {
      scan_word (ctx, ch);
      return 0;
    }
  else
    {
      error (ctx, "Invalid token");
      return -1;
    }
}

/* Parser routines.  */

/* XG grammar definition grammar.

   gram: prod | gram prod

   prod: word ':' rhs ';'

   rhs: symbol-list
      | rhs '|' symbol-list

   symbol-list: symbol | symbol-list symbol

   symbol: word | token-literal
*/

static int
parse_symbol_list (parse_ctx *ctx, xg_production *prod)
{
  while (ctx->token == TOKEN_WORD || ctx->token == TOKEN_LITERAL)
    {
      if (getlex (ctx) < 0)
        return -1;
    }

  return 0;
}

static int
parse_rhs (parse_ctx *ctx, const char *lhs __attribute__ ((unused)))
{
  if (parse_symbol_list (ctx) < 0)
    return -1;

  while (ctx->token == '|')
    {
      if (getlex (ctx) < 0 || parse_symbol_list (ctx) < 0)
        return -1;
    }

  return 0;
}

static int
parse_prod (parse_ctx *ctx)
{
  char *lhs;
  xg_production *prod;
  xg_symbol_def *lhs;

  /* Match the left hand side.  */
  if (ctx->token != TOKEN_WORD)
    {
      error (ctx, "Invalid production definition -- expected WORD");
      return -1;
    }

  /* Look if the symbol was already defined.  */
  lhs = xg_symtab_lookup (&ctx->symtab, ctx->value.word);
  if (lhs == 0)
    {
      lhs = xg_symbol_new (ctx->value.word);
      if (lhs == 0)
        {
          free (ctx->value.word);
          return -1;
        }
    }
  lhs->terminal = 0;

  if (getlex (ctx) < 0 || ctx->token != ':')
    {
      error (ctx, "Invalid production definition -- expected : (colon)");
      goto error;
    }

  if (getlex (ctx) < 0 || parse_rhs (ctx, lhs) < 0)
    goto error;

  if (ctx->token != ';')
    {
      error (ctx, "Invalid production definition -- expected ; (semicolon)");
      goto error;
    }

  if (getlex (ctx) < 0)
    goto error;

  free (lhs);
  return 0;

error:
  free (lhs);
  return -1;
}

static int
parse_gram (parse_ctx *ctx)
{
  if (getlex  (ctx) < 0)
    return -1;

  while (ctx->token > 0)
    {
      if (parse_prod (ctx) < 0)
        return -1;
    }

  return (ctx->token == 0) ? 0 : -1;
}

xg_grammar *
xg_grammar_read (const char *name)
{
  int sts;
  parse_ctx ctx;
  xg_grammar *gram;

  /* Initialize the parser context.  */
  ctx.name = name;
  ctx.lineno = 1;
  ctx.token = 0;

  /* Open the input file stream.  */
  if ((ctx.in = fopen (name, "r")) != 0)
    {
      /* Create the symbol table.  */
      if (xg_symtab_init (&ctx.symtab) == 0)
        {
          /* Create the grammar object.  */
          if ((ctx.gram = xg_grammar_new ()) != 0)
            {
              /* Reserve the first production for the standard S'->S
                 augmentation.  */
              if (xg_grammar_add_production (&ctx.gram, 0) == 0)
                {
                  /* Parse the grammar description.  */
                  sts = parse_gram (&ctx);
                }
              else
                {
                  error_oom ();
                  xg_grammar_del (&ctx.gram);
                }
            }
          else
            error_oom ();
          xg_symtab_destroy (&ctx.symtab);
        }
      else
        error_oom ();
      fclose (ctx.in);
    }
  else
    {
      fprintf (stderr, "xg : Cannot open input file ``%s''\n", name);
      return 0;
    }

  if (sts == 0)
    return ctx.gram;
  else
    {
      xg_grammar_del (ctx.gram);
      return 0;
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 6cce817f-a739-48dc-8ea8-99bcb3484f81
 * End:
 */

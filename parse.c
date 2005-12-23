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
#include "xg.h"
#include <ulib/cache.h>

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

  /* The build-in-progress grammar. */
  xg_grammar *gram;
};
typedef struct parse_ctx parse_ctx;

/* Error reporting.  */
#define errorv(ctx, fmt, ...)                           \
  ulib_log_printf (xg_log, "%s:%d: ERROR: " fmt,        \
                   ctx->name, ctx->lineno, __VA_ARGS__)

#define error(ctx, fmt, ...)                                            \
  ulib_log_printf (xg_log, "%s:%d: ERROR: " fmt, ctx->name, ctx->lineno)

/* Token encoding.  */
#define TOKEN_WORD 257
#define TOKEN_LITERAL 258
#define TOKEN_START  259

/* Lexical analyzer.  */
static int
skip_comment (parse_ctx *ctx)
{
  int ch;

  while ((ch = getc (ctx->in)) != EOF)
    {
      if (ch == '\n')
        ++ctx->lineno;
      else if (ch == '*')
        {
          if ((ch = getc (ctx->in)) == EOF)
            break;
          
          if (ch == '/')
            /* End of the comment found.  */
            return 0;
        }
    }

  error (ctx, "End of file within a comment");
  return -1;
}

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
      if (cnt + 1 >= n)
        {
          n += 10;
          word = xg_realloc (word, n);
        }
      word [cnt++] = ch;
      ch = getc (ctx->in);
    }
  word [cnt++] = '\0';
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
      else if (ch == '/')
        {
          ch = getc (ctx->in);
          if (ch == '*')
            {
              if (skip_comment (ctx) < 0)
                return -1;
              ch = ' ';
            }
          else
            {
              ungetc (ch, ctx->in);;
              ch = '/';
            }
        }
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
  else if (isalpha (ch) || ch == '%')
    {
      scan_word (ctx, ch);
      if (strcmp (ctx->value.word, "%start") == 0)
        {
          free (ctx->value.word);
          ctx->token = TOKEN_START;
        }
      return 0;
    }
  else
    {
      error (ctx, "Invalid token ``%c''", ch);
      return -1;
    }
}

/* Parser routines.  */

/* XG grammar definition grammar.

   gram: decls

   decls: decl | decls decl

   decl: directive | prod

   directive: '%start' word ';'

   prod: word ':' rhs ';'

   rhs: symbol-list
      | rhs '|' symbol-list

   symbol-list: symbol | symbol-list symbol

   symbol: word | token-literal
*/

/* Find or create a symbol with name NAME.  In either case the
   function consumes the NAME parameter.  */
static xg_symbol_def *
find_or_create_symbol (parse_ctx *ctx, char *name)
{
  xg_symbol_def *def;

  if ((def = xg_symtab_lookup (&ctx->symtab, name)) == 0)
    {
      if ((def = xg_symbol_new (name)) == 0)
        {
          xg_free (name);
          return 0;
        }

      xg_symtab_insert (&ctx->symtab, def);
      if ((def->code = xg_grammar_add_symbol (ctx->gram, def)) < 0)
        return 0;
    }
  else
    xg_free (name);

  return def;
}

static int
parse_symbol_list (parse_ctx *ctx, xg_production *prod)
{
  xg_symbol_def *def;
  while (ctx->token == TOKEN_WORD || ctx->token == TOKEN_LITERAL)
    {
      if (ctx->token == TOKEN_WORD)
        {
          if ((def = find_or_create_symbol (ctx, ctx->value.word)) == 0
              || xg_production_add (prod, def->code) < 0)
            return -1;
        }
      else /* ctx->token == TOKEN_LITERAL */
        {
          if (xg_production_add (prod, ctx->value.chr) < 0)
            return -1;
        }

      if (getlex (ctx) < 0)
        return -1;
    }

  return 0;
}

static int
parse_rhs_alternative (parse_ctx *ctx, xg_symbol_def *lhs)
{
  xg_production *prod;

  /* Create a production for the alternaive and parse its right hand
     side.  Add the production to the grammar.  */
  if ((prod = xg_production_new (lhs->code)) == 0
      || parse_symbol_list (ctx, prod) < 0
      || xg_grammar_add_production (ctx->gram, prod) < 0
      || xg_symbol_def_add_production (lhs, xg_grammar_production_count (ctx->gram) - 1) < 0)
    return -1;
  else
    return 0;
}

static int
parse_rhs (parse_ctx *ctx, xg_symbol_def *lhs)
{
  /* Parse the first alternative.  */
  if (parse_rhs_alternative (ctx, lhs) < 0)
    return -1;

  while (ctx->token == '|')
    {
      /* Parse the subsequent alternatives.  */
      if (getlex (ctx) < 0
          || parse_rhs_alternative (ctx, lhs) < 0)
        return -1;
    }

  return 0;
}

static int
parse_prod (parse_ctx *ctx)
{
  xg_symbol_def *lhs;

  /* Match the left hand side.  */
  if (ctx->token != TOKEN_WORD)
    {
      error (ctx, "Invalid production definition -- expected WORD");
      return -1;
    }

  /* Find or create the left hand side symbol.  */
  if ((lhs = find_or_create_symbol (ctx, ctx->value.word)) == 0)
    return -1;
  lhs->terminal = 0;

  if (getlex (ctx) < 0 || ctx->token != ':')
    {
      error (ctx, "Invalid production definition -- expected : (colon)");
      return -1;
    }

  /* Parser the right hand side.  */
  if (getlex (ctx) < 0 || parse_rhs (ctx, lhs) < 0)
    return -1;

  if (ctx->token != ';')
    {
      error (ctx, "Invalid production definition -- expected ; (semicolon)");
      return -1;
    }

  if (getlex (ctx) < 0)
    return -1;

  return 0;
}

static int
parse_start_directive (parse_ctx *ctx)
{
  xg_symbol_def *start_sym;

  if (getlex (ctx) < 0)
    return -1;

  if (ctx->token != TOKEN_WORD)
    {
      error (ctx, "Invalid start directive -- expected  WORD");
      return -1;
    }

  if ((start_sym = find_or_create_symbol (ctx, ctx->value.word)) == 0)
    return -1;

  if (ctx->gram->start != 0)
    {
      error (ctx, "Duplicate start symbol");
      return -1;
    }

  ctx->gram->start = start_sym->code;

  if (getlex (ctx) < 0)
    return -1;

  if (ctx->token != ';')
    {
      error (ctx, "Invalid start directive -- expected ; (semicolon)");
      return -1;
    }

  if (getlex (ctx) < 0)
    return -1;

  return 0;
}

static int
parse_decls (parse_ctx *ctx)
{
  if (getlex  (ctx) < 0)
    return -1;

  while (ctx->token > 0)
    {
      if (ctx->token == TOKEN_START)
        {
          if (parse_start_directive (ctx) < 0)
            return -1;
        }
      else if (parse_prod (ctx) < 0)
        return -1;
    }

  return (ctx->token == 0) ? 0 : -1;
}

xg_grammar *
xg_grammar_read (const char *name)
{
  int sts;
  parse_ctx ctx;
  char *start_name;
  xg_symbol_def *start_sym;
  xg_production *start;

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
              /* Create the start production and add it ro the
                 grammar.  Production details will be filled
                 later.  */
              if ((start = xg_production_new (0)) != 0
                  && xg_grammar_add_production (ctx.gram, start) == 0)
                {
                  /* Parse the grammar description.  */
                  sts = parse_decls (&ctx);
                }
              else
                xg_grammar_del (ctx.gram);
            }
          xg_symtab_destroy (&ctx.symtab);
        }
      fclose (ctx.in);
    }
  else
    {
      ulib_log_printf (xg_log, "ERROR: Cannot open input file ``%s''", name);
      return 0;
    }

  if (sts < 0)
    goto error;

  /* Create the augmented grammar start symbol.  */
  if ((start_name = malloc (sizeof ("<start>"))) == 0)
    goto error;

  strcpy (start_name, "<start>");
  if ((start_sym = xg_symbol_new (start_name)) == 0)
    goto error_name;
  start_sym->terminal = 0;
  if ((start_sym->code = xg_grammar_add_symbol (ctx.gram, start_sym)) < 0)
    goto error;

  /* Fill the start production details.  */
  if (ctx.gram->start == 0)
    {
      start = xg_grammar_get_production (ctx.gram, 1);
      if (start == 0)
        {
          ulib_log_printf (xg_log, "ERROR: Grammar has no productions");
          goto error;
        }
      ctx.gram->start = start->lhs;
    }

  /* Create the grammar augmentation.  */
  start = xg_grammar_get_production (ctx.gram, 0);
  start->lhs = start_sym->code;
  if (xg_production_add (start, ctx.gram->start) < 0)
    goto error;

  ctx.gram->start = start_sym->code;

  return ctx.gram;

error_name:
  free (start_name);
error:
  xg_grammar_del (ctx.gram);
  ulib_gcrun ();
  return 0;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 6cce817f-a739-48dc-8ea8-99bcb3484f81
 * End:
 */

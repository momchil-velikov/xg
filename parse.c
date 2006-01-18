/* parse.c - grammar definition parser
 *
 * Copyright (C) 2005, 2006 Momchil Velikov
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
 * along with XG; if not, write to the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
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

  /* Next token precedence level.  */
  unsigned int prec;
};
typedef struct parse_ctx parse_ctx;

/* Error reporting.  */
#define errorv(ctx, fmt, ...)                           \
  ulib_log_printf (xg_log, "%s:%d: ERROR: " fmt,        \
                   ctx->name, ctx->lineno, __VA_ARGS__)

#define error(ctx, fmt)                                                 \
  ulib_log_printf (xg_log, "%s:%d: ERROR: " fmt, ctx->name, ctx->lineno)

/* Token encoding.  */
#define TOKEN_WORD    256
#define TOKEN_LITERAL 257
#define TOKEN_START   258
#define TOKEN_TOKEN   259
#define TOKEN_LEFT    260
#define TOKEN_RIGHT   261
#define TOKEN_NASSOC  262
#define TOKEN_PREC    263

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

/* Check whether the CTX->VALUE.WORD contains at least one alphabetic
   character.  On error, release the word.  */
static int
check_valid_word (parse_ctx *ctx)
{
  const char *p = ctx->value.word;

  while (*p)
    {
      if (isalpha (*p))
        return 0;
      ++p;
    }

  errorv (ctx, "Invalid token ``%s''",  ctx->value.word);
  free (ctx->value.word);
  return -1;
}

/* Check whether a word is on of the reserved keywords.  */
static int
recognize_keyword (parse_ctx *ctx)
{
  static const struct kw
  {
    const char *name;
    unsigned int token;
  } kw [] =
      {
        { "%start",    TOKEN_START  },
        { "%token",    TOKEN_TOKEN  },
        { "%left",     TOKEN_LEFT   },
        { "%right",    TOKEN_RIGHT  },
        { "%nonassoc", TOKEN_NASSOC },
        { "%prec",     TOKEN_PREC },
        { 0, 0 }
      };

  const struct kw *p;

  for (p = kw; p->name; ++p)
    {
      if (strcmp (ctx->value.word, p->name) == 0)
        {
          free (ctx->value.word);
          ctx->token = p->token;
          return 0;
        }
    }

  return check_valid_word (ctx);
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
              ungetc (ch, ctx->in);
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
  else
    {
      scan_word (ctx, ch);
      if (*ctx->value.word == '%')
        return recognize_keyword (ctx);
      else
        return check_valid_word (ctx);
    }
}

/* Parser routines.  */

/* XG grammar definition grammar.

   gram: decls

   decls: decl | decls decl

   decl: directive | prod

   directive: '%start' word ';'
            | '%token' symbol-list ';'
            | '%left' symbol-list ';'
            | '%right' symbol-list ';'
            | '%nonassoc' symbol-list ';'


   prod: word ':' rhs ';'

   rhs: symbol-list
      | rhs '|' symbol-list

   symbol-list: symbol | symbol-list symbol

   symbol: word | token-literal
*/

/* Find or create a symbol definition for the token literal CH.  */
static xg_symdef *
find_or_create_symbol_ch (parse_ctx *ctx, xg_sym ch)
{
  xg_symdef *def;

  if ((def = xg_grammar_get_symbol (ctx->gram, ch)) == 0)
    {
      if ((def = xg_symdef_new (0)) == 0)
        return 0;

      def->terminal = xg_explicit_terminal;
      if (xg_grammar_set_symbol (ctx->gram, ch, def) < 0)
        return 0;
    }

  return def;
}

/* Find or create a symbol with name NAME.  In either case the
   function consumes the NAME parameter.  */
static xg_symdef *
find_or_create_symbol (parse_ctx *ctx, char *name)
{
  xg_symdef *def;

  if ((def = xg_symtab_lookup (&ctx->symtab, name)) == 0)
    {
      if ((def = xg_symdef_new (name)) == 0)
        {
          xg_free (name);
          return 0;
        }

      xg_symtab_insert (&ctx->symtab, def);
      if (xg_grammar_add_symbol (ctx->gram, def) < 0)
        return 0;
    }
  else
    xg_free (name);

  return def;
}

static int
parse_rhs_alternative (parse_ctx *ctx, xg_symdef *lhs)
{
  xg_prod *prod;
  xg_symdef *def;

  /* Create a production for the alternaive ... */
  if ((prod = xg_prod_new (lhs->code)) == 0)
    return -1;

  /* ... parse its right hand side ...  */
  while (ctx->token == TOKEN_WORD || ctx->token == TOKEN_LITERAL)
    {
      if (ctx->token == TOKEN_WORD)
        {
          if ((def = find_or_create_symbol (ctx, ctx->value.word)) == 0
              || xg_prod_add (prod, def->code) < 0)
            return -1;
        }
      else /* ctx->token == TOKEN_LITERAL */
        {
          if ((def = find_or_create_symbol_ch (ctx, ctx->value.chr)) == 0
              || xg_prod_add (prod, def->code) < 0)
            return -1;
        }

      if (getlex (ctx) < 0)
        return -1;
    }

  /* Parse optional explicit precedence specification.  */
  if (ctx->token == TOKEN_PREC)
    {
      if (getlex (ctx) < 0)
        return -1;

      if (ctx->token == TOKEN_WORD)
        {
          if ((def = find_or_create_symbol (ctx, ctx->value.word)) == 0)
            return -1;

          if (def->assoc == xg_assoc_unknown)
            {
              errorv (ctx, "Unknown precedence and associativity of ``s''",
                      def->name);
              return -1;
            }

        }
      else if (ctx->token == TOKEN_LITERAL)
        {
          if ((def = find_or_create_symbol_ch (ctx, ctx->value.chr)) == 0)
            return -1;

          if (def->assoc == xg_assoc_unknown)
            {
              errorv (ctx, "Unknown precedence and associativity of '%c'",
                      def->code);
              return -1;
            }
        }
      else
        {
          error (ctx, "Expected a non-terminal after %prec");
          return -1;
        }

      prod->prec = def->code;

      if (getlex (ctx) < 0)
        return -1;
    }

  /* ... and add the production to the grammar and to the left hand
     side symbol definition.  */
  if (xg_grammar_add_prod (ctx->gram, prod) < 0
      || xg_symdef_add_prod (lhs, xg_grammar_prod_count (ctx->gram) - 1) < 0)
    return -1;
  else
    return 0;
}

static int
parse_rhs (parse_ctx *ctx, xg_symdef *lhs)
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
  xg_symdef *lhs;

  /* Match the left hand side.  */
  if (ctx->token != TOKEN_WORD)
    {
      error (ctx, "Invalid production definition -- expected WORD");
      return -1;
    }

  /* Find or create the left hand side symbol.  */
  if ((lhs = find_or_create_symbol (ctx, ctx->value.word)) == 0)
    return -1;

  if (lhs->terminal == xg_explicit_terminal)
    {
      errorv (ctx, "Symbol ``%s'' already declared as terminal", lhs->name);
      return -1;
    }
  lhs->terminal = xg_non_terminal;

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
  xg_symdef *start_sym;

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

static void
perform_token_directive_operation (parse_ctx *ctx, xg_symdef *def, int dir)
{
  def->terminal = xg_explicit_terminal;
  def->prec = ctx->prec;

  switch (dir)
    {
    case TOKEN_TOKEN:
      def->prec = 0;
      def->assoc = xg_assoc_none;
      break;

    case TOKEN_LEFT:
      def->assoc = xg_assoc_left;
      break;

    case TOKEN_RIGHT:
      def->assoc = xg_assoc_right;
      break;

    case TOKEN_NASSOC:
      def->assoc = xg_assoc_none;
      break;

    default:
      assert ("invalid token directive" == 0);
    }
}

static int
parse_token_directive (parse_ctx *ctx)
{
  int dir;
  xg_symdef *def;

  dir = ctx->token;

  if (getlex (ctx) < 0)
    return -1;

  while (ctx->token == TOKEN_WORD || ctx->token == TOKEN_LITERAL)
    {
      if (ctx->token == TOKEN_WORD)
        {
          if ((def = find_or_create_symbol (ctx, ctx->value.word)) == 0)
            return -1;
        }
      else /* ctx->token == TOKEN_LITERAL */
        {
          if ((def = find_or_create_symbol_ch (ctx, ctx->value.chr)) == 0)
            return -1;
        }

      perform_token_directive_operation (ctx, def, dir);

      if (getlex (ctx) < 0)
        return -1;
    }

  if (ctx->token != ';')
    {
      error (ctx, "Invalid token directive -- expected ; (semicolon)");
      return -1;
    }

  if (getlex (ctx) < 0)
    return -1;

  if (dir == TOKEN_LEFT || dir == TOKEN_RIGHT || dir == TOKEN_NASSOC)
    ++ctx->prec;

  return 0;
}

static int
parse_decls (parse_ctx *ctx)
{
  int sts;

  if (getlex  (ctx) < 0)
    return -1;

  while (ctx->token > 0)
    {
      switch (ctx->token)
        {
        case TOKEN_START:
          sts = parse_start_directive (ctx);
          break;

        case TOKEN_TOKEN:
        case TOKEN_LEFT:
        case TOKEN_RIGHT:
        case TOKEN_NASSOC:
          sts = parse_token_directive (ctx);
          break;

        default:
          sts = parse_prod (ctx);
        }

      if (sts < 0)
        return -1;
    }

  return (ctx->token == 0) ? 0 : -1;
}

/* Set precedence and associativity of productions.  */
static void
finish_productions (xg_grammar *g)
{
  unsigned int i, nprod, nsyms;
  const xg_sym *sym;
  xg_prod *p;

  /* Process each production (except the start one).  */
  nprod = xg_grammar_prod_count (g);
  for (i = 1; i < nprod; ++i)
    {
      p = xg_grammar_get_prod (g, i);

      /* Skip null productions and prodictions, which has their
         precedence and associativity already assigned.  */
      if ((nsyms = xg_prod_length (p)) == 0 || p->prec != XG_EPSILON)
        continue;

      /* Set the production precedence and associativity to that of
         the rightmost terminal symbol.  */
      sym = xg_prod_get_symbols (p) + nsyms;
      while (nsyms--)
        {
          --sym;
          if (xg_grammar_is_terminal_sym (g, *sym))
            {
              const xg_symdef *def = xg_grammar_get_symbol (g, *sym);
              p->prec = def->code;
              break;
            }
        }
    }
}

xg_grammar *
xg_grammar_read (const char *name)
{
  int sts = -1;
  parse_ctx ctx;
  char *start_name;
  xg_symdef *start_sym;
  xg_prod *start;

  /* Initialize the parser context.  */
  ctx.name = name;
  ctx.lineno = 1;
  ctx.token = 0;
  ctx.prec = 1;

  /* Open the input file stream.  */
  if ((ctx.in = fopen (name, "r")) != 0)
    {
      /* Create the symbol table.  */
      if (xg_symtab_init (&ctx.symtab) == 0)
        {
          /* Create the grammar object.  */
          if ((ctx.gram = xg_grammar_new ()) != 0)
            {
              /* Create the start production and add it to the
                 grammar.  Production details will be filled
                 later.  */
              if ((start = xg_prod_new (0)) != 0
                  && xg_grammar_add_prod (ctx.gram, start) == 0)
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
  if ((start_sym = xg_symdef_new (start_name)) == 0)
    goto error_name;
  start_sym->terminal = xg_non_terminal;
  if (xg_grammar_add_symbol (ctx.gram, start_sym) < 0)
    goto error;

  /* Fill the start production details.  */
  if (ctx.gram->start == 0)
    {
      start = xg_grammar_get_prod (ctx.gram, 1);
      if (start == 0)
        {
          ulib_log_printf (xg_log, "ERROR: Grammar has no productions");
          goto error;
        }
      ctx.gram->start = start->lhs;
    }

  /* Create the grammar augmentation.  */
  start = xg_grammar_get_prod (ctx.gram, 0);
  start->lhs = start_sym->code;
  if (xg_prod_add (start, ctx.gram->start) < 0
      || xg_prod_add (start, XG_EOF) < 0
      || xg_symdef_add_prod (start_sym, 0) < 0)
    goto error;

  ctx.gram->start = start_sym->code;
  
  /* Set precedence and associativity of productions.  */
  finish_productions (ctx.gram);

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

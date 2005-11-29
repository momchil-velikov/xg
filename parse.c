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

/* Current line number.  */
static unsigned int lineno = 1;

/* Error reporting.  */
static void
error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "<input> : %d : ", lineno);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
}

/* Token semantic value type.  */
union sval
{
  int token;
  char *word;
};

/* Token encoding.  */
#define TOKEN_WORD 257
#define TOKEN_LITERAL 258

/* Lexical analyzer.  */
static int
handle_escape (FILE *in)
{
  int ch;

  ch = getc (in);
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
      error ("Invalid escape sequence");
      return -1;
    }
}

static int
getlex (FILE *in, union sval *value)
{
  int ch;
  char *word;
  unsigned int n, cnt;

  /* Skip whitespace.  */
  do
    {
      ch = getc (in);
      if (ch == '\n')
        ++lineno;
    }
  while (isspace (ch));

  switch (ch)
    {
    case EOF:
      return 0;

    case ':':
    case '|':
    case ';':
      return ch;
    }

  if (ch == '\'')
    {
      /* Scan a token literal.  */
      int ch1;

      ch = getc (in);
      if (ch == EOF)
        {
          error ("Invalid token literal");
          return -1;
        }

      if (ch == '\\' && (ch = handle_escape (in)) == -1)
        return -1;

      ch1 = getc (in);
      if (ch1 != '\'')
        {
          error ("Invalid token literal");
          return -1;
        }

      value->token = ch;
      return TOKEN_LITERAL;
    }

  /* Scan word.  */
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
      ch = getc (in);
    }

  value->word = word;
  return TOKEN_WORD;
}

/* Parser routines.  */

/* XG grammar definition grammar.  

   gram: prod | gram prod

   prod: WORD ':' rhs ';'

   rhs: symbol-list
      | rhs '|' symbol-list

   symbol-list: symbol | symbol-list symbol

   symbol: WORD | token-literal
*/

static int
parse_symbol_list (FILE *in, int token, union sval *val)
{
  while (token == TOKEN_WORD || token == TOKEN_LITERAL)
    token = getlex (in, val);

  return token;
}

static int
parse_rhs (FILE *in, const char *lhs, int token, union sval *val)
{
  token = parse_symbol_list (in, token, val);
  while (token == '|')
    {
      token = getlex (in, val);
      if (token == -1)
        return -1;
      token = parse_symbol_list (in, token, val);
    }

  return token;
}

static int
parse_prod (FILE *in, int token, union sval *val)
{
  char *lhs;

  if (token != TOKEN_WORD)
    {
      error ("Invalid production definition -- expected WORD");
      return -1;
    }

  lhs = val->word;
  token = getlex (in, val);
  
  if (token != ':')
    {
      error ("Invalid production definition -- expected : (colon)");
      goto error;
    }
  token = getlex (in, val);

  token = parse_rhs (in, lhs, token, val);

  if (token != ';')
    {
      error ("Invalid production definition -- expected ; (semicolon)");
      goto error;
    }
  token = getlex (in, val);
  
  free (lhs);
  return token;

error:
  free (lhs);
  return -1;
}

static int
parse_gram (FILE *in)
{
  int token;
  union sval val;

  token = getlex  (in, &val);
  while (token && token != -1)
    token = parse_prod (in, token, &val);

  return token;
}

static int
usage ()
{
  fprintf (stderr, "usage: xg <filename>\n");
  return -1;
}

int
main (int argc, char *argv [])
{
  FILE *in;

  if (argc != 2)
    return usage ();

  in = fopen (argv [1], "rt");
  if (in == 0)
    return -1;

  if (parse_gram (in) == 0)
    {
      puts ("success");
      return 0;
    }
  else
    {
      puts ("failure");
      return -1;
    }
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 6cce817f-a739-48dc-8ea8-99bcb3484f81
 * End:
 */

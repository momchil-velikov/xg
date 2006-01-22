#ifndef FOR_BISON
#include <xg-c-parser.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef FOR_BISON
int yylex ()
#else
static int get_token (void **value)
#endif
{
  int ch;
  char *p;
  int val;

  do
    {
      ch = getc (stdin);
    }
  while (isspace (ch));

  if (ch == EOF)
    return 0;

  val = 0;
  do
    {
      val = val * 10 + (ch - '0');
      ch = getc (stdin);
    }
  while (isdigit (ch));

  ungetc (ch, stdin);
  return val;
}

static void
print (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
}

#ifdef FOR_BISON
extern int yyparse ();
#define PARSE(CTX) yyparse ()
#else
extern int xg_parse (xg_parse_ctx *ctx);
#define PARSE(CTX) xg_parse (CTX)
#endif

int
main ()
{
#if defined (FOR_BISON)
#if !defined (YYDEBUG) || YYDEBUG
  extern int yydebug;
  yydebug = 0;
#endif
#else
  xg_parse_ctx ctx = { get_token, print, 0, };
#endif

  if (PARSE (&ctx) == 0)
    puts ("success");
  else
    puts ("error");

  return 0;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: e8968f02-18e7-4228-86da-cfb0ab198a52
 * End:
 */

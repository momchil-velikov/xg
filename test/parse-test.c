#include <xg-c-parser.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

static int
get_token (void **value)
{
  int ch;
  static char buf [32];
  char *p;

  do
    {
      ch = getc (stdin);
    }
  while (isspace (ch));

  if (ch == EOF)
    return 0;

  p = buf;
  do
    {
      *p++ = ch;
      ch = getc (stdin);
    }
  while (isdigit (ch));

  ungetc (ch, stdin);
  *p++ = '\0';

  return strtol (buf, 0, 0);
}

static void
print (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
}

extern int xg_parse (xg_parse_ctx *ctx);

int
main ()
{
  xg_parse_ctx ctx = { get_token, print, 1, };

  if (xg_parse (&ctx) == 0)
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

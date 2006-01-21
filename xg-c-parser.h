/* xg-c-parser.h - Common declarations for all C parsers.
 *
 * Author: Momchil Velikov
 *
 * This file is in the public domain.
 */
#ifndef xg__c_parser_h
#define xg__c_parser_h 1

#include <stdlib.h>
#include <assert.h>

/* Parser stack entry.  */
struct xg__stkent
{
  /* DFA state.  */
  unsigned int state;

  /* Semantic value.  */
  void *value;
};
typedef struct xg__stkent xg__stkent;

/* Parser stack.  */
struct xg__stack
{
  /* Allocated size.  */
  unsigned int alloc;

  /* Stack entries.  */
  xg__stkent *base;

  /* Top entry.  */
  xg__stkent *top;
};
typedef struct xg__stack xg__stack;

/* Initialize the parser stack.  */
static inline int
xg__stack_init (xg__stack *stk)
{
  stk->alloc = 10;
  stk->base = malloc (stk->alloc * sizeof (xg__stkent));
  if (stk->base == 0)
    return -1;
  stk->top = stk->base;
  return 0;
}

/* Destroy the parser stack.  */
static inline void
xg__stack_destroy (xg__stack *stk)
{
  free (stk->base);
}

/* Grow a stack.  */
static int
xg__stack_grow (xg__stack *stk)
{
  unsigned int na = stk->alloc * 2 + 1;
  xg__stkent *np = realloc (stk->base, na * sizeof (xg__stkent));
  if (np == 0)
    return -1;

  stk->top = np + (stk->top - stk->base);
  stk->base = np;
  stk->alloc = na;

  return 0;
}

/* Push a state on the stack.  */
static inline int
xg__stack_push (xg__stack *stk, unsigned int state)
{
  if (stk->top - stk->base == stk->alloc && xg__stack_grow (stk) < 0)
    return -1;

  stk->top->state = state;
  stk->top->value = 0;
  ++stk->top;

  return 0;
}

/* Pop N entries from the stack.  */
static inline void
xg__stack_pop (xg__stack *stk, unsigned int n)
{
  assert (n < stk->top - stk->base);
  stk->top -= n;
}

/* Return top of the stack.  */
static inline xg__stkent *
xg__stack_top (xg__stack *stk)
{
  return stk->top - 1;
}


/* Parser context struct.  */
struct xg_parse_ctx
{
  /* Scanner function (initialized by user).  */
  int (*get_token) (void **value);

  /* Debug print function.  */
  void (*print) (const char *fmt, ...);

  /* Enable debugging flag.  */
  int debug;

  /* Parser automaton stack (initialized by the parser function). */
  xg__stack stk;
};
typedef struct xg_parse_ctx xg_parse_ctx;


#ifdef NDEBUG

#define XG__TRACE_SHIFT(TOKEN) do {} while (0)
#define XG__TRACE_NEXT_TOKEN(TOKEN) do {} while (0)
#define XG__TRACE_PUSH(STATE) do {} while (0)
#define XG__TRACE_STACK_DUMP() do {} while (0)
#define XG__TRACE_REDUCE(PROD) do {} while (0)

#else /* ! NDEBUG */

/* Print the parsing stack.  */
static void
xg__stack_dump (const xg_parse_ctx *ctx)
{
  xg__stkent *ent;

  for (ent = ctx->stk.base; ent < ctx->stk.top; ++ent)
    ctx->print ("%d ", ent->state);
  ctx->print ("\n");
}

#define XG__TRACE_SHIFT(SYM)                                            \
  do                                                                    \
    {                                                                   \
      if (ctx->debug)                                                   \
        if (SYM < 256)                                                  \
          ctx->print ("Shifting '%c'\n", SYM);                          \
        else                                                            \
          ctx->print ("Shifting %s\n", xg__symbol_name [SYM - 256]);    \
    }                                                                   \
  while (0)

#define XG__TRACE_NEXT_TOKEN(TOKEN)                                     \
  do                                                                    \
    {                                                                   \
      if (ctx->debug)                                                   \
        if (TOKEN < 256)                                                \
          ctx->print ("Next token is '%c'\n", TOKEN);                   \
        else                                                            \
          ctx->print ("Next token is %s\n", xg__symbol_name [TOKEN - 256]); \
    }                                                                   \
  while (0)

#define XG__TRACE_PUSH(STATE)                       \
  do                                                \
    {                                               \
      if (ctx->debug)                               \
        ctx->print ("Entering state %u\n", STATE);  \
    }                                               \
  while (0)

#define XG__TRACE_STACK_DUMP()                  \
  do                                            \
    {                                           \
      if (ctx->debug)                           \
        {                                       \
          ctx->print ("Stack is: ");            \
          xg__stack_dump (ctx);                 \
        }                                       \
    }                                           \
  while (0)

#define XG__TRACE_REDUCE(PROD)                          \
  do                                                    \
    {                                                   \
      if (ctx->debug)                                   \
        ctx->print ("Reducing by production %u: %s\n",  \
                    PROD, xg__prod [PROD]);             \
    }                                                   \
  while (0)

#endif /* NDEBUG */

#define XG__SHIFT                                       \
  do                                                    \
    {                                                   \
      XG__TRACE_SHIFT (token);                          \
      xg__stack_top (&ctx->stk)->value = value;         \
      if ((token = ctx->get_token (&value)) == -1)      \
        goto lexer_error;                               \
      XG__TRACE_NEXT_TOKEN (token);                     \
    }                                                   \
  while (0)

#define XG__PUSH(n)                             \
  do                                            \
    {                                           \
      XG__TRACE_PUSH (n);                       \
      xg__stack_push (&ctx->stk, n);            \
      XG__TRACE_STACK_DUMP ();                  \
    }                                           \
  while (0)

#define XG__REDUCE(PROD, LHS, LEN)              \
  do                                            \
    {                                           \
      XG__TRACE_REDUCE (PROD);                  \
      xg__stack_pop (&ctx->stk, LEN);           \
      lhs = LHS;                                \
    }                                           \
  while (0)

#define XG__PARSER_FUNCTION_START               \
  /* Current token.  */                         \
  int token;                                    \
                                                \
  /* Token or production semantic value.  */    \
  void *value;                                  \
                                                \
  /* Reduced symbol.  */                        \
  unsigned int lhs;                             \
                                                \
  if (xg__stack_init (&ctx->stk) < 0)           \
    return -1;                                  \
                                                \
  if ((token = ctx->get_token (&value)) == -1)  \
    goto lexer_error;                           \
  XG__TRACE_NEXT_TOKEN (token);                 \
                                                \
  goto push_0

#define XG__PARSER_FUNCTION_END(N)               \
  do                                             \
    {                                            \
      xg__stack_destroy (&ctx->stk);             \
      return N;                                  \
    }                                            \
  while (0)

#endif /* xg__c_parser_h 1 */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 2b10c430-4a16-42f7-ab1b-71f035cf73ec
 * End:
 */

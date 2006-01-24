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
  int alloc;

  /* Stack entries.  */
  xg__stkent *base;

  /* Top entry.  */
  xg__stkent *top;
};
typedef struct xg__stack xg__stack;

#define XG__INITIAL_STACK_SIZE 200

/* Initialize the parser stack.  */
static inline int
xg__stack_init (xg__stack *stk)
{
  stk->alloc = XG__INITIAL_STACK_SIZE;
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

/* Ensure there's enough space in the stack for at least one push.  */
static inline int
xg__stack_ensure (xg__stack *stk)
{
  return (stk->top - stk->base < stk->alloc) ? 0 : xg__stack_grow (stk);
}

/* Push a state on the stack.  */
static inline void
xg__stack_push (xg__stack *stk, unsigned int state)
{
  assert (stk->top - stk->base < stk->alloc);

  stk->top->state = state;
  stk->top->value = 0;
  ++stk->top;
}

/* Pop N entries from the stack.  */
static inline void
xg__stack_pop (xg__stack *stk, unsigned int n)
{
  assert ((int) n < stk->top - stk->base);
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
};
typedef struct xg_parse_ctx xg_parse_ctx;

#ifndef NDEBUG
/* Print the parsing stack.  */
static void
xg__stack_dump (const xg_parse_ctx *ctx, const xg__stack *stk)
{
  xg__stkent *ent;

  for (ent = stk->base; ent < stk->top; ++ent)
    ctx->print ("%d ", ent->state);
  ctx->print ("\n");
}
#endif


#ifdef NDEBUG

#define XG__TRACE_SHIFT(TOKEN) do {} while (0)
#define XG__TRACE_NEXT_TOKEN(TOKEN) do {} while (0)
#define XG__TRACE_PUSH(STATE) do {} while (0)
#define XG__TRACE_STACK_DUMP() do {} while (0)
#define XG__TRACE_REDUCE(PROD) do {} while (0)

#else /* ! NDEBUG */

#define XG__TRACE_SHIFT(SYM)                                            \
  do                                                                    \
    {                                                                   \
      if (ctx->debug)                                                   \
        {                                                               \
          if (SYM < 256)                                                \
            ctx->print ("Shifting '%c'\n", SYM);                        \
          else                                                          \
            ctx->print ("Shifting %s\n", xg__symbol_name [SYM - 256]);  \
        }                                                               \
    }                                                                   \
  while (0)

#define XG__TRACE_NEXT_TOKEN(TOKEN)                                     \
  do                                                                    \
    {                                                                   \
      if (ctx->debug)                                                   \
        {                                                               \
          if (TOKEN == -1)                                              \
            ctx->print ("Next token is <lexer-error>\n");               \
          else if (TOKEN < 256)                                         \
            ctx->print ("Next token is '%c'\n", TOKEN);                 \
          else                                                          \
            ctx->print ("Next token is %s\n", xg__symbol_name [TOKEN - 256]); \
        }                                                               \
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
          xg__stack_dump (ctx, &stk);            \
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

#define XG__SHIFT                               \
  do                                            \
    {                                           \
      XG__TRACE_SHIFT (token);                  \
      xg__stack_top (&stk)->value = value;      \
      token = ctx->get_token (&value);          \
      XG__TRACE_NEXT_TOKEN (token);             \
      if (xg__stack_ensure (&stk) < 0)          \
        goto internal_error;                    \
    }                                           \
  while (0)

#define XG__PUSH(N)                             \
  do                                            \
    {                                           \
      XG__TRACE_PUSH (N);                       \
      xg__stack_push (&stk, N);                 \
      XG__TRACE_STACK_DUMP ();                  \
      state = N;                                \
    }                                           \
  while (0)

#define XG__REDUCE(PROD, LEN)                   \
  do                                            \
    {                                           \
      XG__TRACE_REDUCE (PROD);                  \
      xg__stack_pop (&stk, LEN);                \
      state = xg__stack_top (&stk)->state;      \
    }                                           \
  while (0)

#define XG__PARSER_FUNCTION_START               \
  /* Current token.  */                         \
  int token;                                    \
                                                \
  /* Token or production semantic value.  */    \
  void *value;                                  \
                                                \
  /* Current state.  */                         \
  unsigned int state;                           \
                                                \
  /* Parse automaton stack.  */                 \
  xg__stack stk;                                \
                                                \
  if (xg__stack_init (&stk) < 0)                \
    return -1;                                  \
                                                \
  token = ctx->get_token (&value);              \
  XG__TRACE_NEXT_TOKEN (token);                 \
                                                \
  goto push_0

#define XG__PARSER_FUNCTION_END(N)               \
  do                                             \
    {                                            \
      xg__stack_destroy (&stk);                  \
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

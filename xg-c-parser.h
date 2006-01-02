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

/* Push a state on the stack.  */
static inline int
xg__stack_push (xg__stack *stk, unsigned int state)
{
  if (stk->top - stk->base == stk->alloc)
    {
      unsigned int nsz = stk->alloc * 2 + 1;
      xg__stkent *nw = realloc (stk->base, stk->alloc * sizeof (xg__stkent));
      if (nw == 0)
        return -1;
      stk->top = nw + (stk->top - stk->base);
      stk->base = nw;
      stk->alloc = nsz;
    }

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

  /* Parser automaton stack (initialized by the parser function). */
  xg__stack stk;
};
typedef struct xg_parse_ctx xg_parse_ctx;

#endif /* xg__c_parser_h 1 */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 2b10c430-4a16-42f7-ab1b-71f035cf73ec
 * End:
 */

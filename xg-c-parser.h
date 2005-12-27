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
struct xg_stkent
{
  /* DFA state.  */
  unsigned int state;

  /* Semantic value.  */
  void *value;
};
typedef struct xg_stkent xg_stkent;

/* Parser stack.  */
struct xg_stack
{
  /* Allocated size.  */
  unsigned int alloc;

  /* Stack entries.  */
  xg_stkent *base;

  /* Top entry.  */
  xg_stkent *top;
};
typedef struct xg_stack xg_stack;

/* Initialize the parser stack.  */
static inline int
xg_stack_init (xg_stack *stk)
{
  stk->alloc = 10;
  stk->base = malloc (stk->alloc * sizeof (xg_stkent));
  if (stk->base == 0)
    return -1;
  stk->top = stk->base;
  return 0;
}

/* Push a state on the stack.  */
static inline int
xg_stack_push (xg_stack *stk, unsigned int state)
{
  if (stk->top - stk->base == stk->alloc)
    {
      unsigned int nsz = stk->alloc * 2 + 1;
      xg_stkent *nw = realloc (stk->base, stk->alloc * sizeof (xg_stkent));
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
xg_stack_pop (xg_stack *stk, unsigned int n)
{
  assert (n < stk->top - stk->base);
  stk->top -= n;
}

/* Return top of the stack.  */
static inline xg_stkent *
xg_stack_top (xg_stack *stk)
{
  return stk->top - 1;
}


/* Scanner function: must be supplied by user.  */
int xg_get_token (void **value);

#endif /* xg__c_parser_h 1 */

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * arch-tag: 2b10c430-4a16-42f7-ab1b-71f035cf73ec
 * End:
 */

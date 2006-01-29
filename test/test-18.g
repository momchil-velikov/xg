/* regression test for
   xg: conflicts.c:150: resolve_shift_reduce_conflicts: Assertion `tr->sym != 0 && tr->sym != 1' failed.  */

%start S ;

S : T ;

T : S ;

/* arch-tag: 85275b6f-3b59-464a-b989-587cf7c3622a */

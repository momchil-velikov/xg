/* regression test for
   xg: conflicts.c:150: resolve_shift_reduce_conflicts: Assertion `tr->sym != 0 && tr->sym != 1' failed.  */

%start S ;

S : T ;

T : S ;


%start S ;

S : A | A B | B ;

A : C ;
B : D ;

C : p | /* empty */ ;

D : q ;

/* arch-tag: 31287bcf-ee26-490f-9c96-3ff29ad29a89 */

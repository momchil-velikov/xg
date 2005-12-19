%start S ;

S : A | A B | B ;

A : C ;
B : D ;

C : p | /* empty */ ;

D : q ;

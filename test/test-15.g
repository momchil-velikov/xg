/* LALR(1), but not SLR(1) */
	
Z : A ;

A :   a B b 
    | a d c
    | b B c 
    | b d d ;

B : d ;


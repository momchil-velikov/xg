/* LALR(1), but not SLR(1) */
	
Z : A ;

A :   a B b 
    | a d c
    | b B c ;
    | b d d ;

B : d ;

/* arch-tag: e1fdee24-d21b-4969-9a43-336d9b9be4af */

%start E ;

E :  T E' ;
E' : '+' T E' | /* empty */ ;
T : F T' ;
T' : '*' F T' | /* empty */ ;
F : '(' E ')' | IDENT ;


/* arch-tag: 4b3348e0-5c45-4b99-a058-c222382a1769 */

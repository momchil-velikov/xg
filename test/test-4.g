%start E ;

E :  T E' ;
E' : '+' T E' | /* empty */ ;
T : F T' ;
T' : '*' F T' | /* empty */ ;
F : '(' E ')' | IDENT ;



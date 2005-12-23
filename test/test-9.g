%start E ;

E : E '+' T | T ;

T : T '*' F | F ;

F :  '(' E ')' | ID ;

/* arch-tag: e7830f5b-c522-4467-8bad-8e631fcca7c0 */

%start stmt ;

stmt : if expr then stmt stmt' | other-stmt ;

stmt' : else stmt | /* empty */ ;

/* arch-tag: 9cb1a288-8ba2-433a-a9ba-9034f7a35387 */

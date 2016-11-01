%start stmt ;

stmt : if expr then stmt stmt' | other-stmt ;

stmt' : else stmt | /* empty */ ;


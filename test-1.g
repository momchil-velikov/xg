expr :
        expr/no-commas
    |   expr ',' expr/no-commas
    ;   

expr/no-commas :
        IDENT
    |   expr/no-commas '+' expr/no-commas
    |   expr/no-commas '*' expr/no-commas
    ;       

stmt :
        expr
    |   expr '=' expr
    ;

stmt-list :
        stmt ';'
    |   stmt-list stmt ';'
    ;

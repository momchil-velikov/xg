%start stmt-list ;

%left '*' '/' '%';
%left '+' '-';
%right '=';

expr :
        expr/no-commas
    |   expr ',' expr/no-commas
    ;   

expr/no-commas :
        IDENT
    |   expr/no-commas '+' expr/no-commas
    |   expr/no-commas '-' expr/no-commas
    |   expr/no-commas '*' expr/no-commas
    |   expr/no-commas '/' expr/no-commas
    |   expr/no-commas '%' expr/no-commas
    ;       

stmt :
        expr
    |   expr '=' expr
    ;

stmt-list :
        stmt ';'
    |   stmt-list stmt ';'
    ;

/* arch-tag: b3f2b9c3-9b87-4358-b9e0-d766ea6d13c0 */

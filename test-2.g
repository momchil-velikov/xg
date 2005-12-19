digit : '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' ;

expr : digit
    |  ID
    |  expr '+' expr
    |  expr '*' expr
    |  ID '(' args ')'
    ;

args : /* empty */
    |  expr
    |  args ',' expr
    ;


%start expr ;

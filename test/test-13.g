/* type (id)->id = expr;
   type (id) (type); */

stmt-list :
        stmt ';' 
    |   stmt-list stmt ';'
    ;

stmt : decl | expr ;

decl : 't' declarator ;

declarator :
        'i'
    |   declarator '(' 't' declarator ')'
    |  '(' declarator ')'
    ;

expr :
        'i'
    |   expr '(' expr ')'
    |   '(' expr ')'
    |  't' '(' expr ')'
    |   expr '.' 'i'
    ;


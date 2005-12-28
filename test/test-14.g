/* type (id)->id = expr;
   type (id) (type); */

stmt-list :
        stmt ';' 
    |   stmt-list stmt ';'
    ;

stmt : decl | expr ;

decl : 'i' declarator ;

declarator :
        'i'
    |   declarator '(' 'i' declarator ')'
    |  '(' declarator ')'
    ;

expr :
        'i'
    |   expr '(' expr ')'
    |   '(' expr ')'
    |  'i' '(' expr ')'
    |   expr '.' 'i'
    ;

/* arch-tag: be9892ec-a4e6-4713-904b-d7804b7efa24 */

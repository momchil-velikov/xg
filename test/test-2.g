
%left ')' ;
%left '+' '-' ;
%left '*' ;
%right UMINUS ;
%right '(' ;

expr : NUMBER
    |  ID
    |  ID '(' args' ')'
    |  '(' expr ')'
    | '-' expr %prec UMINUS
    |  expr '+' expr
    |  expr '-' expr
    |  expr '*' expr
    ;

args' : /* empty */ | args ;

args : expr
    |  args ',' expr
    ;

%start expr ;

/* arch-tag: de7e1e00-3af9-4cef-a73d-c1aa5bac6518 */

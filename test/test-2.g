
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


/* Partial Pascal type declarations grammar. Not LALR(1).  */

%left '+' '-' ;
%left '*' '/' ;
     
     
type_decl : TYPE ID '=' type ';'
    ;
     
type : '(' id_list ')'
    | expr DOTDOT expr
    ;
     
id_list : ID
    | id_list ',' ID
    ;
     
expr : '(' expr ')'
    | expr '+' expr
    | expr '-' expr
    | expr '*' expr
    | expr '/' expr
    | ID
    ;


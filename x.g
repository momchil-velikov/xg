expr : expr/no-parens
     | '(' expr ')'
     ;

expr/no-parens : IDENT
	       | expr '+' expr
	       | expr '*' expr
	       ;

stmt : expr
     | expr '=' expr
     ;

stmt-list : stmt ';'
          | stmt-list stmt ';'
          ;

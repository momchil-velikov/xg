expr : IDENT
     | expr '+' expr ;

stmt : expr
     | expr '=' expr ;

stmt-list : stmt ';'
          | stmt-list stmt ';'
	  ;

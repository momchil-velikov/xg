stmt-list : stmt ';'
          | stmt-list stmt ';'
	  ;

stmt : '(' list ')' '=' '(' list ')' 
     | '(' list ')'
     ;

list : expr 
     | list ',' expr
     ;


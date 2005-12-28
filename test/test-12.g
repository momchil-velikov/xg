stmt-list : stmt ';'
          | stmt-list stmt ';'
	  ;

stmt : '(' list ')' '=' '(' list ')' 
     | '(' list ')'
     ;

list : expr 
     | list ',' expr
     ;

/* arch-tag: 6b9a8b94-1da2-4cdc-b9a1-468e726e99d1 */

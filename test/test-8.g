 S : '(' L ')' ;

 S : a ;

 L : S L' ;

 L' : ',' S L' ;

 L' : /* empty */ ;



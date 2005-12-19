 S : '(' L ')' ;

 S : a ;

 L : S L' ;

 L' : ',' S L' ;

 L' : /* empty */ ;


/* arch-tag: 1d44d3a9-97b6-4829-9030-1f17c5b9fb59 */

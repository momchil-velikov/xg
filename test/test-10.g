%start S ;

S : L '=' R | R ;


L : '*' R | id ;

R : L ;


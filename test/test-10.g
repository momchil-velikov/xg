%start S ;

S : L '=' R | R ;


L : '*' R | id ;

R : L ;

/* arch-tag: 600e3845-2b0c-44c4-975a-9613e66eaeb7 */

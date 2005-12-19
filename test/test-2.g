
expr : NUMBER
    |  ID
    |  expr '+' expr
    |  expr '*' expr
    |  ID '(' args' ')'
    ;

args' : /* empty */ | args ;

args : expr
    |  args ',' expr
    ;

%start expr ;

/* arch-tag: de7e1e00-3af9-4cef-a73d-c1aa5bac6518 */

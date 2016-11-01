/* ISO/IEC 9988:1999 grammar (ISO C) */

%token ID TYPEDEF_NAME INT_CST STRING_CST SIZEOF ;
%token CHAR INT FLOAT  ;


%token ARROW INC DEC ;
%token LSHIFT RSHIFT ;
%token LE GE ;
%token EQ NE ;
%token ANDAND OROR ;
%token MUL_ASSIGN DIV_ASSIGN  MOD_ASSIGN  PLUS_ASSIGN ;
%token MINUS_ASSIGN  LSHIFT_ASSIGN  RSHIFT_ASSIGN AND_ASSIGN XOR_ASSIGN ;
%token OR_ASSIGN ;

%token TYPEDEF EXTERN STATIC AUTO REGISTER ;

%token VOID CHAR SHORT INT LONG FLOAT DOUBLE SIGNED UNSIGNED BOOL ;
%token COMPLEX IMAGINARY ;

%token STRUCT UNION ENUM ;

%token CONST RESTRICT VOLATILE INLINE ;

%token ELLIPSIS ;

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN ;

%start translation_unit ;

identifier_opt : /* empty */ | ID ;

/* 6.5.1 Primary expressions.  */

primary_expr :
        ID
    |   INT_CST
    |   STRING_CST
    |   '(' expression ')'
    ;


/* 6.5.2  Postfix operators.  */

postfix_expr :
        primary_expr
    |   postfix_expr '[' expression ']'
    |   postfix_expr '(' argument_expr_list_opt ')'
    |   postfix_expr '.'  ID
    |   postfix_expr ARROW ID
    |   postfix_expr INC
    |   postfix_expr DEC
    |   '(' type_name ')' '{' initializer_list '}'
    |   '(' type_name ')' '{' initializer_list ',' '}'
    ;

argument_expr_list_opt :
        /* empty */
    |   argument_expr_list
    ;

argument_expr_list :
        assignment_expr
    |   argument_expr_list ',' assignment_expr
    ;


/* 6.5.3  Unary operators.  */

unary_expr :
        postfix_expr
    |   INC unary_expr
    |   DEC unary_expr
    |   unary_operator cast_expr
    |   SIZEOF unary_expr
    |   SIZEOF '(' type_name ')'
    ;

unary_operator : '&' | '*' | '+' | '-' | '~' | '!' ;


/* 6.5.4  Cast operators.  */

cast_expr :
        unary_expr
    |  '(' type_name ')' cast_expr
    ;



/* 6.5.5  Multiplicative operators.  */

multiplicative_expr :
        cast_expr
    |   multiplicative_expr '*' cast_expr
    |   multiplicative_expr '/' cast_expr
    |   multiplicative_expr '%' cast_expr
    ;


/* 6.5.6  Additive operators.  */

additive_expr :
        multiplicative_expr
    |   additive_expr '+' multiplicative_expr
    |   additive_expr '-' multiplicative_expr
    ;


/* 6.5.7  Bitwise shift operators.  */

shift_expr :
        additive_expr
    |   shift_expr LSHIFT additive_expr
    |   shift_expr RSHIFT additive_expr
    ;


/* 6.5.8  Relational operators.  */

relational_expr :
        shift_expr
    |   relational_expr '<'  shift_expr
    |   relational_expr '>'  shift_expr
    |   relational_expr LE shift_expr
    |   relational_expr GE shift_expr
    ;



/* 6.5.9  Equality operators.  */

equality_expr :
        relational_expr
    |   equality_expr EQ relational_expr
    |   equality_expr NE relational_expr
    ;


/* 6.5.10  Bitwise AND operator.  */

and_expr :
        equality_expr
    |   and_expr '&' equality_expr
    ;


/* 6.5.11  Bitwise exclusive OR operator.  */

exclusive_or_expr :
        and_expr
    |   exclusive_or_expr '^' and_expr
    ;


/* 6.5.12  Bitwise inclusive OR operator.  */

inclusive_or_expr :
        exclusive_or_expr
    |   inclusive_or_expr '|' exclusive_or_expr
    ;


/* 6.5.13  Logical AND operator.  */

logical_and_expr :
        inclusive_or_expr
    |   logical_and_expr ANDAND inclusive_or_expr
    ;


/* 6.5.14  Logical OR operator.  */

logical_or_expr :
        logical_and_expr
    |   logical_or_expr OROR logical_and_expr
    ;


/* 6.5.15  Conditional operator.  */

conditional_expr :
        logical_or_expr
    |   logical_or_expr '?' expression ':' conditional_expr
    ;


/* 6.5.16  Assignment operators.  */

assignment_expr_opt : /* empty */ | assignment_expr ;

assignment_expr :
        conditional_expr
    |   unary_expr assignment_operator assignment_expr
    ;

assignment_operator : '=' | MUL_ASSIGN | DIV_ASSIGN | MOD_ASSIGN | PLUS_ASSIGN
    | MINUS_ASSIGN | LSHIFT_ASSIGN | RSHIFT_ASSIGN | AND_ASSIGN | XOR_ASSIGN
    | OR_ASSIGN
    ;


/* 6.5.17  Comma operator.  */

expr_opt : /* empty */ | expression ;

expression :
        assignment_expr
    |   expression ',' assignment_expr
    ;


/* 6.6  Constant expressions.  */

constant_expr : conditional_expr ;


/* 6.7  Declarations.  */

declaration :
        declaration_specifiers init_declarator_list_opt ';' ;

declaration_specifiers_opt : /* empty */ | declaration_specifiers ;

declaration_specifiers :
        storage_class_specifier declaration_specifiers_opt
    |   type_specifier declaration_specifiers_opt
    |   type_qualifier declaration_specifiers_opt
    |   function_specifier declaration_specifiers_opt
    ;

init_declarator_list_opt : /* empty */ | init_declarator_list ;

init_declarator_list :
        init_declarator
    |   init_declarator_list ',' init_declarator
    ;
     
init_declarator :
        declarator
    |   declarator '=' initializer
    ;


/* 6.7.1  Storage-class specifiers.  */

storage_class_specifier :
        TYPEDEF
    |   EXTERN
    |   STATIC
    |   AUTO
    |   REGISTER
    ;


/* 6.7.2  Type specifiers.  */

type_specifier :
        VOID
    |   CHAR
    |   SHORT
    |   INT
    |   LONG
    |   FLOAT
    |   DOUBLE
    |   SIGNED
    |   UNSIGNED
    |   BOOL
    |   COMPLEX
    |   IMAGINARY
    |   struct_or_union_specifier
    |   enum_specifier
    |   TYPEDEF_NAME
    ;


/* 6.7.2.1  Structure and union specifiers.  */

struct_or_union_specifier :
        struct_or_union identifier_opt '{' struct_declaration_list '}'
    |   struct_or_union ID
    ;
     
struct_or_union :
        STRUCT
    |   UNION
    ;

struct_declaration_list :
        struct_declaration
    |   struct_declaration_list struct_declaration
    ;

struct_declaration :
        specifier_qualifier_list struct_declarator_list ';'
    ;
specifier_qualifier_list_opt : /* empty */ | specifier_qualifier_list ;

specifier_qualifier_list :
        type_specifier specifier_qualifier_list_opt
    |   type_qualifier specifier_qualifier_list_opt
    ;
     
struct_declarator_list :
        struct_declarator
    |   struct_declarator_list ',' struct_declarator
    ;
     
struct_declarator :
        declarator
    |   declarator_opt ':' constant_expr
    ;


/* 6.7.2.2  Enumeration specifiers.  */

enum_specifier :
        ENUM identifier_opt '{' enumerator_list '}'
    |   ENUM identifier_opt '{' enumerator_list ',' '}'
    |   ENUM ID
    ;

enumerator_list :
        enumerator
    |   enumerator_list ',' enumerator
    ;
     
enumerator :
        ID
    |   ID '=' constant_expr
    ;


/* 6.7.3  Type qualifiers.  */

type_qualifier :
        CONST
    |   RESTRICT
    |   VOLATILE
    ;



/* 6.7.4  Function specifiers.  */

function_specifier : INLINE ;

       
/* 6.7.5  Declarators.  */

declarator_opt : /* empty */ | declarator ;

declarator : pointer_opt direct_declarator ;

direct_declarator :
        ID
    |   '(' declarator ')'
    |   direct_declarator '[' type_qualifier_list_opt assignment_expr_opt ']'
    |   direct_declarator '[' STATIC type_qualifier_list_opt assignment_expr ']'
    |   direct_declarator '[' type_qualifier_list STATIC assignment_expr ']'
    |   direct_declarator '[' type_qualifier_list_opt '*' ']'
    |   direct_declarator '(' parameter_type_list ')'
    |   direct_declarator '(' identifier_list_opt ')'
    ;

pointer_opt : /* empty */ | pointer ;

pointer :
        '*' type_qualifier_list_opt
    |   '*' type_qualifier_list_opt pointer
    ;

type_qualifier_list_opt : /* empty */ | type_qualifier_list ;

type_qualifier_list :
        type_qualifier
    |   type_qualifier_list type_qualifier
    ;
        
parameter_type_list_opt : /* empty */ | parameter_type_list ;

parameter_type_list :
        parameter_list
    |   parameter_list ',' ELLIPSIS
    ;
     
parameter_list :
        parameter_declaration
    |   parameter_list ',' parameter_declaration
    ;
     
parameter_declaration :
        declaration_specifiers declarator
    |   declaration_specifiers abstract_declarator_opt
    ;
     
identifier_list_opt : /* empty */ | identifier_list ;

identifier_list :
        ID
    |   identifier_list ',' ID
    ;


/* 6.7.6  Type names.  */

type_name : specifier_qualifier_list abstract_declarator_opt ;

abstract_declarator_opt : /* empty */ | abstract_declarator ;

abstract_declarator :
        pointer
    |   pointer_opt direct_abstract_declarator
    ;

direct_abstract_declarator_opt : /* empty */ | direct_abstract_declarator ;

direct_abstract_declarator :
        '(' abstract_declarator ')'
    |   '(' parameter_type_list_opt ')'
    |   direct_abstract_declarator_opt '[' assignment_expr_opt ']'
    |   direct_abstract_declarator_opt '[' '*' ']'
    |   direct_abstract_declarator '(' parameter_type_list_opt ')'
    ;

/* 6.7.7  Type definitions.  */

/* typedef_name : ID ; */


/* 6.7.8  Initialization.  */

initializer :
        assignment_expr
    |   '{' initializer_list '}'
    |   '{' initializer_list ',' '}'
    ;

initializer_list :
        designation_opt initializer
    |   initializer_list ',' designation_opt initializer
    ;

designation_opt : /* empty */ | designation ;
     
designation : designator_list '=' ;

designator_list :
        designator
    |   designator_list designator
    ;
        
designator :
        '[' constant_expr ']'
    |   '.' ID
    ;


/* 6.8  Statements and blocks.  */

statement :
        labeled_statement
    |   compound_statement
    |   expression_statement
    |   selection_statement
    |   iteration_statement
    |   jump_statement
    ;


/* 6.8.1  Labeled statements.  */

labeled_statement :
        ID ':' statement
    |   CASE constant_expr ':' statement
    |   DEFAULT ':' statement
    ;


/* 6.8.2  Compound statement.  */

compound_statement :
        '{' block_item_list_opt '}'
    ;

block_item_list_opt : /* empty */ | block_item_list ;

block_item_list :
        block_item
    |   block_item_list block_item
    ;
     
block_item :
        declaration
    |   statement
    ;


/* 6.8.3  Expression and null statements.  */

expression_statement : expr_opt ';' ;


/* 6.8.4  Selection statements.  */

selection_statement :
        IF '(' expression ')' statement
    |   IF '(' expression ')' statement ELSE statement
    |   SWITCH '(' expression ')' statement
    ;


/* 6.8.5  Iteration statements.  */

iteration_statement :
        WHILE '(' expression ')' statement
    |   DO statement WHILE '(' expression ')' ';'
    |   FOR '(' expr_opt ';' expr_opt ';' expr_opt ')' statement
    |   FOR '(' declaration ';' expr_opt ';' expr_opt ')' statement
    ;

/* 6.8.6  Jump statements.  */

jump_statement :
        GOTO ID ';'
    |   CONTINUE ';'
    |   BREAK ';'
    |   RETURN expr_opt ';'
    ;


/* 6.9  External definitions.  */

translation_unit :
        external_declaration
    |   translation_unit external_declaration
    ;
     
external_declaration :
        function_definition
    |   declaration
    ;


/* 6.9.1  Function definitions.  */

function_definition :
        declaration_specifiers declarator
        declaration_list_opt
        compound_statement
    ;

declaration_list_opt : /* empty */ | declaration_list ;

declaration_list :
        declaration
    |   declaration_list declaration
    ;


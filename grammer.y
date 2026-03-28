%token IDENT INTEGER FLOAT CHAR STRING

%start statement_list
%%

statement_list
    : statement_list statement
    | statement
    ;

statement
    : statement_block
    | var_decl
    | for_stmt
    | while_stmt
    | if_stmt
    | expr_stmt
    | break_stmt
    | continue_stmt
    ;

var_decl
    : type IDENT ';'
    | type IDENT '=' expression ';'
    ;

for_stmt
    : 'for' '(' statement_opt ';' expression_opt ';' expression_opt ')' statement_block
    ;

while_stmt
    : 'while' '(' expression ')' statement_block
    ;

if_stmt
    : 'if' '(' expression ')' statement_block
    | 'if' '(' expression ')' statement_block 'else' statement_block
    ;

expr_stmt
    : expression ';'
    ;

break_stmt
    : 'break' ';'
    ;

continue_stmt
    : 'continue' ';'
    ;

statement_block
    : '{' statement_list '}'
    | statement
    ;

type
    : 'u8'
    | 'u16'
    | 'u32'
    | 'u64'
    | 'usize'
    | 'i8'
    | 'i16'
    | 'i32'
    | 'i64'
    | 'isize'
    | 'f32'
    | 'f64'
    | 'bool'
    ;

statement_opt
    : statement
    | empty
    ;

expression_opt
    : expression
    | empty
    ;

expression
    : assignment_expr
    ;

assignment_expr
    : conditional_expr
    | conditional_expr assignment_op assignment_expr
    ;

conditional_expr
    : logical_or_expr
    | logical_or_expr '?' expression ':' conditional_expr
    ;

logical_or_expr
    : logical_and_expr
    | logical_and_expr '||' logical_or_expr
    ;

logical_and_expr
    : bitwise_or_expr
    | bitwise_or_expr '&&' logical_and_expr
    ;

bitwise_or_expr
    : bitwise_xor_expr
    | bitwise_xor_expr '|' bitwise_or_expr
    ;

bitwise_xor_expr
    : bitwise_and_expr
    | bitwise_and_expr '^' bitwise_xor_expr
    ;

bitwise_and_expr
    : equality_expr
    | equality_expr '&' bitwise_and_expr
    ;

equality_expr
    : relational_expr
    | equality_expr equality_op relational_expr
    ;

relational_expr
    : shift_expr
    | relational_expr relational_op shift_expr
    ;

shift_expr
    : additive_expr
    | shift_expr shift_op additive_expr
    ;

additive_expr
    : multiplicative_expr
    | additive_expr additive_op multiplicative_expr
    ;

multiplicative_expr
    : unary_expr
    | multiplicative_expr multiplicative_op unary_expr
    ;

unary_expr
    : postfix_expr
    | unary_op unary_expr
    ;

postfix_expr
    : primary_expr
    | postfix_expr postfix_op
    ;

primary_expr
    : IDENT
    | INTEGER
    | FLOAT
    | CHAR
    | STRING
    | 'TRUE'
    | 'FALSE'
    | '(' expression ')'
    ;

assignment_op
    : '='
    | '+='
    | '-='
    | '*='
    | '/='
    | '%='
    | '&='
    | '^='
    | '|='
    | '<<='
    | '>>='
    ;

equality_op
    : '=='
    | '!='
    ;

relational_op
    : '<'
    | '>'
    | '<='
    | '>='
    ;

shift_op
    : '<<'
    | '>>'
    ;

additive_op
    : '+'
    | '-'
    ;

multiplicative_op
    : '*'
    | '/'
    | '%'
    ;

unary_op
    : '++'
    | '--'
    | '&'
    | '*'
    | '+'
    | '-'
    | '~'
    | '!'
    ;

postfix_op
    : '++'
    | '--'
    ;

empty : ;

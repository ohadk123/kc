%token IDENT INTEGER FLOAT CHAR STRING

%start translation_unit
%%

translation_unit
    : top_level_decl
    | translation_unit top_level_decl
    ;

top_level_decl
    : function_decl
    ;

function_decl
    : type IDENT '(' parameter_list_opt ')' statement_block
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
    | 'void'
    ;

parameter_list_opt
    : parameter_list
    | empty
    ;

parameter_list
    : type IDENT
    | parameter_list ',' type IDENT
    ;

statement_block
    : '{' statement_list '}'
    ;

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
    | return_stmt
    ;

var_decl
    : type IDENT ';'
    | type IDENT '=' expression ';'
    ;

for_stmt
    : 'for' '(' expression_opt ';' expression_opt ';' expression_opt ')' statement
    | 'for' '(' type IDENT '=' expression ';' expression_opt ';' expression_opt ')' statement
    ;

while_stmt
    : 'while' '(' expression ')' statement
    ;

if_stmt
    : 'if' '(' expression ')' statement
    | 'if' '(' expression ')' statement'else' statement
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

return_stmt
    : 'return' expression_opt ';'
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
    | postfix_expr '(' argument_list_opt ')'
    ;

argument_list_opt
    : argument_list
    | empty
    ;

argument_list
    : assignment_expr
    | argument_list ',' assignment_expr
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

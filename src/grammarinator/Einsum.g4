grammar Einsum;

start: einsum EOF;

einsum: input_list ARROW output_indices;

input_list: tensor_indices (COMMA tensor_indices)*;

// Matches between 2 and 6 characters
tensor_indices: 
      INDEX_CHAR INDEX_CHAR 
    | INDEX_CHAR INDEX_CHAR INDEX_CHAR 
    | INDEX_CHAR INDEX_CHAR INDEX_CHAR INDEX_CHAR 
    | INDEX_CHAR INDEX_CHAR INDEX_CHAR INDEX_CHAR INDEX_CHAR 
    | INDEX_CHAR INDEX_CHAR INDEX_CHAR INDEX_CHAR INDEX_CHAR INDEX_CHAR 
    ;

output_indices: tensor_indices;

INDEX_CHAR: [i-n];
ARROW: '->';
COMMA: ',';
WS: [ \t\r\n]+ -> skip;
#pragma once
typedef enum token_type {
    L_RESERVE = 1,
    L_CONSTANT,
    L_STRING,
    L_IDENTIFIER,
    L_END,
} token_type;

typedef struct lex_token {
    int line;
    int index;
    char *buf;
    token_type type;
    struct lex_token *prev;
    struct lex_token *next;
} lex_token;
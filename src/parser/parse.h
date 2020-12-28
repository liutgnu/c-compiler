#pragma once
#include <stdbool.h>
#include "../lexer/token.h"
#include <pthread.h>
#include "linked_list_buffer.h"

struct ast_token;

typedef struct parser
{
    int current_lex_index;
    int lex_length;
    lex_token *head;
    void (*advance_nonexpect)(struct parser *);
    void (*advance_expect)(struct parser *, const char *expect);
    struct ast_token *(*expression)(struct parser *, int);

    // for parser try-parse use
    pthread_t tid;
    void *status;
    bool in_try_thread;
    buf_list *try_ast_token_head;
} parser;

parser *alloc_parser(lex_token *, int);
void free_ast_tree(struct ast_token *);
lex_token *peek_token_at_index(parser *, int);
lex_token *peek_current_token(parser *);
bool try_parse_as_expression_at_index(parser *, int , int);
struct ast_token *get_current_token(parser *);
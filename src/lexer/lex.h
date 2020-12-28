#pragma once
#include "token.h"
lex_token* lex(const char *, int *);
void for_each_node_of_list(lex_token *head, void (*act)(lex_token *));
void free_all_node_of_list(lex_token *);
lex_token *get_node_by_index_of_list(lex_token *, int);
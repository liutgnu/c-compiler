#include "parse.h"
#include "ast_token.h"
#include "../lexer/token.h"
#include "../lexer/lex.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>

static void advance_expect(parser *psr, const char *expect)
{
    lex_token *token = get_node_by_index_of_list(psr->head, psr->current_lex_index);
    if (token && !strncmp(token->buf, expect, strlen(expect))) {
        psr->current_lex_index++;
    } else {
        if (!token) {
            printf("expect %s, got NULL\n", expect);
        } else {
            printf("expect %s, got %s at line %d token index %d\n",
                expect, token->buf, token->line, token->index);
        }
        exit(-1);
    }
}

static void advance_nonexpect(parser *psr)
{
    psr->current_lex_index++;
}

ast_token *get_current_token(parser *psr)
{
    ast_token *token = convert_from_lex_token
        (get_node_by_index_of_list(psr->head, psr->current_lex_index));
    return token;
}

lex_token *peek_token_at_index(parser *psr, int index)
{
    return get_node_by_index_of_list(psr->head, index);
}

lex_token *peek_current_token(parser *psr)
{
    return peek_token_at_index(psr, psr->current_lex_index);
}

// parse expression related
static ast_token *expression(parser *psr, int bp)
{
    ast_token *token = get_current_token(psr);
    if (psr->in_try_thread) {  
        // we are in the try-parse mode, so we save allocated token into list, so we can release them later
        append_value_to_buf_list(&psr->try_ast_token_head, token);
        // incase we try to parse expression on A_END, which will cause indexing of lex token overflow
        if (token->type == A_END) {
            psr->status = (void *)-1;
            pthread_exit(psr->status);
        }
    }
    advance_nonexpect(psr);
    ast_token *left = token->ops.nud(psr, token);
    ast_token *current_token;
    for (current_token = get_current_token(psr); bp < current_token->bp;) {
        token = current_token;
        if (psr->in_try_thread) {
            append_value_to_buf_list(&psr->try_ast_token_head, token);
        }
        advance_nonexpect(psr);
        left = token->ops.led(psr, token, left);
        current_token = get_current_token(psr);
    }
    if (!psr->in_try_thread) {
        // if we are in the try-parse mode, we need not free these tokens
        free(current_token->value);
        free(current_token->child_list);
        free(current_token);
    } else {
        append_value_to_buf_list(&psr->try_ast_token_head, current_token);
    }
    return left;
}

static void *try_thread(void *args)
{
    parser *psr_copy = (parser *)args;
    ast_token *root = expression(psr_copy, BP_NONE);
    psr_copy->status = (void *)0;
    pthread_exit(psr_copy->status);
}

static void free_try_ast_token(void *value)
{
    ast_token *tk = (ast_token *)(value);
    if (tk->value)
        free(tk->value);
    if (tk->child_list)
        free(tk->child_list);
    free(tk);
}

bool try_parse_as_expression_at_index(parser *psr, int bp, int index)
{
    parser parser_copy = {0};
    memcpy(&parser_copy, psr, sizeof(parser));
    /* 
    * note! try_parse_as_expression_at_index can in iteration, so parser try-parse related field need to clean up.
    * especially the <try_ast_token_head> field, other fields are always auto updated.
    */
    parser_copy.try_ast_token_head = NULL;
    parser_copy.in_try_thread = true;
    parser_copy.current_lex_index = index;
    void *status;

    pthread_create(&parser_copy.tid, NULL, try_thread, &parser_copy);
    pthread_join(parser_copy.tid, &status);

    while(parser_copy.try_ast_token_head) {
        remove_last_node_of_buf_list(&parser_copy.try_ast_token_head, free_try_ast_token);
    }

    if (!(int)status)
        return true;
    else
        return false;
}
/**********************************/

// parse statement related
static ast_token *statement(parser *psr)
{
    ast_token *token = get_current_token(psr);
    if (token->ops.std != default_std) {
        psr->advance_nonexpect(psr);
        return token->ops.std(psr, token);
    } else {
        token = psr->expression(psr, BP_NONE);
        free(token->value);
        free(token->child_list);
        free(token);
        psr->advance_expect(psr, ";");
        return token;
    }
}

/**********************************/
parser *alloc_parser(lex_token *head, int lex_length)
{
    parser *psr = (parser *)malloc(sizeof(parser));
    assert(psr != NULL);
    memset(psr, 0, sizeof(parser));
    psr->lex_length = lex_length;
    psr->head = head;
    psr->current_lex_index = 0;
    psr->advance_expect = advance_expect;
    psr->advance_nonexpect = advance_nonexpect;
    psr->expression = expression;
    return psr;
}

void free_ast_tree(ast_token *token)
{
    if (!token)
        return;
    free(token->value);
    token->value = NULL;
    for (int i = 0; i < token->child_num; ++i) {
        free_ast_tree(token->child_list[i]);
    }
    free(token->child_list);
    free(token);
}
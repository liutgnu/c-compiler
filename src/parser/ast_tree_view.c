#include "ast_token.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "linked_list_buffer.h"

static bool depth_checker(buf_list *list, void *arg)
{
    return (int)(list->value) == (int)arg;
}

static void print_token_with_indent(ast_token *token, int depth, 
    bool is_last_child, buf_list** depth_head)
{
    for (int i = 0; i < depth; ++i) {
        if (is_last_child && i == depth - 1) {
            printf("\xE2\x94\x94\xE2\x94\x80");
        } else if (!is_last_child && i == depth - 1) {
            printf("\xE2\x94\x9c\xE2\x94\x80");
        }
        else if (check_buf_list(depth_head, depth_checker, (void *)i)) {
            printf("\xE2\x94\x82 ");
        }
        else 
            printf("  ");
    }
    print_ast_token(token);
}

void print_ast_tree(ast_token *token, bool is_last_child) {
    /*
    * The following variables will stay to its origin status after iterate over a tree,
    * so no need to clean up.
    */ 
    static int depth = 0; 
    static buf_list *depth_head = NULL;

    ++depth;
    int child_len = token->child_num;
    print_token_with_indent(token, depth, is_last_child, &depth_head);
    for (int i = 0; i < child_len; ++i) {
        if (i == 0) {
            append_value_to_buf_list(&depth_head, (void *)depth);
        }
        if (i == child_len - 1) {
            remove_last_node_of_buf_list(&depth_head, NULL);
        }
        print_ast_tree(token->child_list[i], i == child_len - 1);
    }
    --depth;
}
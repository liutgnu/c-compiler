#include "../lexer/lex.h"
#include "ast_token.h"
#include "parse.h"
#include "ast_tree_view.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    lex_token *lex_head = NULL;
    ast_token *ast_root = NULL;
    parser *psr = NULL;

    struct stat st;
    char *f_buf;
    int lex_token_length;

    if (argc != 2) {
        printf("Useage: ./a.out <c_file>");
        return -1;
    }
    int fd = open(argv[1], O_RDONLY);
    assert(fd > 0);

    if (fstat(fd, &st)) {
        printf("stat file %s error!\n", argv[1]);
        return -1;
    }

    f_buf = (char *)malloc(st.st_size);
    read(fd, f_buf, st.st_size);

    lex_head = lex(f_buf, &lex_token_length);
    psr = alloc_parser(lex_head, lex_token_length);
    ast_root = psr->expression(psr, BP_NONE);
    print_ast_tree(ast_root, true);

    free_ast_tree(ast_root);
    free(psr);
    free_all_node_of_list(lex_head);
    free(f_buf);
    return 0;
}
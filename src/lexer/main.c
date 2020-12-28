#include "lex.h"
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

void print_lex_token(lex_token *token)
{
    printf("< %s >, (L,I)=%d,%d, %d\n", token->buf, token->line,
        token->index, token->type);
}

int main(int argc, char **argv)
{
    lex_token *head = NULL;
    struct stat st;
    char *f_buf;
    int lex_length;

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

    head = lex(f_buf, &lex_length);
    printf("lex token length: %d\n", lex_length);
    for_each_node_of_list(head, print_lex_token);

    free_all_node_of_list(head);
    free(f_buf);
    return 0;
}
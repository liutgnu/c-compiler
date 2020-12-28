#pragma once
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct buf_list {
    void *value;
    struct buf_list *next;
} buf_list;

void append_value_to_buf_list(buf_list **, void *);
void remove_last_node_of_buf_list(buf_list **head, void (*act)(void *));
bool check_buf_list(buf_list **head, bool (*checker)(buf_list *, void *), void *);
buf_list *alloc_buf_list(void);
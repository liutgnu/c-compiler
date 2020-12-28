
#include "linked_list_buffer.h"
#include <stdio.h>

void append_value_to_buf_list(buf_list **head, void *value)
{
    buf_list *tmp;
    buf_list *node = (buf_list *)malloc(sizeof(buf_list));
    assert(head != NULL && node != NULL);
    memset(node, 0, sizeof(buf_list));
    node->value = value;
    if (!*head) {
        *head = node;
    } else {
        for (tmp = *head; tmp->next != NULL; tmp = tmp->next);
        tmp->next = node;
    }
}

void remove_last_node_of_buf_list(buf_list **head, void (*act)(void *))
{
    buf_list *tmp;
    assert(head != NULL);
    if ((*head)->next == NULL) {
        if (act)
            act((*head)->value);
        free(*head);
        *head = NULL;
    } else {
        for (tmp = *head; tmp->next->next != NULL; tmp = tmp->next) {
        }
        if (act)
            act(tmp->next->value);
        free(tmp->next);
        tmp->next = NULL;
    }
}

bool check_buf_list(buf_list **head, bool (*checker)(buf_list *, void *), void *arg)
{
    buf_list *tmp;
    assert(head != NULL);
    for (tmp = *head; tmp != NULL; tmp = tmp->next) {
        if (checker(tmp, arg))
            return true;
    }
    return false;
}
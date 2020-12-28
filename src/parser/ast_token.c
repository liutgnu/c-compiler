#include "ast_token.h"
#include "../lexer/token.h"
#include "../lexer/lex.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include "parse.h"

// deal with token pretty print
void print_ast_token(ast_token *token)
{
    char *type = NULL;
    char *value = NULL;
    char buf[64] = {0};
    switch (token->type) {
        case A_RESERVE: {
            type = "RESERVE";
            value = (char *)token->value;
            break;
        }
        case A_CONSTANT_NUM: {
            type = "NUM";
            snprintf(buf, sizeof(buf), "%ld", *(long int *)token->value);
            value = buf;
            break;
        }
        case A_CONSTANT_CHAR: {
            type = "CHAR";
            value = (char *)&token->value;
            break;
        }
        case A_STRING: {
            type = "STRING";
            value = (char *)token->value;
            break;
        }
        case A_IDENTIFIER: {
            type = "IDENTIFIER";
            value = (char *)token->value;
            break;
        }
        case A_STATEMENTS: {
            type = "STATEMENTS";
            value = NULL;
            break;
        }
        // case A_FUNCTION: type = "FUNCTION"; break;
        default:
            assert(0);  // for debug
    }
    printf("< %s > %s\n", value, type);
}
/*********************************************/

// deal with escaped chars
static char *escape_str(const char *str, int str_len)
{
    int i, j;
    char *new_str = (char *)malloc(str_len + 1);
    assert(new_str != NULL);
    memset(new_str, 0, str_len + 1);
    for (i = 0, j = 0; i < str_len;) {
        if (*(str + i) == '\\') {
            switch (*(str + i + 1)) {
                case '\\': new_str[j++] = '\\'; break;
                case '\'': new_str[j++] = '\''; break;
                case '"': new_str[j++] = '"'; break;
                case '0': new_str[j++] = '\0'; break;
                case 'n': new_str[j++] = '\n'; break;
                case 't': new_str[j++] = '\t'; break;
                case 'v': new_str[j++] = '\v'; break;
                case 'f': new_str[j++] = '\f'; break;
                default: {
                    new_str[j++] = str[i++];
                    continue;
                }
            }
            i += 2;
        } else {
            new_str[j++] = str[i++];
        }
    }
    return new_str;
}
/*********************************************/

// functions for std

// functions for nud, led
static ast_token *condition_led(struct parser *psr, ast_token *me, ast_token *left)
{
    me->child_num = 3;
    me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
    assert(me->child_list != NULL);
    me->child_list[0] = left;
    me->child_list[1] = psr->expression(psr, me->bp);
    me->child_list[0]->parent = me;
    me->child_list[1]->parent = me;
    psr->advance_expect(psr, ":");
    me->child_list[2] = psr->expression(psr, me->bp);
    me->child_list[2]->parent = me;
    return me;
}

static bool check_lex_token_value(lex_token *token, char *value)
{
    return !strcmp(token->buf, value);
}

static int locate_parent_pair(struct parser *psr)
{
    assert(!strcmp((char *)(peek_token_at_index(psr, psr->current_lex_index - 1)->buf), "("));
    int status = 1, i;
    for (i = psr->current_lex_index; status != 0 && i < psr->lex_length; ++i) {
        if (!strcmp((char *)(peek_token_at_index(psr, i)->buf), "("))
            status += 1;
        if (!strcmp((char *)(peek_token_at_index(psr, i)->buf), ")"))
            status -= 1;
    }
    assert(i < psr->lex_length);
    return i; 
}

static ast_token *prefix_nud(struct parser *, ast_token *);
static ast_token *parent_nud(struct parser *psr, ast_token *me)
{
    int parent_end_index = locate_parent_pair(psr);

    if (try_parse_as_expression_at_index(psr, BP_UNARY, parent_end_index)) {
        // parse as type cast: (type)<expression>
        const char *operator_value = "()";
        me->child_num = parent_end_index - psr->current_lex_index;
        me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
        assert(me->child_list != NULL);
        for (int i = 0; i < me->child_num - 1; ++i) {
            me->child_list[i] = get_current_token(psr);
            me->child_list[i]->parent = me;
            psr->advance_nonexpect(psr);
            if (psr->in_try_thread) {
                append_value_to_buf_list(&psr->try_ast_token_head, me->child_list[i]);
            }
        }
        psr->advance_expect(psr, ")");
        me->child_list[me->child_num - 1] = psr->expression(psr, BP_UNARY);
        free(me->value);
        me->value = malloc(strlen(operator_value) + 1);
        memset(me->value, 0, strlen(operator_value) + 1);
        memcpy(me->value, "()", strlen(operator_value));
        return me;
    } else {
        // parse as expression: (expression)...
        ast_token *token = psr->expression(psr, BP_NONE);
        psr->advance_expect(psr, ")");
        if (!psr->in_try_thread) {
            /*
            * because if we are in try-parse, thread end will clean all allocated nodes,
            * thus we don't need to clean up here
            */
            free(me->value);
            free(me);
        }
        return token;
    }
}

static ast_token *parent_led(struct parser *psr, ast_token *me, ast_token *left)
{
    // parse as expression: (expression)...
    assert(left->type == A_IDENTIFIER); // xx(a, b, c);
    me->child_num = 2;
    me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
    assert(me->child_list != NULL);
    me->child_list[0] = left;
    me->child_list[1] = psr->expression(psr, BP_NONE);
    psr->advance_expect(psr, ")");
    me->child_list[0]->parent = me;
    me->child_list[1]->parent = me;
    return me;
}

static ast_token *bracket_led(struct parser *psr, ast_token *me, ast_token *left)
{
    me->child_num = 2;
    me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
    assert(me->child_list != NULL);
    me->child_list[0] = left;
    me->child_list[1] = psr->expression(psr, BP_NONE);
    me->child_list[0]->parent = me;
    me->child_list[1]->parent = me;
    psr->advance_expect(psr, "]");
    return me;
}

static ast_token *r_affinity_infix_led(struct parser *psr, ast_token *me, ast_token *left)
{
    me->child_num = 2;
    me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
    assert(me->child_list != NULL);
    me->child_list[0] = left;
    me->child_list[1] = psr->expression(psr, me->bp - 1);
    me->child_list[0]->parent = me;
    me->child_list[1]->parent = me;
    return me;
}

static ast_token *prefix_nud(struct parser *psr, ast_token *me)
{
    me->child_num = 1;
    me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
    assert(me->child_list != NULL);
    me->child_list[0] = psr->expression(psr, BP_UNARY);
    me->child_list[0]->parent = me;
    return me;
}

static ast_token *infix_led(struct parser *psr, ast_token *me, ast_token *left)
{
    me->child_num = 2;
    me->child_list = (ast_token **)malloc(me->child_num * sizeof(ast_token *));
    assert(me->child_list != NULL);
    me->child_list[0] = left;
    me->child_list[1] = psr->expression(psr, me->bp);
    me->child_list[0]->parent = me;
    me->child_list[1]->parent = me;

    return me;
}

static ast_token *literal_nud(struct parser *psr, ast_token *me)
{
    return me;
}

ast_token *default_nud(struct parser *psr, ast_token *me)
{
    lex_token *ltk = get_node_by_index_of_list(psr->head, psr->current_lex_index);

    if (psr->in_try_thread) {
        psr->status = (void *)-1;
        pthread_exit(psr->status);
    } else {
        printf("syntax error: no nud at line %d, token index %d\n",
            ltk->line, ltk->index);
        exit(-1);
    }
    return NULL;
}

ast_token *default_led(struct parser *psr, ast_token *me, ast_token *left)
{
    lex_token *ltk = get_node_by_index_of_list(psr->head, psr->current_lex_index);

    if (psr->in_try_thread) {
        psr->status = (void *)-1;
        pthread_exit(psr->status);
    } else {
        printf("syntax error: no led at line %d, token index %d\n",
            ltk->line, ltk->index);
        exit(-1);
    }
    return NULL;
}

ast_token *default_std(struct parser *psr, ast_token *me)
{
    lex_token *ltk = get_node_by_index_of_list(psr->head, psr->current_lex_index);
    printf("syntax error: no std at line %d, token index %d\n",
        ltk->line, ltk->index);
    exit(-1);
    return NULL;
}

/*********************************************/
static bool str_in_list(char *src, int len, ...)
{
    va_list args;
    va_start(args, len);
    for (int i = 0; i < len; ++i) {
        if (!strcmp(src, va_arg(args, char *)))
            return true;
    }
    return false;
}

ast_token *convert_from_lex_token(lex_token *ltk)
{
    ast_token *atk = (ast_token *)malloc(sizeof(ast_token));
    assert(atk != NULL);
    memset(atk, 0, sizeof(ast_token));
    atk->ops.nud = default_nud;
    atk->ops.led = default_led;
    atk->ops.std = default_std;
    switch (ltk->type) {
        case L_STRING: {
            atk->ops.nud = literal_nud;
            atk->value = escape_str(&ltk->buf[1], strlen(ltk->buf) - 2);
            if (ltk->buf[0] == '"') {
                atk->type = A_STRING;
            } else {
                assert(ltk->buf[0] == '\'');
                atk->type = A_CONSTANT_CHAR;
                char ch = *(char *)(atk->value);
                free(atk->value);
                atk->value = (void *)ch;
            }
            break;
        }
        case L_CONSTANT: {
            atk->ops.nud = literal_nud;
            atk->type = A_CONSTANT_NUM;
            atk->value = (long int *)malloc(sizeof(long int));
            assert(atk->value != NULL);
            if (ltk->buf[0] == '0') {
                if (ltk->buf[1] == 'x' || ltk->buf[1] == 'X') {
                    *(long int *)(atk->value) = strtoll(ltk->buf, NULL, 16);
                } else {
                    *(long int *)(atk->value) = strtoll(ltk->buf, NULL, 8);
                }
            } else {
                *(long int *)(atk->value) = strtoll(ltk->buf, NULL, 10);
            }
            assert(*(long int *)(atk->value) != LLONG_MIN && *(long int *)(atk->value) != LLONG_MAX);
            break;
        }
        case L_IDENTIFIER: {
            atk->ops.nud = literal_nud;
            atk->type = A_IDENTIFIER;
            atk->value = escape_str(ltk->buf, strlen(ltk->buf));
            break;
        }
        case L_END: {
            atk->type = A_END;
            break;
        }
        case L_RESERVE: {
            atk->type = A_RESERVE;
            atk->value = malloc(strlen(ltk->buf) + 1);
            assert(atk->value != NULL);
            memset(atk->value, 0, strlen(ltk->buf) + 1);
            strncpy(atk->value, ltk->buf, strlen(ltk->buf));
            
            // left affinity infix
            if (str_in_list(ltk->buf, 2, "/", "%")) {
                atk->bp = BP_FACTOR;
                goto infix;
            }
            if (str_in_list(ltk->buf, 4, ")", "}", ";", "]")) {
                atk->bp = BP_NONE;
                goto infix;
            }
            if (str_in_list(ltk->buf, 2, "==", "!=")) {
                atk->bp = BP_EQUAL;
                goto infix;
            }
            if (str_in_list(ltk->buf, 4, "<", ">", "<=", ">=")) {
                atk->bp = BP_CMP;
                goto infix;
            }
            if (str_in_list(ltk->buf, 1, "||")) {
                atk->bp = BP_LOGIC_OR;
                goto infix;
            }
            if (str_in_list(ltk->buf, 1, "&&")) {
                atk->bp = BP_LOGIC_AND;
                goto infix;
            }
            if (str_in_list(ltk->buf, 1, "|")) {
                atk->bp = BP_OR;
                goto infix;
            }
            if (str_in_list(ltk->buf, 1, "^")) {
                atk->bp = BP_XOR;
                goto infix;
            }
            if (str_in_list(ltk->buf, 2, "<<", ">>")) {
                atk->bp = BP_SHIFT;
                goto infix;
            }
            if (str_in_list(ltk->buf, 1, ",")) {
                atk->bp = BP_COMMA;
                goto infix;
            }
            if (str_in_list(ltk->buf, 2, ".", "->")) {
                atk->bp = BP_HIGHEST;
                goto infix;
            }
            // infix and prefix
            if (str_in_list(ltk->buf, 1, "&")) {
                atk->bp = BP_AND;
                goto prefix_and_infix;
            }
            if (str_in_list(ltk->buf, 2, "+", "-")) {
                atk->bp = BP_TERM;
                goto prefix_and_infix;
            }
            if (str_in_list(ltk->buf, 1, "*")) {
                atk->bp = BP_FACTOR;
                goto prefix_and_infix;
            }
            if (str_in_list(ltk->buf, 2, "++", "--")) {
                atk->bp = BP_HIGHEST;
                goto prefix_and_infix;
            }
            // right affinity infix
            if (str_in_list(ltk->buf, 11, 
                "=", "/=", "*=", "%=", "+=", "-=", "<<=", ">>=", "&=", "|=", "^=")) {
                atk->bp = BP_ASSIGN;
                goto r_affinity_infix;
            }
            // prefix
            if (str_in_list(ltk->buf, 3, "!", "~", "sizeof")) {
                goto prefix;
            }
            // parent
            if (str_in_list(ltk->buf, 1, "(")) {
                atk->bp = BP_HIGHEST;
                atk->ops.led = parent_led;
                atk->ops.nud = parent_nud;
                break;
            }
            // bracket
            if (str_in_list(ltk->buf, 1, "[")) {
                atk->bp = BP_HIGHEST;
                atk->ops.led = bracket_led;
                break;
            }
            // ?:
            if (str_in_list(ltk->buf, 1, "?")) {
                atk->bp = BP_CONDITION;
                atk->ops.led = condition_led;
                break;
            }
            assert(0);
infix:
            atk->ops.led = infix_led;
            break;
prefix:
            atk->ops.nud = prefix_nud;
            break;
prefix_and_infix:
            atk->ops.led = infix_led;
            atk->ops.nud = prefix_nud;
            break;
r_affinity_infix:
            atk->ops.led = r_affinity_infix_led;
            break;
            // reserve keywords
        }
        default:
            assert(0);
    }
    return atk;
}
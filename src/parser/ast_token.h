#pragma once
#include "parse.h"
#include "../lexer/token.h"

typedef enum ast_token_type {
    A_RESERVE = 1,
    A_CONSTANT_NUM,
    A_CONSTANT_CHAR,
    A_STRING,
    A_IDENTIFIER,
    A_STATEMENTS,
    A_FUNCTION,
    A_END
} ast_token_type;

typedef enum bp_type {
    BP_NONE = 0,        // no binding power

    // The lower the list, the higher the power
    BP_COMMA = 5,       // left_affinity ,      done
    BP_ASSIGN = 10,     // right_affinity = /= *= %= += -= <<= >>= &= |= ^=     done
    BP_CONDITION = 15,  // right_affinity ?:    done
    BP_LOGIC_OR = 20,   // left_affinity ||     done
    BP_LOGIC_AND = 25,  // left_affinity &&     done
    BP_OR = 30,         // left_affinity |      done
    BP_XOR = 33,        // left_affinity ^      done
    BP_AND = 36,        // left_affinity &      done
    BP_EQUAL = 40,      // left_affinity == !=  done
    BP_CMP = 45,        // left_affinity < > <= >=  done
    BP_SHIFT = 47,      // left_affinity << >>  done
    BP_TERM = 50,       // left_affinity + -    done
    BP_FACTOR = 60,	    // left_affinity * / %  done
    BP_UNARY = 70,      // right_affinity + - ! ~ & * ++ -- sizeof (type) todo
    BP_HIGHEST = 100,   // left_affinity . () [] -> ++ -- done
} bp_type;

typedef struct ast_token {
    int child_num;
    int bp;
    ast_token_type type;
    void *value;

    struct ast_token *parent;
    struct ast_token **child_list;

    struct ops {
        struct ast_token *(*nud)(parser *, struct ast_token *);
        struct ast_token *(*led)(parser *, struct ast_token *, struct ast_token *);
        struct ast_token *(*std)(parser *, struct ast_token *);
    } ops;

} ast_token;

ast_token *convert_from_lex_token(lex_token *);
void print_ast_token(ast_token *);
ast_token *default_nud(struct parser *, ast_token *);
ast_token *default_led(struct parser *, ast_token *, ast_token *);
ast_token *default_std(struct parser *, ast_token *);
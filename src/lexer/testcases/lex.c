// linked list related
static void append_to_end_of_list(lex_token **head, lex_token *tk)
{
	if (!*head) {
		*head = tk;
		tk->next = *head;
		tk->prev = *head;
	} else {
		tk->next = *head;
		tk->prev = (*head)->prev;
		(*head)->prev->next = tk;
		(*head)->prev = tk;
	}
}

static void remove_last_node_of_list(lex_token **head, void (*act)(lex_token *))
{
    lex_token *tmp;
    if (!*head) {
        return;
    }
    if ((*head)->prev == (*head)) {
        (*act)(*head);
        *head = NULL;
    } else {
        tmp = (*head)->prev;
        tmp->prev->next = tmp->next;
        tmp->next->prev = tmp->prev;
        (*act)(tmp);
    }
}

void iterate_over_list(lex_token *head, void (*act)(lex_token *))
{
    lex_token *tmp;
    if (!head) {
        return;
    }
    tmp = head;
    (*act)(tmp);
    for (tmp = tmp->next; tmp != head; tmp = tmp->next) {
        (*act)(tmp);
    }
}

static void clean_up_token(lex_token *token)
{
    if(token) {
        free(token->buf);
    }
    free(token);
}

void free_all_node_of_list(lex_token *head)
{
    while(head) {
        remove_last_node_of_list(&head, clean_up_token);
    }
}
/*********************************************/

// reserve keyword related
static const char *reserve_token_list[] = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "int",
    "long",
    "register",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
};

static bool in_reserve_list(const char *p)
{
    int i;
    for (i = 0; i < sizeof(reserve_token_list) / sizeof(const char *); ++i) {
        if (!strncmp(reserve_token_list[i], p, strlen(reserve_token_list[i])))
            return true;
    }
    return false;
}
/*********************************************/

// checkers related
static bool is_blank(const char *p)
{
    return (*p == ' ') || (*p == '\t') || (*p == '\v') || (*p == '\f');
}

static bool is_letter(const char *p)
{
    return ((*p >= 'a') && (*p <= 'z')) || ((*p >= 'A') && (*p <= 'Z')) || (*p == '_');
}

static bool is_digit(const char *p)
{
    return (*p >= '0') && (*p <= '9');
}

static bool is_hex(const char *p)
{
    return ((*p >= 'a') && (*p <= 'f')) || ((*p >= 'A') && (*p <= 'F')) || is_digit(p);
}
/*********************************************/

// alloc lex_token related
static lex_token *alloc_lex_token(int line, int index, const char *p, 
                                int length, token_type type)
{
    lex_token *token = NULL;
    token = (lex_token *)malloc(sizeof(lex_token));
    assert(token != NULL);
    memset(token, 0, sizeof(lex_token));
    token->line = line;
    token->index = index;
    token->type = type;
    if (length >= 1) {
        token->buf = (char *)malloc(length + 1);
        assert(token->buf != NULL);
        memset(token->buf, 0, length + 1);
        strncpy(token->buf, p, length);
    }
    return token;
}
/*********************************************/

static lex_token *get_token(const char *curser, int *skip, int *line, int *index)
{
    lex_token *token = NULL;
    if (is_blank(curser)) { // [ \t\v\f]
        int i;
        for (i = 0; is_blank(curser + i); ++i);
        *skip = i;
        return NULL;
    }
    if (is_letter(curser)) { // {L}({L}|{D})*
        int i;
        for (i = 0; is_letter(curser + i) || is_digit(curser + i); ++i);
        *skip = i;
        if (in_reserve_list(curser)) { // reserved keyword
            token = alloc_lex_token(*line, *index, curser, i, RESERVE);
        } else { // identifier
            token = alloc_lex_token(*line, *index, curser, i, IDENTIFIER);
        }
        *index += 1;
        return token;
    }
    if (is_digit(curser)) { // 0{D}+, {D}+, 0[xX]{H}+
        int i;
        for (i = 0; is_digit(curser + i); ++i);
        if (i == 1 && (*(curser + i) == 'x' || *(curser + i) == 'X')) { // hex
            for (i = i + 1; is_digit(curser + i) || is_hex(curser + i); ++i);
        }
        *skip = i;
        token = alloc_lex_token(*line, *index, curser, i, CONSTANT);
        *index += 1;
        return token;
    }
    switch(*curser) {
        case '\n': {
            *line += 1;
            *index = 0;
            *skip = 1;            
            return NULL;
        }
        case '"': { // \"(\\.|[^\\"])*\"
            int i;
            for (i = 1; *(curser + i) != '\0'; ++i) {
                if (*(curser + i) == '"') {  
                    int count = 0;
                    for (int j = i - 1; *(curser + j) == '\\'; --j, ++count);
                    if (count % 2 == 0) {
                        *skip = i + 1;
                        *index += 1;
                        return alloc_lex_token(*line, *index, curser, i + 1, STRING);
                    }
                }
            }
            printf("not pair \" line %d, token index %d\n", *line, *index);
            exit(-1); 
        }
        case '\'': { // '(\\.|[^\\'])+'
            int  i;
            for (i = 1; *(curser + i) != '\0' && i < 4; ++i) {
                if (*(curser + i) == '\'') {
                    if (i == 1) { // ''
                        printf("empty \'\' line %d, token index %d\n", *line, *index);
                        exit(-1);                         
                    } else {
                        int count = 0;
                        for (int j = i - 1; *(curser + j) == '\\'; --j, ++count);
                        if (count % 2 == 0) {
                            *skip = i + 1;
                            *index += 1;
                            return alloc_lex_token(*line, *index, curser, i + 1, STRING);
                        }
                    }
                }
            }
            printf("not pair \' line %d, token index %d\n", *line, *index);
            exit(-1);
        }
        case '>': {
            switch (*(curser + 1)) {
                case '>': {     // '>>'
                    if (*(curser + 2) == '=') // '>>='
                        goto len3;
                }
                case '=':       // '>='
                    goto len2;
                default:        // '>'
                    goto len1;
            }
        }
        case '<': {
            switch (*(curser + 1)) {
                case '<': {     // '<<'
                    if (*(curser + 2) == '=') // '<<='
                        goto len3;
                }
                case '=':       // '<='
                    goto len2;
                default:        // '<'
                    goto len1;
            }
        }
        case '-': {
            switch (*(curser + 1)) {
                case '=':       // -=
                case '>':       // ->
                case '-':       // --
                    goto len2;
                default:        // -
                    goto len1;
            }
        }
        case '+': {
            switch (*(curser + 1)) {
                case '+':       // ++
                case '=':       // +=
                    goto len2;
                default:        // +
                    goto len1;
            }
        }
        case '/': {
            switch (*(curser + 1)) {
                case '=':       // /=
                    goto len2;
                case '*': {     // /*
                    int i;
                    for (i = 2; *(curser + i) != '\0'; ++i) {
                        if (!strncmp("*/", curser + i, 2)) {
                            *skip = i + 2;
                            return NULL;
                        }
                        if (*(curser + i) == '\n') {
                            *line += 1;
                            *index = 0;
                        }
                    }
                }
                case '/': {
                    int i;
                    for (i = 1; *(curser + i) != '\0'; ++i) {
                        if (*(curser + i) == '\n') {
                            *skip = i + 1;
                            *line += 1;
                            *index = 0;
                            return NULL;
                        }
                    }
                }
                default:        // /
                    goto len1;
            }
        }
        case '&': {
            switch (*(curser + 1)) {
                case '&':       // &&
                case '=':       // &=
                    goto len2;
                default:        // &
                    goto len1;
            }
        }
        case '|': {
            switch (*(curser + 1)) {
                case '=':       // |=
                case '|':       // ||
                    goto len2;
                default:        // |
                    goto len1;
            }
        }
        case '.': {
            switch (*(curser + 1)) {
                case '.': {
                    switch (*(curser + 2)) {
                        case '.':   // ...
                            goto len3;
                        default:
                            printf("unknown .. line %d, token index %d\n", *line, *index);
                            exit(-1);
                    }
                }
                default:        // .
                    goto len1;
            }
        }
        case '%':
        case '^':
        case '!':
        case '=':
        case '*': {
            switch (*(curser + 1)) {
                case '=':       // ?=
                    goto len2;
                default:        // ?
                    goto len1;
            }
        }
        case '[':
        case ']':
        case '{':
        case '}':
        case '(':
        case ')':
        case '?':
        case '~':
        case ',':
        case ':':
        case ';':
len1:
        *skip = 1;
        ++*index;
        return alloc_lex_token(*line, *index, curser, 1, RESERVE);
len2:
        *skip = 2;
        ++*index;
        return alloc_lex_token(*line, *index, curser, 2, RESERVE);
len3:
        *skip = 3;
        ++*index;
        return alloc_lex_token(*line, *index, curser, 3, RESERVE);

        default: //correct char should never reach here
            printf("unknown char %c at line %d, token index %d\n", *curser, *line, *index);
            exit(-1);
    }
}

lex_token* lex(const char *src)
{
    int skip;
    lex_token *head = NULL;
    int line = 1;
    int index = 0;
    const char *curser = src;

    while (*curser != '\0') {
        lex_token *token = get_token(curser, &skip, &line, &index);
        if (token) {
            append_to_end_of_list(&head, token);
        }
        curser += skip;
    }
    return head;
}
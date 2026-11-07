#include "pun.h"

#define token_arr_last(ta) (ta.tokens[ta.size - 1])

static token new_token(token_kind kind, char *start, char *end)
{
    token t;
    t.kind = kind;
    t.loc = start;
    t.len = end - start;
    t.val = 0;
    return t;
}

static token_arr new_token_arr()
{
    token_arr ta;
    ta.size = 0;
    ta.capacity = 32;
    ta.tokens = calloc(ta.capacity, sizeof(token));
    return ta;
}

static void token_arr_add(token_arr *ta, token t)
{
    if (ta->size == ta->capacity)
    {
        ta->capacity *= 2;
        ta->tokens = realloc(ta->tokens, ta->capacity * sizeof(token));
    }
    
    ta->tokens[ta->size] = t;
    ta->size++;
}

bool token_equal(token t, char *c)
{
    return memcmp(t.loc, c, t.len) == 0 && c[t.len] == '\0';
}

token token_peek(token_arr *ta, size_t pos)
{
    return ta->tokens[pos];
}

static char *read_file(const char *path)
{
    int file_size;
    char *buf;
    size_t bytes_read;
    FILE *f = fopen(path, "r");

    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    rewind(f);

    buf = calloc(1, file_size + 1);
    bytes_read = fread(buf, 1, file_size, f);
    buf[bytes_read] = '\0';

    return buf;
}

static char punct_len(char *p)
{
    if (strncmp(p, "->", 2) == 0 || (strncmp(p, "::", 2) == 0) || (strncmp(p, "==", 2) == 0) ||
        (strncmp(p, "~=", 2) == 0) || (strncmp(p, "<=", 2) == 0) || (strncmp(p, ">=", 2) == 0) ||
        (strncmp(p, "<:", 2) == 0) || (strncmp(p, ":>", 2) == 0) || strncmp(p, "./", 2) == 0)
        return 2;

    return 1;
}

static token_arr tokenize(char *p)
{
    token_arr ta = new_token_arr();

    while (*p)
    {
        if (isspace(*p) || isalpha(*p))
        {
            p++;
            continue;
        }

        if (isdigit(*p))
        {
            char *start = p;
            while (isdigit(*p))
                p++;
            
            token_arr_add(&ta, new_token(TOKEN_IDENT, start, p));
            continue;
        }
        
        if (ispunct(*p))
        {
            if (*p == '&' && isdigit(*(p + 1)))
            {
                char *start = p;
                token tok = new_token(TOKEN_NUM, start, p);
                token_arr_add(&ta, tok);
                token_arr_last(ta).val = strtoul(p + 1, &p, 10);
                token_arr_last(ta).len = p - start;
                continue;
            }
            else
            {
                char *start = p;
                p += punct_len(p);
                token_arr_add(&ta, new_token(TOKEN_PUNCT, start, p));
                continue;
            }
        }
        
        p++;
    }
    
    ta.tokens[ta.size] = new_token(TOKEN_EOF, p, p);
    return ta;
}

token_arr tokenize_file(const char *path)
{
    return tokenize(read_file(path));
}

/*
 * ie.h — Indented Expressions: a syntax (like JSON), not a language.
 * Reads ie source and produces a flat token stream, one token at a time.
 *
 * Single-header library, stb/nob style. In exactly one .c file:
 *
 *     #define IE_IMPLEMENTATION
 *     #include "ie.h"
 *
 * Single-instance: it uses global state, so it is not reentrant or
 * thread-safe. Each ie_tokenize() call resets that state, so sequential
 * calls are fine.
 */
#ifndef IE_H
#define IE_H

#include <stdio.h> /* FILE */

/* Called once per token. `user` is passed straight through from ie_tokenize. */
typedef void (*Ie_Token_Fn)(const char *token, void *user);

/* Tokenize ie source read from `input`, invoking on_token for each token.
 * Returns 0 on success, 1 on a parse error (message printed to stderr). */
int ie_tokenize(FILE *input, Ie_Token_Fn on_token, void *user);

/* Open `path` ("-" is stdin), tokenize it, close it. Same return as above. */
int ie_tokenize_path(const char *path, Ie_Token_Fn on_token, void *user);

#endif /* IE_H */


#ifdef IE_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define ESC "\x1b"
#define RED_TEXT "31"
#define YELLOW_TEXT "33"
#define RESET ESC "[0m"

#define CTEXT(c, s) ESC "[" c "m" s RESET

#define ere \
    do { \
        fprintf(stderr, \
                CTEXT(YELLOW_TEXT, "\n%s : %d : %s\n"), \
                __FILE__, __LINE__, __func__); \
    } while (0);

/* fully variadic so __VA_ARGS__ is never empty: avoids the GNU ,##__VA_ARGS__
 * extension and stays portable across c99 / clang / MSVC. die() is always
 * called with at least a (literal) format string. */
#define die(...) \
    do { \
        fprintf(stderr, ESC "[" RED_TEXT "m" "\nerror: "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, RESET "\n"); \
        ere; \
        longjmp(ie__err, 1); \
    } while (0);

#define debug_var(s,v) \
    fprintf(stderr, #v ": %" s "\n", v)

typedef unsigned int uint;
typedef void (void_fn)(void);

static jmp_buf ie__err;

static int echo_newlines = 1;
static int cur_indent = 0;
static int new_indent = 0;
static uint indent_width = 4;

static char *in = NULL;
static char line_buffer[256] = "";

static char token_buffer[256] = "";
static char next_token_buffer[256] = "";

static FILE *f = NULL;

static int prefix_index = 0;
static char prefix_chars[64] = "";
static void (*prefix_fns[64])(void);

static char *token_breakchars = " ,()[]{}\n";

static int state_index = 0;
static void (*state_fns[16])(void);

static int wrapped_index = 0;
static char *wrapped[64];

void_fn * lookup_prefix(char c);

char* next_word(void);

#define LIST_OF_STATES \
    X(prefix_head) \
    X(prefix_body) \
    X(prefix_newline) \
    X(prefix_indent) \
    X(prefix_end) \
    X(inline_body) \
    X(inline_prefix) \
    X(inline_prefix_end) \
    X(inline_infix) \
    X(inline_infix_end) \
    X(inline_postfix) \
    X(inline_postfix_end) \
    X(neoteric) \
    X(neoteric_end)

#define X(s) \
    void s(void);
LIST_OF_STATES
#undef X


void
push_state(void (*fn)(void))
{
    int max_depth = sizeof(state_fns) / sizeof(state_fns[0]);
    state_index += 1;
    if (state_index >= max_depth)
        die("nesting too deep: state stack overflow (max %d)", max_depth);
    state_fns[state_index] = fn;
}


void
debug_stack(void)
{
    int i = 0;

    if (state_index < 0)
        die("cmd stack underflow");

    /*fprintf(stderr, "%s", line_buffer);*/
    /*debug_var("p", in);*/
    /*debug_var("zu", in - line_buffer);*/
    /*debug_var("s", line_buffer);*/
    /*debug_var("s", token_buffer);*/


    fprintf(stderr, "\nstack:\n");
    for (i = 0; i <= state_index; i += 1) {
        char *fn = NULL;

        if (state_fns[i] == NULL) {
            fn = "(null)";

#define X(s) \
    } else if (state_fns[i] == s) { \
        fn = #s;

        LIST_OF_STATES
        } else {
            debug_var("p", state_fns[i]);
            die("other");
        }
#undef X
        fprintf(stderr, "    %d, %s\n", i, fn);
    }
    fprintf(stderr, "\n");
}


void
chomp(char c)
{
    while (*in == c)
        in += 1;
}


char *
read_line(void)
{
    /*ere;*/
    char *r = fgets(line_buffer, 256, f);
    if (r == NULL)
        line_buffer[0] = '\0';
    in = &line_buffer[0];
    /*ere;*/
    /*fprintf(stderr, "line:\n%s\n", line_buffer);*/
    return r;
}


void
read_token(void)
{
    char *tok;
    int tok_len = 0;

    if (*in == '\0')
        if (!read_line())
            return;

    tok = in;

    while (!strchr(token_breakchars, *in))
        in += 1;

    tok_len = in - tok;

    /*ere;*/
    if (tok_len == 0) {
        if (*in == ' ')
            die("what?");
        in += 1;
        tok_len = 1;
    }

    chomp(' ');

    if (tok_len) {
        strncpy(token_buffer, tok, tok_len);
        token_buffer[tok_len] = '\0';
    } else {
        token_buffer[0] = '\0';
    }
}


void
read_string(void)
{
    char *tok;
    int tok_len = 0;

    /*ere;*/
    /*debug_var("d", *in);*/

    if (*in == '\0')
        if (!read_line())
            return;

    tok = in;

    in += 1;

    while (*in != '"' && *in != '\n' && *in != '\0') {
        if (*in == '\\') {
            in += 1;
            if (*in == '\n' || *in == '\0')
                die("invalid char:%d, '%c'", *in, *in);
        } else {
            in += 1;
        }
    }

    if (*in != '"')
        die("invalid char:%d, '%c'", *in, *in);

    in += 1;

    tok_len = in - tok;

    chomp(' ');

    if (tok_len) {
        strncpy(token_buffer, tok, tok_len);
        token_buffer[tok_len] = '\0';
    } else {
        token_buffer[0] = '\0';
    }

    /*ere;*/
    /*debug_token();*/
}


void
peek_token(void)
{
    char *old = in;
    read_token();
    in = old;
}


void
parse_indent(void)
{
    /*ere;*/
    char *first_char = NULL;
    uint diff = 0;

    if (*in == '\0')
        read_line();

    while(*in == '\n')
        if (read_line() == NULL) {
            new_indent = 0;
            return;
        }

    first_char = in;
    chomp(' ');

    diff = in - first_char;
    /*ere;*/
    /*debug_var("d", diff);*/
    if (diff % indent_width != 0)
        die("invalid indent");

    new_indent = diff / indent_width;
    return;
}


void
neoteric(void)
{
    state_fns[state_index] = neoteric_end;
    inline_body();
}


void
neoteric_end(void)
{
    /*ere;*/
    strncpy(token_buffer, "]", 256);
    state_index -= 1;
}


void
is_neoteric(void)
{
    /*ere;*/
    /*debug_var("s", line_buffer);*/
    /*debug_var("d", in > line_buffer);*/
    /*debug_var("p", in);*/
    /*debug_var("d", *in);*/
    /*debug_var("d", strchr("([{", *in));*/
    if (*in != '\0' &&
        strchr("([{", *in) &&
        *(in - 1) != ' ') {
        /*ere;*/
        /*debug_var("c", *in);*/
        /*debug_token();*/
        /*debug_stack();*/
        strncpy(next_token_buffer, token_buffer, 256);
        strncpy(token_buffer, "[", 256);
        push_state(neoteric);
    }
}

void
prefix_body(void)
{
    /*ere;*/
    void (*prefix_fn)(void) = NULL;

    if (*in == '\0') {
        /* EOF */
        state_fns[state_index] = prefix_end;
        prefix_end();
        return;
    }

    if (*in == ' ')
        die("space")

    if (*in == '\n') {
        state_fns[state_index] = prefix_newline;
        prefix_newline();
        return;
    }

    if ((prefix_fn = lookup_prefix(*in))) {
        prefix_fn();
        return;
    }

    read_token();
    is_neoteric();
}


void
prefix_newline(void)
{
    if (*in == '\0') {
        strncpy(token_buffer, "", 256);
        return;
    }

    if (*in != '\n') {
        /*debug_var("d", *in);*/
        die("opps");
    }

    /*ere;*/
    in += 1;

    state_fns[state_index] = prefix_indent;

    if (echo_newlines)
        strncpy(token_buffer, "newline", 256);
    else
        prefix_indent();

    return;
}


void
prefix_indent(void)
{
    /*ere;*/
    /*debug_var("d", cur_indent);*/

    parse_indent();
    int diff = new_indent - cur_indent;
    /*ere;*/
    /*debug_var("d", new_indent);*/
    /*debug_var("d", diff);*/

    peek_token();

    if (!strcmp("\\", token_buffer)) {
        if (diff > 1) {
            die(">1");
        } else if (diff == 1) {
            new_indent = cur_indent;
            read_token();
            state_fns[state_index] = prefix_body;
            prefix_body();
            return;
        } else if (diff == 0) {
            die("0");
        } else {
            debug_var("d", diff);
            die("?");
        }
    }

    if (diff > 1) {
        die(">1");
    } else if (diff == 1) {
        cur_indent = new_indent;
        state_fns[state_index] = prefix_end;
        push_state(prefix_head);
        prefix_head();
        return;
    }

    state_fns[state_index] = prefix_end;
    prefix_end();

}


void
prefix_end(void)
{
    /*ere;*/
    /*debug_var("d", cur_indent);*/
    /*debug_var("d", new_indent);*/
    /*debug_stack();*/

    strncpy(token_buffer, "]", 256);

    if (new_indent == cur_indent) {
        state_fns[state_index] = prefix_head;
    } else if (new_indent < cur_indent) {
        cur_indent  -= 1;
        state_index -= 1;
    } else {
        die("opps");
    }

    return;
}


int
is_wrapped(char *s)
{
    int i;

    for (i = 0; i < wrapped_index; i += 1)
        if (!strncmp(wrapped[i], s, 256))
            return -1;

    return 0;
}


void
prefix_head(void)
{
    /*ere;*/
    /*debug_var("d", *in);*/
    peek_token();

    if (*in == '\0') {
        /*ere;*/
        /*debug_var("d", cur_indent);*/
        /*debug_var("d", new_indent);*/

        if(cur_indent) {
            new_indent = 0;
            state_fns[state_index] = prefix_end;
            prefix_end();
            return;
        }

        token_buffer[0] = '\0';
        return;
    }

    strncpy(token_buffer, "[", 256);
    state_fns[state_index] = prefix_body;
}


void
inline_prefix(void)
{
    in += 1;
    chomp(' ');

    strncpy(token_buffer, "[", 256);
    push_state(inline_body);
}


void
inline_prefix_end(void)
{
    in += 1;
    chomp(' ');

    strncpy(token_buffer, "]", 256);
    state_index -= 1;
    return;
}


void
inline_infix(void)
{
    /*ere;*/
    /*debug_var("c", *in);*/

    in += 1;
    chomp(' ');

    /*debug_var("c", *in);*/

    /*ere;*/
    /*read_token();*/
    /*debug_token();*/

    if (is_wrapped(token_buffer)) {
        die("wrapped");
    }

    strncpy(token_buffer, "(", 256);
    push_state(inline_body);
    /*debug_stack();*/
    /*debug_token();*/
}


void
inline_infix_end(void)
{
    /*ere;*/
    /*debug_stack();*/

    in += 1;
    chomp(' ');

    strncpy(token_buffer, ")", 256);
    state_index -= 1;
}


void
inline_postfix(void)
{
    in += 1;
    chomp(' ');
    strncpy(token_buffer, "{", 256);
    push_state(inline_body);
}


void
inline_body(void)
{
    /*ere;*/
    void_fn *prefix_fn = NULL;

    /*debug_var("c", *in);*/

    if ((prefix_fn = lookup_prefix(*in))) {
        /*ere;*/
        prefix_fn();
        /*ere;*/
        return;
    }

    read_token();
    is_neoteric();
}


void
inline_postfix_end(void)
{
    in += 1;
    chomp(' ');
    state_index -= 1;
    strncpy(token_buffer, "}", 256);
}


void
define_wrap(char *s)
{
    int max_size = sizeof(wrapped) / sizeof(wrapped[0]);

    if (wrapped_index >= max_size)
        die("wrapped overflow");

    wrapped[wrapped_index] = s;
    wrapped_index += 1;
}


void_fn *
lookup_prefix(char c)
{
    int i;
    for (i = 0; i < prefix_index; i += 1)
        if (c == prefix_chars[i])
            return prefix_fns[i];
    return NULL;
}


void
define_prefix(char c, void (*fn)(void))
{
    int max_size = sizeof(prefix_chars);

    if (prefix_index >= max_size)
        die("prefix overflow");

    prefix_chars[prefix_index] = c;
    prefix_fns[prefix_index]   = fn;
    prefix_index += 1;
}


char *
next_word(void)
{
    void_fn *fn;

    /*ere;*/
    /*debug_stack();*/

    if (next_token_buffer[0] != '\0') {
        /*ere;*/
        strncpy(token_buffer, next_token_buffer, 256);
        next_token_buffer[0] = '\0';
        return token_buffer;
    }

    if (state_index < 0) {
        /*ere;*/
        debug_stack();
        die("state underflow");
    }
    /*ere;*/

    if ((fn = state_fns[state_index]) == NULL)
        die("fn is NULL");

    fn();
    return token_buffer;
}


/* reset all global state so ie_tokenize() can be called repeatedly */
static void
ie__reset(void)
{
    cur_indent = 0;
    new_indent = 0;
    line_buffer[0] = '\0';
    token_buffer[0] = '\0';
    next_token_buffer[0] = '\0';

    prefix_index = 0;
    define_prefix('[', inline_prefix);
    define_prefix(']', inline_prefix_end);
    define_prefix('(', inline_infix);
    define_prefix(')', inline_infix_end);
    define_prefix('{', inline_postfix);
    define_prefix('}', inline_postfix_end);
    define_prefix('"', read_string);

    wrapped_index = 0;
    memset(wrapped, 0, sizeof(wrapped));
    define_wrap(":");

    in = line_buffer;
    state_index = 0;
    state_fns[state_index] = prefix_head;
}


int
ie_tokenize(FILE *input, Ie_Token_Fn on_token, void *user)
{
    if (setjmp(ie__err))
        return 1;   /* a die() landed here */

    f = input;
    ie__reset();

    while (next_word(), token_buffer[0] != '\0')
        on_token(token_buffer, user);

    if (state_index) {
        debug_stack();
        die("unexpected end of input");
    }

    return 0;
}


int
ie_tokenize_path(const char *path, Ie_Token_Fn on_token, void *user)
{
    FILE *input;
    int rc;

    if (!strcmp(path, "-")) {
        input = stdin;
    } else if ((input = fopen(path, "r")) == NULL) {
        fprintf(stderr, "ie: cannot open %s\n", path);
        return 1;
    }

    rc = ie_tokenize(input, on_token, user);

    if (input != stdin)
        fclose(input);

    return rc;
}

#undef ESC
#undef RED_TEXT
#undef YELLOW_TEXT
#undef RESET
#undef CTEXT
#undef ere
#undef die
#undef debug_var
#undef LIST_OF_STATES

#endif /* IE_IMPLEMENTATION */


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

#include <string.h>
#include <setjmp.h>

#define ESC "\x1b"
#define RED_TEXT "31"
#define RESET ESC "[0m"

/* fully variadic so __VA_ARGS__ is never empty: avoids the GNU ,##__VA_ARGS__
 * extension and stays portable across c99 / clang / MSVC. die() is always
 * called with at least a (literal) format string. */
#define die(...) \
    do { \
        fprintf(stderr, ESC "[" RED_TEXT "m" "\nerror: "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, RESET "\n"); \
        longjmp(ie__err, 1); \
    } while (0)

#define INDENT 4

/*
 * The tokenizer is a state machine with an explicit stack. Each call to the
 * state on top of the stack emits exactly one token into token_buffer (an
 * empty token ends the stream). Indentation pushes prefix_* states, brackets
 * push inline_body, and a word directly followed by a bracket ("f(x)") pushes
 * neoteric, which wraps the call in [ ].
 */
static jmp_buf ie__err;
static FILE *f;

static char line_buffer[256];
static char *in;                   /* read position in line_buffer */

static char token_buffer[256];
static char next_token_buffer[256];   /* a word held back by neoteric */

static int cur_indent;
static int new_indent;

static void (*state_fns[16])(void);
static char state_close[16];       /* closer expected by an inline_body state */
static int state_index;

static const char breakchars[] = " ,()[]{}\n";

static void prefix_head(void);
static void prefix_body(void);
static void prefix_indent(void);
static void prefix_end(void);
static void inline_body(void);
static void neoteric(void);
static void neoteric_end(void);


static void
emit(const char *s)
{
    strcpy(token_buffer, s);
}


static void
chomp(char c)
{
    while (*in == c)
        in += 1;
}


/* the token is tok..in: copy it out and skip the spaces after it */
static void
take(const char *tok)
{
    size_t n = in - tok;
    memcpy(token_buffer, tok, n);
    token_buffer[n] = '\0';
    chomp(' ');
}


static void
set_state(void (*fn)(void))
{
    state_fns[state_index] = fn;
}


static void
push_state(void (*fn)(void))
{
    int max_depth = sizeof(state_fns) / sizeof(state_fns[0]);
    state_index += 1;
    if (state_index >= max_depth)
        die("nesting too deep: state stack overflow (max %d)", max_depth);
    state_fns[state_index] = fn;
}


static int
read_line(void)
{
    in = line_buffer;
    if (fgets(line_buffer, sizeof(line_buffer), f) == NULL) {
        line_buffer[0] = '\0';
        return 0;
    }
    if (!strchr(line_buffer, '\n') && getc(f) != EOF)
        die("line too long (max %d chars)", (int)sizeof(line_buffer) - 2);
    return 1;
}


/* Read the next line's indent into new_indent. Returns 1 if the line was
 * blank (empty or only spaces) and has been consumed: the caller emits its
 * newline token and calls again. At EOF new_indent is 0. */
static int
parse_indent(void)
{
    char *first_char;

    for (;;) {
        if (*in == '\0' && !read_line()) {
            new_indent = 0;
            return 0;
        }

        first_char = in;
        chomp(' ');

        if (*in == '\n') {
            in += 1;
            return 1;
        }
        if (*in != '\0')
            break;
        /* spaces then EOF with no newline: no newline token */
    }

    if ((in - first_char) % INDENT != 0)
        die("indent must be a multiple of %d spaces", INDENT);

    new_indent = (in - first_char) / INDENT;
    return 0;
}


static void
read_string(void)
{
    const char *tok = in;

    in += 1;
    while (*in != '"' && *in != '\n' && *in != '\0') {
        if (*in == '\\' && (in[1] == '\n' || in[1] == '\0'))
            die("unterminated string: \\ at end of line");
        in += 1;
    }

    if (*in != '"')
        die("unterminated string");

    in += 1;
    take(tok);
}


static void
read_word(void)
{
    const char *tok = in;

    while (!strchr(breakchars, *in))
        in += 1;
    if (in == tok)
        in += 1;   /* a lone breakchar, e.g. "," */
    take(tok);

    /* "f(" with no space: emit [ now, the word next, and wrap the call */
    if (*in != '\0' && strchr("([{", *in) && in[-1] != ' ') {
        strcpy(next_token_buffer, token_buffer);
        emit("[");
        push_state(neoteric);
    }
}


static void
inline_open(void)
{
    char c = *in;
    const char *tok = in;

    in += 1;
    take(tok);
    push_state(inline_body);
    state_close[state_index] = ")]}"[strchr("([{", c) - "([{"];
}


static void
inline_close(void)
{
    const char *tok = in;

    if (state_fns[state_index] != inline_body || state_close[state_index] != *in)
        die("unexpected '%c'", *in);

    in += 1;
    take(tok);
    state_index -= 1;
}


/* one token from the middle of a line */
static void
read_item(void)
{
    switch (*in) {
    case '(': case '[': case '{': inline_open();  break;
    case ')': case ']': case '}': inline_close(); break;
    case '"':                     read_string();  break;
    default:                      read_word();    break;
    }
}


static void
inline_body(void)
{
    if (*in == '\n' || *in == '\0')
        die("unclosed '%c' at end of line", "([{"[strchr(")]}", state_close[state_index]) - ")]}"]);

    read_item();
}


static void
neoteric(void)
{
    set_state(neoteric_end);
    inline_body();
}


static void
neoteric_end(void)
{
    emit("]");
    state_index -= 1;
}


/* start of a line: open its [ */
static void
prefix_head(void)
{
    if (*in != '\0') {
        emit("[");
        set_state(prefix_body);
        return;
    }

    /* EOF */
    if (cur_indent) {
        new_indent = 0;
        set_state(prefix_end);
        prefix_end();
        return;
    }
    token_buffer[0] = '\0';
}


static void
prefix_body(void)
{
    if (*in == '\0') {
        /* EOF: unwind every open indent level */
        new_indent = 0;
        set_state(prefix_end);
        prefix_end();
        return;
    }

    if (*in == '\n') {
        in += 1;
        set_state(prefix_indent);
        emit("newline");
        return;
    }

    read_item();
}


/* after a newline: blank line, continuation, child, or close */
static void
prefix_indent(void)
{
    int diff;

    if (parse_indent()) {
        emit("newline");
        return;
    }

    diff = new_indent - cur_indent;
    if (diff > 1)
        die("indented more than one level");

    if (in[0] == '\\' && strchr(breakchars, in[1])) {
        if (diff != 1)
            die("line continuation \\ must be indented one level");
        in += 1;
        chomp(' ');
        set_state(prefix_body);
        prefix_body();
        return;
    }

    if (diff == 1) {
        cur_indent = new_indent;
        set_state(prefix_end);
        push_state(prefix_head);
        prefix_head();
        return;
    }

    set_state(prefix_end);
    prefix_end();
}


/* close a line's [ and either start the next sibling or pop a level */
static void
prefix_end(void)
{
    emit("]");

    if (new_indent < cur_indent) {
        cur_indent -= 1;
        state_index -= 1;
    } else {
        set_state(prefix_head);
    }
}


int
ie_tokenize(FILE *input, Ie_Token_Fn on_token, void *user)
{
    if (setjmp(ie__err))
        return 1;   /* a die() landed here */

    f = input;
    line_buffer[0] = '\0';
    in = line_buffer;
    next_token_buffer[0] = '\0';
    cur_indent = 0;
    new_indent = 0;
    state_index = 0;
    state_fns[0] = prefix_head;

    while (parse_indent())   /* leading blank lines */
        on_token("newline", user);
    if (new_indent != 0)
        die("first line must not be indented");

    for (;;) {
        if (next_token_buffer[0] != '\0') {
            strcpy(token_buffer, next_token_buffer);
            next_token_buffer[0] = '\0';
        } else {
            state_fns[state_index]();
        }

        if (token_buffer[0] == '\0')
            break;
        on_token(token_buffer, user);
    }

    if (state_index)
        die("unexpected end of input");

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
#undef RESET
#undef die
#undef INDENT

#endif /* IE_IMPLEMENTATION */

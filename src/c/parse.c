#include <stdio.h> /* puts, fopen, fprintf */
#include <stdlib.h> /* exit */
#include <string.h> /* strncmp */

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

#define die(msg, ...) \
    do { \
        fprintf(stderr, \
                CTEXT(RED_TEXT, "\nerror: " msg "\n"), \
                ## __VA_ARGS__); \
        ere; \
        exit(1); \
    } while (0);

#define debug_var(s,v) \
    fprintf(stderr, #v ": %" s "\n", v)

typedef unsigned char u8;
typedef unsigned int uint;
typedef void (void_fn)(void);

int echo_newlines = 1;
int echo_indents = 0;
int cur_indent = 0;
int new_indent = 0;
uint indent_width = 4;

char *in = NULL;
char line_buffer[256] = "";

char token_buffer[256] = "";
char next_token_buffer[256] = "";

FILE *f = NULL;

int prefix_index = 0;
char prefix_chars[64] = "";
void (*prefix_fns[64])(void);

char *token_breakchars = " ,()[]{}\n";

int state_index = 0;
void (*state_fns[16])(void);

int cmds_index = 0;
char cmds_buffer[16 * (256 + 1)] = "";

int wrapped_index = 0;
char *wrapped[64];

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
debug_line(void)
{
    int offset = 0;
    char *c = line_buffer;
    /*debug_var("zu", in);*/
    /*debug_var("zu", c);*/
    fprintf(stderr, "line:\n");
    fprintf(stderr, "c: %c\n", *c);
    for (;*c != '\0'; c += 1) {
        if (in > c)
            offset += 1;
        switch (*c) {
        case 32:
            fprintf(stderr, ".");
            break;
        case 10:
            if (in > c)
                offset += 1;
            fprintf(stderr, "\\n");
            break;
        default:
            fprintf(stderr, "%c", *c);
        }
    }
    fprintf(stderr, "\\0\n");

    fprintf(stderr, "%*s^\n", offset, "");
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
debug_token(void)
{
    fprintf(stderr, "tok: %d, '%s'\n", strlen(token_buffer), token_buffer);
}


void
debug_dump(void *v, int n)
{
    char *c = v;

    fprintf(stderr, "dumping: %d bytes @ %p\n\n", n, v);

    while (n > 0) {
        ptrdiff_t offset = c - v;
        int i = 4;
        int line_length = n > 7 ? 8 : n % 8;

        fprintf(stderr, "%.03zu ", offset);

        for (i = 0; i < 8; i++) {
            char *x = c + i;
            char *gap = i % 2 ? " " : "";
            if (i < line_length)
                fprintf(stderr, "%02x%s", *x, gap);
            else
                fprintf(stderr, "  %s", gap);
        }

        fprintf(stderr, " ");

        for (i = 0; i < 8; i++) {
            char *x = c + i;
            char xx = *x > 10 ? *x : '.';
            char *gap = i % 2 ? " " : "";
            if (i < line_length)
                fprintf(stderr, "%c", xx);
            else
                fprintf(stderr, " ");
        }

        fprintf(stderr, "\n");

        c += 8;
        n -= 8;
    }

    fprintf(stderr, "\n");
}


char *
alloc_cmd(char *s)
{
    char *rval = &cmds_buffer[cmds_index];
    int len = strlen(s);
    int max_size = sizeof(cmds_buffer) / sizeof(cmds_buffer[0]);
    /*debug_var("d", max_size);*/

    if (len > 255)
        die("command string is too long: %d > 255", len);

    /*debug_var("d", cmds_index);*/
    cmds_index += len + 1;
    /*debug_var("d", cmds_index);*/

    if (cmds_index >= max_size)
        die("cmds_buffer overflowed");

    strncpy(rval, s, len+ 1);
    cmds_buffer[cmds_index] = (u8)len;
    cmds_index += 1;
    /*debug_var("d", cmds_index);*/

    /*debug_var("s", rval);*/
    /*debug_dump(cmds_buffer, cmds_index);*/

    /*debug_var("d", len);*/
    return rval;
}


char *
alloc_prefixed_cmd(char *prefix, char* s)
{
    static char prefixed[256] = "";
    int prefix_len = strlen(prefix);
    int s_len = strlen(s);
    int total_len = prefix_len + s_len;

    if (total_len > 255)
        die("prefixed cmd is too long: %d > 255", total_len);

    /*ere;*/
    strncat(prefixed, prefix, 256);
    /*debug_var("s", prefixed);*/
    /*debug_dump(prefixed, 0x20);*/

    strncat(prefixed, s, 256 - prefix_len);
    /*debug_dump(prefixed, 0x20);*/
    /*debug_var("s", prefixed);*/

    return alloc_cmd(prefixed);
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
        state_index += 1;
        state_fns[state_index] = neoteric;
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

    if (prefix_fn = lookup_prefix(*in)) {
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
        state_index += 1;
        state_fns[state_index] = prefix_head;
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
prefix_cmd(void)
{
    /*ere;*/
    char *end  = NULL;
    void_fn *prefix_fn = NULL;

    if (*in == '\0' && read_line() == NULL) {
        token_buffer[0] = '\0';
        return;
    }

    if (*in == '\0')
        die("null char")

    if (*in == '\n')
        die("newline")

    if (*in == ' ')
        die("space")

    if (prefix_fn = lookup_prefix(*in)) {
        die("can you start a line with a prefix char?");
        /*prefix_fn();*/
        return;
    }

    read_token();
    /*debug_token();*/

    if (is_wrapped(token_buffer)) {
        /*cmds[state_index] = alloc_prefixed_cmd("end-", token_buffer);*/
        state_fns[state_index] = prefix_body;
        return;
    }


    /*cmds[state_index] = alloc_cmd(token_buffer);*/
    state_fns[state_index] = prefix_body;
    prefix_body();
}


void
inline_prefix(void)
{
    in += 1;
    chomp(' ');

    strncpy(token_buffer, "[", 256);
    state_index += 1;
    state_fns[state_index] = inline_body;
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
    void_fn *prefix_fn = NULL;

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
    state_index += 1;
    /*cmds[state_index] = NULL;*/
    state_fns[state_index] = inline_body;
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
    state_index += 1;
    strncpy(token_buffer, "{", 256);
    state_fns[state_index] = inline_body;
}


void
inline_body(void)
{
    /*ere;*/
    void_fn *prefix_fn = NULL;

    /*debug_var("c", *in);*/

    if (prefix_fn = lookup_prefix(*in)) {
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
        return;
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


void
init(void)
{
    memset(wrapped, 0, sizeof(wrapped));
    define_wrap(":");

    define_prefix('[', inline_prefix);
    define_prefix(']', inline_prefix_end);
    define_prefix('(', inline_infix);
    define_prefix(')', inline_infix_end);
    define_prefix('{', inline_postfix);
    define_prefix('}', inline_postfix_end);
    define_prefix('"', read_string);

    in = line_buffer;

    state_index = 0;
    state_fns[state_index] = prefix_head;
}


int
main(int argc, char **argv)
{
    char **arg = argv + 1;

    if (!*arg)
        die("missing input filename");

    if (!strcmp(*arg, "-")) {
        /*puts("stdin");*/
        f = stdin;
    } else {
        if ((f = fopen(*arg, "r")) == NULL)
            die("failed to open file");
    }

    init();

    int limit = 0xff;
    while (next_word(), token_buffer[0] != '\0') {
        /*ere;*/
        /*debug_line();*/
        /*debug_stack();*/
        /*debug_var("d", token_buffer[0]);*/
        /*debug_var("c", token_buffer[0]);*/
        /*debug_var("d", token_buffer[1]);*/
        /*debug_var("c", token_buffer[1]);*/
        /*debug_var("d", strlen(token_buffer));*/
        /*debug_var("s", token_buffer);*/

        if (echo_indents)
            for (int i = cur_indent; i; i -= 1)
                fprintf(stderr, "    ");

        printf("%s\n", token_buffer);
        if (!limit--) {
            ere;
            debug_var("d", *in);
            debug_var("c", *in);
            die("limit");
        }
    }

    /*printf("\n");*/

    if (*in)
        debug_var("c", *in);

    if (state_index) {
        debug_stack();
        die("ere");
    }

    fclose(f);

    /*puts("bye");*/
    return 0;
}


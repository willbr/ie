/*
 * Exhaustive fuzzer for ie.h. Driven by `./build fuzz [maxlen]`.
 *
 * Tries every string of up to `maxlen` symbols drawn from ALPHABET (one
 * symbol is a full indent, so short strings still reach the indent logic)
 * and checks, for each one:
 *
 *   runaway     more tokens than the input could honestly produce (a hang)
 *   bad token   an empty token, or one containing a newline
 *   unbalanced  ie_tokenize returned 0 but the [ ( { tokens don't nest
 *
 * Crashes and memory errors are caught by the sanitizers build.c compiles
 * this with. Inputs are enumerated shortest first, so the examples printed
 * for each failure kind are minimal.
 */
#define IE_IMPLEMENTATION
#include "ie.h"

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static const char *ALPHABET[] = {
    "a", " ", "    ", "\n", "\\", ",", "\"",
    "(", ")", "[", "]", "{", "}",
};
#define NSYM ((int)(sizeof(ALPHABET) / sizeof(ALPHABET[0])))
#define MAX_LEN 16
#define EXAMPLES 5

enum { OK, RUNAWAY, BAD_TOKEN, UNBALANCED, NKIND };
static const char *kind_names[NKIND] = { "ok", "runaway", "bad token", "unbalanced" };
static long kind_count[NKIND];

static jmp_buf escape;
static int token_count;
static int token_limit;
static int bad_token;
static char nest[2048];   /* > max token_limit, so no bounds check */
static int nest_depth;
static int unbalanced;


static void
check_token(const char *token, void *user)
{
    (void)user;

    if (++token_count > token_limit)
        longjmp(escape, 1);   /* skips ie_tokenize's frame; next call resets it */

    if (token[0] == '\0' || strchr(token, '\n')) {
        bad_token = 1;
        return;
    }

    if (token[1] != '\0')
        return;

    char *open = strchr("([{", token[0]);
    char *close = strchr(")]}", token[0]);
    if (open) {
        nest[nest_depth++] = token[0];
    } else if (close) {
        if (nest_depth == 0 || nest[nest_depth - 1] != "([{"[close - ")]}"])
            unbalanced = 1;
        else
            nest_depth -= 1;
    }
}


static int
run(const char *src, size_t len)
{
    FILE *fp;
#ifdef _WIN32
    fp = tmpfile();
    fwrite(src, 1, len, fp);
    rewind(fp);
#else
    fp = fmemopen((void *)src, len ? len : 1, "r");
    if (len == 0)
        fgetc(fp);   /* fmemopen rejects size 0: open 1 byte and consume it */
#endif

    token_count = 0;
    token_limit = 64 + 16 * (int)len;
    bad_token = 0;
    nest_depth = 0;
    unbalanced = 0;

    int kind = OK;
    if (setjmp(escape)) {
        kind = RUNAWAY;
    } else {
        int rc = ie_tokenize(fp, check_token, NULL);
        if (bad_token)
            kind = BAD_TOKEN;
        else if (rc == 0 && (unbalanced || nest_depth != 0))
            kind = UNBALANCED;
    }

    fclose(fp);
    return kind;
}


static void
print_escaped(const char *s, size_t len)
{
    putchar('"');
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n')      fputs("\\n", stdout);
        else if (s[i] == '"')  fputs("\\\"", stdout);
        else if (s[i] == '\\') fputs("\\\\", stdout);
        else                   putchar(s[i]);
    }
    putchar('"');
}


int
main(int argc, char **argv)
{
    int max_len = argc > 1 ? atoi(argv[1]) : 5;
    if (max_len < 0 || max_len > MAX_LEN) {
        fprintf(stderr, "maxlen must be 0..%d\n", MAX_LEN);
        return 1;
    }

    /* the parser reports every rejected input on stderr: silence it */
#ifdef _WIN32
    freopen("NUL", "w", stderr);
#else
    freopen("/dev/null", "w", stderr);
#endif

    int digits[MAX_LEN];
    char src[MAX_LEN * 4 + 1];
    long total = 0;

    for (int n = 0; n <= max_len; n++) {
        memset(digits, 0, sizeof(digits));
        for (;;) {
            size_t len = 0;
            for (int i = 0; i < n; i++) {
                size_t l = strlen(ALPHABET[digits[i]]);
                memcpy(src + len, ALPHABET[digits[i]], l);
                len += l;
            }
            src[len] = '\0';

            int kind = run(src, len);
            total += 1;
            if (kind != OK) {
                if (kind_count[kind] < EXAMPLES) {
                    printf("%-10s ", kind_names[kind]);
                    print_escaped(src, len);
                    putchar('\n');
                }
                kind_count[kind] += 1;
            }

            int i = 0;
            while (i < n && ++digits[i] == NSYM)
                digits[i++] = 0;
            if (i == n)
                break;
        }
    }

    long failed = kind_count[RUNAWAY] + kind_count[BAD_TOKEN] + kind_count[UNBALANCED];
    printf("\n%ld inputs up to %d symbols: %ld failed", total, max_len, failed);
    for (int k = 1; k < NKIND; k++)
        if (kind_count[k])
            printf(", %s %ld", kind_names[k], kind_count[k]);
    printf("\n");
    return failed ? 1 : 0;
}

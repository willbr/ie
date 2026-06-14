/*
 * Golden-file test runner for the ie parser. Cross-platform via nob.h.
 * Links ie.h directly and tokenizes in-process (no subprocess).
 *
 *   out/test          parse each fixture's in.txt and diff it against out.txt
 *   out/test bless     regenerate out.txt from the current parser output
 *
 * Run from the repo root. Driven by `./build test` / `./build bless`.
 *
 * Fixtures: tests/tokeniser/<group>/<name>/{in,out}.txt
 * A <name> beginning with "err-" is expected to be rejected (non-zero exit).
 */
#define NOB_IMPLEMENTATION
#include "nob.h"
#define IE_IMPLEMENTATION
#include "ie.h"

#define TESTS "tests/tokeniser"

int bless = 0;
int pass = 0;
int fail = 0;
int blessed = 0;


/* drop trailing newlines so the compare ignores the final-newline question */
void
rstrip(Nob_String_Builder *sb)
{
    while (sb->count && (sb->items[sb->count - 1] == '\n' || sb->items[sb->count - 1] == '\r'))
        sb->count -= 1;
}


/* ie token callback: append "token\n" to the caller's string builder */
void
collect(const char *token, void *user)
{
    nob_sb_appendf((Nob_String_Builder *)user, "%s\n", token);
}


/* naive positional line diff — enough to see what a snapshot mismatch changed */
void
print_diff(char *want, char *got)
{
    char *wl[8192], *gl[8192];
    int wn = 0, gn = 0;
    char *p;

    for (p = strtok(want, "\n"); p && wn < 8192; p = strtok(NULL, "\n"))
        wl[wn++] = p;
    for (p = strtok(got, "\n"); p && gn < 8192; p = strtok(NULL, "\n"))
        gl[gn++] = p;

    int max = wn > gn ? wn : gn;
    for (int i = 0; i < max; i++) {
        char *a = i < wn ? wl[i] : NULL;
        char *b = i < gn ? gl[i] : NULL;
        if (a && b && strcmp(a, b) == 0)
            continue;
        if (a) printf("    - %s\n", a);
        if (b) printf("    + %s\n", b);
    }
}


void
run_test(const char *dir, const char *name)
{
    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;

    const char *in_path = nob_temp_sprintf("%s/in.txt", dir);
    const char *out_path = nob_temp_sprintf("%s/out.txt", dir);

    Nob_String_Builder got = {0};
    int rc = ie_tokenize_path(in_path, collect, &got);

    /* err-* fixtures are expected to be rejected; only meaningful in check mode */
    if (strncmp(base, "err-", 4) == 0) {
        if (!bless) {
            if (rc != 0) {
                pass += 1;
            } else {
                fail += 1;
                printf("FAIL %s — expected rejection, parser accepted it\n", name);
            }
        }
        nob_sb_free(got);
        return;
    }

    if (rc != 0) {
        if (bless)
            printf("skip (parser failed): %s\n", name);
        else {
            fail += 1;
            printf("FAIL %s — parser exited non-zero\n", name);
        }
        nob_sb_free(got);
        return;
    }
    rstrip(&got);

    if (bless) {
        FILE *fp = fopen(out_path, "wb");
        if (!fp) {
            printf("error: cannot write %s\n", out_path);
        } else {
            fwrite(got.items, 1, got.count, fp);
            fputc('\n', fp);
            fclose(fp);
            blessed += 1;
        }
        nob_sb_free(got);
        return;
    }

    Nob_String_Builder want = {0};
    nob_read_entire_file(out_path, &want);   /* missing file -> empty -> mismatch */
    rstrip(&want);

    int eq = got.count == want.count && memcmp(got.items, want.items, got.count) == 0;
    if (eq) {
        pass += 1;
    } else {
        fail += 1;
        printf("FAIL %s\n", name);
        nob_sb_append_null(&want);
        nob_sb_append_null(&got);
        print_diff(want.items, got.items);
    }

    nob_sb_free(got);
    nob_sb_free(want);
}


int
main(int argc, char **argv)
{
    nob_minimal_log_level = NOB_WARNING;   /* keep nob quiet during the run */

    if (argc > 1 && strcmp(argv[1], "bless") == 0)
        bless = 1;

    Nob_File_Paths groups = {0};
    if (!nob_read_entire_dir(TESTS, &groups)) {
        nob_log(NOB_ERROR, "cannot read %s (run from the repo root)", TESTS);
        return 1;
    }

    for (size_t i = 0; i < groups.count; i++) {
        const char *g = groups.items[i];
        if (g[0] == '.')
            continue;

        const char *group_path = nob_temp_sprintf("%s/%s", TESTS, g);
        if (nob_get_file_type(group_path) != NOB_FILE_DIRECTORY)
            continue;

        Nob_File_Paths cases = {0};
        nob_read_entire_dir(group_path, &cases);
        for (size_t j = 0; j < cases.count; j++) {
            const char *c = cases.items[j];
            if (c[0] == '.')
                continue;

            const char *dir = nob_temp_sprintf("%s/%s", group_path, c);
            if (nob_get_file_type(dir) != NOB_FILE_DIRECTORY)
                continue;

            run_test(dir, nob_temp_sprintf("%s/%s", g, c));
        }
        nob_da_free(cases);
    }
    nob_da_free(groups);

    if (bless) {
        printf("blessed %d fixtures\n", blessed);
        return 0;
    }

    printf("\n");
    if (fail > 0) {
        printf("%d passed, %d FAILED\n", pass, fail);
        return 1;
    }
    printf("all %d tests passed\n", pass);
    return 0;
}

/*
 * Golden-file test runner for the ie parser (src/c/parse.c).
 *
 *   build/test          parse each fixture's in.txt and diff it against out.txt
 *   build/test bless     regenerate out.txt from the current parser output
 *
 * Run from the repo root (paths below are relative). Wired up as
 * `make test` / `make bless`.
 *
 * Fixtures: src/tests/tokeniser/<group>/<name>/{in,out}.txt
 * A <name> beginning with "err-" is expected to be rejected (non-zero exit).
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define PARSE "build/parse"
#define TESTS "src/tests/tokeniser"
#define BUF   (64 * 1024)
#define MAXLINES 8192

int bless = 0;
int pass = 0;
int fail = 0;
int blessed = 0;


/* strip trailing newlines so the compare ignores the final-newline question */
void
rstrip(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}


int
is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


/* read the whole file into buf; returns -1 if it can't be opened */
int
read_file(const char *path, char *buf, size_t cap)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        buf[0] = '\0';
        return -1;
    }
    size_t n = fread(buf, 1, cap - 1, f);
    buf[n] = '\0';
    fclose(f);
    return (int)n;
}


/* run PARSE on in_path, capture stdout into buf, return its exit code */
int
run_parser(const char *in_path, char *buf, size_t cap)
{
    char cmd[1024];
    snprintf(cmd, sizeof cmd, "%s '%s' 2>/dev/null", PARSE, in_path);

    FILE *p = popen(cmd, "r");
    if (!p) {
        buf[0] = '\0';
        return -1;
    }
    size_t n = fread(buf, 1, cap - 1, p);
    buf[n] = '\0';
    int status = pclose(p);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}


/* naive positional line diff — enough to see what a snapshot mismatch changed */
void
print_diff(char *want, char *got)
{
    char *wl[MAXLINES], *gl[MAXLINES];
    int wn = 0, gn = 0;
    char *p;

    for (p = strtok(want, "\n"); p && wn < MAXLINES; p = strtok(NULL, "\n"))
        wl[wn++] = p;
    for (p = strtok(got, "\n"); p && gn < MAXLINES; p = strtok(NULL, "\n"))
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
    char in_path[1024], out_path[1024];
    snprintf(in_path, sizeof in_path, "%s/in.txt", dir);
    snprintf(out_path, sizeof out_path, "%s/out.txt", dir);

    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;

    static char got[BUF], want[BUF];

    /* err-* fixtures are expected to be rejected; only meaningful in check mode */
    if (strncmp(base, "err-", 4) == 0) {
        if (bless)
            return;
        int code = run_parser(in_path, got, sizeof got);
        if (code != 0) {
            pass++;
        } else {
            fail++;
            printf("FAIL %s — expected rejection, parser accepted it\n", name);
        }
        return;
    }

    int code = run_parser(in_path, got, sizeof got);
    if (code != 0) {
        if (bless) {
            printf("skip (parser failed): %s\n", name);
            return;
        }
        fail++;
        printf("FAIL %s — parser exited non-zero\n", name);
        return;
    }
    rstrip(got);

    if (bless) {
        FILE *f = fopen(out_path, "w");
        if (!f) {
            printf("error: cannot write %s\n", out_path);
            return;
        }
        fprintf(f, "%s\n", got);
        fclose(f);
        blessed++;
        return;
    }

    read_file(out_path, want, sizeof want);
    rstrip(want);

    if (strcmp(got, want) == 0) {
        pass++;
    } else {
        fail++;
        printf("FAIL %s\n", name);
        print_diff(want, got);
    }
}


int
main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "bless") == 0)
        bless = 1;

    DIR *groups = opendir(TESTS);
    if (!groups) {
        fprintf(stderr, "cannot open %s (run from the repo root)\n", TESTS);
        return 1;
    }

    struct dirent *g;
    while ((g = readdir(groups))) {
        if (g->d_name[0] == '.')
            continue;

        char group_path[1024];
        snprintf(group_path, sizeof group_path, "%s/%s", TESTS, g->d_name);
        if (!is_dir(group_path))
            continue;

        DIR *cases = opendir(group_path);
        if (!cases)
            continue;

        struct dirent *t;
        while ((t = readdir(cases))) {
            if (t->d_name[0] == '.')
                continue;

            char dir[1024], name[1024];
            snprintf(dir, sizeof dir, "%s/%s", group_path, t->d_name);
            if (!is_dir(dir))
                continue;
            snprintf(name, sizeof name, "%s/%s", g->d_name, t->d_name);

            run_test(dir, name);
        }
        closedir(cases);
    }
    closedir(groups);

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

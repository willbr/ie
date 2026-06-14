/*
 * build.c — the build system for ie. No make required.
 *
 * Bootstrap once:
 *     cc -o build build.c
 *
 * Then:
 *     ./build              build out/parse and out/test
 *     ./build run [file]   build the parser and run it on a file
 *     ./build test         build and run the golden-file tests
 *     ./build bless        regenerate the test fixtures from current output
 *     ./build clean        remove build artifacts
 *
 * Editing build.c is enough — it rebuilds itself before running.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define CC      "cc"
#define CFLAGS  "-std=c99 -Wall -Wextra"
#define DEFAULT_F "tests/tokeniser/c/c00-hello-world/in.txt"


/* run a shell command, echo it, return its exit code */
int
run(const char *fmt, ...)
{
    char cmd[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cmd, sizeof cmd, fmt, ap);
    va_end(ap);

    printf("+ %s\n", cmd);
    int rc = system(cmd);
    if (rc == -1)
        return -1;
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
}


void
die_if(int rc)
{
    if (rc != 0)
        exit(rc);
}


/* if build.c is newer than the build binary, recompile and re-exec */
void
rebuild_self(char **argv)
{
    struct stat src, bin;
    if (stat("build.c", &src) != 0 || stat("build", &bin) != 0)
        return;
    if (src.st_mtime <= bin.st_mtime)
        return;

    printf("+ build.c changed, rebuilding self\n");
    unlink("build.old");
    if (rename("build", "build.old") != 0) {
        perror("rename");
        exit(1);
    }
    if (run("%s -o build build.c", CC) != 0) {
        rename("build.old", "build");
        exit(1);
    }
    execv(argv[0], argv);
    perror("execv");
    exit(1);
}


void
build_parse(void)
{
    mkdir("out", 0755);
    die_if(run("%s %s -o out/parse parse.c", CC, CFLAGS));
}


void
build_test(void)
{
    mkdir("out", 0755);
    die_if(run("%s %s -o out/test test.c", CC, CFLAGS));
}


int
main(int argc, char **argv)
{
    rebuild_self(argv);

    const char *cmd = argc > 1 ? argv[1] : "all";

    if (strcmp(cmd, "all") == 0) {
        build_parse();
        build_test();
    } else if (strcmp(cmd, "run") == 0) {
        build_parse();
        const char *f = argc > 2 ? argv[2] : DEFAULT_F;
        return run("out/parse %s", f);
    } else if (strcmp(cmd, "test") == 0) {
        build_parse();
        build_test();
        return run("out/test");
    } else if (strcmp(cmd, "bless") == 0) {
        build_parse();
        build_test();
        return run("out/test bless");
    } else if (strcmp(cmd, "clean") == 0) {
        return run("rm -rf out build.old");
    } else {
        fprintf(stderr, "unknown command: %s\n", cmd);
        fprintf(stderr, "usage: ./build [all|run [file]|test|bless|clean]\n");
        return 1;
    }

    return 0;
}

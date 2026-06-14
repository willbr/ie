/*
 * build.c — the build system for ie. No make required. Cross-platform via nob.h.
 *
 * Bootstrap once (any C compiler):
 *     cc -o build build.c          # or: gcc/clang -o build build.c, or: cl build.c
 *
 * Then:
 *     ./build              build out/parse and out/test
 *     ./build run [file]   build the parser and run it on a file
 *     ./build test         build and run the golden-file tests
 *     ./build bless        regenerate the test fixtures from current output
 *     ./build clean        remove build artifacts
 *
 * Editing build.c is enough — nob rebuilds it before running.
 */
#define NOB_IMPLEMENTATION
#include "nob.h"

#define PARSE "out/parse"
#define TEST  "out/test"
#define DEFAULT_F "tests/tokeniser/c/c00-hello-world/in.txt"


bool
compile(const char *output, const char *input)
{
    if (!nob_mkdir_if_not_exists("out"))
        return false;
    Nob_Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cc_output(&cmd, output);
    nob_cc_inputs(&cmd, input);
    return nob_cmd_run(&cmd);
}


void
rm(const char *path)
{
    if (nob_file_exists(path) > 0)
        nob_delete_file(path);
}


int
main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *sub = argc > 1 ? argv[1] : "all";

    if (strcmp(sub, "all") == 0) {
        if (!compile(PARSE, "parse.c")) return 1;
        if (!compile(TEST, "test.c")) return 1;
    } else if (strcmp(sub, "run") == 0) {
        if (!compile(PARSE, "parse.c")) return 1;
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, PARSE, argc > 2 ? argv[2] : DEFAULT_F);
        return nob_cmd_run(&cmd) ? 0 : 1;
    } else if (strcmp(sub, "test") == 0) {
        if (!compile(PARSE, "parse.c")) return 1;
        if (!compile(TEST, "test.c")) return 1;
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, TEST);
        return nob_cmd_run(&cmd) ? 0 : 1;
    } else if (strcmp(sub, "bless") == 0) {
        if (!compile(PARSE, "parse.c")) return 1;
        if (!compile(TEST, "test.c")) return 1;
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, TEST, "bless");
        return nob_cmd_run(&cmd) ? 0 : 1;
    } else if (strcmp(sub, "clean") == 0) {
        rm("out/parse");      rm("out/parse.exe");
        rm("out/test");       rm("out/test.exe");
        rm("out/.captured");  rm("out/.stderr");
        rm("build.old");      rm("build.old.exe");
    } else {
        nob_log(NOB_ERROR, "unknown command: %s", sub);
        nob_log(NOB_INFO, "usage: ./build [all|run [file]|test|bless|clean]");
        return 1;
    }

    return 0;
}

/* parse — CLI for ie.h: read an ie file (or - for stdin), print one token per line. */
#define IE_IMPLEMENTATION
#include "ie.h"

static void
print_token(const char *token, void *user)
{
    (void)user;
    printf("%s\n", token);
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file|->\n", argv[0]);
        return 1;
    }
    return ie_tokenize_path(argv[1], print_token, NULL);
}

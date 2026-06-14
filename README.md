# ie - Indented Expressions

Syntax = Lisp + Python

`ie` is a *syntax*, not a language — like JSON or s-expressions. It maps
indented text onto a flat token stream: indentation and brackets become
explicit `[ ]`, and every token is printed one per line. What you do with
those tokens afterwards is up to you.

I love:
https://dwheeler.com/readable/alternative-s-expressions.html

## build

No make. The build system is `build.c` (cross-platform via the vendored
`nob.h`). Bootstrap it once with any C compiler, then drive everything
through it:

    cc -o build build.c      # or: gcc/clang -o build build.c  ·  Windows: cl build.c
    ./build                  # build out/parse and out/test
    ./build run [file]       # run the parser on a file
    ./build test             # run the golden-file tests
    ./build bless            # regenerate test fixtures from current output
    ./build clean            # remove build artifacts

`build.c` rebuilds itself when edited. Works on Linux, macOS, and Windows
(MSVC, MinGW, or Clang).

## example

`out/parse file.ie` reads a file and writes one token per line. Given

    puts "hi"

    define square(x)
        x * x

it emits (shown compactly here — the real output is one token per line):

    [puts "hi" newline]
    [define [square ( x )] newline
        [x * x newline]]

Indentation opens and closes `[ ]`, `newline` marks the end of a line, and a
neoteric call `square(x)` becomes `[square ( x )]`. The bracket characters
`(` `)` `[` `]` `{` `}` pass through as their own tokens. Indentation is
spaces, 4 per level.

## reader

For each token, read the first char:

    breakchar? (ends the current word)
        space
        ,            comma
        ( ) [ ] { }  brackets
        newline

    " starts a string

    otherwise, read a word up to the next breakchar

Brackets group: `[ ]` prefix, `( )` infix, `{ }` rpn.

## roadmap

Designed but not implemented in `parse.c` yet:

    prefix transforms   *deref  &addr  'quote  !not  %binary  $hex
    number promotion    decimal / hex / binary
    heredocs            puts <<end ... end
    regex objects       /^haha$/
    version pragma      version 0
    source locations    file:row:col:token  in the output

The prefix transforms are meant to rewrite a token, e.g. `*something` becomes
`deref something`.

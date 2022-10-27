from .tokenise import tokenise_file, tokenise_lines
from .parse1_indent import parse_indent
from .parse2_syntax import parse_syntax, puts_expr, remove_newline
from .promote import promote_token
from textwrap import dedent
from itertools import tee
from rich import print

if __name__ == "__main__":
    example = dedent("""
    puts "hi"

    define square(x)
        x * x

    puts square(10)

    a = 5
    puts (a < 10)

    puts <<eof
    bye
    bye
    baby

    eof
    """)

    print('example', repr(example))
    print(example)
    print('*' * 20)

    lines = example.splitlines(keepends=True)
    print('lines', repr(lines))

    preview, tokens = tee(tokenise_lines(lines))
    #print('tokens ', ' '.join(map(repr, preview)))

    preview, tokens2 = tee(parse_indent(tokens))
    #print('tokens2', ' '.join(map(repr, preview)))

    ast     = parse_syntax(tokens2, promote_token)

    print('*' * 20)

    for x in ast:
        puts_expr(x)

    print('*' * 20)

    sast = remove_newline(ast)

    for x in sast:
        puts_expr(x)

    print('*' * 20)


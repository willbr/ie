from .tokenise import tokenise_file, tokenise_lines
from .parse1_indent import parse_indent
from .parse2_syntax import parse_syntax, puts_expr
from .promote import promote_token
from textwrap import dedent
from itertools import tee
from rich import print

if __name__ == "__main__":
    example = dedent("""
    a

        b

        c

    d

    """).strip()
    print('example', repr(example))
    print(example)

    lines = example.splitlines(keepends=True)
    print('lines', repr(lines))

    preview, tokens = tee(tokenise_lines(lines))
    print('tokens ', ' '.join(map(repr, preview)))

    preview, tokens2 = tee(parse_indent(tokens))
    print('tokens2', ' '.join(map(repr, preview)))

    ast     = parse_syntax(tokens2, promote_token)
    puts_expr(ast)


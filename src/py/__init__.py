import sys
from .tokenise import tokenise_file, tokenise_lines
from .parse1_indent import parse_indent
from .parse2_syntax import parse_file, parse_lines, puts_expr


if __name__ == "__main__":
    filename = sys.argv[1]
    ast = parse_file(filename)
    for x in ast:
        puts_expr(x)


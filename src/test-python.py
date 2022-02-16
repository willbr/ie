from rich.markup import escape
from pathlib import Path
from py.tokenise import tokenise_file
from py.parse1_indent import parse_indent
from py.parse2_syntax import parse_file, expr_to_string
from difflib import unified_diff
from textwrap import dedent
from rich.console import Console

console = Console(markup=False)
python_print = print
print = console.print

def read_file(fn):
    with open(fn) as f:
        return [l.strip('\n') for l in f.readlines()]


def read_indent(fn):
    tokens = tokenise_file(fn)
    tokens2 = parse_indent(tokens)
    return tokens2


def read_syntax(fn):
    ast = parse_file(fn)
    s = expr_to_string(ast)
    lines = s.split('\n')
    return lines


here = Path(__file__).parent

tokeniser_test_folders = list(here.glob("tests/tokeniser/*/*"))
parse1_indent_test_folders = list(here.glob("tests/parse1_indent/*/*"))
parse2_syntax_test_folders = list(here.glob("tests/parse2_syntax/*/*"))

specs = [
        (tokeniser_test_folders, tokenise_file),
        (parse1_indent_test_folders, read_indent),
        (parse2_syntax_test_folders, read_syntax),
        ]

for folders, reader in specs:
    for fld in folders:
        # print(repr(fld))

        in_file = fld.joinpath("in.txt")
        out_file = fld.joinpath("out.txt")

        if fld.stem.startswith("err-"):
            result = []
            try:
                result = list(reader(in_file))
                print(fld)
                print(fld.stem)
                raise ValueError("test didn't fail")
            except:
                pass
        else:
            result = list(reader(in_file))
        # print(list(map(escape,result)))

        expected_result = read_file(out_file)
        # print(lines)
        if expected_result == [] and \
            result != [] and \
            not fld.stem.startswith("err-"):
            with open(out_file, 'w') as f:
                print(f"Writing to {repr(out_file)}:")
                print(result)
                f.write('\n'.join(result))
        else:
            pass
            # print("er:")
            # print(repr(out_file))
            # print(expected_result)
            # print()

        diff = list(unified_diff(result, expected_result))
        if diff:
            diff_lines = '\n'.join(diff)
            input_lines = read_file(in_file)
            print(f"""
{repr(fld)}

input:
{repr(in_file)}
{input_lines}

result:
{result}

expected:
{repr(out_file)}
{expected_result}

diff:
{diff_lines}
{repr(fld)}
""")
            exit(1)


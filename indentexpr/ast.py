from typing import NamedTuple
from collections.abc import Iterable
import re

from .utilities import (
        strip_newlines,
        split_on_newline,
        hline,
        peek,
        )


from .tokeniser import (
        Token,
        tokenise,
        convert_indent_to_brackets,
        )


if __name__ == '__main__':
    from rich.console import Console
    from rich.traceback import install

    install(show_locals=False, max_frames=1)

    console = Console(markup=False)
    python_print = print
    print = console.print


def parse_tree(tokens):
    x = []
    stack = [x]
    for t in tokens:
        if t.type == 'LBRACKET':
            x = []
            stack.append(x)
        elif t.type == 'RBRACKET':
            stack.pop()
            tos = stack[-1]
            if len(stack) == 1:
                yield x
            else:
                tos.append(x)
                x = tos
        else:
            x.append(t)

    assert len(stack) == 1
    tos = stack[0]
    assert len(tos) == 0


def is_atom(x):
    if isinstance(x, str):
        return True
    if isinstance(x, Token):
        return True
    if isinstance(x, Iterable):
        return False
    return True

def mapleaf(fn, x):
    if is_atom(x):
        return fn(x)
    return [mapleaf(fn, leaf) for leaf in x]

def mapbranch(fn, x):
    if is_atom(x):
        return x
    return fn([mapbranch(fn, leaf) for leaf in x])


def tree_values(tree):
    return mapleaf(lambda x: x.value, tree)


def split_on_token(x, token_type):
    current = []
    top = [current]
    for y in x:
        if not isinstance(y, Token):
            current.append(y)
            continue

        if y.type != token_type:
            current.append(y)
            continue

        current = []
        top.append(current)

    return top


def parse_neo_infix(x):
    cmd, *i = x
    px = parse_infix(i)
    return [cmd, px]


def parse_infix(x):
    r = split_on_token(x, 'COMMA')
    tr = [transform_infix(y) for y in r]
    if len(tr) == 1:
        return tr[0]
    else:
        return tr


def transform_infix(x):
    """
    transform infix to prefix

    (1 + 2 + 3)
    [+ [+ 1 2] 3]
    {1 2 + 3 +}

    (1 + 2 + 3 + 4)
    [+ [+ [+ 1 2] 3] 4]
    {1 2 + 3 + 4 +}
    """

    if len(x) == 1:
        return x[0]

    first_arg, first_op, second_arg, *rest = x
    xx = [first_op, first_arg, second_arg]

    for i in range(3, len(x)):
        t = ix[i]
        if i % 2:
            assert first_op == t
        else:
            xx = [first_op, xx, t]
    return xx


def parse_syntax(x, syntax_fns):
    head, *tail = x
    if not isinstance(head, Token):
        return x
    if head.type != 'WORD':
        return x

    fn = syntax_fns.get(head.value, None)
    if fn is None:
        return x

    if True:
        r = mapleaf(lambda x: x.value, x)

    y = fn(tail)
    return y

code = """
fn  main(argc, argv) -> int
    when now > "09:00"
        puts "good morning"
    return (9 * 9)
"""

def parse_file(filename):
    with open(filename) as f:
        s = f.read()
        ast = parse_string(s, filename)
        return ast


def parse_string(s, filename):
    tokens = tokenise(s, filename)
    tokens2 = convert_indent_to_brackets(tokens)
    ast = parse_tree(tokens2)
    return ast


if __name__ == '__main__':
    if True:
        hline(title='# code')
        print(code)

    tokens = list(tokenise(code, 'code'))
    if False:
        hline(title='# tokens')
        for token in tokens:
            print(str(token))

    tokens2 = list(convert_indent_to_brackets(tokens))
    if False:
        hline(title='# parsed indents')
        if False:
            for token in tokens2:
                print(str(token))

        print(' '.join(str(t.value) for t in tokens2 if t.type != 'NEWLINE'))

    if True:
        syntax_fns = {
                'infix': parse_infix,
                'neo-infix': parse_neo_infix,
                }
        hline(title='# tree')
        ast = parse_tree(tokens2)
        for expr in ast:
            x = mapbranch(lambda x: parse_syntax(x, syntax_fns), expr)
            r = mapleaf(lambda x: x.value, x)
            print(r)


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
        tokenize,
        convert_indent_to_brackets,
        )


if __name__ == '__main__':
    from rich.console import Console
    from rich.traceback import install

    install(show_locals=True)

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


def maptree(fn, tree):
    if isinstance(tree, Iterable) and not isinstance(tree, str) and not isinstance(tree, Token):
        return [maptree(fn, leaf) for leaf in tree]
    else:
        return fn(tree)


def tree_values(tree):
    return maptree(lambda x: x.value, tree)


code = """
def main[argc, argv]
    puts "hello"
"""


def parse_file(filename):
    with open(filename) as f:
        s = f.read()
        ast = parse_string(s, filename)
        return ast


def parse_string(s, filename):
    tokens = tokenize(s, filename)
    tokens2 = convert_indent_to_sexp(tokens)
    ast = parse_tree(tokens2)
    return ast


if __name__ == '__main__':
    if True:
        hline(title='# code')
        print(code)

    tokens = list(tokenize(code, 'code'))
    if True:
        hline(title='# tokens')
        for token in tokens:
            print(str(token))
            #print(token.type, repr(token.value))

    tokens2 = list(convert_indent_to_brackets(tokens))
    if True:
        hline(title='# parsed indents')
        if False:
            for token in tokens2:
                print(str(token))

        print(' '.join(str(t.value) for t in tokens2 if t.type != 'NEWLINE'))

    if True:
        hline(title='# tree')
        ast = parse_tree(tokens2)
        for expr in ast:
            #print(expr)
            r = maptree(lambda x: x.value, expr)
            print(r)


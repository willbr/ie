from typing import NamedTuple
from collections.abc import Iterable
from dataclasses import dataclass
import re

from .utilitise import (
        strip_newlines,
        split_on_newline,
        maptree,
        tree_values,
        hline,
        peek,
        )

if __name__ == '__main__':
    from rich.console import Console
    from rich.traceback import install

    install(show_locals=True)

    console = Console(markup=False)
    python_print = print
    print = console.print


@dataclass
class Token():
    type: str
    value: str
    line: int
    column: int
    filename: str

    def __str__(self):
        return f"{self.type} {repr(self.value)}"


def tokenize(code, filename):
    code = code.strip()

    word_regex = r'[^ \t\n\"(){}[\],;]+'
    token_specification = [
        ('NUMBER',   r'\d+(\.\d*)?'),
        ('NEOTERIC', word_regex + r'[({[]'),
        ('WORD',     word_regex),
        ('LPAREN',   r'\('),
        ('RPAREN',   r'\)'),
        ('LBRACKET', r'\['),
        ('RBRACKET', r'\]'),
        ('LBRACE',   r'\{'),
        ('RBRACE',   r'\}'),
        ('COMMA',   r','),
        ('SEMICOLON',   r';'),
        ('STRING',   r'"(\\"|[^"])*"'),
        ('NEWLINE',  r'\n[ ]*'),
        ('SKIP',     r'[ ]+'),
        ('MISMATCH', r'.'),
    ]
    tok_regex = '|'.join('(?P<%s>%s)' % pair for pair in token_specification)

    column = 1
    line_num = 1
    line_start = 0

    cur_indent = 0
    indent_width = 4

    stack = []

    for mo in re.finditer(tok_regex, code):
        kind = mo.lastgroup
        value = mo.group()
        column = mo.start() - line_start
        if kind == 'NUMBER':
            value = float(value) if '.' in value else int(value)
        elif kind == 'NEWLINE':
            yield Token(kind, value[0], line_num, column, filename)
            line_start = mo.end()
            line_num += 1

            indent_chars = value[1:]
            indent = len(indent_chars) // indent_width
            delta = indent - cur_indent

            if delta > 1:
                assert False
            elif delta == 1:
                yield Token('INDENT', '', line_num, column, filename)
            elif delta == 0:
                yield Token('NOINDENT', '', line_num, column, filename)
            else:
                for i in range(0, delta, -1):
                    yield Token('DEDENT', '', line_num, column, filename)

            cur_indent = indent

            continue
        elif kind == 'SKIP':
            continue
            pass
        elif kind == 'MISMATCH':
            raise RuntimeError(f'{value!r} unexpected on line {line_num}')

        elif kind == 'NEOTERIC':
            name = value[:-1]
            char = value[-1]
            offset = line_num + len(value) - 1
            yield Token('LBRACKET', char, offset, column, filename)
            stack.append(char)

            prefix_table = {
                    '(' : 'neo-infix',
                    '{' : 'neo-brace',
                    '[' : 'subscript',
                    }
            neo_prefix = prefix_table[char]
            yield Token('WORD', neo_prefix, offset, column, filename)

            yield Token('WORD', name, line_num, column, filename)
            continue
        elif kind in ['LPAREN', 'LBRACE','LBRACKET']:
            stack.append(value)
            yield Token('LBRACKET', value, line_num, column, filename)
            continue
        elif kind in ['RPAREN', 'RBRACE','RBRACKET']:
            pair_table = {
                    ')':'(',
                    '}':'{',
                    ']':'[',
                    }
            expected = pair_table[value]
            tos = stack[-1]
            assert tos == expected
            stack.pop()
            yield Token('RBRACKET', value, line_num, column, filename)
            continue
            #assert False

        yield Token(kind, value, line_num, column, filename)


def convert_indent_to_brackets(tokens):
    #breakpoint()
    depth = -1

    for token, next_token in peek(tokens):
        if token.type == 'INDENT':
            yield Token('LBRACKET', '[', token.line, token.column, token.filename)
            depth += 1
            continue
        elif token.type == 'NOINDENT':
            yield Token('RBRACKET', ']', token.line, token.column, token.filename)
            yield Token('LBRACKET', '[', token.line, token.column, token.filename)
            continue
        elif token.type == 'DEDENT':
            yield Token('RBRACKET', ']', token.line, token.column, token.filename)
            if next_token.type != 'DEDENT':
                yield Token('RBRACKET', ']', token.line, token.column, token.filename)
                yield Token('LBRACKET', '[', token.line, token.column, token.filename)
            depth -= 1
            continue

        if depth == -1:
            yield Token('LBRACKET', '[', token.line, token.column, token.filename)
            depth = 1

        yield token

    for i in range(depth):
        yield Token('RBRACKET', ']', token.line, token.column, token.filename)


code = """
sum 1 2 3
    :
        sum 1 2
        sum 3 4
"""


if __name__ == '__main__':
    if True:
        hline(title='# code')
        print(code)

    tokens = list(tokenize(code, 'code'))
    if True:
        hline(title='# tokens')
        for token in tokens:
            print(str(token))

    tokens2 = list(convert_indent_to_brackets(tokens))
    if True:
        hline(title='# sexp')
        if False:
            for token in tokens2:
                print(token)

        print(' '.join(str(t.value) for t in tokens2 if t.type != 'NEWLINE'))



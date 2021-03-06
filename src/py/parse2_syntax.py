from pprint import pprint
from .tokenise import tokenise_file
from .parse1_indent import parse_indent
from .promote import promote_token

import fileinput
import sys


def parse_file(filename, promote=promote_token):
    tokens  = tokenise_file(filename)
    tokens2 = parse_indent(tokens)
    ast = parse_syntax(tokens2, promote)
    return ast


def parse_lines(s, promote=promote_token):
    tokens  = tokenise_lines(s)
    tokens2 = parse_indent(tokens)
    ast     = parse_syntax(tokens2, promote)
    return ast


def parse_syntax(tokens, promote):
    stack = [[]]

    for token in tokens:
        if token == '[':
            stack.append([])
        elif token == '(':
            stack.append(["ie/infix"])
        elif token == '{':
            stack.append(["ie/postfix"])
        elif token == 'ie/neoteric':
            stack.append(["ie/neoteric"])
        elif token in (']', ')', '}'):
            tos = stack.pop()
            try:
                if stack[-1][0] == 'ie/neoteric':
                    stack[-1].append(tos)
                    tos = stack.pop()
            except IndexError:
                pass
            stack[-1].append(tos)
        else:
            v = promote(token)
            stack[-1].append(v)

    prog = stack[0]
    return prog


def is_atom(x):
    if isinstance(x, list):
        return False

    if isinstance(x, tuple):
        return False

    return True


def print_expr(x, depth=0, print_brackets=True):
    s = expr_to_string(x, depth, print_brackets)
    print(s, end="")


def expr_to_string(x, depth=0, print_brackets=True):
    if is_atom(x):
        return str(x)

    buffer = []

    if print_brackets:
        buffer.append("[")

    if x == []:
        if print_brackets:
            buffer.append("]")
        return ''.join(buffer)

    car, *cdr = x

    if not is_atom(car):
        if not print_brackets:
            buffer.append("\\\\")
        buffer.append("\n" + (depth+1) * "    ")

    if car == 'ie/newline':
        buffer.append("\n" + (depth+1) * "    ")

    buffer.append(expr_to_string(car, depth+1, print_brackets))

    prev_it = car

    for it in cdr:
        if is_atom(it):
            if prev_it == 'ie/newline' and it == 'ie/newline':
                pass
            elif is_atom(prev_it):
                buffer.append(" ")
            else:
                buffer.append("\n" + (depth+1) * "    ")
                if not print_brackets:
                    buffer.append("\\ ")
        else:
            buffer.append("\n" + (depth+1) * "    ")

        if prev_it == 'ie/newline' and it == 'ie/newline':
            buffer.append("\n" + (depth+1) * "    ")

        buffer.append(expr_to_string(it, depth+1, print_brackets))

        prev_it = it

    if print_brackets:
        buffer.append("]")

    return ''.join(buffer)


def puts_expr(x, print_brackets=True):
    print_expr(x, 0, print_brackets)
    print()


def remove_newline(prog, newline='ie/newline'):
    return remove_markers(prog, [newline])


def remove_markers(prog, markers=['ie/newline', 'ie/backslash']):
    if is_atom(prog):
        return prog

    return [remove_markers(x, markers) for x in prog if x not in markers]


if __name__ == '__main__':
    prog = parse_file(sys.argv[1])
    for s in remove_newline(prog):
        puts_expr(s, True)
    print()



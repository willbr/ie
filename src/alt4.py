import fileinput

from sys import argv
from dataclasses import dataclass
from pprint import pprint

states = []
cmds = []

next_token = None

line_buffer = ""
line_buffer_len = 0
line_offset = 0

input = None

break_chars = " (){}[],\n"
evil_chars = '\r\t'
indent_width = 2
indent = 0
new_indent = 0


def get_word():
    state_fn = states[-1]
    return state_fn()


def push_state(fn, cmd=None):
    states.append(fn)
    cmds.append(cmd)


def pop_state():
    states.pop()
    return cmds.pop()


def main():
    global input
    global get_word

    input = fileinput.input(argv[1])
    get_line()

    push_state(get_indent_head, None)

    word = get_word()
    while word:
        print(f"{word=}")
        word = get_word()


def get_indent_head():
    t = get_token()
    if t is None:
        return None

    cmds[-1] = t

    push_state(get_indent_body)

    return get_word()


def get_indent_body():
    nt = peek_token()
    if nt is None:
        return None

    if nt == '\n':
        get_token()
        parse_indent()
        if new_indent == indent + 1:
            nt = peek_token()
            if nt == None:
                pass
            elif nt == '\\':
                get_token()
                t = get_token()
                return t
            else:
                assert False
        else:
            assert False

        pop_state()
        t = pop_state()
        push_state(get_indent_head, None)
        return t
    elif nt == '\\':
        get_token()
        t = get_token()
        return t

    t = get_token()

    return t


def get_line():
    global line_buffer
    global line_buffer_len
    global line_offset
    line_buffer = input.readline()
    line_buffer_len = len(line_buffer)
    line_offset = 0


def chomp(c):
    global line_offset
    nc = line_buffer[line_offset:line_offset+1]
    while nc == c and nc != '':
        line_offset += 1
        nc = line_buffer[line_offset]


def peek_token():
    global next_token
    if next_token == None:
        next_token = get_token()

    return next_token


def get_token():
    global line_offset
    global next_token

    if next_token:
        t, next_token = next_token, None
        return t

    if line_offset >= line_buffer_len:
        get_line()
        while line_buffer == '\n':
            get_line()
        if line_buffer == '':
            return None

    c = line_buffer[line_offset:line_offset+1]

    if c in evil_chars:
        print(f"{c=}")
        assert False

    start_pos = end_pos = line_offset

    if c == ' ':
        chomp(' ')
        token = line_buffer[start_pos:line_offset]
        return token

    if c in break_chars:
        line_offset += 1
        return c

    for c in line_buffer[line_offset:]:
        if c in break_chars:
            break
        end_pos += 1

    token = line_buffer[start_pos:end_pos]
    line_offset = end_pos

    chomp(' ')

    return token


def parse_indent():
    global new_indent

    s = peek_token()
    if s is None:
        return

    # print(f"{s=}")
    if s[0] == ' ':
        s = get_token()
        new_indent = len(s) // indent_width
        if len(s) % indent_width != 0:
            print(f"{len(s) % indent_width}")
            raise SyntaxError
    else:
        new_indent = 0


if __name__ == "__main__":
    main()

import pdb
import sys
import argparse


running = True
indent_width = 2
i = 0
code = ""
code_len = 0

prefix_chars = "(){}[],\"\'&*"
break_chars = " (){}[],\n"
indent = 0

trace = pdb.set_trace


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def print_token(t):
    print(t)
    # print((indent*"  ")+t)


def read_indent():
    global i
    global indent

    i += 1

    while i < code_len and code[i] == '\n':
        i += 1

    j = i
    while j < code_len and code[j] == " ":
        j += 1

    if j == code_len:
        i = j
        return

    if code[j] == '\n':
        i = j
        return

    delta = j - i
    if delta % 2 != 0:
        raise ValueError
    new_indent = delta // indent_width

    i = j

    if new_indent > indent + 1:
        eprint(f"\n=> goodbye {new_indent=} {indent=} {delta=}\n")
        raise ValueError

    indent = new_indent


def chomp(chars):
    global i
    while i < code_len and code[i] in chars:
        i += 1


def read_word():
    global i

    j = i
    while j < code_len and code[j] not in break_chars:
        j += 1

    if code[j] in "(){}[]":
        raise ValueError(f"invalid char following word: {code[i:j+1]}")

    word = code[i:j]
    i = j

    print_token(word)


def read_expr():
    global i
    # eprint("expr:", repr(code[i:]))

    if code[i:i+1] == '\n':
        raise ValueError(f"invalid char {code[i]=}")

    start_indent = indent

    print_token('[')

    while i < code_len:
        if code[i] == '\n':
            read_indent()
            if indent > start_indent:
                # eprint("indent:", repr(code[i:]))
                if code[i:i+1] == '':
                    break
                elif code[i:i+1] == '\\':
                    i += 1
                    if code[i:i+1] != ' ':
                        raise ValueError(f"invalid char {code[i:i+1]=}")
                    i += 1
                else:
                    read_expr()
                    if indent == start_indent:
                        break
            elif indent == start_indent:
                # eprint("newline:", repr(code[i:]))
                if code[i:i+1] != '\\':
                    break
                i += 1
                if code[i:i+1] != ' ':
                    raise ValueError(f"invalid char {code[i:i+1]=}")
            elif indent < start_indent:
                dedent = start_indent - indent
                # eprint(f"dedent: {dedent}, ", repr(code[i:]))
                break
        elif code[i] == ' ':
            raise ValueError(f"invalid char {code[i]=}")
        elif code[i] in prefix_chars:
            read_prefix()
        else:
            read_word()
        chomp(" ")

    print_token(']')


def read_prefix():
    global i

    if code[i] in "({[":
        print_token(code[i])
        i += 1
    elif code[i] in ")}]":
        print_token(code[i])
        i += 1
        if code[i:i+1] not in " \n(){}[],":
            raise ValueError(f"invalid char following close: {code[i]=}")
    elif code[i] in ",":
        print_token(code[i])
        i += 1
        if code[i:i+1] not in " \n(){}[]":
            raise ValueError(f"invalid char following close: {code[i]=}")
    elif code[i] == '*':
        j = i + 1
        while j < code_len and code[j] not in break_chars:
            j += 1
        word = code[i:j]

        if j == i + 1:
            print_token(word)
            i = j
        else:
            assert False, "deref?"
    elif code[i] == '"':
        j = i + 1
        in_string = True
        while j < code_len:
            c = code[j]
            j += 1
            if c == '"':
                in_string = False
                break
            elif c == '\\':
                assert False
        word = code[i:j]
        print(word)
        i = j
    else:
        raise ValueError(f"unknown prefix char: {code[i]=}")


def main():
    global code
    global code_len
    global indent

    parser = argparse.ArgumentParser(description='alt transformer')
    parser.add_argument('--echo-code', action='store_true')
    args = parser.parse_args()

    with open("src/tokens9.ie") as f:
        code = f.read()
        if args.echo_code:
            eprint(code.replace(' ', '.').replace('\n','\\n\n'))
            eprint()

    code_len = len(code)

    while i < code_len:
        # eprint("main:", repr(code[i:]))
        read_expr()

    eprint("\nend ########################################\n")


if __name__ == "__main__":
    main()



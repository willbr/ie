from rich import print
from rich.markup import escape
from rich.traceback import install
from rich.console import Console

install(show_locals=True)

console = Console(markup=False)
python_print = print
print = console.print

#####################################################

import fileinput
import textwrap
import sys


class Token(str):
    def __new__(cls, content, *args, **kwargs):
        return super().__new__(cls, content)

    def __init__(self, content, col_offset=0, lineno=0, filename=""):
        self.filename = filename
        self.lineno = lineno
        self.col_offset = col_offset


class Tokeniser():
    filename = ""
    lineno = 0
    file = None
    i = 0
    line = ""
    len_line = 0
    next_token = None
    breakchars = " \t\n,;()[]{}"
    prefix = {}

    def __init__(self):
        self.prefix = {
                '"':  read_string,
                '<<': read_heredoc,
                }
        pass

    def __del__(self):
        self.file.close()

    def __repr__(self):
        return f"<Tokeniser self.line={repr(self.line)}, {self.i=}"

    def read_tokens(self):
        try:
            while True:
                w = self.next_word()
                yield w
        except StopIteration:
            pass

    def get_line(self):
        self.line = next(self.file)
        self.len_line = len(self.line)
        self.lineno += 1
        self.i = 0

    def chomp(self, chars):
        while self.i < self.len_line and self.line[self.i] in chars:
            self.i += 1

    def chomp_until(self, break_chars):
        while self.i < self.len_line and self.line[self.i] not in break_chars:
            self.i += 1

    def read_token(self):
        start_pos = self.i
        self.chomp_until(self.breakchars)

        len_word = self.i - start_pos
        assert len_word
        word = self.line[start_pos:self.i]

        try:
            next_char = self.line[self.i]
        except IndexError:
            next_char = None

        if next_char == None:
            pass
        elif next_char in '({[':
            t = self.new_token(word, start_pos)
            self.push_token(t)

            word = "ie/neoteric"

        return self.new_token(word, col_offset=start_pos)

    def push_token(self, token):
        if self.next_token != None:
            print(repr(token))
            print(repr(self.next_token))
            assert False
        self.next_token = token

    def next_word(self):
        if self.next_token:
            t = self.next_token
            self.next_token = None
            return t

        if self.i >= self.len_line:
            self.get_line()

        col_offset = self.i
        lineno = self.lineno

        c = self.line[self.i]
        cc = self.line[self.i:self.i+2]
        prefix_fn = self.prefix.get(c, self.prefix.get(cc, None))

        if self.line[self.i] == '\n':
            self.get_line()
            self.chomp(' ')
            try:
                if self.i > 0 and self.line[self.i] != '\n':
                    t = self.new_token(' ' * self.i, col_offset)
                    self.push_token(t)
            except IndexError:
                pass
            word = 'ie/newline'

        elif self.line[self.i] == '\t':
            assert False

        elif prefix_fn:
            word = prefix_fn(self)

        elif self.line[self.i] in self.breakchars:
            word = self.line[self.i]
            t = self.new_token(word)
            self.i += 1

            try:
                next_char = self.line[self.i]
            except IndexError:
                next_char = ''

            if word == '[':
                pt = self.new_token("ie/prefix", self.i - 1)
                self.push_token(pt)
            elif word == ',':
                if next_char not in ' \n':
                    nt = self.new_token(next_char)
                    self.die(nt, "comma must be followed by white space")
            elif word in ')}]':
                if next_char not in ' ,\n)}]':
                    self.die(t, f"close marker '{word}' must be followed by another close marker ')}}]', comma  or whitespace")

        else:
            word = self.read_token()

        self.chomp(' ')
        return self.new_token(word, col_offset, lineno)


    def new_token(self, content, col_offset=None, lineno=None, filename=None):
        if col_offset == None:
            col_offset = self.i + 1

        if lineno == None:
            lineno = self.lineno

        if filename == None:
            filename = self.filename

        return Token(content, col_offset, lineno, filename)

    def die(self, t, msg):
        l = self.line.strip()
        i = self.i
        err_msg =textwrap.dedent(f"""
        {t.filename}:{t.lineno}:{t.col_offset}: {msg}

        {l}
        {" "*i}^
        """).strip()
        raise SyntaxError(err_msg)


def read_string(t):
    start_pos = t.i
    assert t.line[t.i] == '"'
    t.i += 1

    while t.i < t.len_line and t.line[t.i] not in '"':
        t.i += 1

    assert t.line[t.i] == '"'

    t.i += 1
    word = t.line[start_pos:t.i]
    return word

def read_heredoc(t):
    assert t.line[t.i] == '<'
    t.i += 1
    assert t.line[t.i] == '<'
    t.i += 1
    assert t.line[t.i] not in ' \t\n'
    end_marker = t.line[t.i:].strip()
    body = []
    while True:
        t.get_line()
        line = t.line.strip()
        if line == end_marker:
            break

        body.append(t.line)
    word = ''.join(body)
    assert word[-1] == '\n'
    word = f'"{word[:-1]}"'

    return word

def tokenise_file(filename):
    t = Tokeniser()
    t.filename = filename
    t.file = fileinput.input(filename)
    tokens  = t.read_tokens()
    return tokens

def tokenise_lines(lines):
    t = Tokeniser()
    t.filename = "lines"
    t.file = (line for line in lines)
    tokens  = t.read_tokens()
    return tokens

if __name__ == "__main__":
    filename = sys.argv[1]
    try:
        tokens  = tokenise_file(filename)
        print('\n'.join(tokens))
    except SyntaxError as e:
        print(f"[r]ERROR:[/]\n{escape(str(e))}")
        exit(1)


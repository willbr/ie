from typing import NamedTuple
from collections.abc import Iterable
import re

from itertools import (
    tee, islice, chain
    )

def hline(n=1, c='#', width=60, title=None):
    print('\n'*n)
    if title:
        print(f' {title}')
    print(c*width)
    print('\n'*n)


def peek(iterable):
    a, b = tee(iterable)
    c = chain(islice(b, 1, None), [None])
    return zip(a, c)


def strip_newlines(tree):
    return [leaf for leaf in tree if getattr(leaf, 'type', None) != 'NEWLINE']


def split_on_newline(tree: list):
    for i, x in enumerate(tree):
        if not isinstance(x, Token):
            continue
        elif x.type == 'NEWLINE':
            lhs = tree[:i]
            rhs = tree[i+1:]
            return lhs, rhs
    return tree.copy(), []


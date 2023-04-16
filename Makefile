
ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
	PATH := $(PATH);./bin/
	mkdir = mkdir
	make = make
	rm = del
	EXE = .exe
	ignore := rem
else
    detected_OS := $(shell uname)  # same as "uname -s"
	PATH := $(PATH):./bin/
	mkdir = mkdir
	make = make
	rm = rm
	EXE =
	ignore := echo ignore
endif

wwip:
	watchexec -cr -f "*.py" "make wip"

wip:
	python -m src.py

pytok-errors: 
	- python src/py/tokenise.py src/tests/tokeniser/tokens/err-00-comma/in.txt
	- python src/py/tokenise.py src/tests/tokeniser/tokens/err-01-whitespace/in.txt

wpytok-errors:
	watchexec -cr "make pytok-errors"


all:
	python -m indentexpr.ast

wall:
	watchexec -cr "make all"

install:
	python -m pip install -e .


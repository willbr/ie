
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

f  := ../tests/tokeniser/c/c00-hello-world/in.txt

ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
	f := $(subst /,\,$(f))
endif

w:
	watchexec -cr "make tokens3" --ignore "*.c"

build/%.c: %.c.ie
	toco -o $@ $<

tokens2: tokens2.c
	type $f
	tcc -run $< $f

tokens3: build/tokens3.c
	type $(f)
	tcc -run $< $f


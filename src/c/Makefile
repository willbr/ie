f=..\tests\tokeniser\c\c00-hello-world\in.txt

w:
	watchexec -cr "make tokens3"

build/%.c: %.c.ie
	toco -o $@ $<

tokens2: tokens2.c
	type $f
	tcc -run $< $f

tokens3: build/tokens3.c
	toco -o $< tokens3.c.ie 
	type $f
	tcc -run $< $f


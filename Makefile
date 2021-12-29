phony: watch run test1 test2 test3

test0: .\src\tokens2.ie
	type $<
	tcc -Wall -run src/tokeniser.c - < $<
	tcc -run src/tokeniser.c - < $< | python src/eval.py
	tcc -run src/tokeniser.c - < $< | tcc -run src/eval.c

test1: .\src\tokens1.ie
	type $<
	tcc -Wall -run src/tokeniser.c - < $<
	tcc -run src/tokeniser.c - < $< | python src/eval.py

test2: .\src\tokens2.ie
	type $<
	tcc -run src/main.c < $<
	tcc -run src/main.c < $< | python src/transform.py
	tcc -run src/main.c < $< | python src/transform.py | python src/eval.py

test3: .\src\tokens3.ie
	type $<
	tcc -run src/main.c < $<
	tcc -run src/main.c < $< | python src/transform.py
	tcc -run src/main.c < $< | python src/transform.py | python src/eval.py

test4: .\src\tokens4.ie
	type $<
	tcc -run src/main.c < $<
	tcc -run src/main.c < $< | python src/transform.py
	tcc -run src/main.c < $< | python src/transform.py | python src/eval.py

test5: .\src\tokens5.ie
	type $<
	tcc -run src/main.c < $<
	tcc -run src/main.c < $< | python src/transform.py
	tcc -run src/main.c < $< | python src/transform.py | python src/eval.py

alt:
	python src/alt.py --echo-code
	python src/alt.py | python src/eval.py --trace

alt2: .\src\tokens0.ie src/*.py
	type $<
	python src/alt2.py --echo-code < $<

walt2:
	watchexec -cr "make alt2"

alt3: .\src\tokens5.ie src/*.py
	type $<
	python src/alt3.py $<

walt3:
	watchexec -cr "make alt3"

alt4: .\src\c4.ie src/*.py
	type $<
	rem python src/alt4.py --wrap-everything $<
	python src/alt4.py --wrap-everything $< | python src/c-printer.py -

walt4:
	watchexec -cr "make alt4"

run: src/*.c
	tcc -run src/parse.c

test-c:
	python src/py/test-c.py

wtest-c:
	watchexec -cr "make test-c"

test-py: src/py/*.py
	python -m unittest src/test-eval.py -f

wtest-py:
	watchexec -cr "make test-py"

pparse:
	python src/py/pluck-test.py 3 | tcc -run src/c/parse.c - | python src/py/parse.py

wpparse:
	watchexec -cr "make pparse"

eval:
	python src/py/pluck-test.py 3 | tcc -run src/c/parse.c - | python src/py/eval.py

weval:
	watchexec -cr "make eval"

toco: src\examples\c4.ie
	type $<
	rem type $< | tcc -run src/c/parse.c -
	rem type $< | tcc -run src/c/parse.c - | python src/py/parse.py
	type $< | tcc -run src/c/parse.c - | python src/py/toco.py

wtoco:
	watchexec -cr "make toco"

simple: src\examples\tokens3.ie
	type $<
	type $< | tcc -run src/c/simple-tokens.c -
	type $< | tcc -run src/c/simple-tokens.c - | python src/py/parse.py
	rem type $< | tcc -run src/c/simple-tokens.c - | python src/py/toco.py

wsimple:
	watchexec -cr "make simple"

indent: src\examples\indent2.ie
	type $<
	rem type $< | tcc -run src/c/simple-tokens.c -
	type $< | tcc -run src/c/simple-tokens.c - | python src/py/parse.py

windent:
	watchexec -cr "make indent"

watch:
	watchexec -cr "make run"

pytok: src\examples\indent4.ie
	type $<
	python src/py/tokenise.py $< | python src/py/parse2.py -
	python src/py/tokenise.py $< | python src/py/parse2.py - | python src/py/parse.py

wpytok:
	watchexec -cr "make pytok"


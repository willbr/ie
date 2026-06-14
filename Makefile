CC     := cc
CFLAGS := -std=c99 -Wall -Wextra
BUILD  := build
PARSE  := $(BUILD)/parse
TESTBIN := $(BUILD)/test

f ?= src/tests/tokeniser/c/c00-hello-world/in.txt

all: $(PARSE)

$(PARSE): src/c/parse.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

run: $(PARSE)
	$(PARSE) $(f)

$(TESTBIN): test.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

test: $(PARSE) $(TESTBIN)
	$(TESTBIN)

bless: $(PARSE) $(TESTBIN)
	$(TESTBIN) bless

clean:
	rm -rf $(BUILD)

.PHONY: all run test bless clean

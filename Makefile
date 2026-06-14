CC     := cc
CFLAGS := -std=c99 -Wall -Wextra
BUILD  := build
PARSE  := $(BUILD)/parse

f ?= src/tests/tokeniser/c/c00-hello-world/in.txt

all: $(PARSE)

$(PARSE): src/c/parse.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

run: $(PARSE)
	$(PARSE) $(f)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean

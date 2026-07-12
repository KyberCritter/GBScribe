# Host-side unit tests (gcc). Run: make -C tests/host run   (in WSL or any POSIX box)
CC      := gcc
CFLAGS  := -O1 -g -Wall -Wextra -I../../src
BIN     := bin

TESTS := $(BIN)/test_layout $(BIN)/test_printer

.PHONY: run clean
run: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; ./$$t; done

$(BIN)/test_layout: test_layout.c ../../src/core/layout.c ../../src/core/document.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

# test_printer arrives with M4; keep the target harmless until then
$(BIN)/test_printer:
	@mkdir -p $(BIN)
	@printf '#!/bin/sh\nexit 0\n' > $@ && chmod +x $@

clean:
	rm -rf $(BIN)

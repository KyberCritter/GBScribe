# Host-side unit tests (gcc). Run: make -C tests/host run   (in WSL or any POSIX box)
CC      := gcc
CFLAGS  := -O1 -g -Wall -Wextra -I../../src
BIN     := bin

TESTS := $(BIN)/test_layout $(BIN)/test_printer

.PHONY: run clean
run: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; ./$$t; done

$(BIN)/test_layout: test_layout.c ../../src/core/layout.c ../../src/core/document.c ../../src/core/manual.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/test_printer: test_printer.c ../../src/printer/gbp_packet.c ../../src/printer/gbp_print.c \
                     ../../src/core/layout.c ../../src/core/document.c ../../src/gfx/font2bpp.c \
                     decoder/gpbdecoder
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

decoder/gpbdecoder:
	$(MAKE) -C decoder

clean:
	rm -rf $(BIN)

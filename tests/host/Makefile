# Host-side unit tests (gcc). Run: make -C tests/host run   (in WSL or any POSIX box)
CC      := gcc
CFLAGS  := -O1 -g -Wall -Wextra -I../../src -I../../lib/AdvancePrint/include
BIN     := bin

TESTS := $(BIN)/test_layout $(BIN)/test_printer

.PHONY: run clean
run: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; ./$$t; done

$(BIN)/test_layout: test_layout.c ../../src/core/layout.c ../../src/core/document.c ../../src/core/manual.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

# Printer path now compiles the AdvancePrint sources directly (the same ones the
# ROM links) plus GBScribe's generated print font.
$(BIN)/test_printer: test_printer.c \
                     ../../lib/AdvancePrint/src/advanceprint_packet.c \
                     ../../lib/AdvancePrint/src/advanceprint_text.c \
                     ../../lib/AdvancePrint/src/advanceprint_page.c \
                     ../../lib/AdvancePrint/src/advanceprint_font.c \
                     ../../src/gfx/font2bpp.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BIN)

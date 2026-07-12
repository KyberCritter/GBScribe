// Host unit tests for the portable core: document + layout engines.
#include <stdio.h>
#include <string.h>
#include "core/document.h"
#include "core/layout.h"

static int fails, checks;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

static Layout lo;

static void reflow(const char *s, int width, WrapMode m)
{
    layout_reflow(&lo, s, (int)strlen(s), width, m);
}

static void expect_lines(const char *s, int width, WrapMode m, const char **want, int n)
{
    reflow(s, width, m);
    CHECK(lo.count == n, "line count %d != %d for '%s'", lo.count, n, s);
    for (int i = 0; i < n && i < lo.count; i++) {
        int len = layout_line_len(&lo, s, (int)strlen(s), i);
        CHECK(len == (int)strlen(want[i]) && !strncmp(s + lo.start[i], want[i], len),
              "line %d: got '%.*s' want '%s' (src '%s')", i, len, s + lo.start[i], want[i], s);
    }
}

static void test_wrap_basic(void)
{
    // no wrapping needed
    expect_lines("hello", 20, WRAP_POST_WORD, (const char *[]){"hello"}, 1);
    // empty text = one empty line
    reflow("", 20, WRAP_POST_WORD);
    CHECK(lo.count == 1 && lo.start[0] == 0, "empty doc");
}

static void test_wrap_mid_word(void)
{
    expect_lines("abcdefgh", 5, WRAP_MID_WORD, (const char *[]){"abcde", "fgh"}, 2);
    expect_lines("abc def gh", 5, WRAP_MID_WORD, (const char *[]){"abc d", "ef gh"}, 2);
    // exactly full line, no spill
    expect_lines("abcde", 5, WRAP_MID_WORD, (const char *[]){"abcde"}, 1);
}

static void test_wrap_post_word(void)
{
    expect_lines("abc def", 5, WRAP_POST_WORD, (const char *[]){"abc ", "def"}, 2);
    // long word falls back to hard break
    expect_lines("abcdefgh", 5, WRAP_POST_WORD, (const char *[]){"abcde", "fgh"}, 2);
    expect_lines("ab cdefgh", 5, WRAP_POST_WORD, (const char *[]){"ab ", "cdefg", "h"}, 3);
    // space at the boundary: overflows invisibly, next word starts clean
    expect_lines("abcde fgh", 5, WRAP_POST_WORD, (const char *[]){"abcde ", "fgh"}, 2);
    expect_lines("abcde   fgh", 5, WRAP_POST_WORD, (const char *[]){"abcde   ", "fgh"}, 2);
}

static void test_newlines(void)
{
    expect_lines("ab\ncd", 5, WRAP_POST_WORD, (const char *[]){"ab", "cd"}, 2);
    expect_lines("ab\n\ncd", 5, WRAP_POST_WORD, (const char *[]){"ab", "", "cd"}, 3);
    // trailing newline creates a final empty line
    expect_lines("ab\n", 5, WRAP_POST_WORD, (const char *[]){"ab", ""}, 2);
    expect_lines("\n", 5, WRAP_POST_WORD, (const char *[]){"", ""}, 2);
}

static void test_caret_line(void)
{
    const char *s = "abc def";           // lines: "abc " / "def" at width 5
    reflow(s, 5, WRAP_POST_WORD);
    CHECK(layout_caret_line(&lo, 0) == 0, "caret 0");
    CHECK(layout_caret_line(&lo, 3) == 0, "caret 3");
    CHECK(layout_caret_line(&lo, 4) == 1, "caret 4 = start of line 1");
    CHECK(layout_caret_line(&lo, 7) == 1, "caret at end");

    reflow("ab\ncd", 5, WRAP_POST_WORD);
    CHECK(layout_caret_line(&lo, 2) == 0, "caret before newline");
    CHECK(layout_caret_line(&lo, 3) == 1, "caret after newline");
}

static void test_document(void)
{
    static Document d;
    doc_init(&d);
    CHECK(d.len == 0 && d.caret == 0, "init");

    for (const char *p = "held"; *p; p++) doc_insert(&d, *p);
    CHECK(d.len == 4 && !strncmp(d.text, "held", 4), "append");

    // insert in the middle: he|ld -> hello world... keep simple: caret 2, insert 'X'
    doc_caret_set(&d, 2);
    doc_insert(&d, 'X');
    CHECK(d.len == 5 && !strncmp(d.text, "heXld", 5) && d.caret == 3, "mid insert");

    doc_backspace(&d);
    CHECK(d.len == 4 && !strncmp(d.text, "held", 4) && d.caret == 2, "backspace");

    doc_delete(&d);
    CHECK(d.len == 3 && !strncmp(d.text, "hed", 3) && d.caret == 2, "delete");

    doc_caret_set(&d, 0);
    CHECK(!doc_backspace(&d), "backspace at start refused");
    doc_caret_set(&d, d.len);
    CHECK(!doc_delete(&d), "delete at end refused");

    // fill to capacity
    doc_init(&d);
    for (int i = 0; i < DOC_MAX + 10; i++) doc_insert(&d, 'a');
    CHECK(d.len == DOC_MAX, "capacity clamped");
}

static void test_full_page_doc(void)
{
    // a page worth of text wraps to the expected number of 20-col lines
    static char big[16001];
    for (int i = 0; i < 1000; i++) big[i] = (i % 10 == 9) ? ' ' : 'a';
    big[1000] = 0;
    reflow(big, 20, WRAP_POST_WORD);
    CHECK(lo.count == 50, "1000 chars of 10-char words at 20 cols = 50 lines (got %d)", lo.count);
    reflow(big, 20, WRAP_MID_WORD);
    CHECK(lo.count == 50, "mid-word same density (got %d)", lo.count);
}

int main(void)
{
    test_wrap_basic();
    test_wrap_mid_word();
    test_wrap_post_word();
    test_newlines();
    test_caret_line();
    test_document();
    test_full_page_doc();
    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}

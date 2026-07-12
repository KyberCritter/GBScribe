// Document model: flat char buffer + caret. Portable C — no GBA headers.
#ifndef CORE_DOCUMENT_H
#define CORE_DOCUMENT_H

#include <stdbool.h>

#define DOC_MAX 16000

typedef struct {
    char text[DOC_MAX];
    int  len;
    int  caret;        // insertion point, 0..len
} Document;

void doc_init(Document *d);
bool doc_insert(Document *d, char c);      // insert at caret, caret advances
bool doc_backspace(Document *d);           // delete char before caret
bool doc_delete(Document *d);              // delete char at caret
void doc_caret_set(Document *d, int pos);  // clamped

#endif

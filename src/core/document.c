#include <string.h>
#include "document.h"

void doc_init(Document *d)
{
    d->len = 0;
    d->caret = 0;
}

bool doc_insert(Document *d, char c)
{
    if (d->len >= DOC_MAX)
        return false;
    memmove(d->text + d->caret + 1, d->text + d->caret, d->len - d->caret);
    d->text[d->caret++] = c;
    d->len++;
    return true;
}

bool doc_backspace(Document *d)
{
    if (d->caret == 0)
        return false;
    memmove(d->text + d->caret - 1, d->text + d->caret, d->len - d->caret);
    d->caret--;
    d->len--;
    return true;
}

bool doc_delete(Document *d)
{
    if (d->caret >= d->len)
        return false;
    memmove(d->text + d->caret, d->text + d->caret + 1, d->len - d->caret - 1);
    d->len--;
    return true;
}

void doc_caret_set(Document *d, int pos)
{
    if (pos < 0) pos = 0;
    if (pos > d->len) pos = d->len;
    d->caret = pos;
}

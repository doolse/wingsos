#ifndef _winforms_h_
#define _winforms_h_

#include <winlib.h>

typedef struct HTMLCell {
    struct HTMLCell *Next;
    struct HTMLCell *Prev;
    int NewRow;
    uint ColSpan;
    uint RowSpan;
    int Type;
    char *Value;
    char *Name;
    char TabLay[6];
} HTMLCell;

typedef struct HTMLRow {
    struct HTMLRow *Next;
    struct HTMLRow *Prev;
    int Height;
} HTMLRow;

typedef struct HTMLTable {
    HTMLCell *FirstCell;
    HTMLRow *FirstRow;
    int Rows;
    int Cols;
} HTMLTable;

extern JTab *JFormCreate(HTMLTable *Table);
extern HTMLTable *JFormLoad(char *name);

#endif

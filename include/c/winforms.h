#ifndef _winforms_h_
#define _winforms_h_

#include <winlib.h>

typedef struct HTMLCell {
    struct HTMLCell *Next;
    struct HTMLCell *Prev;
    struct HTMLTable *Inner;
    JWin *Win;
    int NewRow;
    uint ColSpan;
    uint RowSpan;
    int Type;
    char *Value;
    char *Name;
    int Width;
    char TabLay[6];
} HTMLCell;

typedef struct HTMLRow {
    struct HTMLRow *Next;
    struct HTMLRow *Prev;
    int Height;
} HTMLRow;

typedef struct HTMLTable {
    struct HTMLTable *Next;
    struct HTMLTable *Prev;
    char *Name;
    HTMLCell *FirstCell;
    HTMLRow *FirstRow;
    int Rows;
    int Cols;
} HTMLTable;

typedef struct HTMLForms {
    HTMLTable *FirstTable;
} HTMLForms;

extern JTab *JFormCreate(HTMLTable *Table);
extern HTMLForms *JFormLoad(char *fname);
extern HTMLTable *JFormGetTable(HTMLForms *forms, char *name);
extern JWin *JFormGetControl(HTMLTable *table, char *name);



#endif

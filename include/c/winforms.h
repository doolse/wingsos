#ifndef _winforms_h_
#define _winforms_h_

#include <winlib.h>
#include <xmldom.h>

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
    SizeHints Hints;
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

typedef struct XMLGuiMap {
    char *XMLNode;
    char *GuiName;
    uint16 Field;
    uint Type;
    JWin *Win;
} XMLGuiMap;

typedef void(*Create_call)(HTMLCell *Cell, void *state);

extern JTab *JFormCreate(HTMLTable *Table, Create_call call, void *state);
extern HTMLForms *JFormLoad(char *fname);
extern HTMLTable *JFormGetTable(HTMLForms *forms, char *name);
extern JWin *JFormGetControl(HTMLTable *table, char *name);

extern void JMapBind(HTMLTable *table, XMLGuiMap *mappings, uint nrmappings);
extern void JMapFromXML(void *record, DOMElement *root, XMLGuiMap *mappings, uint nrmappings);
extern void JMapToXML(void *record, DOMElement *root, XMLGuiMap *mappings, uint nrmappings);
extern void JMapToGUI(void *record, XMLGuiMap *mappings, uint nrmappings);
extern void JMapFromGUI(void *record, XMLGuiMap *mappings, uint nrmappings);

enum
{
    T_INT16= 1,
    T_INT32 = 2,
    T_STRING = 3,
};

#endif

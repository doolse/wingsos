#include <winlib.h>

JBut *JButStandard(const char *ID)
{
    JStdItem item;
    if (JStdLookup(ID, &item))
	ID = item.Label;
    return JButInit(NULL, ID);
}

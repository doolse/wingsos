#include <winlib.h>

static JStdItem stditems[] =
{
    {JSTD_OK, "OK"},
    {JSTD_CANCEL,"CANCEL"},
    {JSTD_YES,"YES"},
    {JSTD_NO, "NO"},
    {JSTD_APPLY, "APPLY"}
};

#define NRSTD 5

int JStdLookup(const char *id, JStdItem *ret)
{
    uint i;
    JStdItem *item;
    
    item = &stditems[0];
    for (i=0; i<NRSTD; i++)
    {
	if (!strcmp(id, item->ID))
	    break;
	item++;
    }
    if (i==NRSTD)
	return 0;
    memcpy(ret, item, sizeof(JStdItem));
    return 1;
}

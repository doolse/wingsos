#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <xmldom.h>
#include <exception.h>

int main(int argc, char *argv[]) {

    DOMElement *root;
    char *name;
    char *data;
    int ex;
    name = fpathname("xmltest.xml", getappdir(), 1);
    printf("Parsing '%s'\n", name);
    Try
    {
        root = XMLloadFile(name);
        printf("Parsed\n");
        XMLprint(root, stdout);
    }
    Catch2(ex, data)
    {
	fprintf(stderr, "Code %x\n", ex);
	errexc(ex, data);
    }
}

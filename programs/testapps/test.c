#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <xmldom.h>
#include <exception.h>

int main (int argc, char *argv[])
{
	int a;
	DOMElement *root;
	Try {
		root = XMLloadFile("/wings/programs/utils/xmltest.xml");
		XMLprint(root, stdout);
		{
		DOMNode *rem;
		DOMElement *word2 = XMLgetNode(root, "xml/words/word")->NextElem;
		XMLsetAttr(word2, "name", "value");
		XMLprint(word2, stdout);
		XMLsetAttr(word2, "name", "value2");
		XMLprint(word2, stdout);
		rem = XMLfindAttr(word2, "attr");
		XMLremNode(rem);
		XMLprint(word2, stdout);
		XMLwriteFile(root, "file.txt");
		}
	}
	Catch(a) {
		printf("Caught an exception %u\n", a);
	}
}

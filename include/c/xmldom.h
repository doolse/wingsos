#ifndef _xmldom_h_
#define _xmldom_h_

#include <stdio.h>

enum {
NodeType_Element=1,
NodeType_Attribute,
NodeType_Document,
NodeType_Comment,
NodeType_CDATA,
};

typedef struct DOMNode_s {
	struct DOMNode_s *Next;
	struct DOMNode_s *Prev;
	struct DOMElement_s *Parent;
	char *Name;
	char *Value;
	unsigned short Type;
	int First;
} DOMNode;

typedef struct DOMElement_s {
	DOMNode Node;
	struct DOMElement_s *NextElem;
	struct DOMElement_s *PrevElem;
	int FirstElem;
	int CDATA;
	DOMNode *Attr;
	struct DOMElement_s *Children;
	struct DOMElement_s *Elements;
	unsigned int NumElements;
} DOMElement;

DOMElement *XMLloadFile(char *fname);
void XMLsaveFile(DOMElement *root, char *fname);
DOMElement *XMLload(FILE *fp);
DOMElement *XMLgetNode(DOMElement *root, char *path);
char *XMLgetAttr(DOMElement *node, char *name);
DOMNode *XMLfindAttr(DOMElement *node, char *name);
void XMLsetAttr(DOMElement *node, char *name, char *value);
void *XMLnewNode(int Type, char *name, char *value);
void XMLinsert(DOMElement *elem, void *insp, void *node);
void XMLremNode(void *node);
void XMLprint(void *node, FILE *out);

#endif

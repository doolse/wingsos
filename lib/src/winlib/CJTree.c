#include <winlib.h>
#include <string.h>
#include <stdlib.h>
#include <wgs/util.h>

VNode *JTreeAddView(JTree *Self, VNode *Parent, TNode *Node)
{
    VNode *ret;
    TModel *mod = Self->Model;
    int flags = Node->Flags;
    ret = calloc(sizeof(VNode), 1);
    ret->Value = Node;
    ret->Tree = Self;
    ret->NextView = Node->NextView;
    ret->Flags = flags;
    ret->Parent = Parent;
    Node->NextView = ret;
    if (flags&JItemF_Expandable)
    {
	JIter *iter = TModelIter(mod, Node);
	while(JIterHasNext(iter))
	{
	    TNode *obj = JIterNext(iter);
	    JTreeAddView(Self, ret, obj);
	}
	TModelFinIter(mod, iter);
    }
    if (Parent)
    {
	Vec *vec = Parent->Children;
	if (!vec)
	{
	    vec = VecInit(NULL);
	    Parent->Children = vec;
	}
	VecAdd(vec, ret);
    }
    return ret;
}

void JTreeRemView(JTree *Self, VNode *Node)
{
    VNode *Parent = Node->Parent;
    Vec *vec = Parent->Children;
//    printf("We're removing this now %lx %lx %lx\n", Self, Parent, Node);
    VecRemove(vec, Node);
}


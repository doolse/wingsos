#include <winlib.h>
#include <string.h>
#include <stdlib.h>
#include <wgs/util.h>

void *JTModelRoot(JTModel *Self)
{
    return Self->Root;
}

JIter *JTModelIter(JTModel *Self, DefNode *Node)
{
    if (!Node->Children)
    {
	return JIterInit(NULL);
    }
    else return VecIter(Node->Children);
}

void JTModelFinIter(JTModel *Self, JIter *iter)
{
    free(iter);
}

int JTModelCount(JTModel *Self, DefNode *Node)
{
    return 1;
}

void JTModelAppend(JTModel *Self, DefNode *Parent, DefNode *Node)
{
    VNode *view;
    Vec *vec = Parent->Children;
    if (!vec)
    {
	vec = VecInit(NULL);
	Parent->Children = vec;
    }
    VecAdd(vec, Node);
    view = Parent->tnode.NextView;
    while (view)
    {
	JTreeAddView(view->Tree, view, &Node->tnode);
	view = view->NextView;
    }
}

void JTModelRemove(DefNode *Node)
{
    
}

void JTModelExpand(JTModel *Self, DefNode *Node)
{
    if (Self->Expander)
	Self->Expander(Self, Node);
}

#include <winlib.h>
#include <string.h>
#include <stdlib.h>
#include <wgs/util.h>

void *JLModelRoot(JTModel *Self)
{
    return &Self->Root;
}

JIter *JLModelIter(JLModel *Self, DefNode *Node)
{
    return VecIter(Self->Vec);
}

void JLModelFinIter(JLModel *Self, JIter *iter)
{
    free(iter);
}

int JLModelCount(JLModel *Self, DefNode *Node)
{
    return 1;
}

void JLModelAppend(JLModel *Self, TNode *Node)
{
    VNode *view;
    VecAdd(Self->Vec, Node);
    view = Self->Root.NextView;
    while (view)
    {
	JTreeAddView(view->Tree, view, Node);
	view = view->NextView;
    }
}

void JLModelRemove(JLModel *Self, DefNode *Node)
{
    
}

void JLModelExpand(JTModel *Self, DefNode *Node)
{
    return;
}

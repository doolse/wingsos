#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

typedef struct OurModel {
struct OurModel *Next;
struct OurModel *Prev;
struct OurModel *Children;
struct OurModel *Parent;
char *Name;
char *FullName;
char Length[12];
int Dir;
} OurModel;

OurModel *head;
JItem *itemhead;

JItem RootItem;
OurModel RootModel;
		
void expand(JItem *pare) {

	DIR *dir;
	struct dirent *entry;
	struct stat buf;
	int err;
	char *fullname;
	OurModel *cur;
	JItem *item;
	OurModel *mpare = pare->Data;
	
	if (!(pare->Flags&JItemF_Expanded) && !pare->Children)
	{
		dir = opendir(mpare->FullName);
		if (!dir) 
		{
			perror("ls");
			return;
		}
		else
		{
			while (entry = readdir(dir)) {
				fullname = fpathname(entry->d_name, mpare->FullName, 1);
				err = stat(fullname, &buf);
				if (err != -1)
				{
					JItem *head = pare->Children;
					OurModel *mhead = mpare->Children;

					cur = calloc(1, sizeof(OurModel));
					item = calloc(1, sizeof(JItem));

					cur->Name = strdup(entry->d_name);
					cur->FullName = fullname;
					sprintf(cur->Length, "%ld", buf.st_size);
					if (S_ISDIR(buf.st_mode))
					{
						cur->Dir = 1;
						item->Flags |= JItemF_Expandable;
					}

					item->Parent = pare;
					item->Data = cur;
					cur->Parent = mpare;

					pare->Children = addQueueB(head, head, item);
					mpare->Children = addQueueB(mhead, mhead, cur);
					printf("File %s\n", fullname);
				}
				else 
				{
					fprintf(stderr, "ls:");
					perror(entry->d_name);
					free(fullname);
				}
			}
		closedir(dir);
		}
	}
	pare->Flags ^= JItemF_Expanded;
}

int main(int argc, char *argv[]) {

	void *App,*wnd,*tree;
	
   	retexit(1);
	App = JAppInit(NULL,0);
	wnd = JWndInit(NULL, NULL, 0, "Hello", 0);
	
	RootItem.Data = &RootModel;
	RootItem.Flags = JItemF_Expandable;
	RootModel.Name = ".";
	RootModel.FullName = ".";
	RootModel.Dir = 1;
	
	expand(&RootItem);
	tree = JTreInit(NULL, wnd, 0, &RootItem, expand);
	JWinGeom(tree, 0, 0, 0, 0, GEOM_TopLeft | GEOM_BotRight2);
	JTreAddColumns(tree, NULL, 
		"Name", OFFSET(OurModel, Name), 80, JColF_STRING|JColF_Indent, 
		"Length", OFFSET(OurModel, Length), 40, JColF_CHARS, 
		NULL); 
	JWinShow(wnd);
	JAppLoop(App);
}

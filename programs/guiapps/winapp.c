#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

typedef struct OurModel {
DefNode defnode;
char *Name;
unsigned char *Icon;
unsigned char *Icon2;
char *FullName;
unsigned long Length;
char *LengthStr;
char LengthCh[12];
} OurModel;

OurModel RootModel;

unsigned char icon[] = {
0x00, 0x07, 0x08, 0x17, 0x10, 0x10, 0x10, 0x0f,
0x00, 0x00, 0xf8, 0x04, 0x04, 0x04, 0x04, 0xf8,
0x97, 0x97
};

unsigned char icon2[] = {
0x1f, 0x10, 0x16, 0x10, 0x17, 0x10, 0x1f, 0x00,
0xf8, 0x08, 0xe8, 0x08, 0x68, 0x08, 0xf8, 0x00,
0x01, 0x01
};
		
unsigned char icon3[] = {
0x00, 0x0e, 0x11, 0x2e, 0x20, 0x10, 0x10, 0x0f,
0x00, 0x00, 0xfc, 0x0e, 0x0e, 0x04, 0x04, 0xf8,
0x97, 0x97
};

void itemclicked(JTree *tree, OurModel *item)
{
    printf("Clicked %lx\n", item);
    JTModelRemove(NULL, (DefNode *)item);
}

void expand(JTModel *Model, OurModel *pare) {

	DIR *dir;
	struct dirent *entry;
	struct stat buf;
	int err;
	unsigned int num;
	char *fullname;
	OurModel *cur;
	
	dir = opendir(pare->FullName);
	if (!dir) 
	{
		perror("ls");
		return;
	}
	while (entry = readdir(dir)) 
	{
		fullname = fpathname(entry->d_name, pare->FullName, 1);
		err = stat(fullname, &buf);
		if (err != -1)
		{
			cur = calloc(1, sizeof(OurModel));

			cur->Name = strdup(entry->d_name);
			cur->Icon = icon2;
			cur->FullName = fullname;
			cur->Length = buf.st_size;
			cur->LengthStr = cur->LengthCh;
			sprintf(cur->LengthCh, "%ld", buf.st_size);
			if (S_ISDIR(buf.st_mode))
			{
				cur->defnode.tnode.Flags = JItemF_Expandable;
				cur->Icon = icon;
				cur->Icon2 = icon3;
			}
			printf("File %lx %lx %lx %s\n", Model, pare, cur, fullname);
			
			JTModelAppend(Model, (DefNode *)pare, (DefNode *)cur);
			num++;
		}
		else 
		{
			fprintf(stderr, "ls:");
			perror(entry->d_name);
			free(fullname);
		}
	}
	closedir(dir);
	if (!num)
	{
		pare->defnode.tnode.Flags ^= JItemF_Expandable;
	}
}

/*
void expand(JLModel *Model, OurModel *pare) {

	DIR *dir;
	struct dirent *entry;
	struct stat buf;
	int err;
	unsigned int num;
	char *fullname;
	OurModel *cur;
	
	dir = opendir(pare->FullName);
	if (!dir) 
	{
		perror("ls");
		return;
	}
	while (entry = readdir(dir)) 
	{
		fullname = fpathname(entry->d_name, pare->FullName, 1);
		err = stat(fullname, &buf);
		if (err != -1)
		{
			cur = calloc(1, sizeof(OurModel));

			cur->Name = strdup(entry->d_name);
			cur->Icon = icon2;
			cur->FullName = fullname;
			cur->Length = buf.st_size;
			cur->LengthStr = cur->LengthCh;
			sprintf(cur->LengthCh, "%ld", buf.st_size);
			if (S_ISDIR(buf.st_mode))
			{
//				cur->defnode.tnode.Flags = JItemF_Expandable;
				cur->Icon = icon;
				cur->Icon2 = icon3;
			}
			printf("File %lx %lx %lx %s\n", Model, pare, cur, fullname);
			
			JLModelAppend(Model, (TNode *)cur);
			num++;
		}
		else 
		{
			fprintf(stderr, "ls:");
			perror(entry->d_name);
			free(fullname);
		}
	}
	closedir(dir);
	if (!num)
	{
//		pare->defnode.tnode.Flags ^= JItemF_Expandable;
	}
}
*/
int main(int argc, char *argv[]) {
	void *App,*wnd,*wnd2,*scr,*scr2;
	JTree *tree,*tree2;
	SizeHints sizes;
	JTModel *Model;
	JMeta * metadata = malloc(sizeof(JMeta));
	
   	retexit(1);
	App = JAppInit(NULL,0);

        metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
	metadata->title = "winapp";
	metadata->icon = NULL;
	metadata->showicon = 1;
	metadata->parentreg = -1;

	wnd2 = JWndInit(NULL, "Window 2", JWndF_Resizable,metadata);

	metadata = malloc(sizeof(JMeta));
	metadata->showicon = 1;
	metadata->parentreg = ((JW*)wnd2)->RegID;
	
	wnd = JWndInit(NULL, "Window 1", 0,metadata);
	JAppSetMain(App,wnd2);

	Model = JTModelInit(NULL, (DefNode *)&RootModel, (TreeExpander)expand);
//	Model = JLModelInit(NULL);
	RootModel.Name = ".";
	RootModel.FullName = ".";
	expand(Model, &RootModel);
	
	tree = JTreeInit(NULL, (TModel *)Model);
	tree2 = JTreeInit(NULL, (TModel *)Model);
	JWinCallback(tree, JTree, Clicked, (JTreeClicked)itemclicked);
	
	JTreeAddColumns(tree, NULL, 
		"Name", OFFSET(OurModel, Name), 120, JColF_STRING|JColF_Icon|JColF_2Icons|JColF_Indent, 
		"Length", OFFSET(OurModel, Length), 40, JColF_STRING|JColF_LongSort, 
		NULL); 
	scr = JScrInit(NULL, tree, JScrF_VNotEnd);

	JTreeAddColumns(tree2, NULL, 
		"Name", OFFSET(OurModel, Name), 120, JColF_STRING|JColF_Icon|JColF_2Icons|JColF_Indent, 
		"Length", OFFSET(OurModel, Length), 40, JColF_STRING|JColF_LongSort, 
		NULL); 
	scr2 = JScrInit(NULL, tree2, JScrF_VNotEnd);
	
/*	JTreeSort(tree, NULL);
	JTreeSort(tree2, NULL);
	JViewSync(tree);
	JViewSync(tree2);*/
	
/*	JCntGetHints(tree, &sizes);
	JWSetBounds(wnd, 0,0, sizes.PrefX, sizes.PrefY);*/

	JCntAdd(wnd, scr);
	JCntAdd(wnd2, scr2);
	JWinShow(wnd);
	JWinShow(wnd2);
	JAppLoop(App);
	return(0);
}


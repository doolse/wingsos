#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

typedef struct OurModel {
JTreeRow treerow;
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

void expand(OurModel *pare) {

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
				((JListRow *)cur)->Flags = JItemF_Expandable;
				cur->Icon = icon;
				cur->Icon2 = icon3;
			}
			printf("File %s\n", fullname);
			
			JTreAppendRow(pare, cur);
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
		((JListRow *)pare)->Flags ^= JItemF_Expandable;
	}
}

int main(int argc, char *argv[]) {

	void *App,*wnd,*wnd2,*tree,*tree2,*scr,*scr2;
	SizeHints sizes;
	
   	retexit(1);
	App = JAppInit(NULL,0);
	wnd = JWndInit(NULL, "Hello", 0);
	wnd2 = JWndInit(NULL, "Hello 2", 0);
	
	RootModel.Name = ".";
	RootModel.FullName = ".";
	RootModel.treerow.jlr.Flags = JItemF_Expandable;
	expand(&RootModel);
	
	tree = JTreInit(NULL, &RootModel, expand);
	tree2 = JTreInit(NULL, &RootModel, expand);
	
	JTreAddColumns(tree, NULL, 
		"Name", OFFSET(OurModel, Name), 120, JColF_STRING|JColF_Icon|JColF_2Icons|JColF_Indent, 
		"Length", OFFSET(OurModel, Length), 40, JColF_STRING|JColF_LongSort, 
		NULL); 
	scr = JScrInit(NULL, tree, 0);

	JTreAddColumns(tree2, NULL, 
		"Name", OFFSET(OurModel, Name), 120, JColF_STRING|JColF_Icon|JColF_2Icons|JColF_Indent, 
		"Length", OFFSET(OurModel, Length), 40, JColF_STRING|JColF_LongSort, 
		NULL); 
	scr2 = JScrInit(NULL, tree2, 0);
	JTreSort(tree, NULL);
	JTreSort(tree2, NULL);
	JViewSync(tree);
	JViewSync(tree2);
	
/*	JCntGetHints(tree, &sizes);
	JWSetBounds(wnd, 0,0, sizes.PrefX, sizes.PrefY);*/
			
	JCntAdd(wnd, scr);
	JCntAdd(wnd2, scr2);
	JWinShow(wnd);
	JWinShow(wnd2);
	JAppLoop(App);
}

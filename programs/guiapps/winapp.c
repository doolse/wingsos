#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

typedef struct OurModel {
JTreeRow treerow;
unsigned char *Icon;
unsigned char *Icon2;
char *Name;
char *FullName;
char Length[12];
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

void expand(JTre *Self, OurModel *pare) {

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
			sprintf(cur->Length, "%ld", buf.st_size);
			if (S_ISDIR(buf.st_mode))
			{
				cur->treerow.Flags = JItemF_Expandable;
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
		pare->treerow.Flags ^= JItemF_Expandable;
	}
}

int main(int argc, char *argv[]) {

	void *App,*wnd,*tree,*scr;
	SizeHints sizes;
	
   	retexit(1);
	App = JAppInit(NULL,0);
	wnd = JWndInit(NULL, "Hello", 0);
	
	RootModel.Name = ".";
	RootModel.FullName = ".";
	RootModel.treerow.Flags = JItemF_Expandable;
	
	tree = JTreInit(NULL, &RootModel, expand);
	expand(tree, &RootModel);
	JViewSync(tree);
	JTreAddColumns(tree, NULL, 
		"Name", OFFSET(OurModel, Icon), 80, JColF_STRING|JColF_Icon|JColF_2Icons|JColF_Indent, 
		"Length", OFFSET(OurModel, Length), 40, JColF_CHARS, 
		NULL); 
	scr = JScrInit(NULL, tree, 0);
	JCntGetHints(tree, &sizes);
//	JWSetBounds(wnd, 0,0, sizes.PrefX, sizes.PrefY);
	JCntAdd(wnd, scr);
	JWinShow(wnd);
	JAppLoop(App);
}

#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

typedef struct OurModel {
DefNode defnode;
char *Name;
char *FullName;
unsigned long Length;
char *LengthStr;
char LengthCh[12];
} OurModel;

OurModel RootNode;

void expand(JTModel *Model, OurModel *pare) {

	DIR *dir;
	struct dirent *entry;
	struct stat buf;
	int err;
	int isdir;
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
			cur->FullName = fullname;
			cur->Length = buf.st_size;
			cur->LengthStr = cur->LengthCh;
			sprintf(cur->LengthCh, "%ld", buf.st_size);
			if (S_ISDIR(buf.st_mode))
			{
				cur->defnode.tnode.Flags |= JItemF_Expandable;
			}
			printf("File %s\n", fullname);
			
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
}

int main(int argc, char *argv[]) {

    JTModel *Model;
    void *Iter;
    OurModel *OurMod;
    Model = JTModelInit(NULL, (DefNode *)&RootNode, NULL);
    RootNode.Name = ".";
    RootNode.FullName = ".";
    expand(Model, &RootNode);
    Iter = TModelIter((TModel *)Model, (TNode *)&RootNode);
    while (JIterHasNext(Iter))
    {
	OurMod = JIterNext(Iter);
	printf("%s %s\n", OurMod->FullName, OurMod->LengthStr);
    }    
    
}

#include <winlib.h>
#include <wgslib.h>
#include <wgsipc.h>

int main(int argc, char *argv[]) {

	void *window;
	void *App,*fsl;
	char *path = "./";
   
	
   	retexit(1);
	if (argc > 1)
		path = argv[1];
	App = JAppInit(NULL,0);
	fsl = JFslInit(NULL, NULL, 0, path);
	JWinShow(fsl);
	JAppLoop(App);
}

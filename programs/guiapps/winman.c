#include <winlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wgslib.h>
#include <wgsipc.h>

#define backsize 320*25+1000

unsigned char icon[] = {0x7f, 0xc0, 0x83, 0x83, 0x80, 0x80, 0x80, 0x80,
0xfe, 0x03, 0xfd, 0xfd, 0x71, 0x71, 0x71, 0x71,
0x80, 0xb8, 0xbe, 0x9f, 0x8f, 0x83, 0xc0, 0x7f,
0x71, 0x71, 0x71, 0xf1, 0xe1, 0xc1, 0x03, 0xfe,
0xbc, 0xbc, 0xbc, 0xbc};

JWin *LastFoc = NULL;
void *ManClass;

void WinKey(JWin *Self, int key) {
	printf("Key is %d = %c\n", key, key);
}

void WinNotice(JWin *Self, int SubType, int From, void *data) {
	if (SubType == EVS_Deleted) {
		if (Self == LastFoc)
			LastFoc = NULL;
		JWinKill(Self);
	}
}

void WinFocus(JWin *Self, int isfoc) {
	printf("It's %d\n", isfoc);
	if (isfoc == 2) {
		if (LastFoc)
			JWinFocus(LastFoc, 1);
		JWinFocus(Self, 2);
		LastFoc = Self;
	} 
}

void WinNotify(JWin *Self, int type, int region, char *data) {
	RegInfo props;
	JWin *temp;
	char *name = "JOS";
	int flags = 0;
	
	JRegInfo(region, &props);
	printf("Region %d, %d\n",region,type);
	if (type == 3) {
		printf("Called %s\n", props.Name);
	} else
	if (type == 7) {
		if (props.Hasprop) {
			name = props.Name;
			flags = props.Flags;
		}
		printf("The flags are %d\n",flags);
		temp = JNew(ManClass);
		JManInit(temp, NULL, 0, name, flags, region);
		if (props.Reg.X < 8)
			props.Reg.X = 8;
		if (props.Reg.Y < 8)
			props.Reg.Y = 8;
		JWinGeom(temp, props.Reg.X-8, props.Reg.Y-8, props.Reg.XSize+16, props.Reg.YSize+16, 0);
		JWinRePare(temp, region);
		JEGeom(region, 8, 8, props.Reg.XSize, props.Reg.YSize);
		JRegInfo(region, &props);
		JShow(region);
		JWinShow(temp);
	}
}

int main() {

	void *window;
	void *App;
	JWin *BackG,*butcon,*temp;
	FILE *back;
	void *backbmp;
	
	chdir(getappdir());
	App = JAppInit(NULL,0);
	back = fopen("backimg.hbm","rb");
	if (!back) {
		perror("winman");
		exit(1);
	}
	backbmp = malloc(backsize);
	fseek(back, 2, SEEK_CUR);
	fread(backbmp, 1, backsize, back);
	fclose(back);
	
	BackG = JNew(JSubclass(&JBmpClass, -1, MJW_Notice, WinNotify, -1));
	JBmpInit(BackG, NULL, 0,0, 320,200, backbmp);
	ManClass =  JSubclass(&JManClass, -1, 
			MJW_KeyDown, WinKey,
			MJW_Focus, WinFocus,
			MJW_Notice, WinNotice, -1);
	JWinReq(BackG);
	// Request to be the window manager!
	
	butcon = JCntInit(NULL, BackG, 0, 0);
	JWinGeom(butcon, 0, 16, 0, 0, GEOM_BotLeft | GEOM_BotRight2);
	temp = JIcoInit(NULL, butcon, 0, 16, 16, icon);
	temp = JButInit(NULL, butcon, 0, "Ajirc V1.0.."); 
	JWinShow(BackG); 
	retexit(1);
	JAppLoop(App);
}

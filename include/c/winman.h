#define CMD_STARTPROG      0x1000
#define CMD_CHANGEBACKBMP  0x1001
#define CMD_PS             0x1002
#define CMD_MEM            0x1003

#define bitmapsize 320*25
#define colmapsize 1000
#define backsize   bitmapsize+colmapsize

#define JManF_Maximized 2
#define JManF_Minimized 4

typedef struct JMan {
	JCnt JCntParent;
	char *Label;
	int Flags;
	int ButAbsX;
	int ButAbsY;
	int DragX;
	int DragY;
	int Flags2;
	int RestX;
	int RestY;
	unsigned int RestXS;
	unsigned int RestYS;
	int DragType;
        int showicon;
        int parentreg;
	int Region;
} JMan;

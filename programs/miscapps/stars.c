#include <fcntl.h>
#include <stdio.h>
#include <wgsipc.h>
#include <raslib.h>

/* Starfield: Original in Pascal by Vulture/ Outlaw Triad (on pc)*/

#define MaxStars 120
#define Xoff 160
#define Yoff 100
#define Zoff 255
#define WarpSpeed 2

extern void VideoMode();
extern void SetPixel(int,int,int);
extern void EditPalette();
extern int Random(int);
static int needup=0;
static int chan;
static int check;

typedef struct StarFormat {int x,y,z,ox,oy;} StarFormat;

StarFormat Stars[MaxStars];


void CreateStar(StarFormat *star) {
	star->x = Random(320) - Xoff;
	star->y = Random(200) - Yoff;
	star->z = Zoff;
}

void InitializeStars() {
	int i;
	StarFormat *star;

	needup=0;
	star = &Stars[0];
	for (i=0;i<MaxStars;i++,star++) {
		CreateStar(star);
		star->z = Random(255);
	}
}

int Colour(int a) {
	if (a<50)
		return 1;
	else if (a<100)
		return 2;
	else
		return 3;			
}

void doStar(StarFormat *star) {
	int nx,ny;

	if (star->z > 0) {
		nx = ((star->x<<7) / star->z) + Xoff;
		ny = ((star->y<<7) / star->z) + Yoff;
		if (nx>0 && nx<320 && ny>0 && ny<200) {
			SetPixel(nx,ny, Colour(star->z));
			star->ox = nx;
			star->oy = ny;
			star->z -= WarpSpeed;
		}
		else CreateStar(star);
	} else CreateStar(star);
}

void CalcStars() {
	int i;
	StarFormat *star;
	star = &Stars[0];
	for (i=0;i<MaxStars;i++,star++)
		doStar(star);
}

void DeleteStars() {
	int i;
	StarFormat *star;
	star = &Stars[0];
	for (i=0;i<MaxStars;i++,star++) 
		SetPixel(star->ox,star->oy, 0);
}

void VBIRute() {
	int msg=PMSG_LoseScr;
	int ctrl,ch;
	
	
	stopRaster();
	cli();
	if (needup) {
		DeleteStars();
	} 
	CalcStars();
	needup=1; 
	
	if (check) {
		ch = scanKey(&ctrl);
		if (ch == 27 && (ctrl & 2)) {
			sendPulse(chan,&msg);
			check=0;
		}
	} 
	sei();
	setRaster(NULL,250);
}

void main() {
	int i,RcvId;
	void *msg;
	int screen;
	
	chan = makeChan();
	screen = open("/dev/screen",O_PROC, chan);
	if (screen == -1) {
		perror("Shit!");
		exit(1);
	}
	retexit(1);
	scrSwitch(screen, SCRO_This);
	while(1) {
		RcvId = recvMsg(chan, &msg);
		switch (* (int *)msg) {
		case PMSG_GetScr: 
				VideoMode();
				InitializeStars();
				EditPalette();
				getRaster();
				check=1;
				initKey();
				setRaster(VBIRute,250);
				break;
		case PMSG_LoseScr:
				freeRaster();
				scrSwitch(screen, SCRO_Next); 
				break;
		}
		replyMsg(RcvId,0);
	}

}

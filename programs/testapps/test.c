#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <winlib.h>

MenuData helpmenu[]={
{"About", 0, NULL, 0, 1, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData filemenu[]={
{"Connect to", 0, NULL, 0, 0, NULL, NULL},
{"Exit", 0, NULL, 0, 2, NULL, NULL},
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

MenuData themenu[]={
{"Server", 0, NULL, 0, 0, NULL, filemenu}, 
{"Help", 0, NULL, 0, 0, NULL, helpmenu}, 
{NULL, 0, NULL, 0, 0, NULL, NULL}
};

void mainmenu()
{
	printf("Hello\n");
}

void callback(JBut *but)
{
	JTxtAppend(JWGetData(but), "Text\n");
}

int main (int argc, char *argv[])
{
	JW *but2,*fill,*temp;
	JBut *but;
	JCnt *cnt,*cnt2,*txt,*scr;
	SizeHints sizes;
	
	JAppInit(NULL, 0);
	cnt = JWndInit(NULL, "Title", JWndF_Resizable);
	cnt2 = JCardInit(NULL);
	cnt->Orient = 2;
	cnt2->Orient = 0;
	but = JButInit(NULL, "Test button");
	but->Clicked = callback;
	but2 = JStxInit(NULL, "Button number 2");
	JCntAdd(cnt2, JButInit(NULL, "This is a vertical button"));
	JCntAdd(cnt2, JTxfInit(NULL));
	JCntAdd(cnt2, JButInit(NULL, "And this"));
//	JWSetPref(but2, 80, 16);
//	JWSetMax(but2, 120, but2->PrefYS);
//	JWSetMax(but, 160, but->PrefYS);
	printf("cnt %lx\nbut %lx\nbut2 %lx\ncnt2 %lx\n", cnt, but,but2,cnt2);
	JCntAdd(cnt, but);
	txt = JTxtInit(NULL);
	JWSetData(but, txt);
	printf("Txt %lx\n", txt);
	fill = JFilInit(NULL);
	JWSetMax(fill, 32767,32767);
	scr = JScrInit(NULL, txt, 0);
	JCntAdd(cnt, scr);
	JWSetPref(scr, 64,64);
//	JCntAdd(cnt, fill);
	JCntAdd(cnt, but2);
	JCntAdd(cnt, cnt2);
	JCntAdd(cnt, JChkInit(NULL, "Label"));
	JCntGetHints(cnt, &sizes);
	JWSetBounds(cnt, 0,0, sizes.PrefX, sizes.PrefY);
//	JWSetBounds(cnt, 0,0, 320, 120);
	JCntShow(cnt);
	temp = JMnuInit(NULL, themenu, 128, 128, mainmenu);
	JWinShow(temp);
	JTxtAppend(txt, "Hello\nThis is a text area\n123123\ndfgsdfsdfsd\nsdfsdfsdf\n");
//	JWShow(but);
	JAppLoop(NULL);
}

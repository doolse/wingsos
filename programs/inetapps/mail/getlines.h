//Custom GetLines ... 

//doesn't scroll left and right, full editor for a max of size.

char * getmyline(char * original,int size, int x, int y, int password);


//displays a max line width of displaysize, with a larger max string size.
//if the string is larger than the display, it scrolls left and right. 
//can be given a restriction string. characters in the string can't be 
//typed into the editor. 

char * getmylinerestrict(char * original, long size, int displaysize, int x, int y, char * restrict, int password);


//basic, not a full editor, used for fetching simple number inputs.

char * getmylinen(int size, int x, int y);
#define DEL 8
#define ESC 96

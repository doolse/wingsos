//message box object
  
typedef struct msgboxobj_s {
  int numoflines; 
  int showprogress;  
  
  char * msgline[3];  
  
  int progresswidth;  
  ulong numofitems;
  ulong progressposition;
  int linelength;
  
  int top;
  int bottom;
  int left;
  int right;
    
} msgboxobj;
     
msgboxobj * initmsgboxobj(char * msgline1, char * msgline2, char * 
sgline3, int showprogress, ulong numofitems);
void drawmsgboxobj(msgboxobj * mb);

//Message box progress bar management

void updatemsgboxprogress(msgboxobj * mb);
void incrementprogress(msgboxobj * mb);
void setprogress(msgboxobj * mb,ulong progress);

//Simple message box call

void drawmessagebox(char * string1, char * string2, int wait);

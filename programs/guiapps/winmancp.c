#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <wgsipc.h>
#include <winlib.h>
#include <winman.h>
#include <xmldom.h>
#include <taskbar.h>

int channel;
DOMElement * configxml,* confignode;
JLModel * bgimage_m, * tskpos_m;

typedef struct bgimage_s {
  TNode tnode;
  char * filename;
} bgimage_s;

typedef struct tskpos_s {
  TNode tnode;
  char * label;
  char * position;
} tskpos_s;

void populateselbox() {
  bgimage_s * newbgimage;
  DIR * dir;
  struct dirent * entry;
  
  dir = opendir("/wings/gui/winman.app");
  while(entry = readdir(dir)) {
    if(entry->d_type != 6) {
      if(!strcmp(".hbm",entry->d_name + strlen(entry->d_name)-4)) {
        newbgimage = calloc(sizeof(bgimage_s),1);
        newbgimage->filename = strdup(entry->d_name);
        JLModelAppend(bgimage_m,(TNode *)newbgimage);
      }
    }
  }
  closedir(dir);
}

void changedbgimage(TNode *tnode) {
  bgimage_s * bgimage = (bgimage_s *)tnode;
  confignode = XMLgetNode(configxml,"xml/backimg");
  XMLsetAttr(confignode,"filename",bgimage->filename);
}

void changedtskpos(TNode *tnode) {
  tskpos_s * tskpos = (tskpos_s *)tnode;
  confignode = XMLgetNode(configxml,"xml/taskbar");
  XMLsetAttr(confignode,"position",tskpos->position);
}

void apply() {
  confignode = XMLgetNode(configxml,"xml/backimg");
  sendCon(channel,TSKM_UPDATEBG,0,NULL,XMLgetAttr(confignode,"filename"));
  confignode = XMLgetNode(configxml,"xml/taskbar");
  sendCon(channel,TSKM_RELAUNCHTASKBAR,0,NULL,XMLgetAttr(confignode,"position"));
}

void save() {
  XMLsaveFile(configxml,"/wings/gui/winman.app/winmanconfig.xml");
  syncfs("/");
}

void quit() {
  close(channel);
  exit(1);
}

void main(int argc,char * argv[]) {
  JMeta * metadata;
  JCombo * selbox;
  tskpos_s * tskpositem;
  SizeHints sizes;
  void *app,*window,*checkbox,*label,*button,*cnt;

  app = JAppInit(NULL,0);
  
  metadata = malloc(sizeof(JMeta));
  metadata->launchpath = strdup(fpathname(argv[0],getappdir(),1));
  metadata->title = "Control Panel";
  metadata->icon = controlpanel_icon;
  metadata->showicon = 1;
  metadata->parentreg = -1;

  window = JWndInit(NULL,metadata->title,0,metadata);
  JAppSetMain(app,window);

  ((JCnt *)window)->Orient = JCntF_TopBottom;

  label = JStxInit(NULL,"Background Image:");
  JCntAdd(window,label);

  bgimage_m = JLModelInit(NULL);
  populateselbox();

  selbox = JComboInit(NULL,(TModel*)bgimage_m,OFFSET32(bgimage_s,filename),JColF_STRING);  
  selbox->Changed = changedbgimage;
  JCntAdd(window,selbox);

  label = JStxInit(NULL,"Taskbar Position:");
  JCntAdd(window,label);

  tskpos_m = JLModelInit(NULL);
  tskpositem = calloc(sizeof(tskpos_s),1);
  tskpositem->position = strdup("top");
  tskpositem->label = strdup("Top of screen");
  JLModelAppend(tskpos_m,(TNode *)tskpositem);

  tskpositem = calloc(sizeof(tskpos_s),1);
  tskpositem->position = strdup("bottom");
  tskpositem->label = strdup("Bottom of screen");
  JLModelAppend(tskpos_m,(TNode *)tskpositem);

  tskpositem = calloc(sizeof(tskpos_s),1);
  tskpositem->position = strdup("left");
  tskpositem->label = strdup("On the left");
  JLModelAppend(tskpos_m,(TNode *)tskpositem);

  tskpositem = calloc(sizeof(tskpos_s),1);
  tskpositem->position = strdup("right");
  tskpositem->label = strdup("On the right");
  JLModelAppend(tskpos_m,(TNode *)tskpositem);

  selbox = JComboInit(NULL,(TModel*)tskpos_m,OFFSET32(tskpos_s,label),JColF_STRING);  
  selbox->Changed = changedtskpos;
  JCntAdd(window,selbox);

  cnt = JCntInit(NULL);
  ((JCnt*)cnt)->Orient = JCntF_LeftRight;
  JCntAdd(window,cnt);

  button = JButInit(NULL,"Cancel");
  JWinCallback(button,JBut,Clicked,quit);
  JCntAdd(cnt,button);

  button = JButInit(NULL,"Apply");
  JWinCallback(button,JBut,Clicked,apply);
  JCntAdd(cnt,button);

  button = JButInit(NULL,"Save");
  JWinCallback(button,JBut,Clicked,save);
  JCntAdd(cnt,button);

  configxml = XMLloadFile("/wings/gui/winman.app/winmanconfig.xml");
  channel = open("/sys/winman",O_PROC);

  JCntGetHints(window,&sizes);
  JWSetBounds(window,24,32,sizes.PrefX,sizes.PrefY);
  JWndSetProp(window);

  JWinShow(window);
  retexit(0);
  JAppLoop(app);
}


enum itemstati {
  TSKM_KILLED=200,
  TSKM_NEWITEM,
  TSKM_MINIMIZED,
  TSKM_INFOCUS,
  TSKM_INBLUR,
  TSKM_ISLAUNCH, 
  TSKM_GETALL,
  TSKM_GETNUM,
  TSKM_ONLEFT,
  TSKM_ONRIGHT,
  TSKM_ONTOP,
  TSKM_ONBOTTOM,
  TSKM_NEXTWIN,
  TSKM_NEXTAPPWIN,
  TSKM_RELAUNCHTASKBAR,
  TSKM_UPDATEBG
};

enum itemupdatecodes {
  SETCOL=0,
  REFRESH
};

#define COL_BAR  0x0c // this doesn't matter anymore. you never see the bar

#define COL_RUN  0x0c  //not running processes... launchables
#define COL_FOC  0x07
#define COL_NFOC 0x0f
#define COL_MIN  0x0b

typedef struct msgpass_s {
  int code;
  int getnum;
  void * mainwin;
  char * str;
} msgpass;

typedef struct item_s {
  struct item_s * next;
  struct item_s * prev;
  int status;
  void * mainwin;
  void * icon;
  char * title;
  char * launchstr;
  uchar * iconbmp;
} item;

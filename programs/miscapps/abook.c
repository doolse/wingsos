#include <stdio.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include <exception.h>
#include <string.h>
#include <console.h>
#include <xmldom.h>

#define GET_ATTRIB   225
#define GET_ALL_LIST 226
#define MAKE_ENTRY   227
#define PUT_ATTRIB   228
#define DEL_ATTRIB   229
#define DEL_ENTRY    230

#define NOENTRY -2
#define ERROR   -1
#define SUCCESS  0

typedef struct namelist_s {
  int use;
  char * firstname;
  char * lastname;
} namelist;

void drawinterface();

int fd, returncode, input;
char *firstname, *lastname, *attrib, *buf;
char * bufl = NULL;
int size = 0;

char * getmyline(int size, int x, int y) {
  int i,j,count, update;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

  count = 0;
  update = 0;

  /*  ASCII Codes

    32 is SPACE
    126 is ~
    47 is /
    8 is DEL

  */

  con_gotoxy(x,y);
  con_update();

  while(1) {
    i = con_getkey();
    if(i > 31 && i < 127 && count < size) {
      linebuf[count++] = i;
      linebuf[count] = 0;
      update=1;
    } else if(i == 8 && count > 0) {
      count--;
      linebuf[count] = 0;
      update=1;
    } else if(i == '\n' || i == '\r')
      break;
    else
      update=0;

    if(update) {
      for(j = 0;j<size;j++) {
        con_gotoxy(x+j,y);
        putchar(' ');
      }
      con_gotoxy(x,y);
      printf("%s",linebuf);
      con_update();
    }
  }
  return(linebuf);   
}

char * getmylinenospace(int size, int x, int y) {
  int i,j,count, update;
  char * linebuf;

  linebuf = (char *)malloc(size+1);

  count = 0;
  update = 0;

  /*  ASCII Codes

    32 is SPACE
    126 is ~
    47 is /
    8 is DEL

  */

  con_gotoxy(x,y);
  con_update();

  while(1) {
    i = con_getkey();
    if(i > 32 && i < 127 && count < size) {
      linebuf[count++] = i;
      linebuf[count] = 0;
      update=1;
    } else if(i == 8 && count > 0) {
      count--;
      linebuf[count] = 0;
      update=1;
    } else if(i == '\n' || i == '\r')
      break;
    else
      update=0;

    if(update) {
      for(j = 0;j<size;j++) {
        con_gotoxy(x+j,y);
        putchar(' ');
      }
      con_gotoxy(x,y);
      printf("%s",linebuf);
      con_update();
    }
  }
  return(linebuf);   
}

int getattrib() {
  int input;

  getentry:
  con_gotoxy(0,15);
  printf("firstname: ");
  con_update();
  firstname = getmyline(16,11,15);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmyline(16,11,16);
  
  getattrib:
  con_gotoxy(0,17);
  printf("attribute: ");
  con_update();
  attrib = getmylinenospace(16,11,17);

  con_gotoxy(0,18);

  returncode = sendCon(fd, GET_ATTRIB, lastname, firstname, attrib, buf, 50);
  if(returncode == NOENTRY) {
    printf("The Entry does not exist. Try again? (y/n)");
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0);
    con_clrscr();
    drawinterface();
    goto getentry;
  } else if(returncode == ERROR) {
    printf("Error: Buffer Overrun. Press a key.");
    con_update();
    return(0);
  } else {
    printf("%s", buf);
    con_gotoxy(0,19);
    printf("Get another attribute? (y/n)");
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0);
    con_clrscr();
    drawinterface();
    goto getattrib;
  }
}

int addentry() {
  int input;

  con_gotoxy(0,15);
  printf("firstname: ");
  con_update();
  firstname = getmyline(16,11,15);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmyline(16,11,16);

  if(!strlen(lastname)) {
    printf("\nLastname is required. Press any key.");
    con_update();
    con_getkey();
    return(1);
  }

  returncode = sendCon(fd, MAKE_ENTRY, lastname, firstname, NULL, NULL, 0);

  con_gotoxy(0,17);

  if(returncode == NOENTRY) {
    printf("An error occured. Press any key.");
    con_update();
    con_getkey();
    return(0);
  } else if(returncode == ERROR) {
    printf("Error: that entry already exists. Press any key.");
    con_update();
    con_getkey();
    return(0);
  } else {
    printf("Success: New Entry Added. Modify an attribute? (y/n)");
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0);
    else
      modifyattrib(firstname,lastname);
  }
  return(0);
}

int modifyattrib(char * firstname, char * lastname) {
  char * value;
  int input;

  if(firstname == NULL && lastname == NULL) {
    con_gotoxy(0,15);
    printf("firstname: ");
    con_update();
    firstname = getmyline(16,11,15);

    con_gotoxy(0,16);
    printf(" lastname: ");
    con_update();
    lastname = getmyline(16,11,16);
  }

  anotherattrib:

  con_gotoxy(0,18);
  printf("attribute: ");
  con_update();
  attrib = getmylinenospace(16,11,18);

  con_gotoxy(0,19);
  printf("    value: ");
  con_update();
  value = getmyline(67,11,19);

  if(!strcasecmp(attrib,"lastname")) {
    con_gotoxy(0,18);
    printf("The attribute 'lastname' cannot be modified. Press a key.");
    con_update();
    input = con_getkey();
    return(0);
  }
  returncode = sendCon(fd, PUT_ATTRIB, lastname, firstname, attrib, value,0);
  if(returncode == NOENTRY) {
    con_gotoxy(0,20);
    printf("The Entry does not exist.\n");
    con_update();
    con_getkey();
    return(0);
  } 

  con_gotoxy(0,20);
  printf("The Attribute has been modified. Modify another for this entry? (y/n)");
  con_update();

  input = 'z';
  while(input != 'y' && input != 'n')
    input = con_getkey();
  if(input == 'n')
    return(0);
  
  con_clrscr();
  drawinterface();
  goto anotherattrib;
}

int removeattrib() {
  int input;

  getentry:
  con_gotoxy(0,15);
  printf("firstname: ");
  con_update();
  firstname = getmyline(16,11,15);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmyline(16,11,16);
  
  getattrib:
  con_gotoxy(0,17);
  printf("attribute: ");
  con_update();
  attrib = getmylinenospace(16,11,17);

  if(!strcasecmp(attrib,"lastname")) {
    con_gotoxy(0,18);
    printf("The attribute 'lastname' cannot be deleted. Press a key.");
    con_update();
    input = con_getkey();
    return(0);
  }
  returncode = sendCon(fd, DEL_ATTRIB, lastname, firstname, attrib, NULL, 0);
  if(returncode == NOENTRY) {
    con_gotoxy(0,18);
    printf("The Entry does not exist. Try again? (y/n)");
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0); 
    con_clrscr();
    drawinterface();
    goto getentry;
  } else {
    con_gotoxy(0,18);
    printf("The attribute %s has been deleted. Delete another (y/n)", attrib);
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0); 
    con_clrscr();
    drawinterface();
    goto getattrib;
  }
}

int deleteentry() {
  int input;

  getentry:
  con_gotoxy(0,15);
  printf("firstname: ");
  con_update();
  firstname = getmyline(16,11,15);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmyline(16,11,16);

  if(!strlen(lastname)) {
    printf("\nLast name is required. Press any key.");
    con_update();
    con_getkey();
    return(1);
  }

  returncode = sendCon(fd, DEL_ENTRY, lastname, firstname, NULL, NULL, 0);
  if(returncode == NOENTRY) {
    con_gotoxy(0,17);
    printf("The Entry does not exist. Try again? (y/n)");
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0);    
    con_clrscr();
    drawinterface();
    goto getentry;
  } else {
    con_gotoxy(0,17);
    printf("The Entry %s %s was deleted. Delete another? (y/n)", firstname, lastname);
    con_update();
    input = 'z';
    while(input != 'y' && input != 'n')
      input = con_getkey();
    if(input == 'n')
      return(0);
    con_clrscr();
    drawinterface();
    goto getentry;
  }
}

int importentry(DOMElement * entry) {
  char *firstname, *lastname;
  DOMNode * firstattrib, *attrib;

  firstname = XMLgetAttr(entry, "firstname");
  lastname  = XMLgetAttr(entry, "lastname");

  returncode = sendCon(fd, MAKE_ENTRY, lastname, firstname, NULL, NULL, 0);

  if(returncode == NOENTRY)
    return(-1);
  else if(returncode == ERROR)
    return(-1);

  firstattrib = entry->Attr;
  if(strcmp(firstattrib->Name, "firstname") && strcmp(firstattrib->Name, "lastname"))
    sendCon(fd, PUT_ATTRIB, lastname, firstname, firstattrib->Name, firstattrib->Value, 0);

  attrib = firstattrib->Next;

  while(1) {
    if(attrib == firstattrib)
      break;
        
    if(strcmp(attrib->Name, "firstname") && strcmp(attrib->Name, "lastname"))
      sendCon(fd, PUT_ATTRIB, lastname, firstname, attrib->Name, attrib->Value, 0);

    attrib = attrib->Next;
  }

  return(0);  
}

int import() {
  int ex;
  void *exp;
  FILE * fp;
  DOMElement * XML, *entry, *firstentry;
  char * selectedfilename = NULL;
  char * pathstr;
  int size = 0;
  int totalimported = 0;

  spawnlp(S_WAIT, "fileman", NULL);

  fp = fopen("/wings/attach.tmp", "r");
  if(!fp) {
    printf("Could not find selected file.  Press any key.\n");
    con_update();
    con_getkey();
    return(-1);
  }

  getline(&buf, &size, fp);
  fclose(fp);

  pathstr = (char *)malloc(strlen(buf) + strlen(".app/data/addressbook.xml"));
  sprintf(pathstr, "%s.app/data/addressbook.xml", buf);

  Try {
    //printf("%s\n", pathstr);
    XML = XMLloadFile(pathstr);
  }
  Catch2(ex, exp) {
    printf("The addressbook datafile could not be found, or it is corrupt.\n The import could not continue. Press any key.\n");
    con_update();
    con_getkey();
    return(-1);
  }

  printf(" Importing...\n\n");
  con_update();

  firstentry = XMLgetNode(XML, "xml/entry");
  if(!importentry(firstentry))
    totalimported++;  

  entry = firstentry->NextElem;

  while(1) {
    if(entry == firstentry)
      break;

    if(!importentry(entry))
      totalimported++;

    entry = entry->NextElem;
  }

  printf(" %d entries successfully imported.\nPress any key.\n", totalimported);
  con_update();
  con_getkey();

  return(totalimported);
}

void listall() {
  char * listbuf = NULL;
  int buflen = 100;
  namelist * names, * namesptr;
  char * ptr;
  int total, i;

  listbuf = (char *)malloc(buflen);

  returncode = sendCon(fd, GET_ALL_LIST, NULL, NULL, NULL, listbuf, buflen);

  //The addressbook returns an ERROR code if the buffer was too small, 
  //and puts the minimum buffer size needed as ascii in the buffer.

  if(returncode == ERROR) { 
    //printf("buffer initially too small, increasing to size: %s\n\n", listbuf);
    buflen = atoi(listbuf);
    free(listbuf);
    listbuf = (char *)malloc(buflen);
    returncode = sendCon(fd, GET_ALL_LIST, NULL, NULL, NULL, listbuf, buflen);
  }

  if(returncode == ERROR) {
    printf("an unknown error has occurred.\n");
    con_update();
  } else {
    //there will be one less comma then there are entries.

    total = 0;
    ptr = strchr(listbuf, ',');
    while(ptr != NULL) {
      total++;
      ptr++;
      ptr = strchr(ptr, ',');
    }
    if(strlen(listbuf) > 0)
      total++;

    //printf("%s\n", listbuf);
    //printf("there are %d names\n", total);

    names = (namelist *)malloc(sizeof(namelist) * (total +1));
   
    ptr      = listbuf;
    namesptr = names;

    names[total].use = -1;

    while(ptr != NULL && ptr != 0) {
      namesptr->use = 0;
      namesptr->firstname = ptr;
      ptr = strchr(ptr, ' ');
      *ptr = 0;
      ptr++;
      namesptr->lastname = ptr;
      ptr = strchr(ptr, ',');
      if(ptr != NULL) {
        *ptr = 0;
        ptr++;
        namesptr++;
      }
    }

    listbuf = NULL;

    con_clrscr();

    for(i = 0;i<total;i++) {
      if(names[i].use == -1) 
        break;
      printf("%10s, %10s\n", names[i].lastname, names[i].firstname);
      if(i % 24 == 0 && i != 0) {
        printf("Press a key.");
        con_update();
        con_getkey();
        con_clrscr();
      }
    }
    printf("Press a key.");
    con_update();
    con_getkey();
  }
}

void drawinterface() {
  con_clrscr();

  putchar('\n');
  printf("  Address Book Utility\n");
  printf("  ~~~~~~~~~~~~~~~~~~~~");

  con_gotoxy(0,5);
  printf(" (a)dd entry");
  con_gotoxy(0,6);
  printf(" (m)odify attribute");
  con_gotoxy(0,7);
  printf(" (d)elete entry");
  con_gotoxy(0,8);
  printf(" (r)emove attribute");
  con_gotoxy(0,9);
  printf(" (g)et");
  con_gotoxy(0,10);
  printf(" (L)ist all");
  con_gotoxy(0,11);
  printf(" (i)mport");
  con_gotoxy(0,12);
  printf(" (Q)uit");

  con_update();
}

void main(int argc, char *argv[]) {
  struct termios tio;
  int refresh = 0;

  con_init();

  if((fd = open("/sys/addressbook", O_PROC)) == -1) {
    system("addressbook");
    if((fd = open("/sys/addressbook", O_PROC)) == -1) {
      printf("The addressbook service could not be started.\n");
      con_update();
      exit(1);
    }
  }
  
  buf = (char *)malloc(51);

  drawinterface();

  input = 'z';  
  while(input != 'Q') {
    input = con_getkey();

    refresh = 1;

    switch(input) {
      case 'a':
        addentry();
      break;
      case 'm':
        modifyattrib(NULL,NULL);
      break;
      case 'd':
        deleteentry();
      break;
      case 'r':
        removeattrib();
      break;
      case 'g':
        getattrib();
      break;
      case 'i':
        import();
      break;
      case 'L':
        listall();
      break;
      case 'Q':
        con_clrscr();
        exit(1);
      break;
      default:
        refresh = 0;
      break;
    }

    if(refresh)
      drawinterface();
  }
}



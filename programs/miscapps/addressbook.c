#include <stdio.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include <exception.h>
#include <dirent.h>
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

typedef struct fieldstruct_s {
  struct fieldstruct_s * next;
  struct fieldstruct_s * prev;
  
  char * data;
  char * srcstring;

} fieldstruct;

void drawinterface();

int fd, returncode, input;
char *firstname, *lastname, *attrib, *buf;
char * bufl = NULL;
char * abookbuf = NULL;
int size = 0;
int namecolwidth, smaller;
namelist * abook = NULL;
int current = 0;

char * getmylinefix(int size, int x, int y, int password, int spaces) {
  int i,count = 0;
  char * linebuf;
  int startchar;

  linebuf = (char *)malloc(size+1);

  /*  ASCII Codes

    32 is SPACE
    126 is ~
    47 is /
    8 is DEL

  */

  con_gotoxy(x,y);
  con_update();

  if(spaces)
    startchar = 31;
  else
    startchar = 32;

  while(1) {
    i = con_getkey();
    if(i > startchar && i < 127 && i != 47  && count < size) {
      linebuf[count] = i;
      con_gotoxy(x+count,y);
      if(password)
        putchar('*');
      else
        putchar(i);
      linebuf[++count] = 0;
      con_update();
    } else if(i == 8 && count > 0) {
      count--;
      con_gotoxy(x+count,y);
      putchar(' ');
      con_gotoxy(x+count,y);
      linebuf[count] = 0;
      con_update();
    } else if(i == '\n' || i == '\r')
      break;
  }
  return(linebuf);
}

char * strcasestr(char * big, char * little) {
  char * ptr;
  int len;
  char firstchar;

  firstchar = little[0];
  len = strlen(little);

  ptr = big;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, little, len))
        return(ptr);
      ptr++;
    }
  }

  //Switch case of search character
  if(firstchar > 64 && firstchar < 91) 
    firstchar = firstchar + 32;
  else if(firstchar > 96 && firstchar < 123)
    firstchar = firstchar - 32;

  ptr = big;
  while(ptr != NULL) {
    ptr = strchr(ptr, firstchar);
    if(ptr) {
      if(!strncasecmp(ptr, little, len))
        return(ptr);
      ptr++;
    }
  }

  return(NULL);
}

char * itoa(int number) {
  char * string;
  int numlen;

  if(number < 10)
    numlen = 2;
  else if(number < 100)
    numlen = 3;
  else if(number < 1000)
    numlen = 4;
  else if(number < 10000)
    numlen = 5;
  else
    numlen = 6;

  string = malloc(numlen);
  sprintf(string, "%d", number);

  return(string);
}

void curleft(int num) {
  printf("\x1b[%dD", num);
  con_update();
}

void curright(int num) {
  printf("\x1b[%dC", num);
  con_update();
}

void con_reset() {
  //reset the terminal.
  printf("\x1b[0m"); 
}

void movechardown(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y+1);
  putchar(c);
  //con_update();
}

void movecharup(int x, int y, char c){
  con_gotoxy(x, y);
  putchar(' ');
  con_gotoxy(x, y-1);
  putchar(c);
  //con_update();
}

void prepconsole() {
  struct termios tio;

  con_init();

  gettio(STDOUT_FILENO, &tio);
  tio.flags |= TF_ECHO|TF_ICRLF;
  tio.MIN = 1;
  settio(STDOUT_FILENO, &tio);

  con_clrscr();
  con_update();
}

void drawaddressbookselector(int startoffset, int total) {
  int row, col, i, j;
  namelist * aptr = abook;
  char * namestr;

  namestr = malloc(80);

  row = 1;
  col = 2;
  j = namecolwidth - col - 1;

  aptr += startoffset;

  while(row < con_ysize-2 && startoffset < total) {
    sprintf(namestr, "%s %s", aptr->firstname,aptr->lastname);
    namestr[j] = 0;
      
    con_gotoxy(col,row);
    printf("%s", namestr);

    for(i=col+strlen(namestr);i<namecolwidth;i++) 
      putchar(' ');

    aptr++;
    row++;
    startoffset++;
  }
  //con_update();
}

int readfromaddressbookd() {
  int buflen = 100;
  namelist * abookptr;
  char * ptr, * returnbuf;
  int i, returncode, total;

  if(abookbuf != NULL)
    free(abookbuf);

  if(abook != NULL)
    free(abook);

  abookbuf = (char *)malloc(buflen);
  
  returncode = sendCon(fd, GET_ALL_LIST, NULL, NULL, NULL, abookbuf, buflen);
    
  //The addressbook returns an ERROR code if the buffer was too small,
  //and puts the minimum buffer size needed as ascii in the buffer.
  
  if(returncode == ERROR) {
    buflen = atoi(abookbuf);
    free(abookbuf);

    abookbuf = (char *)malloc(buflen);
    returncode = sendCon(fd, GET_ALL_LIST, NULL, NULL, NULL, abookbuf, buflen);
  }

  if(returncode == ERROR) {
    drawmessagebox("An error occurred retrieveing data from the Address Book.", "Press any key.",1);
    return(0);
  } 

  //there will be one less comma then there are entries.
  total = 0;  
  ptr = strstr(abookbuf, ",");
  while(ptr != NULL) {
    total++;
    ptr++;
    ptr = strstr(ptr, ",");
  }
  if(strlen(abookbuf))
    total++;    

  abookptr = abook = malloc(sizeof(namelist) * (total +1));

  ptr = abookbuf;

  abook[total].use = -1;

  while(ptr != NULL) {
    abookptr->use = 0;
    abookptr->firstname = ptr;
    ptr = strstr(ptr, " ");
    *ptr = 0;
    ptr++;
    abookptr->lastname = ptr;

    ptr = strstr(ptr, ",");
    if(ptr != NULL) {
      *ptr = 0;
      ptr++;
      abookptr++;
    }
  }
  return(total);
}

int getattrib(char * attrib) {
  int input;

  returncode = sendCon(fd, GET_ATTRIB, abook[current].lastname, abook[current].firstname, attrib, buf, 50);
  if(returncode == NOENTRY) {
    buf[0] = 0;
    return(0);
  } else if(returncode == ERROR) {
    buf[0] = 0;
    return(returncode);
  } 
  return(0);
}

int addentry() {
  int input;

  con_gotoxy(0,15);
  printf("firstname: ");
  con_update();
  firstname = getmylinefix(16,11,15,0,0);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmylinefix(16,11,16,0,0);

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
    firstname = getmylinefix(16,11,15,0,0);

    con_gotoxy(0,16);
    printf(" lastname: ");
    con_update();
    lastname = getmylinefix(16,11,16,0,0);
  }

  anotherattrib:

  con_gotoxy(0,18);
  printf("attribute: ");
  con_update();
  attrib = getmylinefix(16,11,18,0,0);

  con_gotoxy(0,19);
  printf("    value: ");
  con_update();
  value = getmylinefix(67,11,19,0,1);

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
  firstname = getmylinefix(16,11,15,0,0);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmylinefix(16,11,16,0,0);
  
  getattrib:
  con_gotoxy(0,17);
  printf("attribute: ");
  con_update();
  attrib = getmylinefix(16,11,17,0,0);

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
  firstname = getmylinefix(16,11,15,0,0);

  con_gotoxy(0,16);
  printf(" lastname: ");
  con_update();
  lastname = getmylinefix(16,11,16,0,0);

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

void freelist(fieldstruct * list) {

  free(list->srcstring);

  while(list)
    list = remQueue(list,list);
}

fieldstruct * tolist(char * src, char delim) {
  char * ptr, *tempptr;
  fieldstruct * listhead, * listptr;

  listhead = NULL;
  ptr = src;

  while(ptr) {
    listptr = malloc(sizeof(fieldstruct));
    listptr->srcstring = src;
    listhead = addQueueB(listhead,listhead,listptr);
    listptr->data = ptr;
    tempptr = strchr(ptr,delim);
    if(tempptr) {
      ptr = tempptr;
      *ptr = 0;
      ptr++;
    } else
      break;
  }
  return(listhead);
}

char * listgetat(fieldstruct * listptr, int pos) {
  int i;

  for(i = 0; i < pos; i++)
    listptr = listptr->next;

  return(strdup(listptr->data));
}

void showalllistitems(fieldstruct * list) {
  fieldstruct * listptr;
  listptr = list;
  do {
    printf("'%s'\n", listptr->data);
    con_update();
    listptr = listptr->next;
  } while (listptr != list);
}

int importvcf(char * filepath) {
  int bufferusecount = 0, buffersize = 2048, totalimported = 0, partlen;
  int returncode;
  FILE * fp;
  char * buffer, *strptr, *ptr, c, *endofcard;
  char * lastname, * firstname, * fielddata, * rawstring;
  fieldstruct * datalist;

  ptr = buffer = malloc(buffersize+1);

  fp = fopen(filepath, "r");

  while((c = fgetc(fp)) != EOF) {
    if(c != 0 && c != 0x0a) {
      *ptr++ = c;
      *ptr = 0;
      bufferusecount++;
      if(bufferusecount == buffersize) {
        buffersize *= 2;
        ptr = buffer = realloc(buffer, buffersize+1);
        ptr += bufferusecount;
      }
    }
  } 

  fclose(fp);
  ptr = buffer;

  //printf("%s\n", ptr);

  while(strcasestr(ptr, "begin:vcard")) {
    if(endofcard = strcasestr(ptr, "\rend:vcard")) {

      //GET Name data
      if(strptr = strcasestr(ptr, "\rn:")) {
        if(strptr > endofcard)
          goto nextcard;

        strptr += 3;
        partlen = strchr(strptr,0x0d) - strptr;
        rawstring = calloc(partlen + 1,1);
        strncpy(rawstring,strptr,partlen);

        datalist = tolist(rawstring,';');

        lastname  = listgetat(datalist, 0);
        firstname = listgetat(datalist, 1);

        if(!strlen(lastname))
          goto nextcard;

        returncode = sendCon(fd, MAKE_ENTRY, lastname, firstname, NULL, NULL, 0);

        if(returncode != NOENTRY && returncode != ERROR)
          totalimported++;
        //else
          //goto nextcard;

        //Fetch Middlename

        fielddata = listgetat(datalist, 2);
        if(strlen(fielddata))
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "middlename", fielddata,0);
        free(fielddata);

        //Fetch Title

        fielddata = listgetat(datalist, 3);
        if(strlen(fielddata))
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "nameprefix", fielddata,0);
        free(fielddata);

        //Fetch namesuffix

        fielddata = listgetat(datalist, 4);
        if(strlen(fielddata))
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "namesuffix", fielddata,0);
        free(fielddata);

        freelist(datalist);
      } else 
        goto nextcard;


      //GET Email data
      if(strptr = strcasestr(ptr, "\remail")) {
        if(strptr < endofcard) {
          strptr += 6;
          partlen = strchr(strptr,0x0d) - strptr;
          rawstring = calloc(partlen + 1,1);
          strncpy(rawstring,strptr,partlen);

          datalist  = tolist(rawstring,':');
          fielddata = listgetat(datalist, 1);
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "email", fielddata,0);
          free(fielddata);

          freelist(datalist);
        }
      }

      // ORGANIZATION 
      if(strptr = strcasestr(ptr, "\rORG")) {
        if(strptr < endofcard) {
          strptr += 1;
          partlen = strchr(strptr,0x0d) - strptr;
          rawstring = calloc(partlen + 1,1);
          strncpy(rawstring,strptr,partlen);

          datalist  = tolist(rawstring,':');
          fielddata = listgetat(datalist, 1);
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "organization", fielddata,0);
          free(fielddata);

          freelist(datalist);
        }
      }

      // telephone number 
      if(strptr = strcasestr(ptr, "\rTEL")) {
        if(strptr < endofcard) {
          strptr += 4;
          partlen = strchr(strptr,0x0d) - strptr;
          rawstring = calloc(partlen + 1,1);
          strncpy(rawstring,strptr,partlen);

          datalist  = tolist(rawstring,':');
          fielddata = listgetat(datalist, 1);
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "telephone", fielddata,0);
          free(fielddata);

          freelist(datalist);
        }
      }

      // URL
      if(strptr = strcasestr(ptr, "\rURL")) {
        if(strptr < endofcard) {
          strptr += 4;
          partlen = strchr(strptr,0x0d) - strptr;
          rawstring = calloc(partlen + 1,1);
          strncpy(rawstring,strptr,partlen);

          datalist  = tolist(rawstring,':');
          fielddata = listgetat(datalist, 1);
          sendCon(fd, PUT_ATTRIB, lastname, firstname, "website", fielddata,0);
          free(fielddata);

          freelist(datalist);
        }
      }


    }
    nextcard:
    ptr = strcasestr(ptr, "\rend:vcard");
    if(!ptr)
      break;
    ptr += strlen("\rend:vcard");
  }

  return(totalimported);
} 

int searchforvcf(char * path) {
  int totalimported = 0;
  DIR * dir;
  struct dirent *entry;
  char * nextpath;

  dir = opendir(path);

  while(entry = readdir(dir)) {
    if(entry->d_type == 6) {
      nextpath = malloc(strlen(path) + strlen(entry->d_name) + 2);
      sprintf(nextpath, "%s/%s", path, entry->d_name); 
      totalimported += searchforvcf(nextpath);
      free(nextpath);
    } else if (!strcasecmp(&entry->d_name[strlen(entry->d_name)-4],".vcf")) {
      nextpath = malloc(strlen(path) + strlen(entry->d_name) + 2);
      sprintf(nextpath, "%s/%s", path, entry->d_name); 
      totalimported += importvcf(nextpath);
      free(nextpath);
    }
  }

  return(totalimported);
}

int import() {
  FILE * fp;
  DOMElement * XML, *entry, *firstentry;
  char *pathstr,* selectedfilename = NULL;
  int ex,dovcf,size = 0,totalimported = 0;
  void *exp;

  spawnlp(S_WAIT, "fileman", NULL);

  fp = fopen("/wings/.fm.filepath.tmp", "r");
  if(!fp) {
    printf("Could not find selected file.  Press any key.\n");
    con_update();
    con_getkey();
    return(-1);
  }

  getline(&buf, &size, fp);
  fclose(fp);

  if(!strcasecmp(".vcf",&buf[strlen(buf)-4])) {
    dovcf = 1;
  }
  else
    dovcf = 0;

  if(!dovcf) {
    pathstr = (char *)malloc(strlen(buf) + strlen(".app/data/addressbook.xml"));
    sprintf(pathstr, "%s.app/data/addressbook.xml", buf);

    Try {
      //printf("%s\n", pathstr);
      XML = XMLloadFile(pathstr);
    }
    Catch2(ex, exp) {
      printf("Addressbook datafile not found.\nSearching for .vcf data files.\n");
      dovcf = 2;
    }
  }

  con_update();

  if(!dovcf) {
    printf("Importing previous addressbook...\n\n");
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
  } else if(dovcf == 2){
    printf("Searching for .vcf data files.\n");
    con_update();
    totalimported = searchforvcf(buf);
  } else {
    printf("Importing .vcf data.\n");
    con_update();
    totalimported = importvcf(buf);
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

void drawinterface(int width) {
  int i;

  if(width == 0) {
    con_clrscr();
    con_gotoxy(0,0);
    for(i=0;i<con_xsize;i++)
      putchar('_');

    con_gotoxy((con_xsize-strlen("  Address Book - 2004  "))/2,0);
    printf("  Address Book - 2004  ");

    con_gotoxy(0,con_ysize-2);
    for(i=0;i<con_xsize;i++)
      putchar('_');

    for(i=1;i<con_ysize-1;i++) {
      con_gotoxy(0,i);
      putchar('|');
      con_gotoxy(namecolwidth,i);
      putchar('|');
      con_gotoxy(con_xsize-1,i);
      putchar('|');
    }

    con_gotoxy(0,con_ysize-1);
    printf("(Q)uit, RETURN, Cursor Left, Cursor Right, (i)mport");
  } else if(smaller) {
    for(i=1;i<con_ysize-2;i++) {
       con_gotoxy(namecolwidth,i);
       putchar('|'); 
       putchar(' ');
    } 
    con_gotoxy(namecolwidth,i);
    putchar('|'); 
    putchar('_');
  } else {
    for(i=1;i<con_ysize-2;i++) {
       con_gotoxy(namecolwidth-1,i);
       putchar(' '); 
       putchar('|');
    }
    con_gotoxy(namecolwidth-1,i);
    putchar('_'); 
    putchar('|');
  }

  //con_update();
}

void clearinfo() {
  char * blankstr;
  int i, width;

  width = con_xsize - namecolwidth - 2;
  blankstr = calloc(width+1,1);

  for(i=0;i<width;i++)
    blankstr[i] = ' ';

  for(i=1;i<con_ysize-2;i++) {
    con_gotoxy(namecolwidth+1,i);
    printf("%s",blankstr);
  }
}

void displayinfo() {
  int row = 3;

  clearinfo();

  con_gotoxy(namecolwidth+2,row++);
  printf("current: %d",current);

  con_gotoxy(namecolwidth+2,row++);
  printf("Name: %s %s", abook[current].firstname,abook[current].lastname);

  con_gotoxy(namecolwidth+2,row++);
  getattrib("org");  
  printf("Organization: %s", buf);
  
  row++;

  con_gotoxy(namecolwidth+2,row++);
  getattrib("email");  
  printf("Email: %s", buf);
  con_gotoxy(namecolwidth+2,row++);
  getattrib("tel");  
  printf("Telephone: %s", buf);

  con_update();
}

void main(int argc, char *argv[]) {
  struct termios tio;
  int refresh = 0, start = 0;
  int total,arrowxpos,arrowypos;

  prepconsole();

  if((fd = open("/sys/addressbook", O_PROC)) == -1) {
    system("addressbookd");
    if((fd = open("/sys/addressbook", O_PROC)) == -1) {
      printf("The addressbook service could not be started.\n");
      con_update();
      exit(1);
    }
  }
  
  buf = (char *)malloc(51);

  namecolwidth = con_xsize/3;
  arrowxpos = 1;
  arrowypos = 1;

  drawinterface(0);
  total = readfromaddressbookd();
  drawaddressbookselector(start,total);

  con_gotoxy(arrowxpos,arrowypos);
  putchar('>');  

  displayinfo();

  input = 'z';  
  while(input != 'Q') {
    input = con_getkey();

    refresh = 1;

    switch(input) {
      case CURD:
        refresh = 0;
        if(current < total-1) {
          if(arrowypos < con_ysize-3) {
            movechardown(arrowxpos, arrowypos, '>');
            arrowypos++;
          } else {
            start++;
            drawaddressbookselector(start,total);
            con_gotoxy(arrowxpos, arrowypos);
            putchar('>');
          }
          current++;
          displayinfo();
        }
      break;
      case CURU:
        refresh = 0;
        if(current > 0) {
          if(arrowypos > 1) {
            movecharup(arrowxpos,arrowypos,'>');
            arrowypos--;
          } else {
            start--;
            drawaddressbookselector(start,total);
            con_gotoxy(arrowxpos, arrowypos);
            putchar('>');
          }
          current--;
          displayinfo();
        }
      break;
      case 62: /* > */
        if(namecolwidth+1 < con_xsize-6) {
          namecolwidth++;
          smaller = 0;
        } else
          refresh = 0;
      break;
      case 60: /* < */
        if(namecolwidth > 5) {
          namecolwidth--;
          smaller = 1;
        } else
          refresh = 0;
      break;
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
        getattrib("email");
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

    if(refresh) {
      drawinterface(namecolwidth);
      total = readfromaddressbookd();
      drawaddressbookselector(start,total);
      displayinfo();
    }
  }
}



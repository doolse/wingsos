#include <stdio.h>
#include <wgsipc.h>
#include <wgslib.h>
#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <console.h>

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

int fd, returncode, input;
char *firstname, *lastname, *attrib, *buf;
char * bufl = NULL;
int size = 0;

void getattrib() {

  printf("firstname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  firstname = strdup(bufl);    
  printf("lastname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  lastname = strdup(bufl);
  printf("attribute ? ");
  con_update();
  getline(&bufl, &size, stdin);
  attrib = strdup(bufl);

  firstname[strlen(firstname)-1] = 0;
  lastname[strlen(lastname)-1] = 0;
  attrib[strlen(attrib)-1] = 0;

  returncode = sendCon(fd, GET_ATTRIB, lastname, firstname, attrib, buf, 50);
  if(returncode == NOENTRY)
    printf("The Entry does not exist.\n");
  else if(returncode == ERROR)
    printf("Error: Buffer Overrun.\n");
  else
    printf("%s\n", buf);
}

void addentry() {

  printf("firstname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  firstname = strdup(bufl);
  printf("lastname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  lastname = strdup(bufl);

  firstname[strlen(firstname)-1] = 0;
  lastname[strlen(lastname)-1] = 0;

  returncode = sendCon(fd, MAKE_ENTRY, lastname, firstname, NULL, NULL, 0);
  if(returncode == NOENTRY)
    printf("An error occured.\n");
  else if(returncode == ERROR)
    printf("Error: that entry already exists\n");
  else
    printf("Success: New Entry Added.\n");
}

void modifyattrib() {
  
  printf("firstname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  firstname = strdup(bufl);    
  printf("lastname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  lastname = strdup(bufl);
  printf("attribute ? ");
  con_update();
  getline(&bufl, &size, stdin);
  attrib = strdup(bufl);
  printf("value ? ");
  con_update();
  getline(&bufl, &size, stdin);
  strncpy(buf, bufl, 50);

  firstname[strlen(firstname)-1] = 0;
  lastname[strlen(lastname)-1] = 0;
  attrib[strlen(attrib)-1] = 0;
  buf[strlen(buf)-1] = 0;

  returncode = sendCon(fd, PUT_ATTRIB, lastname, firstname, attrib, buf, 0);
  if(returncode == NOENTRY)
    printf("The Entry does not exist.\n");
  else
    printf("The Attribute has been succesfully modified.\n");
}

void removeattrib() {

  printf("firstname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  firstname = strdup(bufl);    
  printf("lastname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  lastname = strdup(bufl);
  printf("attribute ? ");
  con_update();
  getline(&bufl, &size, stdin);
  attrib = strdup(bufl);

  firstname[strlen(firstname)-1] = 0;
  lastname[strlen(lastname)-1] = 0;
  attrib[strlen(attrib)-1] = 0;

  returncode = sendCon(fd, DEL_ATTRIB, lastname, firstname, attrib, NULL, 0);
  if(returncode == NOENTRY)
    printf("The Entry does not exist.\n");
  else
    printf("The attribute has been successfully deleted.\n", buf);
}
void deleteentry() {

  printf("firstname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  firstname = strdup(bufl);    
  printf("lastname ? ");
  con_update();
  getline(&bufl, &size, stdin);
  lastname = strdup(bufl);

  firstname[strlen(firstname)-1] = 0;
  lastname[strlen(lastname)-1] = 0;

  returncode = sendCon(fd, DEL_ENTRY, lastname, firstname, NULL, NULL, 0);
  if(returncode == NOENTRY)
    printf("The Entry does not exist.\n");
  else
    printf("The Entry was successfully deleted.\n", buf);
}

void listall() {
  char * listbuf = NULL;
  int buflen = 100;
  namelist * names, * namesptr;
  char * ptr;
  int total, i;

  listbuf = (char *)malloc(buflen);
  if(listbuf == NULL)
    exit(1);

  returncode = sendCon(fd, GET_ALL_LIST, NULL, NULL, NULL, listbuf, buflen);

  //The addressbook returns an ERROR code if the buffer was too small, 
  //and puts the minimum buffer size needed as ascii in the buffer.

  if(returncode == ERROR) { 
    //printf("buffer initially too small, increasing to size: %s\n\n", listbuf);
    buflen = atoi(listbuf);
    free(listbuf);
    listbuf = (char *)malloc(buflen);
    if(listbuf == NULL)
      exit(0);
    returncode = sendCon(fd, GET_ALL_LIST, NULL, NULL, NULL, listbuf, buflen);
  }

  if(returncode == ERROR)
    printf("an unknown error has occurred.\n");
  else {
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

    printf("%s\n", listbuf);
    printf("there are %d names\n", total);

    names = (namelist *)malloc(sizeof(namelist) * (total +1));
    if(names == NULL) {
      printf("memory error\n");
      exit(0);
    }
   
    ptr = listbuf;
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

    for(i = 0;i<total;i++) {
      if(names[i].use == -1) 
        break;
      printf("firstname: '%s', lastname: '%s'\n", names[i].firstname, names[i].lastname);
    }

  }
}

void main(int argc, char *argv[]) {
  struct termios tio;

  input = -1;
  gettio(STDOUT_FILENO, &tio);
  tio.MIN = 1;
  tio.flags |= TF_ICANON;
  settio(STDOUT_FILENO, &tio);

  if((fd = open("/sys/addressbook", O_PROC)) == -1) {
    system("addressbook");
    if((fd = open("/sys/addressbook", O_PROC)) == -1) {
      printf("The addressbook service could not be started.\n");
      exit(1);
    }
  }
  
  buf = (char *)malloc(51);
  
  while(input != 'Q') {
    printf("(a)dd entry, (m)odify attribute, (d)elete entry, (r)emove attribute, (g)et\n");
    printf("(Q)uit, (L)ist all\n");
    tio.flags &= ~TF_ICANON;
    settio(STDOUT_FILENO, &tio);
    input = getchar();
    tio.flags |= TF_ICANON;
    settio(STDOUT_FILENO, &tio);

    switch(input) {
      case 'a':
        addentry();
      break;
      case 'm':
        modifyattrib();
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
      case 'L':
        listall();
      break;
      case 'Q':
        exit(1);
      break;
    }
  }
}



//QSend v2.0 for WiNGs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <xmldom.h>
#include <console.h>
#include <termio.h>
#include "qsend.h"

extern char* getappdir();

//All Functions used by Qsend.

void configqsend();

char * sendtomorerecipients(char * recipientstring);
char * getfilenamefromstring(char * lcfile);

char * getaddyfromnick(char * nick);
int  checkvalidaddy(char * arguement);
void dealwithmimeattach(char * attachstr);


//Main Config data, and main server connection
DOMElement * configxml;
FILE * serverio;

//for getline()
char * buf = NULL;
int size = 0;

int verbose = 0;
int quiet = 0;

//constants
char * boundary = "--__gregs_mime_boundary_2003__--";
char * VERSION = "2.0";

void main(int argc, char *argv[]){
  DOMElement * tempelem;
  FILE * lcmail;
  char * tempstr;
  char * subject, * premsgfile, * ccstring, * bccstring;

  char * smtpserver = NULL;
  char * toaddress  = NULL;
  char * attach     = NULL;

  int ch;

  ccstring = bccstring = NULL;

  configxml = XMLloadFile(fpathname("resources/qsendconfig.xml", getappdir(), 1));

  if(!configxml) {
     if(!quiet)
       printf("config file missing.\n");
     exit(EXIT_FAILURE);
  }

  while((ch = getopt(argc, argv, "vqct:s:S:a:C:B:m:")) != EOF) {
    switch(ch){
      case 'v':
        verbose = 1;
      break;

      case 'q':
        quiet = 1;
      break;

      case 'c': 
        configqsend();
        exit(EXIT_SUCCESS);
      break;
   
      case 't':
        if(checkvalidaddy(optarg))
          toaddress = strdup(optarg);
        else
          toaddress = getaddyfromnick(optarg);

        if(!strlen(toaddress))
          exit(EXIT_BADADDRESS);
      break;

      case 's':
        subject = strdup(optarg);
      break;
   
      case 'S':
        smtpserver = strdup(optarg);
      break;

      case 'a':
        attach = strdup(optarg);
      break;

      case 'm':
        premsgfile = strdup(optarg);
      break;

      case 'C':
        ccstring = strdup(optarg);        
      break;
 
      case 'B':
        bccstring = strdup(optarg);
      break;
    }
  }  

  if (toaddress == NULL){
    printf("Usage: qsend [-c -v -q] [-s \"subject\"] [-a path/file,path/file,...]\n");
    printf("             [-m path/file.txt] -t (address/nick)\n");
    printf("             [-C address,address,...][-B address,address]\n");
    printf("             [-S smtp.server.address]\n\n");
    printf("       -c configure, -a attach, -C CC, -B BCC, -v verbose -q quiet\n");
    printf("       -S specify an alternative SMTP server\n");
    printf("       If -m is not used input comes from Standard In\n");
    exit(EXIT_BADADDRESS);
  }

  if (subject == NULL){
    subject = strdup("-no subject-");
  }
  
  if(verbose)
    printf("Making Connection...\n");

  if(!smtpserver) {
    tempelem = XMLgetNode(configxml, "/xml/smtpserver");
    smtpserver = XMLgetAttr(tempelem, "address");
    if(!smtpserver || !strlen(smtpserver))
      exit(EXIT_NOCONFIG);
  }
  //else it was overridden by a commandline arg.

  tempstr = (char *)malloc(strlen("/dev/tcp/:25")+strlen(smtpserver)+1);
  sprintf(tempstr, "/dev/tcp/%s:25", smtpserver); 
  serverio = fopen(tempstr, "r+");
  if(!serverio){
    if(!quiet)
      printf("Could not connect to the server\n");
    exit(EXIT_BADSERVER); 
  }

  if(verbose)
    printf("Connected...\n");

  fflush(serverio);
  getline(&buf, &size, serverio);

  tempelem = XMLgetNode(configxml, "/xml/domain");

  if(verbose)
    printf("sending 'Hello!' to the server...\n");

  fflush(serverio);
  fprintf(serverio, "HELO %s\r\n", XMLgetAttr(tempelem, "name"));

  fflush(serverio);
  getline(&buf, &size, serverio);

  if((buf[0] == '5') || (buf[0] == ' ' && buf[1] == '5')) {
    if(verbose)
      printf("The server replies rudely... 'Go Away'... \n");
    if(!quiet)
      printf("You cannot use this server with your current dial up.\n");
    exit(EXIT_FAILURE);
  } else if(verbose) 
    printf("The server smiles and waves hello back... \n");
  
  tempelem = XMLgetNode(configxml, "/xml/return");

  fflush(serverio);
  fprintf(serverio, "MAIL FROM: <%s>\r\n", XMLgetAttr(tempelem, "address"));

  fflush(serverio);
  getline(&buf, &size, serverio);

  if((buf[0] == '5') || (buf[0] == ' ' && buf[1] == '5')) {
    if(!quiet)
      printf("This server will not accept your message.\n");
    exit(EXIT_FAILURE);
  }

  fflush(serverio);
  fprintf(serverio, "RCPT TO: <%s>\r\n", toaddress);
		
  fflush(serverio);
  getline(&buf, &size, serverio);

  if((buf[0] == '5') || (buf[0] == ' ' && buf[1] == '5')) {
    if(!quiet)
      printf("This server can't send to that recipient. Relaying access denied.\n");
    exit(EXIT_NORELAY);
  }

  if(verbose)
    printf("Added first recipient to envelope.\n");

  if(ccstring != NULL) 
    ccstring = sendtomorerecipients(ccstring);

  if(bccstring != NULL)
    sendtomorerecipients(bccstring);

  fflush(serverio);
  fprintf(serverio, "DATA\r\n");

  fflush(serverio);
  getline(&buf, &size, serverio);

  //*** Write the Header!
  
  fflush(serverio);
  fprintf(serverio, "From: %s<%s>\n", XMLgetAttr(tempelem, "from"), XMLgetAttr(tempelem, "address"));
  fprintf(serverio, "To: %s\n", toaddress);

  fprintf(serverio, "X-Mailer: Qsend v%s for WiNGs.\n", VERSION);
  fprintf(serverio, "MIME-Version: 1.0\n");

  if(ccstring) {
    fprintf(serverio, "cc: <%s>\n", ccstring);
    if(verbose)
      printf("Adding carbon copies to envelope.\n");
  }

  if(attach)
    fprintf(serverio, "Content-Type: multipart/mixed; boundary=\"%s\"\n", boundary);

  fprintf(serverio, "Subject: %s\n", subject);
  fprintf(serverio, "\n");

  //*** Header Terminated Properly... 

  if(attach) {
    fprintf(serverio, "\nThis message is in multipart MIME format\n\n");
    fprintf(serverio, "--%s\n", boundary);
    fprintf(serverio, "Content-Type: text/plain;\n");
    fprintf(serverio, "\n");
  }

  if(premsgfile) {
    lcmail = fopen(premsgfile, "r");

    if(!lcmail) {
      if(!quiet)
        printf("Couldn't open message file. Email Not sent.\n");
      exit(EXIT_FAILURE);
    }

    if(verbose)
      printf("Sending body text of email...\n");

    while(getline(&buf, &size, lcmail) != EOF)
      fprintf(serverio, "%s", buf);

    fclose(lcmail);
  } else {

    //****** Text body From stdin ******
    if(verbose)
      printf("Sending body text of email...\n");

    while(getline(&buf, &size, stdin) != EOF)
      fprintf(serverio, "%s", buf);
  }

  fprintf(serverio, "\n-- \n");

  //Add the signature

  tempelem = XMLgetNode(configxml, "/xml/signature");

  if(!strlen(tempelem->Node.Value)) {
    if(verbose) 
      printf("Appending standard signature.\n");

    fprintf(serverio, "Qsend v%s for WiNGS.\n", VERSION);

  } else {
    if(verbose)
      printf("Appending custom signature.\n");

    fprintf(serverio, "%s", tempelem->Node.Value);
  }

  if(attach) {
    dealwithmimeattach(attach);
    fprintf(serverio, "\n--%s--\n", boundary);
  }
  
  fprintf(serverio, "\n.\r\n");

  fflush(serverio);
  getline(&buf, &size, serverio);

  if(!quiet) {
    printf("Sent.\n\n");
    printf("Qsend v%s for WiNGS. -- (c)2003\n", VERSION);
  }

  fflush(serverio);
  fprintf(serverio, "QUIT\r\n");
  
  fflush(serverio);
  getline(&buf, &size, serverio);

  fclose(serverio);

  exit(EXIT_SUCCESS);
}

char * sendtomorerecipients(char * recipientstring) { 
  char * ptr, * substr, * returnstring;

  returnstring = (char *)malloc(1);
  *returnstring = 0;

  ptr = substr = recipientstring;
   
  while(1) {
    if(*ptr == 0 || ptr == NULL)
      break;

    ptr = strchr(substr, ',');
    if(ptr) {
      *ptr = 0;
      ptr++;
    }

    if(!strchr(substr, '@'))
      substr = getaddyfromnick(substr);

    if(strlen(substr)) {
      fflush(serverio);
      fprintf(serverio, "RCPT TO: <%s>\n", substr);
      fflush(serverio);
      getline(&buf, &size, serverio);

      returnstring = realloc(returnstring, strlen(returnstring)+strlen(substr)+2);    
      strcat(returnstring, substr);
      strcat(returnstring, ",");
    }
    substr = ptr;
  } 

  if(strlen(returnstring))
    returnstring[strlen(returnstring)-1] = 0;
  return(returnstring);
}

char * getaddyfromnick(char * nick) {
  DOMElement * tempelem;
  char * elemstr;

  elemstr = (char *)malloc(strlen("/xml/nicks/")+strlen(nick)+1);  
  sprintf(elemstr, "/xml/nicks/%s", nick);

  tempelem = XMLgetNode(configxml, elemstr);

  free(elemstr);
  
  if(!tempelem)
    return("");
  else
    return(XMLgetAttr(tempelem, "address"));
}

int checkvalidaddy(char * addy) {
  int numofatsigns = 0;
  char * ptr;

  ptr = addy;

  while(ptr = strchr(ptr, '@')) {
    ptr++;
    numofatsigns++;
  }
 
  if(numofatsigns == 1)
    return(1);
  else
    return(0);
}

void dealwithmimeattach(char * attachstr) {
  char * ptr, * filename, * tempstr, *tempfilepath;
  FILE * readfile;

  tempfilepath = fpathname("data/temp.mime", getappdir(), 1);

  while(1) {
      
    if(ptr == NULL || *ptr == 0)
      break;

    if(ptr = strchr(attachstr, ',')) {
      *ptr = 0;
      ptr++;
    } 
    
    filename = getfilenamefromstring(attachstr);

    if(verbose)
      printf("Encoding attachment as base64...\n");

    tempstr = (char *)malloc(strlen("cat  |base64 e >")+strlen(attachstr)+strlen(tempfilepath)+1);
    sprintf(tempstr, "cat %s |base64 e >%s", attachstr, tempfilepath);
    system(tempstr);
    free(tempstr);     

    fprintf(serverio, "\n--%s\n", boundary);
    fprintf(serverio, "Content-Type: application/octet-stream; name=\"%s\"\n", filename);
    fprintf(serverio, "Content-Transfer-Encoding: base64\n");
    fprintf(serverio, "\n");

    readfile = fopen(tempfilepath, "r");

    if(verbose)
      printf("Uploading Encoded attachment...\n");

    while(getline(&buf, &size, readfile) != EOF) 
      fprintf(serverio, "%s", buf);

    attachstr = ptr;
  }
}

char * getfilenamefromstring(char * lcfile){

  while(strchr(lcfile, '/')) {
    lcfile = strchr(lcfile, '/');
    lcfile++;
  }

  return(strdup(lcfile));
}

void configqsend(){
  DOMElement * tempelem;  

  con_init();
  con_modeon(TF_ECHO|TF_ICRLF|TF_ICANON);

  con_clrscr();

  con_gotoxy(0,5);
  printf("    ** Configure Qsend Emailer **\n\n");

  printf("\tWhat is the outgoing mail (SMTP) server of your Service Provider?\n");
  printf("\tExample: post.kos.net or smtp.mac.com\n\n");

  con_update();

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  tempelem = XMLgetNode(configxml, "/xml/smtpserver");
  XMLsetAttr(tempelem, "address", strdup(buf));

  putchar('\n');
  printf("\tWhat is the Domain Name of your Service Provider?\n");
  printf("\tExample: kos.net or mac.com\n");
  printf("\t(sometimes it's what's after the @ in your email address)\n\n");

  con_update();

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  tempelem = XMLgetNode(configxml, "/xml/domain");
  XMLsetAttr(tempelem, "name", strdup(buf));

  putchar('\n');
  printf("\tWhat is your Email address?\n");
  printf("\tExample: greg@kos.net or john@mac.com\n\n");

  con_update();

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  tempelem = XMLgetNode(configxml, "/xml/return");
  XMLsetAttr(tempelem, "address", strdup(buf));

  putchar('\n');
  printf("\tWhat name do you want to have appear in the \"from\" field?\n");
  printf("\tExample: John Smith or a nickname like Commodore Master\n\n");

  con_update();

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  //note this uses the "return" xml node from above
  XMLsetAttr(tempelem, "from", strdup(buf));

  printf("\nQsend is setup and ready.\n");
  printf("Use \"qsend -c\" to reconfigure.\n");

  XMLsaveFile(configxml, fpathname("resources/qsendconfig.xml", getappdir(), 1));

  con_end();
}



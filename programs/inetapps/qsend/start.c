//QSend v2.1 for WiNGs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <xmldom.h>
#include <console.h>
#include <termio.h>
#include "qsend.h"

DOMElement * configxml;
FILE * serverio;
char * buf = NULL;
int size = 0, verbose = 0, quiet = 0;


void dealwithmimeattach(char * attachstr);

void configqsend(){
  DOMElement * tempelem, * smtpelem;  
  int input;

  con_init();
  con_modeon(TF_ECHO|TF_ICRLF|TF_ICANON);

  con_clrscr();

  con_gotoxy(0,5);
  printf("    ** Configure QuickSend64's Defaults **\n");

  // *** Get Email Address ***

  printf("\n\tWhat is your Email address?\n");
  printf("\tExample: greg@kos.net or john@mac.com\n\n");
  con_update();

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  tempelem = XMLgetNode(configxml, "/xml/return");
  XMLsetAttr(tempelem, "address", buf);

  // *** Get Users Name ***

  printf("\n\tWhat name do you want to have appear in the \"from\" field?\n");
  printf("\tExample: John Smith or C64Master\n\n");
  con_update();

  getline(&buf, &size, stdin);
  buf[strlen(buf)-1] = 0;
  XMLsetAttr(tempelem, "from", buf);

  // *** Get SMTP servers ***

  printf("\n\tWhat is the outgoing mail (SMTP) server of your Service Provider?\n");
  printf("\tExample: post.kos.net or smtp.mac.com\n\n");
  con_update();

  tempelem = XMLgetNode(configxml, "/xml/smtp");
  while(tempelem && tempelem->NumElements)
    XMLremNode(XMLgetNode(tempelem, "server"));

  input = 'y';
  while(input == 'y') {  
    getline(&buf, &size, stdin);
    buf[strlen(buf)-1] = 0;

    smtpelem = XMLnewNode(NodeType_Element, "server", "");
    XMLsetAttr(smtpelem, "address", buf);
    XMLinsert(tempelem, NULL, smtpelem);

    printf("Add additional SMTP server? (y)es or (n)o");
    con_update();
    con_modeoff(TF_ICANON);

    input = 0;
    while(input != 'y' && input != 'n')
      input = con_getkey();

    con_modeon(TF_ICANON);

    if(input == 'y') {
      printf("\n\nWhat is the address of the additional server?\n\n");
      con_update();
    }
  }

  printf("\nQuickSend64 is setup and ready.\n");
  printf("Use \"qsend -c\" to reconfigure.\n");

  XMLsaveFile(configxml, fpathname("resources/qsendconfig.xml", getappdir(), 1));

  con_end();
  exit(EXIT_SUCCESS);
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

char * getaddyfromnick(char * nick) {
  DOMElement * tempelem;
  char * ptr;

  ptr = malloc(strlen("/xml/nicks/")+strlen(nick)+1);  
  sprintf(ptr, "/xml/nicks/%s", nick);

  tempelem = XMLgetNode(configxml, ptr);

  free(ptr);
  
  if(!tempelem)
    return("");
  else
    return(XMLgetAttr(tempelem, "address"));
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
      fprintf(serverio, "RCPT TO: <%s>\r\n", substr);
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

char * getfilenamefromstring(char * lcfile){
  char * ptr;

  while(ptr = strchr(lcfile, '/')) 
    lcfile = ptr+1;

  return(strdup(lcfile));
}

void helptext() {
  printf("All of the commandline options listed below are available, however some of\n them, such as quiet or verbose, are only useful to specific cases of use.\n quiet supresses all output, which is useful when other programs call \nQuickSend64 internally.  verbose is used to give the user more feedback, and\nis really only useful when using QuickSend64 as a stand alone program.\n\n");
   
  printf("Usage: qsend [-c -v -q]\n");
  printf("             -t (address/nick) [-s \"subject\"]\n");
  printf("             [-m path/file.txt] [-a path/file,path/file,...]\n");
  printf("             [-C address,address,...][-B address,address]\n");
  printf("             [-S smtp.server.address][-f from name][-F from@address]\n\n");

  printf("       -c configure, -v verbose, -q quiet\n");
  printf("       -t To address or a nickname\n");
  printf("       -s override the default Subject line\n");
  printf("       -m filename and path for Message file\n");
  printf("       -C comma seperated addresses for Carbon Copy\n");
  printf("       -B comma seperated addresses for Blind Carbon copy\n"); 
  printf("       -S override configured SMTP Server\n");
  printf("       -f override configured from name.\n");
  printf("       -F override configured from address\n");
}

int isvalidresponse() {
  if(buf[0] == '2' || (buf[0] == ' ' && buf[1] == '2'))
    return(1);
  else if(buf[0] == '4' || (buf[0] == ' ' && buf[1] == '4'))
    return(0);
  else if(buf[0] == '5' || (buf[0] == ' ' && buf[1] == '5'))
    return(0);
  else
    return(0);
}

void main(int argc, char *argv[]){
  DOMElement * tempelem, * smtpelem;
  FILE * lcmail;
  char * tempstr, *ptr;
  char * domain, * premsgfile, * ccstring, * bccstring;

  char * smtpserver  = NULL;
  char * toaddress   = NULL;
  char * attach      = NULL;
  char * fromname    = NULL;
  char * fromaddress = NULL;
  char * subject     = NULL;

  int ch, smtpcount;

  ccstring = bccstring = NULL;

  configxml = XMLloadFile(fpathname("resources/qsendconfig.xml", getappdir(), 1));

  if(!configxml) {
     if(!quiet)
       printf("config file missing.\n");
     exit(EXIT_FAILURE);
  }

  while((ch = getopt(argc, argv, "vqct:s:S:f:F:a:C:B:m:")) != EOF) {
    switch(ch){
      case 'v':
        verbose = 1;
      break;

      case 'q':
        quiet = 1;
      break;

      case 'c': 
        configqsend();
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

      case 'f':
        fromname = strdup(optarg);
      break;
 
      case 'F':
        fromaddress = strdup(optarg);
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
    if(!quiet)
      helptext();
    exit(EXIT_BADADDRESS);
  }

  if (subject == NULL){
    subject = strdup("No subject");
  }
  
  if(!fromname) {
    tempelem = XMLgetNode(configxml, "/xml/return");
    fromname = XMLgetAttr(tempelem, "from");
    if(!fromname || !strlen(fromname))
      exit(EXIT_NOCONFIG);
  }

  if(!fromaddress) {
    tempelem = XMLgetNode(configxml, "/xml/return");
    fromaddress = XMLgetAttr(tempelem, "address");
    if(!fromaddress || !strlen(fromaddress))
      exit(EXIT_NOCONFIG);
  }

  //Domain is what follows the @ of your email address
  domain = strchr(fromaddress, '@') +1;

  if(smtpserver) {
    smtpcount = 0;
    goto predefinedsmtpserver;
  } else {
    smtpelem = XMLgetNode(configxml, "/xml/smtp");
    smtpcount = smtpelem->NumElements;
    if(!smtpcount)
      exit(EXIT_NOCONFIG);
    smtpelem = XMLgetNode(smtpelem, "server");
  } 

  while(smtpcount--) {
    smtpelem   = smtpelem->NextElem; 
    smtpserver = XMLgetAttr(smtpelem, "address"); 

    predefinedsmtpserver:
  
    if(verbose)
      printf("Opening connection to %s\n", smtpserver);

    tempstr = malloc(strlen("/dev/tcp/:25")+strlen(smtpserver)+1);
    sprintf(tempstr, "/dev/tcp/%s:25", smtpserver); 
    serverio = fopen(tempstr, "r+");

    if(!serverio){
      if(!quiet)
        printf("Could not connect to the server\n");
      continue;
    }

    if(verbose)
      printf("Connected...\n");

    fflush(serverio);
    getline(&buf, &size, serverio);

    if(verbose)
      printf("Saying 'Hello!' to the server...\n");

    fflush(serverio);
    fprintf(serverio, "HELO %s\r\n", domain);

    fflush(serverio);
    getline(&buf, &size, serverio);
  
    if(!isvalidresponse()) {
      if(verbose)
        printf("The server replies rudely... 'Go Away'... \n");
      if(!quiet)
        printf("You cannot use this server through your current internet connection.\n");
  
      continue;

    } else if(verbose) 
      printf("The server smiles and waves hello back... \n");
  
    fflush(serverio);
    fprintf(serverio, "MAIL FROM: <%s>\r\n", fromaddress);

    fflush(serverio);
    getline(&buf, &size, serverio);

    if(!isvalidresponse()) {
      if(!quiet)
        printf("This server will not accept your message.\n");
      continue;
    }

    fflush(serverio);
    fprintf(serverio, "RCPT TO: <%s>\r\n", toaddress);
		
    fflush(serverio);
    getline(&buf, &size, serverio);

    if(!isvalidresponse()) {
      if(!quiet)
        printf("This server can't send to that recipient. Relaying access denied.\n");
      continue;
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
    fprintf(serverio, "From: %s<%s>\r\n", fromname, fromaddress);
    fprintf(serverio, "To: %s\r\n", toaddress);

    fprintf(serverio, "X-Mailer: Qsend v%s for WiNGs.\r\n", VERSION);
    fprintf(serverio, "MIME-Version: 1.0\r\n");

    if(ccstring) {
      fprintf(serverio, "cc: <%s>\r\n", ccstring);
      if(verbose)
        printf("Adding carbon copies to envelope.\n");
    }

    if(attach)
      fprintf(serverio, "Content-Type: multipart/mixed; boundary=\"%s\"\r\n", BOUNDARY);

    fprintf(serverio, "Subject: %s\r\n", subject);
    fprintf(serverio, "\r\n");

    //*** Header Terminated Properly... 

    if(attach) {
      fprintf(serverio, "\r\nThis message is in multipart MIME format\r\n\r\n");
      fprintf(serverio, "--%s\r\n", BOUNDARY);
      fprintf(serverio, "Content-Type: text/plain;\r\n");
      fprintf(serverio, "\r\n");
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

      while(getline(&buf, &size, lcmail) != EOF) {
        ptr = strchr(buf, '\r');
        if(ptr)
          *ptr = 0;
        ptr = strchr(buf, '\n');
        if(ptr)
          *ptr = 0;
        fprintf(serverio, "%s", buf);
        fprintf(serverio, "\r\n");
      }

      fclose(lcmail);
    } else {

      //****** Text body From stdin ******
      if(verbose)
        printf("Sending body text of email...\n");

      while(getline(&buf, &size, stdin) != EOF) {
        ptr = strchr(buf, '\r');
        if(ptr)
          *ptr = 0;
        ptr = strchr(buf, '\n');
        if(ptr)
          *ptr = 0;
        fprintf(serverio, "%s\r\n", buf);
      } 
    }

    fprintf(serverio, "\r\n-- \r\n");

    //Add the signature
 
    tempelem = XMLgetNode(configxml, "/xml/signature");

    if(!strlen(tempelem->Node.Value)) {
      if(verbose) 
        printf("Appending standard signature.\n");

      fprintf(serverio, "QuickSend v%s for WiNGS.\r\n", VERSION);

    } else {
      if(verbose)
        printf("Appending custom signature.\n\n");

      //a bit of trickery to make sure no \n's are sent without a \r

      ptr = strdup(tempelem->Node.Value);
      tempstr = ptr;

      if(strstr(ptr, "\r\n"))
        fprintf(serverio, "%s", tempelem->Node.Value);
      else {
        while(1) {
          if(strchr(ptr, '\n')) {
            *strchr(ptr, '\n') = 0;
            fprintf(serverio, "%s\r\n", ptr);
            if(verbose)
              printf("%s\n", ptr);
            ptr += strlen(ptr) +1;
          } else {
            fprintf(serverio, "%s", ptr);
            if(verbose)
              printf("%s\n", ptr);
            break;
          }         
        }   
      }
      free(tempstr);
    }

    if(attach) {
      dealwithmimeattach(attach);
      fprintf(serverio, "\r\n--%s--\r\n", BOUNDARY);
    }
  
    //send string that signifies the end of the data
    fprintf(serverio, "\r\n.\r\n");

    fflush(serverio);
    getline(&buf, &size, serverio);

    if(!quiet) {
      printf("Sent.\n\n");
      printf("QuickSend v%s for WiNGS. -- (c)2003\n", VERSION);
    }
 
    fflush(serverio);
    fprintf(serverio, "QUIT\r\n");
  
    fflush(serverio);
    getline(&buf, &size, serverio);

    fclose(serverio);

    exit(EXIT_SUCCESS);
  } //end smtpserver while loop.

  if(!quiet)
    printf("Could not deliver message.\n");
  exit(EXIT_FAILURE);
}

void dealwithmimeattach(char * attachstr) {
  char * ptr, *ptr2, * filename, * tempstr, *tempfilepath;
  FILE * readfile;

  tempfilepath = fpathname("temp.mime", getappdir(), 1);
  tempstr = malloc(strlen("cat  |base64 e >")+strlen(attachstr)+strlen(tempfilepath)+1);

  ptr = attachstr;

  while(1) {
      
    if(!ptr || !*ptr)
      break;

    //ptr is set NULL if it can't find another comma
    if(ptr = strchr(attachstr, ',')) {
      *ptr = 0;
      ptr++;
    } 
    
    filename = getfilenamefromstring(attachstr);

    if(verbose)
      printf("Encoding attachment as base64...\n");

    sprintf(tempstr, "cat %s |base64 e >%s", attachstr, tempfilepath);
    system(tempstr);

    fprintf(serverio, "\r\n--%s\r\n", BOUNDARY);
    fprintf(serverio, "Content-Type: application/octet-stream; name=\"%s\"\r\n", filename);
    fprintf(serverio, "Content-Transfer-Encoding: base64\r\n");
    fprintf(serverio, "\r\n");

    readfile = fopen(tempfilepath, "r");

    if(verbose)
      printf("Uploading Encoded attachment...\n");

    while(getline(&buf, &size, readfile) != EOF) {
      ptr2 = strchr(buf, '\r');
      if(ptr2)
        *ptr2 = 0;
      ptr2 = strchr(buf, '\n'); 
      if(ptr2)
        *ptr2 = 0;
      fprintf(serverio, "%s\r\n", buf);
    }

    fclose(readfile);
    attachstr = ptr;
  }
  free(tempstr);     
}

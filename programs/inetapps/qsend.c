#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>

extern char* getappdir();

void createrc();
char * makecarboncopies(char * ccstring);
char * getfilenamefromstring(char * lcfile);
int  getaddyfromnick(char * nick);
int  checkvalidaddy(char * arguement);
int  dealwithmimeattach();

char * buf      = NULL;
char * addy     = NULL;
char * path     = NULL;
char * attach   = NULL;

int size = 0;
int verbose = 0;
int quiet = 0;

char * boundary = "--++GregDACsMIMEBoundary0954++--";
FILE * incoming;

int main(int argc, char *argv[]){
  FILE * qsendrc, * lcmail;
  char sbuf[50];
  char * returnaddress, * subject, * premsgfile, * ccstring, * bccstring;
  int size, i, j, k, ch;

  returnaddress = subject = premsgfile = ccstring = bccstring = NULL; 
  size = i = j = k = ch = 0;
 
  path    = fpathname("resources/qsend.rc", getappdir(), 1);
  qsendrc = fopen(path, "r");

  if(!qsendrc) {
     printf("qsends Resource file could not be found\n");
     createrc();
     printf("You can now try again.\n");
     printf("To Re-enter configure mode use, qsend -c\n");
     exit(0);
  }

  while((ch = getopt(argc, argv, "vqct:s:a:C:B:m:")) != EOF) {
    switch(ch){
      case 'v':
        verbose = 1;
      break;

      case 'q':
        quiet = 1;
      break;

      case 'c': 
        printf("Going into Configure mode. \n");
        createrc();
        printf("Configuration Changed.\n");
        exit(0);
      break;
   
      case 't':
        if(checkvalidaddy(optarg))
          addy = strdup(optarg);
        else
          getaddyfromnick(optarg);
      break;

      case 's':
        subject = strdup(optarg);
      break;

      case 'a':
        attach = (char *)malloc(strlen(optarg)+2);
        sprintf(attach, "%s,", optarg);
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

      default:
        printf("Unrecognized option, %c, Skipping...\n", ch);
      break;
    }
  }  

  if (addy == NULL){
    printf("Usage: qsend [-c -v] [-s \"subject\"] [-a path/file,path/file,...]\n");
    printf("             [-m path/file.txt] -t (address/nick)\n");
    printf("             [-C address,address,...][-B address,address]\n\n");
    printf("       -c = configure, -a = attach, -C = CC, -B = BCC, -v verbose\n");
    printf("       If -m is not used input comes from Standard In\n");
    exit(0);
  }

  if (subject == NULL){
    subject = strdup("-no subject-");
  }
  
  if(verbose)
    printf("Making Connection...\n");
  
  getline(&buf, &size, qsendrc); //get server string from rc
  buf[strlen(buf)-1] = 0;

  sprintf(sbuf, "/dev/tcp/%s:25", buf); //connect to that server
  incoming = fopen(sbuf, "r+");
  if(!incoming){
    if(!quiet)
      printf("Could not connect to the server\n");
    exit(0); 
  }

  if(verbose)
    printf("Connected...\n");

  fflush(incoming);
  getline(&buf, &size, incoming);

  getline(&buf, &size, qsendrc); 
  buf[strlen(buf)-1] = 0;

  if(verbose)
    printf("sending 'Hello!' to the server...\n");

  fflush(incoming);
  fprintf(incoming, "HELO %s\r\n", buf);

  fflush(incoming);
  getline(&buf, &size, incoming);

  if((buf[0] == '5') || (buf[0] == ' ' && buf[1] == '5')) {
    if(verbose)
      printf("The server replies rudely... 'Go Away'... \n");
    if(!quiet)
      printf("You cannot use this server with your current dial up.\n");
    exit(0);
  } else if(verbose) {
    printf("The server smiles and waves hello back... \n");
  }
  
  getline(&buf, &size, qsendrc);
  buf[strlen(buf)-1] = 0;

  fflush(incoming);
  fprintf(incoming, "MAIL FROM: <%s>\r\n", buf);

  returnaddress = strdup(buf);

  fflush(incoming);
  getline(&buf, &size, incoming);

  if((buf[0] == '5') || (buf[0] == ' ' && buf[1] == '5')) {
    if(!quiet)
      printf("This server will not accept your message.\n");
    exit(0);
  }

  //printf("sent main header...\n");

  fflush(incoming);
  fprintf(incoming, "RCPT TO:<%s>\r\n", addy);

  fflush(incoming);
  getline(&buf, &size, incoming);

  if((buf[0] == '5') || (buf[0] == ' ' && buf[1] == '5')) {
    if(!quiet)
      printf("This server can't send to that recipient. Relaying access denied.\n");
    exit(0);
  }

  if(verbose)
    printf("send first recipient... \n");
  //printf("%s", buf);

  if(ccstring != NULL) 
    ccstring = makecarboncopies(ccstring);
  if(bccstring != NULL)
    makecarboncopies(bccstring);

  fflush(incoming);
  fprintf(incoming, "DATA\n");

  //printf("sent data statement...\n");

  fflush(incoming);
  getline(&buf, &size, incoming);

  getline(&buf, &size, qsendrc);
  buf[strlen(buf)-1] = 0;

  fclose(qsendrc);

  //*** Write the Header!
  
  fflush(incoming);
  fprintf(incoming, "From: %s<%s>\n", buf, returnaddress);
  fprintf(incoming, "To: %s\n", addy);

  fprintf(incoming, "X-Mailer: DAC Productions 2003. Email for C64.\n");
  fprintf(incoming, "MIME-Version: 1.0\n");

  if(ccstring) {
    fprintf(incoming, "cc: <%s>\n", ccstring);
    if(verbose)
      printf("Sending Carbon Copies...\n");
  }

  if(attach)
    fprintf(incoming, "Content-Type: multipart/mixed; boundary=\"%s\"\n", boundary);

  fprintf(incoming, "Subject: %s\n", subject);
  fprintf(incoming, "\n");

  //*** Header Terminated Properly... 

  if(attach) {
    fprintf(incoming, "\nThis message is in multipart MIME format\n\n");
    fprintf(incoming, "--%s\n", boundary);
    fprintf(incoming, "Content-Type: text/plain;\n");
    fprintf(incoming, "\n");
  }

  if(premsgfile) {
    lcmail = fopen(premsgfile, "r");

    if(!lcmail) {
      if(!quiet)
        printf("Couldn't open message file. Email Not sent.\n");
      exit(0);
    }

    if(verbose)
      printf("Sending body text of email...\n");

    while(getline(&buf, &size, lcmail)!=-1)
      fprintf(incoming, "%s", buf);

    fclose(lcmail);
  } else {

    //****** Text body From stdin ******
    if(verbose)
      printf("Sending body text of email...\n");

    while(getline(&buf, &size, stdin) != EOF)
      fprintf(incoming, "%s", buf);
  }

  //****** Add the sig *********

  path   = fpathname("resources/.sig", getappdir(), 1);
  lcmail = fopen(path, "r");

  fprintf(incoming, "\n-- \n");

  if(!lcmail)
    fprintf(incoming, "Sent with QuickSend for WiNGS. -- (c)2003\n");
  else {
    if(verbose)
      printf("Appending Custom signature...\n");
    while(getline(&buf, &size, lcmail) != EOF) 
      fprintf(incoming, "%s", buf);

    fclose(lcmail);
  }

  if(attach) {
    dealwithmimeattach();
    fprintf(incoming, "\n--%s--\n", boundary);
  }
  
  if(!quiet)
    printf("Message Delivered.\n\nSent with QuickSend for WiNGS. -- (c)2003\n");
  fprintf(incoming, ".\r\n");
  
  fflush(incoming);
  getline(&buf, &size, incoming);

  fflush(incoming);
  fprintf(incoming, "QUIT\n");
  
  fflush(incoming);
  getline(&buf, &size, incoming);

  fclose(incoming);

  exit(1);
  return(0);
}

char * makecarboncopies(char * ccstring) { 
  char * address  = NULL;
  char * returncc = NULL;
  char * mainaddy = addy;
  char * ccstring2= NULL;
  int k           = 0;
  int j           = 0;
  int size        = 0;

  returncc = (char *)malloc(strlen(ccstring)+50);

  address = (char *)malloc(strlen(ccstring));

   if(ccstring[strlen(ccstring)] != ',') {
     ccstring2 = (char *)malloc(strlen(ccstring)+1);
     sprintf(ccstring2, "%s,", ccstring);
  } else {
    ccstring2 = strdup(ccstring);
  }
  //fprintf(stderr, "I'm in makecarbonCopies()... \n");
  //printf("ccstirng = %s\n", ccstring2);

   for(j = 0; j<strlen(ccstring2);j++) {
     if(ccstring2[j] != ',') {
       address[k++] = ccstring2[j];
     } else {
       address[k] = 0;
       k = 0;
       if(!strstr(address, "@")) {
         if(getaddyfromnick(address)) {
           fflush(incoming);
           fprintf(incoming, "RCPT TO: <%s>\n", addy);
           strcat(returncc, addy);
           strcat(returncc, ", ");
           fflush(incoming);
           getline(&buf, &size, incoming);
         } else
           fprintf(stderr, "Invalid Nick %s\n", address);
       } else {
         fflush(incoming);
         fprintf(incoming, "RCPT TO: <%s>\n", address);
         strcat(returncc, address);
         strcat(returncc, ", ");
         fflush(incoming);
         getline(&buf, &size, incoming);
       }
     }
   }
  
   returncc[strlen(returncc)-2] = 0;
   addy = mainaddy;    
   return(returncc);
}

void createrc(){
  FILE * rcfile;
  
  rcfile = fopen(fpathname("resources/qsend.rc", getappdir(),1), "w");

  if(!rcfile){
    printf("Resource file could not be created.\n");
    exit(0);
  }

  printf("To use QuickSend you must answer the following questions to set up\n The Resource file.\n\n");
  printf("\tWhat is the SMTP server you want to connect to?\n Example: smtp.mac.com\n");

  getline(&buf, &size, stdin);
  fprintf(rcfile, "%s", strdup(buf));

  printf("\tWhat is the Domain Name of the ISP you are dialed up to now?\n Example: trentu.au\n");

  getline(&buf, &size, stdin);
  fprintf(rcfile, "%s", strdup(buf));

  printf("\tWhat is your return email address?\n Example: Greg@this.is.cool\n");

  getline(&buf, &size, stdin);
  fprintf(rcfile, "%s", strdup(buf));

  printf("\tWhat Name do you want to Have Appear in the From field?\n");

  getline(&buf, &size, stdin);
  fprintf(rcfile, "%s", strdup(buf));

  printf("\nQuickSend is setup and ready.\n");
  fclose(rcfile);
}

int getaddyfromnick(char * nick) {
  FILE * nicklist;

  nicklist = fopen(fpathname("resources/nicks.rc", getappdir(),1), "r");

  if(!nicklist) { 
    if(!quiet)
      printf("Invalid Email address!\n");
    exit(0);
  }

  while(getline(&buf, &size, nicklist) != EOF){
    if(!strncmp(buf, nick, strlen(nick))){
      addy = buf;
      
      buf = NULL;
      size = 0;

      addy = strchr(addy, ' ');
      addy++;
   
      return(1);  
    }
  }
  if(!quiet)
    printf("Invalid Email address!\n");
  exit(0);
  return(0);
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

int dealwithmimeattach() {
  char * filepath, * ptr, * filename, * tempstr, *path;
  FILE * readfile;

  filepath = attach;

  path = fpathname("data/temp.mime", getappdir(), 1);

  while(1) {
      
    if(filepath == NULL || *filepath == 0)
      break;

    if(ptr = strchr(filepath, ',')) {
      *ptr = 0;
      ptr++;
    } 
    
    filename = getfilenamefromstring(filepath);

    filepath = ptr;
   
    if(verbose)
      printf("Encoding attachment as base64...\n");

    tempstr = (char *)malloc(strlen("cat  |base64 e >")+strlen(filename)+strlen(path)+1);
    sprintf(tempstr, "cat %s |base64 e >%s", filename, path);
    system(tempstr);     

    fprintf(incoming, "\n--%s\n", boundary);
    fprintf(incoming, "Content-Type: application/octet-stream; name=\"%s\"\n", filename);
    fprintf(incoming, "Content-Transfer-Encoding: base64\n");
    fprintf(incoming, "\n");

    readfile = fopen(path, "r");
    if(!readfile){
      if(!quiet)
        printf("serious problem... temp base64 file not found\n");
      exit(0);
    }

    if(verbose)
      printf("Uploading Encoded attachment...\n");

    while(getline(&buf, &size, readfile) != EOF) {
      fprintf(incoming, "%s", buf);
    }
  }
  return(1);
}

char * getfilenamefromstring(char * lcfile){

  while(strchr(lcfile, '/')) {
    lcfile = strchr(lcfile, '/');
    lcfile++;
  }

  return(strdup(lcfile));
}

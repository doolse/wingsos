#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>

extern char* getappdir();

void createrc();
char * makecarboncopies(char * ccstring);
int  getaddyfromnick(char * nick);
int  checkvalidaddy(char * arguement);
int  dealwithmimeattach();
int  getfilenamefromstring(char * lcfile);

char   buffer[255];
char   filename[16];
char * buf      = NULL;
char * addy     = NULL;
char * path     = NULL;
char * attach   = NULL;
int verbose = 0;
char * boundary = "--++GregDACsMIMEBoundary0954++--";
FILE * incoming;

int main(int argc, char *argv[]){
  FILE * qsendrc;
  FILE * lcmail;
  char sbuf[50];
  char * returnaddress = NULL;
  char * subject       = NULL;
  char * premsgfile    = NULL;
  char * ccstring      = NULL;
  int  size            = 0;
  int  i               = 0;
  int  j               = 0;
  int  k               = 0;
  int  ch;

  path    = fpathname("resources/qsend.rc", getappdir(), 1);
  qsendrc = fopen(path, "r");

  if(!qsendrc) {
     printf("qsends Resource file could not be found\n");
     createrc();
     printf("You can now try again.\n");
     printf("To Re-enter configure mode use, qsend -c\n");
     exit(-1);
  }

  while((ch = getopt(argc, argv, "vct:s:a:C:m:")) != EOF) {
    switch(ch){
      case 'v':
        verbose = 1;
      break;

      case 'c': 
        printf("Going into Configure mode. \n");
        createrc();
        printf("Configuration Changed.\n");
        exit(1);
      break;
   
      case 't':
        if(checkvalidaddy(optarg)) {
          addy = (char *)malloc(strlen(optarg));
          if(addy == NULL){
            printf("Memory allocation Error.\n");
            exit(1);
          }
          addy = strdup(optarg);
        } else
          getaddyfromnick(optarg);
      break;

      case 's':
        subject = (char *)malloc(strlen(optarg));
        if(subject == NULL){
          printf("Memory Allocation Error.\n");
          exit(1);
        } 
        subject = strdup(optarg);
      break;

      case 'a':
        attach = (char *)malloc(strlen(optarg)+1);
        if(attach == NULL){
          printf("Memory Allocation Error.\n");
          exit(1);
        }
        sprintf(buffer, "%s,", optarg);
        attach = strdup(buffer);
      break;

      case 'm':
        premsgfile = (char *)malloc(strlen(optarg));      
        if(premsgfile == NULL) {
          printf("Memory Allocation Error.\n");
          exit(1);
        }
        premsgfile = strdup(optarg);
      break;

      case 'C':
        ccstring = (char *)malloc(strlen(optarg));
        if(ccstring == NULL) {
          printf("Memory Allocation Error.\n");
          exit(1);
        }
        ccstring = strdup(optarg);        
      break;

      default:
        printf("Unrecognized option, %c, Skipping...\n", ch);
      break;
    }
  }  

  if (addy == NULL){
    printf("Usage: qsend [-c -v] [-s \"subject\"] [-a path/file,path/file,...]\n");
    printf("             [-m path/file.txt] -t (address/nick)\n");
    printf("             [-C address,address,...]\n\n");
    printf("       -c for configure, -a for attach, -C for CC. -v verbose\n");
    printf("       If -m is not used input comes from Standard In\n");
    exit(-1);
  }

  if (subject == NULL){
    subject = (char *)malloc(32);
    if(subject == NULL){
      printf("Memory Allocation error.");
      exit(1);
    }
    subject = strdup("C64:WiNGS -no subject-");
  }
  
  if(verbose)
    printf("Making Connection...\n");
  
  getline(&buf, &size, qsendrc); //get server string from rc
  buf[strlen(buf)-1] = 0;

  sprintf(sbuf, "/dev/tcp/%s:25", buf); //connect to that server
  incoming = fopen(sbuf, "r+");
  if(!incoming){
    printf("Could not connect to the server\n");
    exit(-1); 
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
  fprintf(incoming, "HELO %s\n", buf);

  fflush(incoming);
  getline(&buf, &size, incoming);

  if(buf[0] == 5) {
    if(verbose)
      printf("The server replies rudely... 'Go Away'... \n");
    printf("You cannot use this server with your current dial up.\n");
    exit(1);
  } else if(verbose) {
    printf("The server smiles and waves hello back... \n");
  }
  
  getline(&buf, &size, qsendrc);
  buf[strlen(buf)-1] = 0;

  fflush(incoming);
  fprintf(incoming, "MAIL FROM: <%s>\n", buf);

  returnaddress = strdup(buf);

  fflush(incoming);
  getline(&buf, &size, incoming);

  if(buf[0] == 5) {
    printf("This server will not accept your message.\n");
    exit(1);
  }

  //printf("sent main header...\n");

  fflush(incoming);
  fprintf(incoming, "RCPT TO:<%s>\n", addy);

  fflush(incoming);
  getline(&buf, &size, incoming);

  if(buf[0] == 5) {
    printf("This server can't send to that recipient. Relaying access denied.\n");
    exit(1);
  }

  if(verbose);
    printf("send first recipient... \n");
  //printf("%s", buf);

  if(ccstring != NULL) 
    ccstring = makecarboncopies(ccstring);

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

  fprintf(incoming, "X-Mailer: GregDACs_Client_For_C64\n");
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
      printf("Couldn't open message file. Email Not sent.\n");
      exit(1);
    }

    while(getline(&buf, &size, lcmail)!=-1)
      fprintf(incoming, "%s", buf);

    fclose(lcmail);
  } else {

    //****** Text body From stdin ******
    if(verbose)
      printf("Sending body text of email...\n");
    while(getline(&buf, &size, stdin)!=-1)
      fprintf(incoming, "%s", buf);
  }

  //****** Add the sig *********

  path   = fpathname("resources/.sig", getappdir(), 1);
  lcmail = fopen(path, "r");

  fprintf(incoming, "\n-- \n");

  if(!lcmail)
    fprintf(incoming, "Sent with QuickSend for WiNGS. --written by GregDAC\n");
  else {
    if(verbose)
      printf("Appending Custom signature...\n");
    while(getline(&buf, &size, lcmail)!=-1) 
      fprintf(incoming, "%s", buf);

    fclose(lcmail);
  }

  if(attach) {
    dealwithmimeattach();
    fprintf(incoming, "--%s--\n", boundary);
  }
  
  printf("Message Delivered.\n\n~ Sent with QuickSend for WiNGS. --written by GregDAC\n");
  fprintf(incoming, ".\r\n");
  
  fflush(incoming);
  getline(&buf, &size, incoming);

  fflush(incoming);
  fprintf(incoming, "QUIT\n");
  
  fflush(incoming);
  getline(&buf, &size, incoming);

  fclose(incoming);

  return(1);
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
  if(returncc == NULL) {
    printf("memory allocation error.\n");
    exit(1);
  }

  address = (char *)malloc(strlen(ccstring));
  if(address == NULL) {
    printf("memory allocation error.\n");
    exit(1);
  }

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
  char * path = NULL;
  char * buff = NULL;
  int  size   = 0;
  
  path   = fpathname("resources/qsend.rc", getappdir(), 1);
  rcfile = fopen(path, "w");

  if(!rcfile){
    printf("A very strange Error Occurred, and the Resource file could not be created.\n");
    exit(-1);
  }

  printf("To use QuickSend you must answer the following questions to set up\n The Resource file.\n\n");
  printf("\tWhat is the SMTP server you want to connect to?\n Example: smtp.mac.com\n");

  getline(&buff, &size, stdin);
  fprintf(rcfile, "%s", strdup(buff));

  printf("\tWhat is the Domain Name of the ISP you are dialed up to now?\n Example: trentu.au\n");

  getline(&buff, &size, stdin);
  fprintf(rcfile, "%s", strdup(buff));

  printf("\tWhat is your return email address?\n Example: Greg@this.is.cool\n");

  getline(&buff, &size, stdin);
  fprintf(rcfile, "%s", strdup(buff));

  printf("\tWhat Name do you want to Have Appear in the From field?\n");

  getline(&buff, &size, stdin);
  fprintf(rcfile, "%s", strdup(buff));

  printf("That's it! Have FUN!\n");
  fclose(rcfile);
}

int getaddyfromnick(char * nick) {
  char address[60];
  FILE * nicklist;
  char * path = NULL;  
  char * buf  = NULL;
  int  size   = 0;
  int  i      = 0;
  int  j      = 0;

  path     = fpathname("resources/nicks.rc",  getappdir(), 1);
  nicklist = fopen(path, "r");

  if(!nicklist) { 
    printf("Invalid Email address!\n");
    exit(-1);
  }

  while(getline(&buf, &size, nicklist)!=-1){
    if(!strncmp(buf, nick, strlen(nick))){
      buf[strlen(buf)-1] = 0;

      for(i = 0; i <strlen(buf); i++){
        if(i > strlen(nick)){
          address[j]=buf[i]; 
          j++;
        }
        address[j] = 0;        
      }

      addy = (char *)malloc(strlen(address));
      if(addy == NULL) {
        printf("Memory Allocation error\n");
        exit(1);
      }
      addy = strdup(address);           
      return(1);  
    }
  }
  printf("Invalid Email address!\n");
  exit(-1);
  return(0);
}

int checkvalidaddy(char * arguement) {
  int i = 0;

  for(i = 0; i < strlen(arguement); i++)
    if(arguement[i] == '@')
      return(1);
  return(0);
}

int dealwithmimeattach() {
  int  i        = 0;
  int  j        = 0;
  int  size     = 0;
  int  ch;
  int  pipe1[2];
  int  pipe2[2];
  char * lcfile = NULL;
  FILE * attachfile;
  FILE * writefile;
  FILE * readfile;

  lcfile = (char *)malloc(strlen(attach));
  if(lcfile == NULL) {
    printf("Memory Allocation Error.\n");
    exit(1);
  }

  for(i = 0; i<strlen(attach); i++) {
    if(attach[i] != ','){
      lcfile[j] = attach[i];
      j++;
    } else {
      lcfile[j] = 0;
      j = 0;
      
      //printf("first file with path = %s\n", lcfile);

      getfilenamefromstring(lcfile);
   
      //printf("just filename = %s\n", filename);

      if(verbose)
        printf("Encoding attachment as base64...\n");

      path = fpathname("data/temp.mime", getappdir(), 1);
      sprintf(buffer, "cat %s |base64 e >%s", lcfile, path);
      system(buffer);     

      fprintf(incoming, "--%s\n", boundary);
      fprintf(incoming, "Content-Type: application/octet-stream; name=\"%s\"\n", filename);
      fprintf(incoming, "Content-Transfer-Encoding: base64\n");
      fprintf(incoming, "\n");

      readfile = fopen(path, "r");
      if(!readfile){
         printf("serious problem... temp base64 file not found\n");
         exit(1);
      }

      if(verbose)
        printf("Uploading Encoded attachment...\n");

      while(-1 != getline(&buf, &size, readfile)) {
        fprintf(incoming, "%s", buf);
      }

      /*
      pipe(pipe1);
      pipe(pipe2);

      redir(pipe1[0], STDIN_FILENO);
      redir(pipe2[0], STDOUT_FILENO);
      spawnlp(0, "base64", "e", NULL);
      close(pipe2[0]);
      close(pipe1[0]);

      attachfile = fopen(lcfile, "r"); 
      writefile  = fdopen(pipe1[1], "w");

      while((ch = fgetc(attachfile)) != EOF) {
        fputc(ch, writefile);
      }

      fclose(attachfile);
      fclose(writefile);   

      fprintf(incoming, "--%s\n", boundary);
      fprintf(incoming, "Content-Type: application/octet-stream; name=\"%s\"\n", filename);
      fprintf(incoming, "Content-Transfer-Encoding: base64\n");
      fprintf(incoming, "\n");

      readfile = fdopen(pipe2[1], "r");

      while((ch = fgetc(readfile)) != EOF) {
        fputc(ch, incoming);
      }

      fclose(readfile);
      */
    }
  }
 return(1);
}

int getfilenamefromstring(char * lcfile){
  int  i = 0;
  int  j = 0;
  
  for(i = 0; i<strlen(lcfile); i++) {

    if(lcfile[i]!='/') { 
      buffer[j] = lcfile[i];
      j++;
    } else 
      j = 0;
  }
  buffer[j] = 0;
  //printf("in getting filename... filename is %s\n", buffer);

  strcpy(filename, buffer);

  return(1);
}

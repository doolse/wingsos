#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <wgslib.h>

extern char * getappdir();

  int current, i;
  int size=0;
  int howmany=0;
  char * buf = NULL;
  char buf2[512];
  char server[80];
  char username[50];
  char password[50];
  char * path=NULL;
  int sounds;
  char sound[255];
  int delay = 1500;

int main (int argc, char *argv[]){
  FILE * fp;
  
  if(argc < 2){
    printf("Usage: mailwatch servernumber [delay]\n");
    exit(-1);
  }
  
  if(argc == 3)
    delay = atoi(argv[2]);

  path = fpathname("resources/mail.rc", getappdir());
  fp = fopen(path, "r");
  getline(&buf, &size, fp);

  if(atoi(buf) < atoi(argv[1])){
    printf("Error: Invalid Server Choice\n");
    exit(-1);
  }

  for( i = 1; i < atoi(argv[1]); i++) {
    getline(&buf, &size, fp);
    getline(&buf, &size, fp);
    getline(&buf, &size, fp);
  }

  getline(&buf, &size, fp);
  strncpy(username, buf, 50);
  username[strlen(username)-1] = 0;

  getline(&buf, &size, fp);
  strncpy(password, buf, 50);
  password[strlen(password)-1] = 0;

  getline(&buf, &size, fp);
  strncpy(server, buf, 80);
  server[strlen(server)-1] = 0;

  fclose(fp);

//  printf("User: %s, Password: %s, Server: %s\n", username, password, server);

    path = fpathname("resources/sounds.rc", getappdir());
    fp = fopen(path, "r");
    if(!fp)
      sounds = 0;
    else {
      getline(&buf, &size, fp);
      if(buf[0] == 'y') {
        sounds = 1;
        getline(&buf, &size, fp);
        getline(&buf, &size, fp);
        buf[strlen(buf)-1] = 0;
        strcpy(sound, buf);
      } else
        sounds = 0;
    }
    fclose(fp);

  while(1){

    //buf = (char *)malloc(50);
    //if(buf == NULL)
      //exit(-1);
    sprintf(buf2, "data/mailcount%d.dat", atoi(argv[1]));
    //printf("%s", buf2);
    path = fpathname(buf2, getappdir());
    //free(buf);
    //buf = NULL;
    fp = fopen(path, "r");

    if(!fp) {
      printf("setting current to zero\n");
      current = 0;
    } else {
      getline(&buf, &size, fp);
      current = atoi(buf);
    }
    fclose(fp);

    //buf = (char *)malloc(50);
    //if(buf == NULL)
    //  exit(-1);
    sprintf(buf2, "/dev/tcp/%s:110", server);
    fp = fopen(buf2, "r+");
    //free(buf);
    //buf = NULL;
    if(!fp) {
      printf("Could Not connect to the server");
      fclose(fp);
      exit(-1);
    } else {
      getline(&buf, &size, fp);
      fflush(fp);
      fprintf(fp, "USER %s\n", username);

      fflush(fp);
      getline(&buf, &size, fp);
      fflush(fp);
      fprintf(fp, "PASS %s\n", password);

      fflush(fp);
      getline(&buf, &size, fp);
      if(buf[0] == '+'){
        fflush(fp);
        fprintf(fp, "LIST\n");
        fflush(fp);

        getline(&buf, &size, fp);
        while(buf[0] != '.'){
          getline(&buf, &size, fp);
          if(buf[0] != '.')
            howmany++;      
        }
        if(howmany > current) {
          printf("You have %d new messages\n", howmany-current);
          if(sounds){
            //buf = (char *)malloc(255);
            //if(buf == NULL)
            //  exit(-1);   
            sprintf(buf2, "wavplay %s & >/dev/null", sounds);
            system(buf2);
            //free(buf);
            //buf = NULL;
          }
        }
        howmany = 0;
        fclose(fp);
      } else
        fclose(fp);
      printf("before delay\n");
      sleep(delay);
      printf("after delay\n");
      printf("current %d", current);
    }  
  }
  return(0);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wgslib.h>
#include <console.h>

void main() {
  int digimax = 0;
  int uart    = 0;
  int gui     = 0;
  int size    = 0;
  char *buf   = NULL;

  char *instloc = NULL;
  static char buffer[255];

  FILE *setupscript;
  FILE *userinitscript;

  printf("Where do you want to install Wings?\n");
  printf("ie: /hd0/1  is cmd hd, partition 1\n");
  printf("    /rl0/3  is Ramlink, partition 3\n\n");
  printf("You must specify a Native Partition.\n ?\n");

  getline(&instloc, &size, stdin);

  printf("test1\n");
  printf("%s\n", instloc);

  instloc[strlen(instloc)-1]=0;

  printf("test2\n");
  printf("%s\n", instloc);

  if(instloc[strlen(instloc)-1] == '/')
    instloc[strlen(instloc)-1] = 0;

  printf("test3\n");
  printf("%s\n", instloc);

  printf("Checking Destination '%s' \n", instloc);

  exit(1);

  sprintf(buffer, "%s/test1234", instloc);
  setupscript = fopen(buffer, "w");
  if(!setupscript) {
    printf("Invalid setup location\n");
    exit(1);
  }

  sprintf(buffer, "rm %s/test1234", instloc);
  system(buffer);

  printf("\n Do you have a DigiMax (sound device by DAC)? (y/n)\n");

  if('y' == getchar(stdin))
    digimax = 1;
  else
    digimax = 0;

  printf("\n Do you have a Turbo232, Swiftlink or Duart? (y/n)\n");
  
  if('y' == getchar(stdin))
    uart = 1;
  else
    uart = 0;

  printf("\n Do you want the Graphical User Interface to Startup automatically? (y/n)\n");

  if('y' == getchar(stdin))
    gui = 1;
  else 
    gui = 0;

  setupscript = fopen("setup", "w");
  if(!setupscript){
    printf("Error: Couldn't create setup script\n");
    exit(1);
  }

  fprintf(setupscript, "#!sh\n");
  fprintf(setupscript, "mkdir %s/wings\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/drivers\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/libs\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/gui\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/net\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/scripts\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/tools\n", instloc);
  fprintf(setupscript, "mkdir %s/wings/programs\n", instloc);

  fprintf(setupscript, "mv LtSans.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv Sans4x8.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv booter %s/wings/\n", instloc);
  fprintf(setupscript, "mv help.txt %s/wings/\n", instloc);
  fprintf(setupscript, "mv 4x8font %s/wings/\n", instloc);
  fprintf(setupscript, "mv LtSerif.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv Serif4x8.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv bsw.font %s/wings/\n", instloc);
  fprintf(setupscript, "mv con80.drv %s/wings/\n", instloc);
  fprintf(setupscript, "mv initp %s/wings/\n", instloc);
  fprintf(setupscript, "mv setenv %s/wings/\n", instloc);
  fprintf(setupscript, "mv MedSans.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv Short4x8.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv ajirc %s/wings/\n", instloc);
  fprintf(setupscript, "mv winman %s/wings/\n", instloc);
  fprintf(setupscript, "mv backimg.hbm %s/wings/\n", instloc);
  fprintf(setupscript, "mv sh %s/wings/\n", instloc);
  fprintf(setupscript, "mv init %s/wings/\n", instloc);
  fprintf(setupscript, "mv MedSerif.cfnt %s/wings/\n", instloc);
  fprintf(setupscript, "mv win.drv %s/wings/\n", instloc);
  fprintf(setupscript, "mv con.drv %s/wings/\n", instloc);
  fprintf(setupscript, "mv font %s/wings/\n", instloc);

  fprintf(setupscript, "mv net %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv dial %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv chat %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv email %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv cleanup %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv guestbook.an %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv fixscr %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv guia %s/wings/scripts/\n", instloc);
  fprintf(setupscript, "mv gui %s/wings/net/\n", instloc);

  fprintf(setupscript, "echo working... \n");

  fprintf(setupscript, "mv telnet %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv irc %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv telnetd %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv poff %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv web %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv ftp %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv login %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv term %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv ppp %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv passwd %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv netstat %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv httpd %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv connect %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv thes %s/wings/net/\n", instloc);
  fprintf(setupscript, "mv dict %s/wings/net/\n", instloc);

  fprintf(setupscript, "mkdir %s/wings/net/mail.app\n", instloc);  
  fprintf(setupscript, "mkdir %s/wings/net/mail.app/resources\n", instloc);  
  fprintf(setupscript, "mkdir %s/wings/net/mail.app/data\n", instloc);  

  fprintf(setupscript, "mkdir %s/wings/net/qsend.app\n", instloc);  
  fprintf(setupscript, "mkdir %s/wings/net/qsend.app/resources\n", instloc);  
  fprintf(setupscript, "mkdir %s/wings/net/qsend.app/data\n", instloc);  

  fprintf(setupscript, "mv mail %s/wings/net/mail.app/start\n", instloc);
  fprintf(setupscript, "mv mailwatch %s/wings/net/mail.app\n", instloc);
  fprintf(setupscript, "mv sounds.rc %s/wings/net/mail.app/resources\n", instloc);

  fprintf(setupscript, "mv qsend %s/wings/net/qsend.app/start\n", instloc);
  fprintf(setupscript, "mv .sig %s/wings/net/qsend.app\n", instloc);
  fprintf(setupscript, "mv nicks.rc %s/wings/net/qsend.app/resources\n", instloc);

  fprintf(setupscript, "echo working... \n");

  fprintf(setupscript, "mv times8.a65 %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv fsyslib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv unilib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv syscalls.i65 %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv conlib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv libc.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv winlib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv crt.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv raslib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv 65816.i65 %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv fontlib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv serlib.so %s/wings/libs/\n", instloc);
  fprintf(setupscript, "mv stdio.i65 %s/wings/libs/\n", instloc);

  fprintf(setupscript, "mv guitext %s/wings/gui/\n", instloc);
  fprintf(setupscript, "mv launch %s/wings/gui/\n", instloc);
  fprintf(setupscript, "mv winapp %s/wings/gui/\n", instloc);
  fprintf(setupscript, "mv mine %s/wings/gui/\n", instloc);
  fprintf(setupscript, "mv jpeg %s/wings/gui/\n", instloc);

  fprintf(setupscript, "mv cbmfsys.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv iec.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv uart.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv digi.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv isofsys.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv xiec.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv ide.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv pty.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv idefsys.drv %s/wings/drivers/\n", instloc);
  fprintf(setupscript, "mv tcpip.drv %s/wings/drivers/\n", instloc);

  fprintf(setupscript, "echo working...\n");

  fprintf(setupscript, "mv tail %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv kill %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv uptime %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv getenv %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv mvp %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv ja %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv unpu %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv ar65 %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv textinfo %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv clear %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv puzip %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv mem %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv echo %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv update %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv ld65 %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv mv %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv file65 %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv dir %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv wc %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv lpc %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv gunzip %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv hexdump %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv lpr %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv textconvert %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv rm %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv ls %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv ps %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv more %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv lpq %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv cat %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv lprm %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv modcon %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv cp %s/wings/tools/\n", instloc);
  fprintf(setupscript, "mv reset %s/wings/tools/\n", instloc);

  fprintf(setupscript, "mv playlist %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv josmod %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv pico %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv rawplay %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv shot %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv ned.txt %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv stars %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv ned %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv an %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv wavplay %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv credits %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv ja.txt %s/wings/programs/\n", instloc);
  fprintf(setupscript, "mv config %s/wings/\n", instloc);

  fclose(setupscript);

  userinitscript = fopen("config", "w");
  if(!userinitscript) {
    printf("an error has occured. can't create config file.\n");
    exit(1);
  }

  fprintf(userinitscript, "#!sh\n");

  if(uart)
    fprintf(userinitscript, "uart.drv\n");
  else
    fprintf(userinitscript, "#uart.drv\n");

  if(digimax)
    fprintf(userinitscript, "digi.drv -u\n");
  else
    fprintf(userinitscript, "digi.drv\n");

  if(gui)
    fprintf(userinitscript, "gui\n\n\n");
  else
    fprintf(userinitscript, "#gui\n\n\n");

  fprintf(userinitscript, "#Customize this file by adding or removing\n");
  fprintf(userinitscript, "#pounds signs from the start of the above lines.\n");

  fclose(userinitscript);

//  spawnlp(0, "setup", NULL);
 
}


//can convert wavs to wavs and change there format. 16bit->8bit, 
//Stereo->mono, change volume level (not working yet), change sample rate

//Check here. 
//http://www.technology.niagarac.on.ca/courses/comp630/WavFileFormat.html

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wgslib.h>
#include <netinet/in.h>
#include <fcntl.h>

// These structs are complements of Jolz/onslaught. with minor modifications

typedef struct {
  char RiffIdent[4];
  long TotalSize;
  char RiffType[4];
} Riff;

typedef struct {
  char FormatIdent[4];
  long FormatSize;
  int  PCMflag;
  int  Channels;
  long SampleRate;
  long ByteSec;
  int  ByteSamp;
  int  BitSamp;
} Format;

typedef struct {
  char DataIdent[4];
  long DataSize;
} RData;

// ********* Global Variables *****************

Riff   inriffhdr;
Riff   outriffhdr;

Format informat;
Format outformat;

RData  inrdata;
RData  outrdata;

unsigned int * globalBuf  = NULL;
unsigned int * localBuf   = NULL;

FILE * fp;
int  displayhdr = 0;

int  makemono   = 0;    
int  make8bit   = 0;
int  samplerate = 0;
int  voladj     = 0;

unsigned long bufLength = 0;

// ********* Function Declarations ************

void GetHeaderInfo();
void CreateNewHeader();
void DisplayHeaderInfo();

unsigned int * fchangevol();
unsigned int * fmakemono();
unsigned int * fsamplechg2();
unsigned int * fmake8bit();

void helptext() {
  fprintf(stderr, "Usage: wavhead [options] file.wav\n");
  fprintf(stderr, " -?    this help text\n");
  fprintf(stderr, " -h    displays the .wav header info\n");
  fprintf(stderr, " -m    if stereo, makes mono\n");
  fprintf(stderr, " -8    if 16bit, makes 8bit\n");
  fprintf(stderr, " -s 4  quarter the sample rate\n");
  fprintf(stderr, " -s 2  half the sample rate\n");
  fprintf(stderr, " -b #  Specify the size of the buffer in bytes\n");
  fprintf(stderr, " -v #  volume percent of original. range 10,... 1000\n");
  exit(-1);
}

// ********* Main Loop ************************

int main(int argc, char * argv[]) {
  int ch = 0;
  int  i = 0;
  unsigned char m = 0;
  unsigned char n = 0;
  int finished = 0;
  unsigned long buffersize = 10240;

  if(argc < 2)
    helptext();
  
  while((ch = getopt(argc, argv, "?hm8s:b:v:")) != EOF) {
    switch (ch) {
      case '?':
        helptext();
        break;
      case 'h':
        displayhdr = 1;
        break;
      case 'm':
        makemono   = 1;
        break;
      case '8':
        make8bit   = 1;
        break;
      case 's':
        samplerate = atoi(optarg);
        if(!((samplerate == 4) || (samplerate == 2) || (samplerate == 0)))
          helptext();
        break;
      case 'b':
        buffersize = atoi(optarg);
        break;
      case 'v':
        voladj = atoi(optarg);
        if(voladj > 1000) {
          fprintf(stderr, "Volume Adjust Percent reduced to range maximum... 1000\n");
          voladj = 1000;
        } else if(voladj < 10) {
          fprintf(stderr, "Volume Adjust Percent increased to range minimum... 10\n");
          voladj = 10;
        }
        break;
    }
  }

  if(optind>=argc) 
    exit(1);

  fp = fopen(argv[optind], "rb");

  if(!fp){
    fprintf(stderr, "The file %s could not be opened\n", optind);
    exit(1);
  }

  GetHeaderInfo();    

  if(displayhdr)
    DisplayHeaderInfo();

  if((informat.Channels == 0x01) && (makemono)) {
    makemono = 0;
    fprintf(stderr, "The Sample is already mono.\n");
  }

  if(((informat.ByteSamp == 1) || ((informat.Channels == 0x02) && (informat.ByteSamp == 2))) && (make8bit)) {
    make8bit = 0;
    fprintf(stderr, "The Sample is already 8Bit.\n");
  }

  if(!((samplerate)||(makemono)||(make8bit)||(voladj))) {
    fprintf(stderr, "\nThere are no changes to be made to this file.\n");
    exit(1);
  }

  CreateNewHeader();

  fwrite(&outriffhdr, 1, sizeof(Riff),   stdout);
  fwrite(&outformat,  1, sizeof(Format), stdout); 
  fwrite(&outrdata,   1, sizeof(RData),  stdout);

  globalBuf = (unsigned int *)malloc(1);

  while(finished != 1) {    
    free(globalBuf);
    globalBuf = (unsigned int *)malloc(buffersize);
    if(globalBuf == NULL) {
      fprintf(stderr, "Mem error. Try decreasing the buffersize.\n");
      exit(1);
    }

    bufLength = fread(globalBuf, 1, buffersize, fp);

//    fprintf(stderr,"%u\t/%u\t->", globalBuf[0], globalBuf[1]);

    if(bufLength < buffersize)
      finished = 1;

    if(voladj) {
      globalBuf = fchangevol();
      localBuf = NULL;
    }

    if(makemono) {
      globalBuf = fmakemono();
//      fprintf(stderr, "\t%u\t->", globalBuf[0]);
      localBuf = NULL;
    }

    if(make8bit) {
      globalBuf = fmake8bit();
//      fprintf(stderr,"\t%u\t|\n", (unsigned char)globalBuf[0]);
      localBuf = NULL;
    }

    if(!samplerate || samplerate == 1) {

      fwrite(globalBuf, 1, bufLength, stdout);

    } else if(samplerate == 2) {

      //16bit str
      if(outformat.ByteSamp == 4) {
        i = 0;
        while(i < bufLength) {
          fputc(((globalBuf[i++]/2)+(globalBuf[i++]/2)),stdout);
          fputc(((globalBuf[i++]/2)+(globalBuf[i++]/2)),stdout);
        }
      //16bit mono
      } else if((outformat.Channels == 0x01) && (outformat.ByteSamp == 2)) {
        i = 0;
        while(i < bufLength) {
          fputc(((globalBuf[i++]/2)+(globalBuf[i++]/2)),stdout);
          fputc(((globalBuf[i++]/2)+(globalBuf[i++]/2)),stdout);
        }
        
      //8bit str
      } else if((outformat.Channels == 0x02) && (outformat.ByteSamp == 2)) {
        i = 0;
        while(i < bufLength/4) {
          n = ((unsigned char)globalBuf[i]/2) + ((unsigned char)globalBuf[i+2]/2);
          m = ((unsigned char)globalBuf[i+1]/2) + ((unsigned char)globalBuf[i+3]/2);
          fputc(n, stdout);
          fputc(m, stdout);
          i++;
          i++;
        }
  
      //8bit mono
      } else {
        i = 0;
        while(i < bufLength/2) {
          n = ((unsigned char)globalBuf[i]/2) + ((unsigned char)globalBuf[i+1]/2);
          i++;   
          fputc(n, stdout);
        }
      }

    } else if(samplerate == 4) {
      fprintf(stderr, "Trying to output quartered sample rate... not implemented yet... \n");
    }
  }

  fclose(fp);
  return(0);
}

unsigned int * fchangevol() {
  unsigned char bytea = 0;
  signed   int  inta  = 0;
  int i;
  signed   long inttest;
  unsigned int  chartest;

  localBuf = (unsigned int *)malloc(sizeof(globalBuf));
  if(localBuf == NULL) {
    fprintf(stderr, "Memory allocation error.\n");
    exit(1);
  }

  if(((informat.Channels == 0x01) && (informat.ByteSamp == 2)) || (informat.ByteSamp == 4)) {
    for(i = 0; i < bufLength/2; i++) {
      inta    = globalBuf[i];
      inttest = (inta * voladj)/100;

      if(inttest > 32767)
        localBuf[i] = (signed int)32767;
      else if(inttest < -32768)
        localBuf[i] = (signed int)-32768;
      else 
        localBuf[i] = (signed int)inttest; 
    }
  } else {
    for(i = 0; i <bufLength; i++) {
      bytea = (unsigned char)globalBuf[i];
      chartest = 256 + (bytea * (voladj/100) - (255 - 128/(voladj/100)));     
 
      if(chartest < 256)
        localBuf[i] = (unsigned char)0;
      else if(chartest > 511)
        localBuf[i] = (unsigned char)255;
      else
        localBuf[i] = (unsigned char)(chartest - 256);
    }
  }

  free(globalBuf);
  return(localBuf);
}

unsigned int * fmakemono() {
  unsigned char bytea = 0;
  unsigned char byteb = 0;
  unsigned int  inta  = 0;
  unsigned int  intb  = 0;
  int  i;
  int  j = 0;

  localBuf = (unsigned int *)malloc(bufLength/2);

  if(localBuf == NULL) {
    fprintf(stderr, "Mem Alloc Error.\n");  
    exit(1);
  }
    
  if(((informat.Channels == 0x01) && (informat.ByteSamp == 2)) || (informat.ByteSamp == 4)) {
    // 16bit make mono... 
    //fprintf(stderr, "Making 16bit mono... the first 10k\n");

    for(i = 0; i<bufLength/4; i++) {
      inta = globalBuf[j++];
      intb = globalBuf[j++];
//      fprintf(stderr, "int a = %u\n", inta);
//      fprintf(stderr, "int b = %u\n", intb);
//      fprintf(stderr, "int pair average = %u\n", ((inta/2) + (intb/2)));
      localBuf[i] = ((signed int)inta/2) + ((signed int)intb/2);
    }
  } else {
    // 8bit make mono...

    for(i = 0; i<bufLength/2; i++) {
      bytea = (unsigned char)globalBuf[j++];
      byteb = (unsigned char)globalBuf[j++];
      localBuf[i] = (bytea/2) + (byteb/2);
    }
  }

  free(globalBuf);
  bufLength = bufLength/2;
  return(localBuf);
}

unsigned int * fmake8bit() {
  int  i;
  unsigned char * msb;
  unsigned char * lsb;
  unsigned char * lbufptr;
  unsigned char * gbufptr;
  unsigned char * custom = (unsigned char *)(0xD000);

  custom[0x040e] = 0xff;
  custom[0x040f] = 0xff;
  custom[0x0412] = 0x81;
  custom[0x0413] = 0x00;
  custom[0x0414] = 0xf0;
  custom[0x0418] = 0x80;
  
  localBuf = (unsigned int *)malloc(bufLength/2);
  if(localBuf == NULL) {
    fprintf(stderr, "Mem Alloc Error.\n");  
    exit(1);
  }

  gbufptr = (unsigned char *)globalBuf;
  lbufptr = (unsigned char *)localBuf;

  for(i = 0; i<bufLength; i = i+2) {
    lsb = gbufptr;
    gbufptr++;
    msb = gbufptr;
    gbufptr++;
    
    if((*lsb + custom[0x041b]) > 255)
      *msb = *msb+1;

    *lbufptr = *msb+128;
    lbufptr++;
  }
  
  free(globalBuf);
  bufLength = bufLength/2;
  return(localBuf);
}
// ************ CreateNewHeader() **************

void CreateNewHeader() {
  outriffhdr = inriffhdr;
  outformat  = informat;
  outrdata   = inrdata;

  if(makemono) {
    outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/2)+36;
    outformat.Channels   = 0x01;
    outformat.ByteSec    = outformat.ByteSec/2;

    if(informat.ByteSamp == 2)
      outformat.ByteSamp = 1;

    else if(informat.ByteSamp == 4)
      outformat.ByteSamp = 2;

    outformat.BitSamp    = informat.BitSamp/2;
    outrdata.DataSize    = inrdata.DataSize/2;
  }

  if(make8bit) {
    outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/2)+36;
    outformat.ByteSec    = outformat.ByteSec/2;

    if(outformat.ByteSamp == 4)
      outformat.ByteSamp = 2;

    else if(outformat.ByteSamp == 2)
      outformat.ByteSamp = 1;

    outformat.BitSamp    = outformat.BitSamp/2;
    outrdata.DataSize    = outrdata.DataSize/2;
  }

  if(samplerate) {
    if(samplerate == 4) { 
      outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/4)+36;
      outformat.ByteSec    = outformat.ByteSec/4;
      outformat.SampleRate = outformat.SampleRate/4;
      outrdata.DataSize    = outrdata.DataSize/4;
      
    } else if(samplerate == 2) {
      outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/2)+36;
      outformat.ByteSec    = outformat.ByteSec/2;
      outformat.SampleRate = outformat.SampleRate/2;
      outrdata.DataSize    = outrdata.DataSize/2;

    }
  }
}

// ************ GetHeaderInfo() ****************

void GetHeaderInfo() {
  fread(&inriffhdr, 1, sizeof(Riff),   fp);
  fread(&informat,  1, sizeof(Format), fp);
  fread(&inrdata,   1, sizeof(RData),  fp);
}

// ************* DisplayHeaderInfo() *****************

void DisplayHeaderInfo() {

  fprintf(stderr, "\n\n --Header information--\n\n");
  fprintf(stderr, "       Type: %c, %c\n", inriffhdr.RiffIdent[0], inriffhdr.RiffType[0]);
  fprintf(stderr, "       PCM?: ");

  if(informat.PCMflag == 0x01)
    fprintf(stderr, "Yes.\n");
  else
    fprintf(stderr, "No... That's odd.\n");

  fprintf(stderr, "   Channels: ");
  
  if(informat.Channels == 0x01)
    fprintf(stderr, "1, Mono.\n");
  else if(informat.Channels == 0x02)
    fprintf(stderr, "2, Stereo.\n");
  else
    fprintf(stderr, "Unknown... That's odd.\n");

  fprintf(stderr, " SampleRate: %ld\n", informat.SampleRate);
  fprintf(stderr, "   Bit Rate: ");

  if((informat.Channels == 0x01) && (informat.ByteSamp == 2))
    fprintf(stderr, "16bit.\n");
  else if((informat.Channels == 0x02) && (informat.ByteSamp == 2))
    fprintf(stderr, "8bit.\n");
  else if(informat.ByteSamp == 1)
    fprintf(stderr, "8bit.\n");
  else if((informat.Channels == 0x02) && (informat.ByteSamp == 4))
    fprintf(stderr, "16bit.\n");
  else
    fprintf(stderr, "Unknown... That's odd.\n");

  fprintf(stderr, " SampleSize: %ld\n", inrdata.DataSize);
}


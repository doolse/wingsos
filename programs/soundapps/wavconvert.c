//can convert wavs to wavs and change there format. 16bit->8bit, 
//Stereo->mono, change volume level, change sample rate

//Check here for PCM wav info. 
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

char * VERSION = "1.2";

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

       int  samplerate = 0;
signed int  voladj     = 0;
       int  makemono   = 0;    
       int  make8bit   = 0;

unsigned long bufLength = 0;

// ********* Function Declarations ************

unsigned int * fchangesamplerate();
unsigned int * fchangevol();
unsigned int * fmakemono();
unsigned int * fmake8bit();

void GetHeaderInfo() {
  fread(&inriffhdr, 1, sizeof(Riff),   fp);
  fread(&informat,  1, sizeof(Format), fp);
  fread(&inrdata,   1, sizeof(RData),  fp);
}

void CreateNewHeader() {
  outriffhdr = inriffhdr;
  outformat  = informat;
  outrdata   = inrdata;

  if(makemono) {
    outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/2)+36;
    outformat.Channels   = 0x01;

    if(outformat.ByteSamp == 4 || outformat.ByteSamp ==2)
      outformat.ByteSamp /= 2;

    outformat.ByteSec /=2;
    outformat.BitSamp /=2;
    outrdata.DataSize /=2;
  }

  if(make8bit) {
    outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/2)+36;

    if(outformat.ByteSamp == 4 || outformat.ByteSamp ==2)
      outformat.ByteSamp /= 2;

    outformat.ByteSec /=2;
    outformat.BitSamp /=2;
    outrdata.DataSize /=2;
  }

  if(samplerate == 4) { 
    outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/4)+36;
    outformat.ByteSec    /=4;
    outformat.SampleRate /=4;
    outrdata.DataSize    /=4;

  } else if(samplerate == 2) {
    outriffhdr.TotalSize = ((outriffhdr.TotalSize - 36)/2)+36;
    outformat.ByteSec    /=2;
    outformat.SampleRate /=2;
    outrdata.DataSize    /=2;
  }
}

void DisplayHeaderInfo() {

  fprintf(stderr, "\n\n --Header information--\n\n");
  fprintf(stderr, "       Type: %c, %c\n", inriffhdr.RiffIdent[0], inriffhdr.RiffType[0]);
  fprintf(stderr, "       PCM?: ");

  if(informat.PCMflag == 0x01)
    fprintf(stderr, "Yes.\n");
  else
    fprintf(stderr, "No.\n");

  fprintf(stderr, "   Channels: ");
  
  if(informat.Channels == 0x01)
    fprintf(stderr, "1, Mono.\n");
  else if(informat.Channels == 0x02)
    fprintf(stderr, "2, Stereo.\n");

  fprintf(stderr, " SampleRate: %ld Hz\n", informat.SampleRate);

  if(informat.ByteSamp/informat.Channels == 2)
    fprintf(stderr, "    BitRate: 16 bits/sample\n");
  else
    fprintf(stderr, "    BitRate: 8 bits/sample\n");
    
  fprintf(stderr, " SampleSize: %ld bytes\n", inrdata.DataSize);
}

void helptext() {
  fputc('\n', stderr);
  fprintf(stderr, "   ---/%c_-^%c/ .Wav Convert v%s %c_-^%c--_\n", '\\','\\', VERSION,'\\','\\');
  fputc('\n', stderr);
  fprintf(stderr, "");
  fprintf(stderr, "Usage: wavconvert [options] file.wav\n");
  fprintf(stderr, " -h    displays the .wav header info\n");
  fprintf(stderr, " -m    if stereo, makes mono\n");
  fprintf(stderr, " -8    if 16bit, makes 8bit\n");
  fprintf(stderr, " -s 4  quarter the sample rate\n");
  fprintf(stderr, " -s 2  half the sample rate\n");
  fprintf(stderr, " -b #  specify the size of the buffer in bytes\n");
  fprintf(stderr, " -v #  volume percent of original. range 50,... 200\n");
  exit(1);
}

// ********* Main ************************

void main(int argc, char * argv[]) {
  int ch, finished;
  int takeaction = 0;
  unsigned long buffersize = 51200;

  if(argc < 2)
    helptext();
  
  while((ch = getopt(argc, argv, "b:hm8v:s:")) != EOF) {
    switch (ch) {

      case 'b':
        buffersize = atoi(optarg);
      break;

      case 'h':
        displayhdr = 1;
      break;

      case 'm':
        makemono = 1;
      break;

      case '8':
        make8bit = 1;
      break;

      case 'v':
        takeaction = 1;
        voladj = atoi(optarg);
        if(voladj > 200) {
          fprintf(stderr, "Volume adjust percentage reduced to range maximum: 200\n");
          voladj = 200;
        } else if(voladj < 50) {
          fprintf(stderr, "Volume adjust percentage increased to range minimum: 50\n");
          voladj = 50;
        }
      break;

      case 's':
        takeaction = 1;
        samplerate = atoi(optarg);
        if(samplerate != 0 && samplerate != 2 && samplerate != 4)
          helptext();
      break;
    }
  }

  if(optind>=argc)
    helptext();

  fp = fopen(argv[optind], "rb");

  if(!fp){
    fprintf(stderr, "The file %s could not be opened\n", optind);
    exit(1);
  }

  GetHeaderInfo();    

  if(displayhdr)
    DisplayHeaderInfo();

  if(informat.Channels == 0x01 && makemono) {
    makemono = 0;
    fprintf(stderr, "The Sample is already mono.\n");
  }

  if(informat.ByteSamp/informat.Channels == 1 && make8bit) {
    make8bit = 0;
    fprintf(stderr, "The Sample is already 8 Bit.\n");
  }

  if(!(makemono||make8bit||takeaction)) {
    fprintf(stderr, "\nThere are no changes to be made to this file.\n");
    exit(1);
  }

  CreateNewHeader();

  fwrite(&outriffhdr, 1, sizeof(Riff),   stdout);
  fwrite(&outformat,  1, sizeof(Format), stdout); 
  fwrite(&outrdata,   1, sizeof(RData),  stdout);

  //Just allocate 1 byte, to initialize the cycle below.
  globalBuf = (unsigned int *)malloc(1); 

  finished = 0;
  while(finished != 1) {    
    free(globalBuf);
    globalBuf = (unsigned int *)malloc(buffersize);
    bufLength = fread(globalBuf, 1, buffersize, fp);

    if(bufLength < buffersize)
      finished = 1;

    //primary transformations

    if(samplerate)
      globalBuf = fchangesamplerate();

    if(voladj)
      globalBuf = fchangevol();

    if(makemono)
      globalBuf = fmakemono();

    //make8bit last for superiour sound quality;
    if(make8bit) 
      globalBuf = fmake8bit();

    fwrite(globalBuf, 1, bufLength, stdout);
  }
  fclose(fp);
}

unsigned int * fchangesamplerate() {
  int j = 0;
  int i = 0;
  int tempval;
  unsigned char n;

  if(samplerate == 2) {
      
    bufLength /= 2;      
    localBuf = malloc(bufLength);

    //16bit str SampleRate half-ing
    if(informat.ByteSamp == 4) {
        
      while(i < bufLength) {
        tempval = (signed int)((int *)globalBuf)[i]/2;
        tempval += (signed int)((int *)globalBuf)[i+2]/2;
        localBuf[j++] = tempval;

        tempval = (signed int)((int *)globalBuf)[i+1]/2;
        tempval += (signed int)((int *)globalBuf)[i+3]/2;
        localBuf[j++] = tempval;
        i+=4;
      }

    //16bit mono SampleRate half-ing
    } else if((informat.Channels == 0x01) && (informat.ByteSamp == 2)) {

      while(i < bufLength) {
        tempval = (signed int)((int *)globalBuf)[i++]/2;
        tempval += (signed int)((int *)globalBuf)[i++]/2;
        localBuf[j++] = tempval;
     }
        
    //8bit str SampleRate half-ing
    } else if((informat.Channels == 0x02) && (informat.ByteSamp == 2)) {

      while(i < bufLength*2) {
        n = ((unsigned char)((char *)globalBuf)[i]/2) + ((unsigned char)((char *)globalBuf)[i+2]/2);
        ((char *)localBuf)[j++] = n;        

        n = ((unsigned char)((char *)globalBuf)[i+1]/2) + ((unsigned char)((char *)globalBuf)[i+3]/2);
        ((char *)localBuf)[j++] = n;
        i+=4;
      }
  
    //8bit mono SampleRate half-ing
    } else {

      while(i < bufLength*2) {
        n = ((unsigned char)((char *)globalBuf)[i++]/2) + ((unsigned char)((char *)globalBuf)[i++]/2);
        ((char *)localBuf)[j++] = n;
      }

    }

  } else if(samplerate == 4) {

    bufLength /= 4;      
    localBuf = malloc(bufLength);

    //16bit str SampleRate quartering.
    if(informat.ByteSamp == 4) {
      while(i < bufLength) {
        tempval =  (signed int)((int *)globalBuf)[i]/4;
        tempval += (signed int)((int *)globalBuf)[i+2]/4;
        tempval += (signed int)((int *)globalBuf)[i+4]/4;
        tempval += (signed int)((int *)globalBuf)[i+6]/4;
        localBuf[j++] = tempval;

        tempval =  (signed int)((int *)globalBuf)[i+1]/4;
        tempval += (signed int)((int *)globalBuf)[i+3]/4;
        tempval += (signed int)((int *)globalBuf)[i+5]/4;
        tempval += (signed int)((int *)globalBuf)[i+7]/4;
        localBuf[j++] = tempval;
        i+=8;
      }

    //16bit mono SampleRate quartering.
    } else if((informat.Channels == 0x01) && (informat.ByteSamp == 2)) {
      while(i < bufLength) {
        tempval =  (signed int)((int *)globalBuf)[i++]/4;
        tempval += (signed int)((int *)globalBuf)[i++]/4;
        tempval += (signed int)((int *)globalBuf)[i++]/4;
        tempval += (signed int)((int *)globalBuf)[i++]/4;
        localBuf[j++] = tempval;
      }
        
    //8bit str SampleRate quartering.
    } else if((informat.Channels == 0x02) && (informat.ByteSamp == 2)) {
      while(i < bufLength*2) {
        n =  (unsigned char)((char *)globalBuf)[i]/4;
        n += (unsigned char)((char *)globalBuf)[i+2]/4;
        n += (unsigned char)((char *)globalBuf)[i+4]/4;
        n += (unsigned char)((char *)globalBuf)[i+6]/4;
        ((char *)localBuf)[j++] = n;

        n =  (unsigned char)((char *)globalBuf)[i+1]/4;
        n += (unsigned char)((char *)globalBuf)[i+3]/4;
        n += (unsigned char)((char *)globalBuf)[i+5]/4;
        n += (unsigned char)((char *)globalBuf)[i+7]/4;
        ((char *)localBuf)[j++] = n;
        i+=8;
      }
  
    //8bit mono SampleRate quartering.
    } else {
      while(i < bufLength*2) {
        n =  (unsigned char)((char *)globalBuf)[i++]/4;
        n += (unsigned char)((char *)globalBuf)[i++]/4;
        n += (unsigned char)((char *)globalBuf)[i++]/4;
        n += (unsigned char)((char *)globalBuf)[i++]/4;
        ((char *)localBuf)[j++] = n;
      }
    }
  }

  free(globalBuf);
  return(localBuf);
}

unsigned int * fchangevol() {
  signed long inttest;
  signed int  inta;

  unsigned int  chartest;
  unsigned char bytea;

  int i,j;

  localBuf = malloc(bufLength);

  if((informat.Channels == 0x01 && informat.ByteSamp == 2) || informat.ByteSamp == 4) {

    //16bit either mono or stereo 
    //Loop Through reading and writing 2 bytes at a time.
   
    for(i = 0; i < bufLength/2; i++) {
      inta    = globalBuf[i];
      inttest = (signed long)(inta) * voladj;
      inttest /= 100;

      if(inttest > 32767)
        ((signed int *)(localBuf))[i] = 32767;
      else if(inttest < -32768)
        ((signed int *)(localBuf))[i] = -32768;
      else 
        ((signed int *)(localBuf))[i] = inttest; 
    }
  } else {

    //For 8bit files, mono or stereo
    //Loop through reading and writing 1 byte at a time.

    for(i = 0; i <bufLength; i++) {
      bytea = (unsigned char)((unsigned char *)globalBuf)[i];
       
      chartest = (unsigned char)bytea * (unsigned int)voladj;
      chartest = (unsigned int)chartest / 100;
      chartest = (unsigned int)chartest - 255 - ((unsigned int)(128/(unsigned int)voladj)/100);
      chartest = (unsigned int)chartest + 255;

      //chartest = 256 + ((bytea * (voladj/100)) - (255 - 128/(voladj/100)));
 
      if(chartest < 256)
        ((unsigned char *)localBuf)[i] = 0;
      else if(chartest > 511)
        ((unsigned char *)localBuf)[i] = 255;
      else
        ((unsigned char *)localBuf)[i] = (unsigned int)chartest - 256;
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
  int  k = 0;

  bufLength /= 2;
  localBuf = (unsigned int *)malloc(bufLength);

  //If it's already mono, makemono would have been set to 0 and this
  //function would never have been called. So we know it IS stereo.

  // 16bit make mono... 
  if(informat.ByteSamp == 4) {

    for(i = 0; i<(bufLength/2); i++) {
      inta = globalBuf[j++];
      intb = globalBuf[j++];
      localBuf[i] = ((signed int)inta/2) + ((signed int)intb/2);
    }

  // 8bit make mono...
  } else {
    for(i = 0; i<(bufLength*2);) {
      bytea = ((unsigned char *)globalBuf)[i++];
      byteb = ((unsigned char *)globalBuf)[i++];
      ((unsigned char *)localBuf)[j++] = ((unsigned char)bytea/2) + ((unsigned char)byteb/2);
    }
  }

  free(globalBuf);
  return(localBuf);
}

unsigned int * fmake8bit() {
  int  i;
  unsigned char * msb;
  unsigned char * lsb;
  unsigned char * lbufptr;
  unsigned char * gbufptr;
  int randomnum;  

  bufLength /= 2;
  localBuf = (unsigned int *)malloc(bufLength);

  gbufptr = (unsigned char *)globalBuf;
  lbufptr = (unsigned char *)localBuf;

  for(i = 0; i<(bufLength*2); i+=2) {
    lsb = gbufptr;
    gbufptr++;
    msb = gbufptr;
    gbufptr++;
    
    randomnum = rand();
    randomnum>>=8;
    if((*lsb + randomnum) > 255)
      *msb += 1;

    *lbufptr = *msb+128;
    lbufptr++;
  }
  
  free(globalBuf);
  return(localBuf);
}


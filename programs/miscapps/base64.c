/********************************************************************
BASE64.C

By: David Ross <watson@c64.org>
********************************************************************/

#include<stdio.h>
#include<fcntl.h>

void decodeBase64();
void encodeBase64();
int  findCharInArray(char);
void clearBuffer(int*, int);
void displayBuffer(int*, int, int, char);
void displayInstructions();

static char encodingTable [64] = {

      'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
      'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
      'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
      'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'

};

int main(int argc, char *argv[]) {

   int parmOK   = 0;
   int doEncode = 0;

   if(argc == 2) {
     if(*argv[1] == 'e' || *argv[1] == 'E') {
       doEncode = 1;
       parmOK = 1;
     }
     else if(*argv[1] == 'd' || *argv[1] == 'D') {
       doEncode = 0;
       parmOK = 1;
     }
   }

   if(parmOK) {
     if(doEncode)
       encodeBase64();
     else
       decodeBase64();
   }
   else displayInstructions();

   fflush(stdout);

   return 0;

}

void decodeBase64() {

   int inBuffer[4], outBuffer[3];
   int transChar;
   unsigned short numInputChars = 0;
   unsigned short numValidChars = 0;
   int inChar;

   while( (inChar = getc(stdin)) != EOF ) {

         if(inChar == '=') {
           inBuffer[numInputChars] = 0;
           numInputChars++;
         }
         else {
           if( (transChar = findCharInArray((char)inChar)) != -1) {
         inBuffer[numInputChars] = transChar;
                 numInputChars++;
         numValidChars++;
           }
         }

     if(numInputChars == 4) {

       outBuffer [0] = (inBuffer [0] << 2) |
                           ((inBuffer[1] & 0x30) >> 4);

           outBuffer [1] = ((inBuffer [1] & 0x0F) << 4) |
                               ((inBuffer [2] & 0x3C) >> 2);

       outBuffer [2] = ((inBuffer [2] & 0x03) << 6) |
                                           (inBuffer [3] & 0x3F);

           displayBuffer(outBuffer, (numValidChars-1), 3, '\0');

       numInputChars = 0;
       numValidChars = 0;
     }
   }
}

void encodeBase64() {

   static int inBuffer[3], outBuffer[4];
   static unsigned short numInputChars = 0;
   static int i = 0;
   int inChar;

// Check actual EOF status and don't just look for EOF char
   while( (inChar = fgetc(stdin)) != EOF ) {

       inBuffer[numInputChars++] = inChar;

       if(numInputChars == 3) {

         outBuffer[0] = encodingTable[((inBuffer[0] & 0xFC) >> 2)];

         outBuffer[1] = encodingTable[((inBuffer[0] & 0x03) << 4) |
                                                      ((inBuffer[1] & 
0xF0) >> 4)];

         outBuffer[2] = encodingTable[((inBuffer[1] & 0x0F) << 2) |
                                              ((inBuffer[2] & 0xC0) >> 6)];

             outBuffer[3] = encodingTable[(inBuffer[2] & 0x3F)];

                 i += 4;
                 displayBuffer(outBuffer, numInputChars, 4, '=');

                 if( (i % 76) == 0) fputc('\n', stdout);

         numInputChars = 0;

         clearBuffer(inBuffer, 3);
           }
   }

   // Display anything that's left
   if(numInputChars) {

     outBuffer[0] = encodingTable[((inBuffer[0] & 0xFC) >> 2)];

     outBuffer[1] = encodingTable[((inBuffer[0] & 0x03) << 4) |
                                  ((inBuffer [1] & 0xF0) >> 4)];

     outBuffer[2] = encodingTable[((inBuffer[1] & 0x0F) << 2) |
                                  ((inBuffer[2] & 0xC0) >> 6)];

     outBuffer[3] = encodingTable[(inBuffer[2] & 0x3F)];

     displayBuffer(outBuffer,numInputChars,4,'=');

   }
}

int findCharInArray(char findChar) {

   int pos;
   int found = 0;

   for(pos = 0; pos < 64; pos++) {
     if(encodingTable[pos] == findChar) {
       found = 1;
       break;
     }
   }

   if(!found) pos = -1;

   return pos;

}

void clearBuffer(int* buffer, int len) {
   int* i;
   for(i = buffer; (i-buffer) < len; i++) *i = 0;
}

void displayBuffer(int* buffer, int len, int maxLen, char fillChar) {

   int i;

   for(i = 0; i < maxLen; i++) {
     if(i <= len) fputc(*(buffer+i), stdout);
         else if(fillChar != '\0') fputc(fillChar, stdout);
   }
}

void displayInstructions() {

   puts("Usage: base64 [e/d]");
   puts("\n");
   puts("Purpose:");
   puts("Encodes or Decodes data from stdin to/from base64 until EOF.");
   puts("Outputs to stdout.");
   puts("\n");

   puts("Parameters:");
   puts("e - Encode");
   puts("d - Decode");
   puts("\n");

}

/*

   :::::      Dave Ross / Dr. Watson          "Yesterday's technology
::    ===    watson@enteract.com              today...for a better
::    ===                                     tomorrow!"
   :::::      http://www.enteract.com/~watson

*/

/* t1ascii
 *
 * This program takes an Adobe Type-1 font program in binary (PFB) format and
 * converts it to ASCII (PFA) format.
 *
 * Copyright (c) 1992 by I. Lee Hetherington, all rights reserved.
 *
 * Permission is hereby granted to use, modify, and distribute this program
 * for any purpose provided this copyright notice and the one below remain
 * intact.
 *
 * I. Lee Hetherington (ilh@lcs.mit.edu)
 *
 * $Log: T1ascii.c,v $
 * Revision 1.1  2002/06/18 13:29:51  jmaginni
 * Initial revision
 *
 * Revision 1.1  92/05/22  11:47:24  ilh
 * initial version
 *
 * Ported to Microsoft C/C++ Compiler and MS-DOS operating system by
 * Kai-Uwe Herbing (herbing@netmbx.netmbx.de) on June 12, 1992. Code
 * specific to the MS-DOS version is encapsulated with #ifdef _MSDOS
 * ... #endif, where _MSDOS is an identifier, which is automatically
 * defined, if you compile with the Microsoft C/C++ Compiler.
 *
 */

#ifndef lint
static char rcsid[] =
  "@(#) $Id: T1ascii.c,v 1.1 2002/06/18 13:29:51 jmaginni Exp $";
static char copyright[] =
  "@(#) Copyright (c) 1992 by I. Lee Hetherington, all rights reserved.";
#ifdef _MSDOS
static char portnotice[] =
  "@(#) Ported to MS-DOS by Kai-Uwe Herbing (herbing@netmbx.netmbx.de).";
#endif
#endif

/* Note: this is ANSI C. */

#ifdef _MSDOS
  #include <fcntl.h>
  #include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* int32 must be at least 32-bit */

#define MARKER   128
#define ASCII    1
#define BINARY   2
#define DONE     3

static FILE *ifp;
static FILE *ofp;

/* This function reads a four-byte block length. */

static int32 read_length()
{
  int32 length;

  length = (int32) (fgetc(ifp) & 0xff);
  length |= (int32) (fgetc(ifp) & 0xff) << 8;
  length |= (int32) (fgetc(ifp) & 0xff) << 16;
  length |= (int32) (fgetc(ifp) & 0xff) << 24;

  return length;
}

/* This function outputs a single byte in hexadecimal.  It limits hexadecimal
   output to 64 columns. */

static void output_hex(int b)
{
  static char *hexchar = "0123456789ABCDEF";
  static int hexcol = 0;

  /* trim hexadecimal lines to 64 columns */
  if (hexcol >= 64) {
    fputc('\n', ofp);
    hexcol = 0;
  }
  fputc(hexchar[(b >> 4) & 0xf], ofp);
  fputc(hexchar[b & 0xf], ofp);
  hexcol += 2;
}

static void usage()
{
  fprintf(stderr,
          "usage: t1ascii [input [output]]\n");
  exit(1);
}

static void print_banner()
{
  static char rcs_revision[] = "$Revision: 1.1 $";
  static char revision[20];

  /* if (sscanf(rcs_revision, "$Revision: %19s", revision) != 1) */
    revision[0] = '\0';
  fprintf(stderr, "This is t1ascii %s.\n", revision);
}

int main(int argc, char **argv)
{
  int32 length;
  int c, block = 1, last_type = ASCII;

  ifp = stdin;
  ofp = stdout;
  print_banner();

  if (argc > 3)
    usage();

  /* possibly open input & output files */
  if (argc >= 2) {
    ifp = fopen(argv[1], "r");
    if (!ifp) {
      fprintf(stderr, "error: cannot open %s for reading\n", argv[1]);
      exit(1);
    }
  }
  if (argc == 3) {
    ofp = fopen(argv[2], "w");
    if (!ofp) {
      fprintf(stderr, "error: cannot open %s for writing\n", argv[2]);
      exit(1);
    }
  }

  #ifdef _MSDOS
    /* As we are processing a PFB (binary) input */
    /* file, we must set its file mode to binary. */
    _setmode(_fileno(ifp), _O_BINARY);
  #endif

  /* main loop through blocks */

  for (;;) {
    c = fgetc(ifp);
    if (c == EOF) {
      break;
    }
    if (c != MARKER) {
      fprintf(stderr,
              "error:  missing marker (128) at beginning of block %d",
              block);
      exit(1);
    }
    switch (c = fgetc(ifp)) {
    case ASCII:
      if (last_type != ASCII)
        fputc('\n', ofp);
      last_type = ASCII;
      for (length = read_length(); length > 0; length--)
        if ((c = fgetc(ifp)) == '\r')
          fputc('\n', ofp);
        else
          fputc(c, ofp);
      break;
    case BINARY:
      last_type = BINARY;
      for (length = read_length(); length > 0; length--)
        output_hex(fgetc(ifp));
      break;
    case DONE:
      /* nothing to be done --- will exit at top of loop with EOF */
      break;
    default:
      fprintf(stderr, "error: bad block type %d in block %d\n",
              c, block);
      break;
    }
    block++;
  }
  fclose(ifp);
  fclose(ofp);
  syncfs("/");
  return 0;
}

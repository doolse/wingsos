/*
 * data.c - this is the initialization for the standard streams
 */
/* $Header: /cvsroot/wingsos/wgs-src/lib/src/libc/stdio/data.c,v 1.1 2002/06/18 13:32:09 jmaginni Exp $ */

#include	<stdio.h>

struct __iobuf __stdin = {
	0, 0, _IOREAD, 0,
	(unsigned char *)NULL, (unsigned char *)NULL, 
};

struct __iobuf __stdout = {
	0, 1, _IOWRITE, 0,
	(unsigned char *)NULL, (unsigned char *)NULL, 
};

struct __iobuf __stderr = {
	0, 2, _IOWRITE | _IOLBF, 0,
	(unsigned char *)NULL, (unsigned char *)NULL, 
};

FILE *__iotab[FOPEN_MAX] = {
	&__stdin,
	&__stdout,
	&__stderr,
	0
};


#ifndef _RASLIB_H
#define	_RASLIB_H

/* Routines for dealing with raster interrupts */

extern int getRaster();
extern void freeRaster();
extern void setRaster(void (), int rastline);
extern void stopRaster();
extern void initKey();
extern int scanKey(int *ctrl);
extern void cli();
extern void sei();
extern void lock();
extern void unlock();
extern void setD011(int do11);

#endif /* _RASLIB_H */

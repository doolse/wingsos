#ifndef _STDIO_H
#define	_STDIO_H

#include <sys/types.h>

typedef struct __iobuf {
	int		_count;
	int		_fd;
	int		_flags;
	int		_bufsiz;
	unsigned char	*_buf;
	unsigned char	*_ptr;
} FILE;

#define	_IOFBF		0
#define	_IOREAD		1
#define	_IOWRITE	2
#define	_IONBF		4
#define	_IOMYBUF	8
#define	_IOEOF		0x10
#define	_IOERR		0x20
#define	_IOLBF		0x40
#define	_IOREADING	0x80
#define	_IOWRITING	0x100
#define	_IOAPPEND	0x200
#define _IOEMPEOF	0x400
#define _IOBUFEMP	0x800

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2

#define	stdin		(&__stdin)
#define	stdout		(&__stdout)
#define	stderr		(&__stderr)

#define	BUFSIZ		512

#define	EOF		(-1)

#define	FOPEN_MAX	32
#define	FILENAME_MAX	32

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

extern FILE	*__iotab[FOPEN_MAX];
extern FILE	__stdin, __stdout, __stderr;

extern int fputc(int, FILE *);
extern int fgetc(FILE *);
extern int ungetc(int, FILE *);
extern char *fgets(char *, int ,FILE *);
extern int fputs(char *, FILE *);
extern int puts(char *);

extern int printf(const char *, ...);
extern int fprintf(FILE *,const char *, ...);
extern int sprintf(char *,const char *, ...);

extern int vprintf(const char *, void *);
extern int vfprintf(FILE *,const char *, void *);
extern int vsprintf(char *,const char *, void *);

extern int scanf(const char *, ...);
extern int fscanf(FILE *, const char *, ...);
extern int sscanf(const char *, const char *, ...);

extern int vscanf(const char *, void *);
extern int vfscanf(FILE *, const char *, void *);
extern int vsscanf(const char *, const char *, void *);

extern int __fillbuf(FILE *);
extern int __flushbuf(int , FILE *);
extern int fflush(FILE *);
extern FILE *fopen(const char *,const char *);
extern FILE *popen(const char *cmd, const char *);
extern FILE *fopenp(const char *, const char *);
extern FILE *freopen(const char *,const char *,FILE *);
extern int pclose(FILE *);
extern int fclose(FILE *);
extern FILE *fdopen(int, const char *);
extern void perror(const char *);
extern size_t fread(void *,size_t,size_t,FILE *);
extern size_t fwrite(const void *,size_t,size_t,FILE *);
extern int fseek(FILE *,off_t,int);
extern long ftell(FILE *fp);
extern int setvbuf(FILE *,char *buf, int mode, ssize_t);
extern int getline(char **,ssize_t *,FILE *);
extern int getdelim(char **, ssize_t *, int delim, FILE *);
extern int getchar();
extern int putchar(int c);
extern char *tmpnam(char *s);
#define tempnam(a,b) strdup(tmpnam(b))

extern int remove(const char *);
extern int rename(const char *,const char *);

#define rewind(a) fseek(a, 0L, SEEK_SET)

#define unlink(a)	remove(a)
#define getch()		getchar()
#define getc(f)		fgetc(f)
#define putc(c, f)	fputc(c, f)
#define	feof(p)		(((p)->_flags & _IOEOF) != 0)
#define	ferror(p)	(((p)->_flags & _IOERR) != 0)
#define clearerr(p)     ((p)->_flags &= ~(_IOERR|_IOEOF))
#define fileno(p)	((p)->_fd)
#define io_testflag(p,q) ((p)->_flags & (q))

#endif /* _STDIO_H */

#ifndef _stat_h_
#define _stat_h_

#define DT_PIPE 0x05
#define DT_REG 0x02
#define DT_DIR 0x06
#define DT_DEV 0x1e
#define DT_CHR 0x1d
#define DT_UNKNOWN 0x1f
#define DT_BITS 0x1f

#define S_IFDIR 0x06

#define S_IROTH	0x20
#define S_IWOTH 0x40
#define S_IXOTH 0x80

#define S_IRUSR	0x20
#define S_IWUSR 0x40
#define S_IXUSR 0x80

#define S_IRWXU 0
#define S_IRWXG 0
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IXGRP 0
#define S_IRWXO 0


struct stat {
	int st_mode;
	long st_ino;
	int st_dev;
	long st_size;
	long st_mtime;
	long st_mtime_usec;
   	int sizeexact;
};

extern int stat(const char *fname,struct stat *buf);
extern int lstat(const char *fname,struct stat *buf);
extern int fstat(int fd,struct stat *buf);

extern int mkdir(const char *name, int mode);

#define S_ISDIR(mode) ((mode & DT_BITS) == DT_DIR)
#define S_ISREG(mode) ((mode & DT_BITS) == DT_REG)
#define S_ISDEV(mode) ((mode & DT_BITS) == DT_DEV)
#define S_ISCHR(mode) ((mode & DT_BITS) == DT_CHR)

#endif

#include <stdio.h>
#include <wgslib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <iec.h>
#include <errno.h>
#include <dirent.h>
#include <wgsipc.h>
#include <string.h>

unsigned char blkbuf[256];
char aname[128];
unsigned char binfo[32];
int Channel;

struct AutoBlk *RootDev;
struct AutoBlk *IDEDev;
struct AutoBlk *AllDev;
struct IECInfo IInfo;

int cbmloaded,ideloaded,isoloaded;

int getInfo(char *dev, struct DiskInfo *Info) {
	int fd,val;
	
	fd = open(dev,O_STAT);
	if (fd != -1) {
		val = ioctl(fd, IOCTL_BlkInfo, Info);
		if (!val && Info->Flags&BLKF_IEC) {
			ioctl(fd, IOCTL_IECInfo, &IInfo);
		}
		close(fd);
		return !val;
	}
	return 0;
}

struct AutoBlk *makePart(struct AutoBlk *Auto, int fd, int num, uint32 start, uint32 len, int bootpar) {
	struct AutoBlk *New,*Master;
	
	New = malloc(sizeof(struct AutoBlk));
	sprintf(aname, "%s.%d", Auto->DevName, num);
	New->DevName = strdup(aname);
	if (num != bootpar) {
		sprintf(aname, "%s%d/", Auto->MountName, num);
		New->MountName = strdup(aname);
	} else New->MountName = strdup("/");
	ioctl(fd, IOCTL_AddPart, start, len, New->DevName);
	New->Master = Auto;
	New->SubParts = 0;
	Master = Auto->Master;
	Auto->Master = addQueueB(Master, Master, New);
	Auto->SubParts++;
	return New;
}

char *makeName(struct DiskInfo *Info) {
	int devnum;
	char *name;
	
	switch (Info->DevType) {
		case DTYPE_HD:
			name = "hd";
			break;
		case DTYPE_FD:
			name = "fd";
			break;
		case DTYPE_T1581:
			name = "1581.";
			break;
		case DTYPE_T1571:
			name = "1571.";
			break;
		case DTYPE_T1541:
			name = "1541.";
			break;
		case DTYPE_RL:
			name = "rl";
			break;
		default:
			name = "dr";
			break;
	}
	if (Info->Flags & BLKF_IEC) {
		devnum = IInfo.CBMDev;
	} else devnum = 0;
	sprintf(aname, "/mount/%s%d/", name, devnum);
	return aname;
};

void prpName(struct AutoBlk *Auto) {
	Auto->NameID = addName(Auto->MountName, Channel, Auto);
	Auto->Mounted = 0;
}

void detectAll() {
	struct DiskInfo Info;
	struct AutoBlk *Auto;
	char *name;
	DIR *dir;
	struct dirent *entry;
	
	if (chdir("/dev"))
		return;
	dir = opendir(".");
	if (!dir)
		return;
	while(entry = readdir(dir)) {
	//	printf("Entry %s\n", entry->d_name);
		if (getInfo(entry->d_name, &Info)) {
			Auto = malloc(sizeof(struct AutoBlk));
			Auto->DiskType = Info.DevType;
			if (Info.DevType == DTYPE_IDE64)
				IDEDev = Auto;
			Auto->FSysName = NULL;
			Auto->Master = NULL;
			Auto->SubParts = 0;
			Auto->Flags = Info.Flags;
			if (Info.Flags & BLKF_IEC && (IInfo.CBMDev == CBMBootDev || IInfo.CBMDev > 20))
				RootDev = Auto;
			Auto->DevName = fpathname(entry->d_name, "/dev", 0);
			Auto->MountName = strdup(makeName(&Info));
			prpName(Auto);
			printf("Got one = %s,%s,%d!\n", Auto->DevName, Auto->MountName, Auto->NameID); 
			AllDev = addQueueB(AllDev, AllDev, Auto);
		} 
	}
	closedir(dir);
}

void loadCBMFsys() {
	if (!cbmloaded) {
		spawnlp(S_WAIT,"cbmfsys.drv",NULL);
		cbmloaded++;
	}
}

void loadIDEFsys() {
	if (!ideloaded) {
		spawnlp(S_WAIT,"idefsys.drv",NULL);
		ideloaded++;
	}
}

int aioctl(struct AutoBlk *Auto, int ctl) {
	int ret=0;
	int fd;
	
	fd = open(Auto->DevName, O_STAT);
	if (fd != -1) {
		ret = ioctl(fd, ctl);
		close(fd);
	}
	return ret;
};

void mountit(char *sys, struct AutoBlk *Auto, int RcvID) {
	remName(Auto->NameID);
	if (Auto->Flags&BLKF_Removable)
		aioctl(Auto, IOCTL_Change);
	if (mount(sys, Auto->DevName, Auto->MountName)) {
		setErr(RcvID, EIO);
		prpName(Auto);
	} else {
		Auto->Mounted = 1;
		setErr(RcvID, EDOAGAIN);
	}
}

char *getcmdtype(int type) {

	switch (type) {
		case 1:
			return "/sys/fsys.cmd";
		case 2:
			return "/sys/fsys.1541";
		case 3:
			return "/sys/fsys.1571";
		case 4:
			return "/sys/fsys.1581";
	}
	return NULL;
}

void rlparts(struct AutoBlk *Auto) {
	int fd,bootpar=-1;
	uint i;
	uint type;
	uint32 start;
	uint32 len;
	char *Name;
	struct AutoBlk *New;
	
	fd = open(Auto->DevName, O_READ);
	if (fd != -1) {
		if (Auto == RootDev) {
			ioctl(fd, IOCTL_BootInfo, binfo);
			bootpar = binfo[0];
		}
		if ((sendCon(fd, IO_READB, blkbuf, 4UL, 256)&0xffff) == 256) {
			for (i=0;i<32;i++) {
				type = blkbuf[i];
				Name = getcmdtype(type);
				if (Name) {
					start = blkbuf[0x60+i]*256 + blkbuf[0x80+i] + 5;
					start <<= 8;
					len = blkbuf[0x20+i]*256 + blkbuf[0x40+i];
					len <<= 8;
					New = makePart(Auto, fd, i, start, len, bootpar);
					New->FSysName = Name;
				}
			}
		}
		Auto->Mounted = 1;
		close(fd);
	}
}

void cmdparts(struct AutoBlk *Auto) {
	int fd,bootpar=-1;
	uint i,j;
	uint32 start;
	uint32 len;
	uchar *upto;
	char *Name;
	struct AutoBlk *New;
	
	fd = open(Auto->DevName, O_READ);
	if (fd != -1) {
		if (Auto == RootDev) {
			ioctl(fd, IOCTL_BootInfo, binfo);
			bootpar = binfo[0];
		}
		for (j=0;j<32;j++) {
			if ((sendCon(fd, IO_READB, blkbuf, (uint32) 0x400+j, 256)&0xffff) == 256) {
				upto = blkbuf;
				for (i=0;i<8;i++) {
					Name = getcmdtype(upto[2]);
					if (Name) {
						start = (uint16)upto[0x17] + (upto[0x16]<<8) + ((uint32)((uint16)upto[0x15])<<16);
						start <<= 9;
						len = 0x100;
						len <<= 8;
						
						New = makePart(Auto, fd, i+j*8, start, len,bootpar);
						New->FSysName = Name;
					}
					upto += 0x20;
				}
			}
		}
		Auto->Mounted = 1;
		close(fd);
	}
}

void prepSubs(struct AutoBlk *Auto, int RcvID) {
	struct AutoBlk *Cur,*Head = Auto->Master;
	
	if (!Auto->SubParts)
		return;
	remName(Auto->NameID);
	if (Auto->SubParts == 1) {
		Head->NameID = addName(Auto->MountName, Channel, Head);
		Head->Mounted = 0;
		return;
	}
	Cur = Head;
	do {
		prpName(Cur);
		Cur = Cur->Next;
	} while (Cur != Head);
	setErr(RcvID, EDOAGAIN);
}

void detectFS(struct AutoBlk *Auto, int RcvID) {
	int fd;
	char *sys;
	
	switch (Auto->DiskType) {
		case DTYPE_HD:
			loadCBMFsys();
			cmdparts(Auto);
			prepSubs(Auto, RcvID);
			break;
		case DTYPE_RL:
			loadCBMFsys();
			rlparts(Auto);
			prepSubs(Auto, RcvID);
			break;
		case DTYPE_T1541:
			loadCBMFsys();
			mountit("/sys/fsys.1541", Auto, RcvID);
			break;
		case DTYPE_T1571:
			loadCBMFsys();
			sys = "/sys/fsys.1541";
			fd = open(Auto->DevName, O_READ);
			if (fd != -1) {
				if ((sendCon(fd, IO_READB, blkbuf, 357UL, 256)&0xffff) == 256) {
					if (blkbuf[3]&0x80)
						sys = "/sys/fsys.1571";
				}
				close(fd);
			}
			mountit(sys, Auto, RcvID);
			break;
		case DTYPE_T1581:
			loadCBMFsys();
			mountit("/sys/fsys.1581", Auto, RcvID);
			break;
		case DTYPE_TCMD:
			loadCBMFsys();
			mountit("/sys/fsys.cmd", Auto, RcvID);
			break;
		case DTYPE_IDE64:
			loadIDEFsys();
			mountit("/sys/fsys.ide64", Auto, RcvID);
			break;
		default:
			printf("Couldn't find a filesystem for %d!\n", Auto->DiskType);
			break;
		
	}
}

int main(int argc, char *argv[]) {
	struct AutoBlk *Auto,*Auto2;
	uchar *MsgP;
	int RcvID,ret,timer;
	uint Val;
	
	printf("Automounter loaded\n");
	Channel = makeChan();
	detectAll();
	if (!RootDev)
		RootDev = IDEDev;
	if (RootDev) {
		Auto = RootDev;
		remName(Auto->NameID);
		Auto->NameID = addName("/", Channel, Auto);
		free(Auto->MountName);
		Auto->MountName = strdup("/");
	}
	timer = setTimer(-1, 3000, 0, Channel, PMSG_Alarm);
	retexit(0);
	while(1) {
		RcvID = recvMsg(Channel, (void *) &MsgP);
		ret = -1;
		if (RcvID) {
			Auto = (struct AutoBlk *) getSCOID(RcvID);
			switch (MsgP[0]) {
			case IO_OPEN:
				if (!**(char **)(MsgP+2) && *(int *)(MsgP+6) == O_STAT) {
					ret = makeCon(RcvID, NULL);
					break;
				}
				goto dodetect;
			case IO_CLOSE:
				break;
			case IO_FSTAT:
				if (!Auto) {
					ret = _fillStat(MsgP, DT_DIR);
					break;
				}
			default:
				dodetect:
				if (!Auto->FSysName)
					detectFS(Auto, RcvID);
				else {
					mountit(Auto->FSysName, Auto, RcvID);
				}
				break;
			}
		} else {
			timer = setTimer(timer, 4000, 0, Channel, PMSG_Alarm);
			Auto = AllDev;
			if (Auto) {
				do {
					if (Auto->Flags & BLKF_Removable && Auto->Mounted && aioctl(Auto, IOCTL_Change)) {
						// printf("Detected change on %s\n", Auto->DevName);
						if (Auto->SubParts) {
							ret = 0;
							while ((Auto2 = Auto->Master) || ret) {
								ret = umount(Auto2->MountName);
								if (!ret)
									Auto->Master = remQueue(Auto2, Auto2);
							}
							if (!Auto2) {
								Auto->SubParts = 0;
								aioctl(Auto, IOCTL_RemParts);
							}
						} else {
							ret = umount(Auto->MountName);
						}
						if (!ret) {
							prpName(Auto);
							// printf("Unmounted it!\n");
						} else {
							printf("Please put that disk back in!\n");
						}
					}
					Auto = Auto->Next;
				} while (Auto != AllDev);
			}
		}
		replyMsg(RcvID, ret);
	}
	
}

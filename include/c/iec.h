#ifndef _IEC_H
#define	_IEC_H

#define CBMBootDev (*(int *)0x010007)

enum {
DTYPE_HD,
DTYPE_FD,
DTYPE_T1541,
DTYPE_T1571,
DTYPE_T1581,
DTYPE_RL,
DTYPE_IDE64
};

enum {
PTYPE_Unknown,
PTYPE_CMD,
PTYPE_T1541,
PTYPE_T1571,
PTYPE_T1581,
PTYPE_IDE64,
PTYPE_ISO,
PTYPE_FAT
};

struct DiskInfo {
	int DevType;
	int MedType;
	int Flags;
	int BlkSize;
	union {
		unsigned long Blocks;
		struct {
			int Track;
			int Sector;
		} TS;
		struct {
			int Cyl;
			int Heads;
			int Sectors;
		} CHS;
	} Phy;
};

struct IECInfo {
	int Device;
	int SubDev;
	int CBMDev;
	int DevType;
	int IFlags;
};

enum {
BLKF_Inserted	= 1,
BLKF_Removable	= 2,
BLKF_ReadOnly	= 4,
BLKF_Booted	= 8,
BLKF_IEC	= 16,
BLKF_Master	= 32
};

enum {
IOCTL_BlkInfo=0x80,
IOCTL_DiskInfo,
IOCTL_Change,
IOCTL_LightOn,
IOCTL_LightOff,
IOCTL_IECInfo,
IOCTL_AddPart,
IOCTL_RemParts,
IOCTL_BootInfo
};

struct AutoBlk {
	struct AutoBlk *Next;
	struct AutoBlk *Prev;
	struct AutoBlk *Master;
	int DiskType;
	int Flags;
	char *FSysName;
	char *DevName;
	char *MountName;
	int NameID;
	int SubParts;
	int Mounted;
};

#endif

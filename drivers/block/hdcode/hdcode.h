/*-----------------------------------*/

VIA2			= $8400
PIO			= $8800
LED			= $8f00

L90E5			= $90E5

/*-----------------------------------*/


/*-----------------------------------*/


/*-----------------------------------*/

BusC_D	= $01
BusMSG	= $02
BusI_O	= $04
BusATN	= $10
BusBSY	= $20
BusREQ	= $80

/*-----------------------------------*/

/*-----------------------------------*/

	.struct PList_
Num	.dsb $100
Typ	.dsb $100
SB0	.dsb $100
SB1	.dsb $100
SB2	.dsb $100
SB3	.dsb $100
PL0	.dsb $100
PL1	.dsb $100
PL2	.dsb $100
PL3	.dsb $100
Len	= *
	.stend

/*-----------------------------------*/

Err_NoDAcDev		= 1
Err_FunnyBlkSize	= 2
Err_ScsiDevNotPresent	= 3
Err_ScsiLunNotPresent	= 4
Err_ScsiReserved	= 5
Err_ScsiLunNotSupported	= 6

Err_Scsi86	= $81
Err_Scsi87	= $82


SCSIBSS_BlkBuf	= $5000		;$2000


	.abs $1000
PList		.dsb PList_Len
Device_Cap	.24 3
SCSIBSS_MsgBuf	.dsb $102
SCSIBSS_St	.word 2
ParityTab	.dsb $100


	.abs $0b
DSAFE		.byte 1
DSAFE2		.byte 1
DBYTE		.byte 1
NextDevZp_Zp0	.byte 1
PNumberCnt	.byte 1
EntryCnt	.byte 1
EntryPtr	.word 2
ReadZp_Blk	.long 4
ReadZp_Ptr	.word 2
SCSIZp_BufPtr	.word 2

SCSIZp_Dev	.byte 1
SCSIZp_Lun	.byte 1

Zp0		.byte 1

DLOAD 		= 4


/*-------------------------------------

 StartUp for XA-Assembler V2.1

 BA(a)       : Set StartAdress and first 2 Bytes as Load-Adress

 PHASE(a)    : Relocate Code to a
 DEPHASE     : Stop Relocation

 ZEROPAGE(a) : Start Area for Zeropage Vars
 ZEROPAGEEND : End Def. for ZP Vars

 ALIGN(a)    : Align Adress

 JUMPIN      : Set Adress to Start Program via Beam64

-------------------------------------*/

#ifndef __StartUp
__StartUp	= 

BA(a)	=  .text:.DW a:*=a

PHASE(a)	=  -_PHASENEWADR=a:-_PHASEOLDADR=*:*=a
DEPHASE	=  *=_PHASEOLDADR+*-_PHASENEWADR

ZEROPHASE(a)	=  -_ZEROPHASEOLDADR=_ZEROPOINTER:-_ZEROPOINTER=a
ZERODEPHASE	=  -_ZEROPOINTER=_ZEROPHASEOLDADR

INLINE	=  -__INLINEPC=*:*=:-__INLINESTART=*
OUTLINE	=  -__INLINEEND=*:*=__INLINEPC+__INLINEEND-__INLINESTART

ALIGN(a)	=  -_ALIGN0=(*+a-1)&($ffff-a+1):-_ALIGN1=_ALIGN0-*:.dsb _ALIGN1,0

MSize_8	=  .as
MSize_16	=  .al
XSize_8	=  .xs
XSize_16	=  .xl

CHARSET_ASC	=  .DFT 0
CHARSET_PET	=  .DFT 1
CHARSET_SCR	=  .DFT 2

/*-----------------------------------*/

MBYTE	=     	.DSB 1
MWORD	=     	.DSB 2
MLONG	= 		.DSB 3
MAPTR	= 		.DSB 4
MAREA(a)	=  	.DSB a

STRUCT	= 		-_STRUCTUREZP=0
SBYTE	= 		=_STRUCTUREZP:-_STRUCTUREZP+=1
SWORD	= 		=_STRUCTUREZP:-_STRUCTUREZP+=2
SLONG	= 		=_STRUCTUREZP:-_STRUCTUREZP+=3
SAPTR	= 		=_STRUCTUREZP:-_STRUCTUREZP+=4
SAREA(a)	= 	=_STRUCTUREZP:-_STRUCTUREZP+=a
SLEN	= 		=_STRUCTUREZP


BSSORG(a)	= 	-_BSSPOINTER=a
BBYTE	=     	=_BSSPOINTER:-_BSSPOINTER+=1
BWORD	=     	=_BSSPOINTER:-_BSSPOINTER+=2
BLONG	= 		=_BSSPOINTER:-_BSSPOINTER+=3
BAPTR	= 		=_BSSPOINTER:-_BSSPOINTER+=4
BAREA(a)	=  	=_BSSPOINTER:-_BSSPOINTER+=a


ZEROORG(a)	= 	-_ZEROPOINTER=a
ZBYTE	=     	=_ZEROPOINTER:-_ZEROPOINTER+=1
ZWORD	=     	=_ZEROPOINTER:-_ZEROPOINTER+=2
ZLONG	= 		=_ZEROPOINTER:-_ZEROPOINTER+=3
ZAPTR	= 		=_ZEROPOINTER:-_ZEROPOINTER+=4
ZAREA(a)	=  	=_ZEROPOINTER:-_ZEROPOINTER+=a

#endif


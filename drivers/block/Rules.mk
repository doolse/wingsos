BLKDIR := $(DRVDIR)block
VPATH += :$(BLKDIR)
BLKDRV := $Oiec.drv $Oide.drv
ALLOBJ += $(BLKDRV)

$Odos.o65: JAFLAGS = -e

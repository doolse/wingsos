BLKDIR := $(DRVDIR)block
VPATH += :$(BLKDIR)
BLKDRV := $(BD)iec.drv $(BD)ide.drv
ALLOBJ += $(BLKDRV)

$Odos.o65: JAFLAGS = -e

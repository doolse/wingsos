FSYSDIR := $(DRVDIR)fsys
VPATH += :$(FSYSDIR)
FSYSDRV := $(BD)cbmfsys.drv $(BD)idefsys.drv $(BD)pipe.drv 
#$Oisofsys.drv
ALLOBJ += $(FSYSDRV)

$(FSYSDRV): LDFLAGS += -lfsyslib


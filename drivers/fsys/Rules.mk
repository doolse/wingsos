FSYSDIR := $(DRVDIR)fsys
VPATH += :$(FSYSDIR)
FSYSDRV := $Ocbmfsys.drv $Oidefsys.drv $Opipe.drv 
#$Oisofsys.drv
ALLOBJ += $(FSYSDRV)

$(FSYSDRV): LDFLAGS += -lfsyslib


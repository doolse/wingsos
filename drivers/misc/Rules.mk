VPATH += :$(DRVDIR)misc
MISCDRV := $Odigi.drv $Owin.drv $Oxiec.drv
ALLOBJ += $(MISCDRV)

$Owin.drv: LDFLAGS += -t 768 -lfontlib -lraslib

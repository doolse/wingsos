VPATH += :$(DRVDIR)misc
MISCDRV := $Odigi.drv $Owin.drv $Oxiec.drv $Efonts/bsw.font
ALLOBJ += $(MISCDRV)

$Owin.drv: LDFLAGS += -t 768 -lfontlib -lraslib

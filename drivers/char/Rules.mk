CHARDIR := $(DRVDIR)char
VPATH += :$(CHARDIR)
CHARDRV := $Ocon80.drv $Ocon.drv $Ouart.drv $Opty.drv
ALLOBJ += $(CHARDRV)

$(CHARDRV): LDFLAGS += -lserlib
$Ocon80.drv $Ocon.drv: LDFLAGS += -lserlib -lraslib

VPATH += :$(DRVDIR)misc
MISCDRV := $(BD)digi.drv $(BG)win.drv $(BD)xiec.drv $Efonts/bsw.font
ALLOBJ += $(MISCDRV)

$(BD)win.drv: LDFLAGS += -t 768 -lfontlib -lraslib

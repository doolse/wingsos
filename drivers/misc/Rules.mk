VPATH += :$(DRVDIR)misc
MISCDRV := $(BD)digi.drv $(BG)win.drv $(BD)xiec.drv
ALLOBJ += $(MISCDRV)

$(BG)win.drv: LDFLAGS += -t 768 -llibc -lfontlib -lraslib

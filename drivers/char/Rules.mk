CHARDIR := $(DRVDIR)char
VPATH += :$(CHARDIR)
CHARDRV := $(BD)con80.drv $(BD)con.drv $(BD)uart.drv $(BD)pty.drv \
$(BF)4x8font $(BF)LtSerif.cfnt $(BF)Sans4x8.cfnt $(BF)bsw.font \
$(BF)MedSans.cfnt $(BF)Serif4x8.cfnt $(BF)font $(BF)LtSans.cfnt \
$(BF)MedSerif.cfnt $(BF)Short4x8.cfnt
ALLOBJ += $(CHARDRV)

$(CHARDRV): LDFLAGS += -lserlib
$(BD)con80.drv $(BD)con.drv: LDFLAGS += -lserlib -lraslib

CHARDIR := $(DRVDIR)char
VPATH += :$(CHARDIR)
CHARDRV := $(BD)con80.drv $(BD)con.drv $(BD)uart.drv $(BD)pty.drv \
$(BGF)4x8font $(BGF)LtSerif.cfnt $(BGF)Sans4x8.cfnt $(BGF)bsw.font \
$(BGF)MedSans.cfnt $(BGF)Serif4x8.cfnt $(BGF)font $(BGF)LtSans.cfnt \
$(BGF)MedSerif.cfnt $(BGF)Short4x8.cfnt
ALLOBJ += $(CHARDRV)

$(CHARDRV): LDFLAGS += -lserlib
$(BD)con80.drv $(BD)con.drv: LDFLAGS += -lserlib -lraslib

DRVDIR := drivers/

include $(DRVDIR)char/Rules.mk
include $(DRVDIR)net/Rules.mk
include $(DRVDIR)fsys/Rules.mk
include $(DRVDIR)block/Rules.mk
include $(DRVDIR)misc/Rules.mk

$(BD)%.drv: LDFLAGS = -lcrt -llibc
$(BD)%.drv $(BG)%.drv: $O%.o65 $(JL65) $(BDIRS)
	$(JL65) -y $(LDFLAGS) -G -p -o $@ $<

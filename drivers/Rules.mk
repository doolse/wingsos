DRVDIR := drivers/

include $(DRVDIR)char/Rules.mk
include $(DRVDIR)net/Rules.mk
include $(DRVDIR)fsys/Rules.mk
include $(DRVDIR)block/Rules.mk
include $(DRVDIR)misc/Rules.mk

$O%.drv: LDFLAGS = -lcrt -llibc
$O%.drv: $O%.o65 $(JL65)
	$(JL65) -y $(LDFLAGS) -G -p -o $@ $<

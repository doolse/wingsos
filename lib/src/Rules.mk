LIBDIR := lib/src/
VPATH += :$(LIBDIR)
SHLIBS = $(LIBS)

include $(LIBDIR)libc/Rules.mk
include $(LIBDIR)winlib/Rules.mk

LIBS := $Lfontlib.so $Lfsyslib.so $Lconlib.so $Lserlib.so $Lraslib.so
ALLOBJ += $(LIBS) $Lstartpic.o65 $Lstartwgs.o65

$L%.so: $(JL65)
	$(JL65) -s0x100 -y $(LDFLAGS) -o $@ $(filter %.o65, $^)


$L%.o65: %.a65 $(JA)
	$(JA) -o $@ $<

$(LIBS): LDFLAGS += -lcrt -llibc
$(LIBS): %.so : %.o65

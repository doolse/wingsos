LIBDIR := lib/src/
VPATH += :$(LIBDIR)
SHLIBS = $(LIBS)

include $(LIBDIR)libc/Rules.mk
include $(LIBDIR)winlib/Rules.mk
include $(LIBDIR)utils/Rules.mk

LIBS := $Lfontlib.so $Lfsyslib.so $Lconlib.so $Lserlib.so $Lraslib.so $Lxmldom.so
CRT := lib/startpic.o65 lib/startwgs.o65
ALLOBJ += $(LIBS) $(CRT)
	
$L%.so: $O%.o65 $(JL65) $(BDIRS)
	$(JL65) -s0x100 -y $(LDFLAGS) -o $@ $(filter %.o65, $^)

$OCRT.flag: $(CRT)
	touch $@
	
lib/%.o65: %.a65 $(JA)
	$(JA) -o $@ $<

$(LIBS): LDFLAGS += -lcrt -llibc

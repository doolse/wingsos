LIBCDIR := $(LIBDIR)libc/
OB := $(LIBCDIR)obj/

include $(LIBCDIR)crt/Rules.mk
include $(LIBCDIR)ansi/Rules.mk
include $(LIBCDIR)misc/Rules.mk
include $(LIBCDIR)wgsipc/Rules.mk
include $(LIBCDIR)wgslib/Rules.mk
include $(LIBCDIR)string/Rules.mk
include $(LIBCDIR)stdio/Rules.mk
include $(LIBCDIR)unilib/Rules.mk

CLIBS := $Lcrt.so $Llibc.so $Lunilib.so
ALLOBJ += $(CLIBS)
SHLIBS += $(CLIBS)

$(OB)%.o65: %.a65 $(JA)
	$(JA) -o $@ $<

$(OB)%.o65: %.c
	lcc $(CFLAGS) -pic -c -o $@ $<

$(CLIBS): $(JL65) $(BDIRS)
	$(JL65) -s0x100 -y $(LDFLAGS) -o $@ $(filter %.o65, $^)
	
$Lcrt.so: $(CRTOBJ)
$Lunilib.so: $(UNIOBJ)
$Lunilib.so: LDFLAGS += -lcrt -llibc
$Llibc.so: $(LIBCOBJ)
$Llibc.so: LDFLAGS += -lcrt

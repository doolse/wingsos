VPATH += :$(PRGDIR)soundapps
SNDPRG := $(BPS)josmod $(BPS)rawplay $(BPS)wavplay $(BPS)wavhead
ALLOBJ += $(SNDPRG)

$(BPS)josmod: $Oajosmod.o65
$(BPS)josmod: CFLAGS += -Wl-f0x02

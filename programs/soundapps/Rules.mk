VPATH += :$(PRGDIR)soundapps
SNDPRG := $(BPS)josmod $(BPS)rawplay $(BPS)wavplay
ALLOBJ += $(SNDPRG)

$(BPS)josmod: $Oajosmod.o65
$(BPS)josmod: CFLAGS += -Wl-f0x02

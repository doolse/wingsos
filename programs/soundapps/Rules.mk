VPATH += :$(PRGDIR)soundapps
SNDPRG := $(BPS)josmod $(BPS)rawplay $(BPS)wavplay $(BPS)wavconvert $(BPS)wavplaythreads $(BPS)sidplay $(BPS)testsid.dat $(BPS)mitchdane.zip
ALLOBJ += $(SNDPRG)

$(BPS)%: $Etestfiles/%
	cp $< $@

$(BPS)wavplaythreads: CFLAGS += -lconlib -lwinlib
$(BPS)josmod: $Oajosmod.o65
$(BPS)josmod: CFLAGS += -Wl-f0x02
$(BPS)sidplay: $Oasidplay.o65

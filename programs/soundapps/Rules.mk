VPATH += :$(PRGDIR)soundapps
SNDPRG := $Ojosmod $Orawplay $Owavplay
ALLOBJ += $(SNDPRG)

$Ojosmod: $Oajosmod.o65
$Ojosmod: CFLAGS += -Wl-f0x02

VPATH += :$(PRGDIR)miscapps
MISCPRG := $Opuzip $Ogunzip $OT1ascii $Ostars $Oned $Otimes8 $Ounpu $Ogeos $Oc64 $Obase64
ALLOBJ += $(MISCPRG)

$Oned: CFLAGS += -Wl-f2 -Wl-t2048 -lconlib
$Ogunzip: CFLAGS += -Wl-t5000
$Ostars: CFLAGS += -lraslib
$Ostars: $Oastars.o65
$Oan: CFLAGS += -Wl-t2048

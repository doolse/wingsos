VPATH += :$(PRGDIR)guiapps
GUIPRG := $Ojpeg $Ocredits $Owinman $Otutapp $Owinapp $Omine
ALLOBJ += $(GUIPRG)

$(GUIPRG): CFLAGS += -lwinlib -lfontlib
$Omine: $Oamine.o65
$Ojpeg: $Oajpeg.o65

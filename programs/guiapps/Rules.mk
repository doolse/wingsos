VPATH += :$(PRGDIR)guiapps
GUIPRG := $Ojpeg $Ocredits $Owinman $Otutapp $Owinapp $Omine $Ebackgrounds/backimg.hbm
ALLOBJ += $(GUIPRG)

$(GUIPRG): CFLAGS += -lwinlib -lfontlib
$Omine: $Oamine.o65
$Ojpeg: $Oajpeg.o65

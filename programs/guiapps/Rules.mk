VPATH += :$(PRGDIR)guiapps
GUIPRG := $Ojpeg $Ocredits $Owinman $Otutapp $Owinapp $Omine $Olaunch $Oguitext $Ebackgrounds/backimg.hbm
ALLOBJ += $(GUIPRG)

$(GUIPRG): CFLAGS += -lwinlib -lfontlib
$Olaunch:  CFLAGS += -lunilib -lwinlib
$Omine: $Oamine.o65
$Ojpeg: $Oajpeg.o65

VPATH += :$(PRGDIR)guiapps
GUIPRG := $(BG)gui $(BG)backimg.hbm $(BG)winman $(BG)winapp
#$(BPG)jpeg $(BG)credits $(BG)search $(BPD)tutapp $(BPU)mine $(BG)launch $(BG)guitext
ALLOBJ += $(GUIPRG)

$(GUIPRG): CFLAGS += -lwinlib -lfontlib
$(BG)launch:  CFLAGS += -lunilib -lwinlib
$(BPU)mine: $Oamine.o65
$(BPG)jpeg: $Oajpeg.o65
$(BG)winapp: CFLAGS = -w -lwinlib -lfontlib -Wl-t0x400

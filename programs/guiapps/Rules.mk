VPATH += :$(PRGDIR)guiapps
GUIPRG := $(BPG)jpeg $(BG)credits $(BG)winman $(BPD)tutapp $(BG)winapp $(BPU)mine $(BG)launch $(BG)guitext $(BG)backimg.hbm $(BG)gui
ALLOBJ += $(GUIPRG)

$(GUIPRG): CFLAGS += -lwinlib -lfontlib
$(BG)launch:  CFLAGS += -lunilib -lwinlib
$(BPU)mine: $Oamine.o65
$(BPG)jpeg: $Oajpeg.o65

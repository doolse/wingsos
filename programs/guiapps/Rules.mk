VPATH += :$(PRGDIR)guiapps
GUIPRG := $(BG)gui $(BG)backimg.hbm $(BG)winman $(BG)winapp $(BPU)mine $(BPG)jpeg $(BPU)j.jpg $(BG)search $(BG)launch $(BG)guitext $(BG)wordserve $(BG)credits 
#$(BPD)tutapp  
ALLOBJ += $(GUIPRG)

$(GUIPRG): CFLAGS += -lwinlib -lfontlib
$(BG)launch:  CFLAGS += -lunilib -lwinlib
$(BG)wordserve: CFLAGS += -lunilib
$(BPU)mine: CFLAGS += -lunilib -lwinlib -lwgsutil
$(BPU)mine: $Oamine.o65
$(BPG)jpeg: $Oajpeg.o65
$(BG)winman: $OJMan.o65
$(BG)winman: CFLAGS = -w -lwinlib -lfontlib -lwgsutil -Wl-t0x400
$(BG)winapp: CFLAGS = -w -lwinlib -lfontlib -Wl-t0x400
$(BPU)mine: CFLAGS = -w -lwinlib -lfontlib -lwgsutil

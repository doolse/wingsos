VPATH += :$(PRGDIR)miscapps
MISCPRG := $(BPU)puzip $(BPU)gunzip $(BPU)T1ascii $(BPU)stars $(BPU)ned $(BPD)times8 $(BPU)unpu $(BPU)geos $(BPU)c64 $(BPU)base64 $(BPU)textinfo $(BPU)textconvert $(BPU)playlist $(BPU)fileman $(BPU)addressbook.app/start $(BPU)abook
ALLOBJ += $(MISCPRG)

$(BPU)addressbook.app:
	cp -R $(VPATH)addressbook.app $@
	rm -rf $(BPU)addressbook.app/CVS

$(BPU)ned: CFLAGS += -Wl-f2 -Wl-t2048 -lconlib
$(BPU)gunzip: CFLAGS += -Wl-t5000
$(BPU)stars: CFLAGS += -lraslib
$(BPU)stars: $Oastars.o65
$(BPU)an: CFLAGS += -Wl-t2048
$(BPU)playlist: CFLAGS += -Wl-t2048
$(BPU)fileman: CFLAGS += -Wl-t2048 -lconlib -lunilib -lxmldom
$(BPU)addressbook.app/start: $(BPU)addressbook.app
$(BPU)addressbook.app/start: CFLAGS += -lxmldom
$(BPU)abook: CFLAGS += -lconlib -lunilib

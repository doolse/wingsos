VPATH += :$(PRGDIR)testapps
TESTPRG := $(BPU)tail $(BPU)test $(BPU)testasm $(BPU)xmltest.xml $(BPU)modeltest $(BPU)xmltest
ALLOBJ += $(TESTPRG)

$(BPU)%: $Etestfiles/%
	cp $< $@

$(BPU)test: CFLAGS += -lwinlib -lfontlib -Wl-t768
$(BPU)modeltest: CFLAGS += -lwinlib -lwgsutil -lfontlib -Wl-t768
$(BPU)xmltest: CFLAGS += -lxmldom -lwgsutil -Wl-t768

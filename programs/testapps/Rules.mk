VPATH += :$(PRGDIR)testapps
TESTPRG := $(BPU)tail $(BPU)test $(BPU)testasm $(BPU)xmltest.xml
ALLOBJ += $(TESTPRG)

$(BPU)%: $Etestfiles/%
	cp $< $@

$(BPU)test: CFLAGS += -lxmldom

GCC=gcc
BTOOLS := btools/
BO := $(BTOOLS)obj/
JA=$(BTOOLS)ja
JL65=$(BTOOLS)jl65
AR65=$(BTOOLS)ar65
FILE65=$(BTOOLS)file65
JAOBJ = $(BO)asm.o $(BO)mne.o $(BO)target.o $(BO)parse.o
FOBJ = $(BO)file65.o
JLOBJ = $(BO)jl65.o
AROBJ = $(BO)ar65.o
BINDIR = $(HOME)/bin
VPATH += :$(BTOOLS)
BINTOOLS := $(JA) $(JL65) $(AR65) $(FILE65)

$(BO)%.o: %.c
	$(GCC) -c -o $@ $<

$(JAOBJ): $(BTOOLS)asm.h

$(AR65): $(AROBJ)
	gcc -o $@ $(AROBJ)

$(FILE65): $(FOBJ)
	gcc -o $@ $(FOBJ)

$(JL65): $(JLOBJ)
	gcc -o $@ $(JLOBJ)

$(JA): $(JAOBJ)
	gcc -o $@ $(JAOBJ)
	echo `pwd`/include > $(HOME)/.ja
	echo `pwd`/lib >> $(HOME)/.ja

GCC=gcc
BTOOLS := btools/
BO := $(BTOOLS)obj/
JA=$(BTOOLS)ja
JL65=$(BTOOLS)jl65
AR65=$(BTOOLS)ar65
DATA65=$(BTOOLS)data65
FILE65=$(BTOOLS)file65
MKIM=$(BTOOLS)mkimage
NETDISK=$(BTOOLS)netdrive
JAOBJ = $(BO)asm.o $(BO)mne.o $(BO)target.o $(BO)parse.o $(BO)getopt.o
FOBJ = $(BO)file65.o $(BO)getopt.o
JLOBJ = $(BO)jl65.o $(BO)getopt.o
AROBJ = $(BO)ar65.o $(BO)getopt.o
D6OBJ = $(BO)data65.o $(BO)getopt.o
MKOBJ = $(BO)mkimage.o $(BO)getopt.o
NDOBJ = $(BO)netdrive.o
BINDIR = $(HOME)/bin
VPATH += :$(BTOOLS)
BINTOOLS := $(JA) $(JL65) $(AR65) $(FILE65) $(DATA65) $(NETDISK)

$(BO)%.o: %.c
	$(GCC) -c -o $@ $<

$(JAOBJ): $(BTOOLS)asm.h

$(AR65): $(AROBJ)
	gcc -o $@ $(AROBJ)

$(FILE65): $(FOBJ)
	gcc -o $@ $(FOBJ)

$(MKIM): $(MKOBJ)
	gcc -o $@ $(MKOBJ)

$(JL65): $(JLOBJ)
	gcc -o $@ $(JLOBJ)

$(NETDISK): $(NDOBJ)
	gcc -o $@ $(NDOBJ)

$(DATA65): $(D6OBJ)
	gcc -o $@ $(D6OBJ)

$(JA): $(JAOBJ)
	gcc -o $@ $(JAOBJ)
	echo `pwd`/include > $(HOME)/.ja
	echo `pwd`/lib >> $(HOME)/.ja

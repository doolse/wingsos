GCC=gcc
BTOOLS := btools/
BO := $(BTOOLS)obj/

JA=$(BTOOLS)ja
JL65=$(BTOOLS)jl65
AR65=$(BTOOLS)ar65
DATA65=$(BTOOLS)data65
FILE65=$(BTOOLS)file65
MKIM=$(BTOOLS)mkimage
DEBCRASH=$(BTOOLS)debcrash

WJA=$(BPD)ja
WJL65=$(BPD)jl65
WAR65=$(BPD)ar65
WDATA65=$(BPD)data65
WFILE65=$(BPD)file65
WMKIM=$(BPD)mkimage

NETDISK=$(BTOOLS)netdrive
JAOBJ = $(BO)asm.o $(BO)mne.o $(BO)target.o $(BO)parse.o $(BO)getopt.o
FOBJ = $(BO)file65.o $(BO)getopt.o
JLOBJ = $(BO)jl65.o $(BO)getopt.o
AROBJ = $(BO)ar65.o $(BO)getopt.o
D6OBJ = $(BO)data65.o $(BO)getopt.o
DCOBJ = $(BO)debcrash.o $(BO)getopt.o
MKOBJ = $(BO)mkimage.o $(BO)getopt.o
NDOBJ = $(BO)netdrive.o

WJAOBJ = $(O)asm.o65 $(O)mne.o65 $(O)target.o65 $(O)parse.o65
WFOBJ = $(O)file65.o65
WJLOBJ = $(O)jl65.o65
WAROBJ = $(O)ar65.o65
WD6OBJ = $(O)data65.o65
#WMKOBJ = $(O)mkimage.o65

BINDIR = $(HOME)/bin
VPATH += :$(BTOOLS)
BINTOOLS := $(JA) $(JL65) $(AR65) $(FILE65) $(DATA65) $(NETDISK) $(MKIM) $(DEBCRASH)
DEVELPRG := $(WJA) $(WJL65) $(WAR65) $(WFILE65) $(WDATA65)
ALLOBJ += $(DEVELPRG)

$(DEVELPRG): $OCRT.flag $(BDIRS)

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

$(DEBCRASH): $(DCOBJ)
	gcc -o $@ $(DCOBJ)

$(JA): $(JAOBJ)
	gcc -o $@ $(JAOBJ)
	echo `pwd`/include > $(HOME)/.ja
	echo `pwd`/lib >> $(HOME)/.ja

$(WJAOBJ): $(BTOOLS)asm.h

$(WAR65): $(WAROBJ)
	lcc -o $@ -Wl-t1024 $(WAROBJ)

$(WFILE65): $(WFOBJ)
	lcc -o $@ -Wl-t1024 $(WFOBJ)

#$(WMKIM): $(WMKOBJ)
#	lcc -o $@ $<

$(WJL65): $(WJLOBJ)
	lcc -o $@ -Wl-t2048 $(WJLOBJ)

$(WDATA65): $(WD6OBJ)
	lcc -o $@ -Wl-t1024 $(WD6OBJ)

$(WJA): $(WJAOBJ)
	lcc -o $@ -Wl-t2048 $(WJAOBJ)

O:=obj/
E:=extras/
B:=bins/
BS:=bins/system/
BL:=bins/libs/
L:=$(BL)
BF:=$Bfonts/
BD:=$Bdrivers/
BG:=$Bgui/
BP:=$Bprograms/
BPG:=$(BP)graphics/
BPS:=$(BP)sound/
BPD:=$(BP)devel/
BPU:=$(BP)utils/
BPN:=$(BP)net/
BDIRS:=$OBINDIRS.flag
BRDIRS:= $(BS) $(BL) $(BF) $(BD) $(BG) $(BP) $(BPG) $(BPS) $(BPD) $(BPU) $(BPN)
BTARG:= $O% $B% $(BS)% $(BL)% $(BF)% $(BD)% $(BG)% $(BP)% $(BPG)% $(BPS)% $(BPD)% $(BPU)% $(BPN)%
BINDIR = $(HOME)/bin
INSTBINS:=$Bbooter $(BPU)gunzip $Owings.zip $Oinit
ALLOBJ = 
CFLAGS = -w

all: all2

include btools/Rules.mk
include drivers/Rules.mk
include kernel/Rules.mk
include programs/Rules.mk
include lib/src/Rules.mk
	
$(BDIRS):
	mkdir -p $(BRDIRS)
	touch $(BDIRS)
	
$O%.o65: %.a65 $(JA)
	$(JA) $(JAFLAGS) -o $@ $<
	
$O%.o65: %.c
	lcc $(CFLAGS) -c -o $@ $<

$(BTARG): %.c $OCRT.flag $(BDIRS)
	lcc $(CFLAGS) -o $@ $(filter %.c, $^) $(filter %.o65, $^)

$(BTARG): $Ebackgrounds/%
	cp $< $@ 

$(BTARG): $Efonts/%
	cp $< $@ 

$(BTARG): programs/scripts/%
	cp $< $@ 

$(BTARG): $O%.o65 $(JL65)
	$(JL65) -y -llibc -lcrt -G -p -o $@ $(filter %.o65, $^)	

all2: $(ALLOBJ) $Owings.zip $Ojos.d64

$Owings.zip: $(ALLOBJ)
	cd bins/ ; zip -r ../$Owings.zip *

$Oinit: $Oinitfirst.bin
	cp $Oinitfirst.bin $Oinit

$Ojos.d64: $(INSTBINS)
	rm -f $Ojos.d64
	cbmconvert -D8 $Ojos.d64 $^

run: all sendboot wait sendnet
run2: all sendboot wait sendtst
run3: all sendboot wait sendnull

sendkern:
	prmain --prload -r $Ojoskern
sendboot:
	prmain --prload -r $Bbooter
wait:
	sleep 3

	
sendnull:
	prmain --prrfile $Ojos.d64 2>/dev/null
	
sendnet:
	prmain --prrfile $Ojos.d64 </dev/ttyp4 >/dev/ttyp4

sendtst:
	prmain --prrfile $Ojos.d64 <extras/testfiles/coconut.mod

jam:	
	prmain --prload $Edebug/JAM
	prmain --prload -j 0801 $Edebug/doJAM

cleanall: clean
	rm -f $(BO)*.o*
	rm -f $(BINTOOLS)
	
clean:
	rm -f $O*.*
	rm -f $(OB)*.o*
	rm -rf $B
	rm -f `find . -name '*~'`
	rm -rf screenshots/*
	rm -f $(CRT)
	rm -f $Oinit
	

O:=obj/
E:=extras/
B:=bins/
BS:=bins/system/
BL:=bins/libs/
L:=$(BL)
BD:=$Bdrivers/
BG:=$Bgui/
BGF:=$(BG)fonts/
BP:=$Bprograms/
BPG:=$(BP)graphics/
BPS:=$(BP)sound/
BPD:=$(BP)devel/
BPU:=$(BP)utils/
BPN:=$(BP)net/
BDIRS:=$OBINDIRS.flag
BRDIRS:= $(BS) $(BL) $(BGF) $(BD) $(BG) $(BP) $(BPG) $(BPS) $(BPD) $(BPU) $(BPN)
BTARG:= $O% $B% $(BS)% $(BL)% $(BGF)% $(BD)% $(BG)% $(BP)% $(BPG)% $(BPS)% $(BPD)% $(BPU)% $(BPN)%
BINDIR = $(HOME)/bin
INSTBINS:=$Bbooter $(BPU)gunzip $Owings.zip
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

all2: $(ALLOBJ) $Owings.zip $Oinstall.d81 $Owings.d81

$Owings.zip: $(ALLOBJ)
	cd bins/ ; zip -r ../$Owings.zip * -x $(subst bins/,, $(INITRD))

$Oinstall.d81: $(INSTBINS) $Oinitfirst.bin $(MKIM)
	rm -f $@
	cp $Oinitfirst.bin $Oinit
	$(MKIM) -o $@ -vs $(INSTBINS) $Oinit
	rm $Oinit

$Owings.d81: $(ALLOBJ) $(MKIM)
	rm -f $@
	$(MKIM) -o $@ -s -v $Bbooter
	$(MKIM) -o $@ -i $@ -vs /home/jolz/c64/vn38outfit.zip
	$(MKIM) -o $@ -i $@ -v -d wings -r $B $B*

run: all sendboot wait sendnet
run2: all sendboot wait sendtst
run3: all sendboot wait sendnull
run4: all sendboot wait sendinst

sendkern:
	prmain --prload -r $Ojoskern
sendboot:
	prmain --prload -r $Bbooter
wait:
	sleep 3

	
sendinst:
	prmain --prrfile $Oinstall.d81 2>/dev/null

sendnull:
	prmain --prrfile $Owings.d81 2>/dev/null
	
sendnet:
	prmain --prrfile $Owings.d81 </dev/ttyp4 >/dev/ttyp4

sendtst:
	prmain --prrfile $Owings.d81 <extras/testfiles/coconut.mod

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
	

O:=obj/
L:=lib/
E:=extras/
BINDIR = $(HOME)/bin
ALLOBJ = 
CFLAGS = -w

all: all2

include btools/Rules.mk
include drivers/Rules.mk
include kernel/Rules.mk
include programs/Rules.mk
include lib/src/Rules.mk
	
$O%.o65: %.a65 $(JA)
	$(JA) $(JAFLAGS) -o $@ $<
	
$O%.o65: %.c
	lcc $(CFLAGS) -c -o $@ $<

$O%: %.c $LCRT
	lcc $(CFLAGS) -o $@ $(filter %.c, $^) $(filter %.o65, $^)

$O%: $O%.o65 $(JL65)
	$(JL65) -y -llibc -lcrt -G -p -o $@ $(filter %.o65, $^)	

all2: $(ALLOBJ) $Ojos.d64

D64FILES = $Obooter $(SYSPRG) $(CHARDRV) $(SCRIPTS) $(SHLIBS) $(NETPRG) $(NETDRV) $(FSYSDRV) $(MISCDRV) $(GUIPRG) $(SNDPRG) $(MISCPRG)

$Ojos.d64: $(D64FILES)
	rm -f $Ojos.d64
	zip -j $Oallzip.zip $(D64FILES) extras/fonts/* 
	cbmconvert -D8 $Ojos.d64 -n $(D64FILES)
	#mkisofs $(D64FILES) > $Ojos.d64

run: all sendboot wait sendnet
run2: all sendboot wait sendtst
run3: all sendboot wait sendnull

sendkern:
	prmain --prload -r $Ojoskern
sendboot:
	prmain --prload -r $Obooter
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
	rm -f $O*.o*
	rm -f $(OB)*.o*
	rm -f `find . -name '*~'`
	rm -f screenshots/*.prg
	rm -f lib/*.so lib/*.o65
	rm -f lib/src/libc/obj/*.o*
	rm -f $LCRT
	

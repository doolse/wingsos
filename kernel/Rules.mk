KERNOBJ := $Ojmpvector.o65 $Oboot.o65 $Oipc.o65 $Omemm.o65 $Oproc.o65 $Oswitch.o65 $Oloadproc.o65 $Oqueue.o65 $Okernio.o65
KERDIR := kernel/
ALLOBJ += $Ojoskern.c64 $Bbooter 
# $Binstaller
VPATH += :$(KERDIR)
INITRD = $Llibc.so $Lcrt.so $Lfsyslib.so $(BS)initp $(BD)iec.drv $(BD)cbmfsys.drv \
$(BD)xiec.drv $(BD)ide.drv $(BD)idefsys.drv $(BD)con.drv $Lraslib.so $Lserlib.so \
$(BS)automount $Efonts/font

INITRD2 = $Llibc.so $Lcrt.so $Lfsyslib.so $(BS)initp $(BD)iec.drv $(BD)cbmfsys.drv \
$(BD)ide.drv $(BD)idefsys.drv $(BD)con.drv $Lraslib.so $Lserlib.so \
$(BS)automount $Efonts/font

$Okernel.o65: $(KERNOBJ) $(JL65)
	$(JL65) -G -R -p -o $@ $(KERNOBJ)

$Oinitrd.a: $(INITRD) $(AR65)
	$(AR65) -p -o $Oinitrd.tmp $(INITRD)
	pucrunch -d -c0 $Oinitrd.tmp > $@

$Oinitrd2.a: $(INITRD2) $(AR65)
	$(AR65) -p -o $Oinitrd.tmp $(INITRD2)
	pucrunch -d -c0 $Oinitrd.tmp > $@

$Oinitrd.o65: $Oinitrd.a
$Oinitrd2.o65: $Oinitrd2.a

$Ojoskern.c64: $Okernel.o65 $Oinitrd.o65 $Oprserver.o65 $(KERDIR)normalboot.a65
	$(JA) -e -o $@ $(KERDIR)normalboot.a65

$Ojoskern2.c64: $Okernel.o65 $Oinitrd2.o65 $Oprserver.o65 $(KERDIR)instboot.a65
	$(JA) -e -o $@ $(KERDIR)instboot.a65

$Bbooter: $Odos.o65 $Ojoskern.c64
	cat $Odos.o65 $Ojoskern.c64 | pucrunch > $@

$Binstaller: $Odos.o65 $Ojoskern2.c64
	cat $Odos.o65 $Ojoskern2.c64 | pucrunch > $@

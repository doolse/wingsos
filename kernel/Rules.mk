KERNOBJ := $Ojmpvector.o65 $Oboot.o65 $Oipc.o65 $Omemm.o65 $Oproc.o65 $Oswitch.o65 $Oloadproc.o65 $Oqueue.o65 $Okernio.o65
KERDIR := kernel/
ALLOBJ += $Ojoskern $Obooter
VPATH += :$(KERDIR)
INITRD = $Llibc.so $Lcrt.so $Lfsyslib.so $Oinitp $Oiec.drv $Ocbmfsys.drv \
$Oxiec.drv $Oide.drv $Oidefsys.drv $Ocon.drv $Lraslib.so $Lserlib.so \
$Oautomount $Efonts/font

$Okernel.o65: $(KERNOBJ) $(JL65)
	$(JL65) -G -R -p -o $@ $(KERNOBJ)

$Oinitrd.a: $(INITRD) $(AR65)
	$(AR65) -p -o $Oinitrd.tmp $(INITRD)
	pucrunch -d -c0 $Oinitrd.tmp > $@

$Oinitrd.o65: $Oinitrd.a

$Ojoskern: $Okernel.o65 $Oinitrd.o65 $Oprserver.o65 $(KERDIR)loader.a65
	$(JA) -e -o $@ $(KERDIR)loader.a65

$Obooter: $Odos.o65 $Ojoskern
	cat $Odos.o65 $Ojoskern | pucrunch > $Obooter

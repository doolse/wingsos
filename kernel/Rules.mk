KERNOBJ := $Ojmpvector.o65 $Oboot.o65 $Oipc.o65 $Omemm.o65 $Oproc.o65 $Oswitch.o65 $Oloadproc.o65 $Oqueue.o65 $Okernio.o65
KERDIR := kernel/
ALLOBJ += $Ojoskern.prg $Bbooter 
VPATH += :$(KERDIR)
INITRD = $Llibc.so $Lcrt.so $Lfsyslib.so $(BS)initp $(BD)iec.drv $(BD)cbmfsys.drv \
$(BD)xiec.drv $(BD)ide.drv $(BD)idefsys.drv $(BD)con.drv $Lraslib.so $Lserlib.so \
$(BS)automount $Efonts/font

INITRD2 = $Llibc.so $Lcrt.so $Lfsyslib.so $(BS)initp $(BD)iec.drv $(BD)cbmfsys.drv \
$(BD)ide.drv $(BD)idefsys.drv $(BD)con.drv $Lraslib.so $Lserlib.so \
$(BS)automount $Efonts/font

$Okernel.o65: $(KERNOBJ) $(JL65)
	$(JL65) -G -R -p -o $@ $(KERNOBJ)

$Ojoskern.prg: $Okernel.o65 $(KERDIR)loader.a65 $Edebug/prserver.prg $(INITRD) $(AR65) $(DATA65)
	$(AR65) -p $(INITRD) -o file.blah
	pucrunch -d -c0 file.blah > file.blah2
	$(DATA65) -o $Oinitrd.o65 -st -a 0x20000 file.blah2
	$(DATA65) -o $Oprserver.o65 -st -c $Edebug/prserver.prg
	$(JA) -e -o $@ $(KERDIR)loader.a65

$Bbooter: $Odos.o65 $Ojoskern.prg
	cat $Odos.o65 $Ojoskern.prg | pucrunch > $@

VPATH += :$(PRGDIR)inetapps
NETPRG := $Ossh $Onetstat $Ohttpd $Oirc $Otelnet $Otelnetd $Oweb $Oajirc $Opoff $Olpr $Olpq $Olpc $Olprm $Odict $Ohtget $Omail $Oqsend
ALLOBJ += $(NETPRG)

$Onetstat $Otelnetd: CFLAGS += -lunilib
$Ossh: $Oassh.o65
$Ohttpd: CFLAGS += -lunilib -Wl-t768
$Oajirc: CFLAGS += -lwinlib -lfontlib
$Oirc: CFLAGS += -lconlib
$Ohtget: CFLAGS += -lunilib
$Omail: CFLAGS += -lunilib 
$Oqsend: CFLAGS += -lunilib

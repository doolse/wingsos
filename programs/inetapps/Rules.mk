NETDIR := $(PRGDIR)inetapps/
VPATH += :$(NETDIR)
NETPRG := $(BPN)ssh $(BPN)netstat $(BPN)httpd $(BPN)irc $(BPN)telnet $(BPN)telnetd $(BPN)web $(BPN)poff $(BPN)lpr $(BPN)lpq $(BPN)lpc $(BPN)lprm $(BPN)dict $(BPN)thes $(BPN)htget $(BPN)gethttp $(BPN)mail.app/start $(BPN)qsend.app/start $(BPN)ftp $(BPN)update $(BPN)ajirc
ALLOBJ += $(NETPRG)

$(BPN)%: $Etestfiles/%
	cp $< $@

include $(NETDIR)ftp/Rules.mk

$(BPN)mail.app:
	cp -R $(NETDIR)mail.app $@
	rm -rf $(BPN)mail.app/CVS

$(BPN)qsend.app:
	cp -R $(NETDIR)qsend.app $@
	rm -rf $(BPN)qsend.app/CVS

$(BPN)netstat $(BPN)telnetd: CFLAGS += -lunilib
$(BPN)ssh: $Oassh.o65
$(BPN)httpd: CFLAGS += -lunilib -Wl-t768
$(BPN)ajirc: CFLAGS += -lwinlib -lfontlib
$(BPN)irc: CFLAGS += -lconlib
$(BPN)htget: CFLAGS += -lunilib
$(BPN)mail.app/start: $(BPN)mail.app
$(BPN)mail.app/start: CFLAGS += -Wl-t2048 -lunilib -lconlib -lxmldom
$(BPN)qsend.app/start: $(BPN)qsend.app
$(BPN)qsend.app/start: CFLAGS += -Wl-t1024 -lunilib -lconlib -lxmldom

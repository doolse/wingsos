NETDIR := $(PRGDIR)inetapps/
VPATH += :$(NETDIR)
NETPRG := $(BPN)ssh $(BPN)netstat $(BPN)httpd $(BPN)irc $(BPN)telnet $(BPN)telnetd $(BPN)web $(BPN)ajirc $(BPN)poff $(BPN)lpr $(BPN)lpq $(BPN)lpc $(BPN)lprm $(BPN)dict $(BPN)thes $(BPN)htget $(BPN)gethttp $(BPN)mail $(BPN)qsend $(BPN)ftp
ALLOBJ += $(NETPRG)

include $(NETDIR)ftp/Rules.mk

$(BPN)netstat $(BPN)telnetd: CFLAGS += -lunilib
$(BPN)ssh: $Oassh.o65
$(BPN)httpd: CFLAGS += -lunilib -Wl-t768
$(BPN)ajirc: CFLAGS += -lwinlib -lfontlib
$(BPN)irc: CFLAGS += -lconlib
$(BPN)htget: CFLAGS += -lunilib
$(BPN)mail: CFLAGS += -lunilib 
$(BPN)qsend: CFLAGS += -lunilib

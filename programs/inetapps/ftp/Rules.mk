VPATH += :$(NETDIR)ftp

$(BPN)ftp: $Olocal.o65 $Ofile.o65 $Oother.o65 $Onet.o65
$(BPN)ftp: LDFLAGS += -lunilib

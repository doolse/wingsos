VPATH += :$(LIBDIR)utils
UTILOBJ += $OVec.o65 $Oiter.o65 $Oveciter.o65 $Oobj.o65 $Oexcept.o65

ALLOBJ += $Lwgsutil.so
SHLIBS += $Lwgsutil.so

$OVec.o65: Vec.c
	lcc $(CFLAGS) -pic -c -o $@ $<
	
$(UTILOBJ): include/wgs/util.i65
$Lwgsutil.so: $(UTILOBJ)
$Lwgsutil.so: LDFLAGS += -lcrt -llibc

$Lwgsutil.so: $(JL65) $(BDIRS)
	$(JL65) -s0x100 -y $(LDFLAGS) -o $@ $(filter %.o65, $^)

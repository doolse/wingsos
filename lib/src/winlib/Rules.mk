VPATH += :$(LIBDIR)winlib
WINOBJ += $Owinlib.o65 $OJCnt.o65 $OJBut.o65 $OJTxf.o65 $OJBmp.o65 \
$OJWnd.o65 $OJStx.o65 $OJFil.o65 $OJCard.o65 $OJBar.o65 $OJTxt.o65 $OJScr.o65 \
$OJMnu.o65 $OJView.o65 $OJChk.o65 $OJTre.o65 $OJCol.o65 $OJColV.o65 $OCJColV.o65 \
$OJIbt.o65 $OCJTab.o65 $OJTab.o65 $OCJForm.o65
#$OJFra.o65 $OJLst.o65 \
$OJFsl.o65 $OJIco.o65 $Omime.o65 \


ALLOBJ += $Lwinlib.so
SHLIBS += $Lwinlib.so

$OCJColV.o65: CJColV.c
	lcc $(CFLAGS) -pic -c -o $@ $<

$OCJTab.o65: CJTab.c
	lcc $(CFLAGS) -pic -c -o $@ $<

$OCJForm.o65: CJForm.c
	lcc $(CFLAGS) -pic -c -o $@ $<
	
$(WINOBJ): include/widget.i65
$Lwinlib.so: $(WINOBJ)
$Lwinlib.so: LDFLAGS += -lcrt -llibc -lfontlib -lxmldom
$Owinlib.o65: $(LIBDIR)winlib/Graphics.a65

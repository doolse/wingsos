VPATH += :$(LIBDIR)winlib
WINOBJ += $Owinlib.o65 $OJCnt.o65 $OJBut.o65 $OJTxf.o65 $OJBmp.o65 \
$OJWnd.o65 $OJStx.o65 $OJFil.o65 $OJCard.o65 $OJBar.o65 $OJTxt.o65 \
$OJScr.o65 $OJMnu.o65 $OJView.o65 $OJChk.o65  \
$OJIbt.o65 $OCJTab.o65 $OJTab.o65 $OCJForm.o65 \
$OJTree.o65 $OJTreeCol.o65 $OCJTreeCol.o65 \
$OJList.o65 $OJListCol.o65 
#$OCJListCol.o65 

#$OJFra.o65 $OJLst.o65 \
$OJFsl.o65 $OJIco.o65 $Omime.o65 \


ALLOBJ += $Lwinlib.so
SHLIBS += $Lwinlib.so

$OCJTreeCol.o65: CJTreeCol.c
	lcc $(CFLAGS) -pic -c -o $@ $<

$OCJTab.o65: CJTab.c
	lcc $(CFLAGS) -pic -c -o $@ $<

$OCJForm.o65: CJForm.c
	lcc $(CFLAGS) -pic -c -o $@ $<
	
$(WINOBJ): include/widget.i65
$Lwinlib.so: $(WINOBJ)
$Lwinlib.so: LDFLAGS += -lcrt -llibc -lfontlib -lxmldom
$Owinlib.o65: $(LIBDIR)winlib/Graphics.a65

VPATH += :$(LIBDIR)winlib
WINOBJ += $Owinlib.o65 $OJChk.o65 $OJTxf.o65 $OJBut.o65 $OJBmp.o65 \
$OJFra.o65 $OJWnd.o65 $OJStx.o65 $OJTxt.o65 $OJLst.o65 $OJCnt.o65 \
$OJMnu.o65 $OJFsl.o65 $OJIco.o65 $OJBar.o65 $OJScr.o65 $Omime.o65 \
$OJIbt.o65 $OJMan.o65 $OJTre.o65 $OJCol.o65

ALLOBJ += $Lwinlib.so
SHLIBS += $Lwinlib.so

$Lwinlib.so: $(WINOBJ)
$Lwinlib.so: LDFLAGS += -lcrt -llibc -lfontlib
$Owinlib.o65: $(LIBDIR)winlib/Graphics.a65

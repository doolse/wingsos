VPATH += :$(LIBDIR)winlib
WO := $(LIBDIR)winlib/obj/
CWINOBJ = $(WO)CJTab.o65 $(WO)CJForm.o65 $(WO)CJTModel.o65 $(WO)CJLModel.o65 $(WO)CJTree.o65 \
$(WO)CJCombo.o65 $(WO)CJTreeCol.o65

WINOBJ += $(WO)winlib.o65 $(WO)JCnt.o65 $(WO)JBut.o65 $(WO)JTxf.o65 $(WO)JBmp.o65 \
$(WO)JWnd.o65 $(WO)JStx.o65 $(WO)JFil.o65 $(WO)JCard.o65 $(WO)JBar.o65 $(WO)JTxt.o65 \
$(WO)JScr.o65 $(WO)JMnu.o65 $(WO)JView.o65 $(WO)JChk.o65 $(WO)JCombo.o65 \
$(WO)JIbt.o65 $(WO)JTab.o65 $(WO)JTree.o65 $(WO)JTreeCol.o65  \
$(WO)JTModel.o65 $(WO)JLModel.o65 $(WO)JPopup.o65 $(CWINOBJ)


#$OJFra.o65 $OJLst.o65 \
$OJFsl.o65 $OJIco.o65 $Omime.o65 \


ALLOBJ += $Lwinlib.so
SHLIBS += $Lwinlib.so

$(WO)%.o65: %.a65 $(JA)
	$(JA) -o $@ $<

$(WO)%.o65: %.c
	lcc $(CFLAGS) -pic -c -o $@ $<
	
$(WINOBJ): include/widget.i65
$Lwinlib.so: $(WINOBJ)
$Lwinlib.so: LDFLAGS += -lwgsutil -lcrt -llibc -lfontlib -lxmldom
$Owinlib.o65: $(LIBDIR)winlib/Graphics.a65

VPATH += :$(PRGDIR)sysapps
SYSPRG := $(BS)an $(BS)login $(BS)cat $(BS)cp $(BS)mv $(BS)mvp $(BS)rm $(BS)hexdump $(BS)kill $(BS)mem $(BS)more $(BS)sh $(BS)wc $(BS)connect $(BS)echo $(BS)initp $(BS)ls $(BS)ps $(BS)shot $(BS)term $(BS)automount $(BS)modcon $(BS)init $Oinitfirst.bin $(BS)clear $(BS)diskstat
ALLOBJ += $(SYSPRG)

$(BS)initp: $Oainitp.o65
$(BS)install $(BS)login: CFLAGS += -lunilib
$(BS)sh: CFLAGS += -pic -Wl-f0x02
$(BS)sh: $Oash.o65
$(BS)an: CFLAGS += -Wl-t0x400

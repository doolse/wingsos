VPATH += :$(PRGDIR)sysapps
SYSPRG := $Oan $Ologin $Ocat $Ocp $Omv $Omvp $Orm $Ohexdump $Okill $Omem $Omore $Osh $Owc $Oconnect $Oecho $Oinitp $Ols $Ops $Oreset $Oshot $Oterm $Oautomount $Oinstall
ALLOBJ += $(SYSPRG)

$Oinitp: $Oainitp.o65
$Oinstall $Ologin: CFLAGS += -lunilib
$Osh: CFLAGS += -pic -Wl-f0x02
$Osh: $Oash.o65

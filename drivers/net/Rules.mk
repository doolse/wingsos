VPATH += :$(DRVDIR)net
NETDRV := $(BD)tcpip.drv $(BD)ppp $(BD)slip $(BD)netdisk
ALLOBJ += $(NETDRV)

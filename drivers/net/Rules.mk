VPATH += :$(DRVDIR)net
NETDRV := $(BD)tcpip.drv $(BD)ppp $(BD)slip
ALLOBJ += $(NETDRV)

#!/bin/sh
# Boot wings.d81 in VICE's SuperCPU emulator.
#   -sidmodel 0          6581 SID (4 bit $d418 digis are near silent on the 8580)
#   -userportdevice 12   8 bit userport DAC, used by "digi.drv -u" in extras/vice/init
#   -soundbufsize 200    the 32ms default can freeze VICE on some PipeWire setups
cd "$(dirname "$0")" || exit 1
jam wings.d81 || exit 1
exec xscpu64 -sidmodel 0 -userportdevice 12 -soundbufsize 200 \
	-drive8type 1581 -drive8truedrive -autostart-handle-tde \
	-autostart wings.d81:booter "$@"

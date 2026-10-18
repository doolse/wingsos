#!/bin/sh
# Boot wings.d81 in VICE's SuperCPU emulator.
#   -sidmodel 0          6581 SID (4 bit $d418 digis are near silent on the 8580)
#   -userportdevice 12   8 bit userport DAC, used by "digi.drv -u" in extras/vice/init
#   -controlport1device 3  1351 mouse in port 1 (win.drv); Alt+M grabs/releases the mouse
#   -soundbufsize 200    the 32ms default can freeze VICE on some PipeWire setups
#   -acia1 ...           SwiftLink at $de00 (uart.drv, /dev/ser0), connected to a
#                        TCP listener at $WINGS_SERIAL (default 127.0.0.1:25232).
#                        VICE connects when DTR goes up and reconnects after the
#                        driver's reset, so the listener must accept repeatedly.
cd "$(dirname "$0")" || exit 1
jam wings.d81 || exit 1
exec xscpu64 -sidmodel 0 -userportdevice 12 -soundbufsize 200 -controlport1device 3 \
	-acia1 -acia1mode 1 -acia1base 0xde00 -acia1irq 1 -myaciadev 0 \
	-rsdev1 "${WINGS_SERIAL:-127.0.0.1:25232}" -rsdev1baud 38400 \
	-drive8type 1581 -drive8truedrive -autostart-handle-tde \
	-autostart wings.d81:booter "$@"

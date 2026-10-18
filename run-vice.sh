#!/bin/sh
# Boot wings.d81 in VICE's SuperCPU emulator.
#   -sidmodel 0          6581 SID (4 bit $d418 digis are near silent on the 8580)
#   -userportdevice 12   8 bit userport DAC, used by "digi.drv -u" in extras/vice/init
#   -controlport1device 3  1351 mouse in port 1 (win.drv); Alt+M grabs/releases the mouse
#   -soundbufsize 200    the 32ms default can freeze VICE on some PipeWire setups
#   -acia1 ...           Turbo232 at $de00 (uart.drv, /dev/ser0), connected to a
#                        TCP listener at $WINGS_SERIAL (default 127.0.0.1:25232).
#                        VICE connects when DTR goes up and reconnects after the
#                        driver's reset, so the listener must accept repeatedly.
#                        Turbo232 mode adds the 57600-230400 rates (ppp -s);
#                        netup uses 115200. At 230400 VICE drops received bytes.
#   --ide                IDE64 cartridge (IDEDOS ROM ~/.local/share/vice/ide64-scpu.crt)
#                        with wingside.hdd as the master drive; "ideup" in WiNGs
#                        mounts it as /wings/. Geometry must match MakeIdeImg.
#                        VICE's autostart gives up once IDEDOS hooks the KERNAL,
#                        so the booter is LOADed by typing into the keyboard
#                        buffer through the remote monitor (127.0.0.1:6510).
cd "$(dirname "$0")" || exit 1
jam wings.d81 || exit 1
if [ "$1" = --ide ]; then
	shift
	[ -f wingside.hdd ] || jam wingside.hdd || exit 1
	python3 - <<'EOF' &
import socket, time
# Wait for the monitor to listen without connecting: a connection drops
# VICE into the monitor.
def listening():
    with open("/proc/net/tcp") as f:
        return any(l.split()[1].endswith(":196E") and l.split()[3] == "0A"
                   for l in f.readlines()[1:])
for _ in range(60):
    if listening():
        break
    time.sleep(0.5)
else:
    raise SystemExit("run-vice.sh: no VICE remote monitor")
time.sleep(5)    # IDEDOS boot to READY.
s = socket.create_connection(("127.0.0.1", 6510))
s.settimeout(2)
# Wait for each command's prompt, or VICE stays in the monitor.
for cmd in (b'keybuf load"booter",8\\nrun\\n', b'x'):
    s.sendall(cmd + b'\n')
    try:
        s.recv(4096)
    except socket.timeout:
        pass
s.close()
EOF
	set -- -cartcrt "${IDE64_ROM:-$HOME/.local/share/vice/ide64-scpu.crt}" -IDE64version 0 \
		-IDE64image1 wingside.hdd +IDE64autosize1 -IDE64cyl1 64 -IDE64hds1 16 -IDE64sec1 63 \
		-remotemonitor -remotemonitoraddress ip4://127.0.0.1:6510 -8 wings.d81 "$@"
else
	set -- -autostart-handle-tde -autostart wings.d81:booter "$@"
fi
exec xscpu64 -sidmodel 0 -userportdevice 12 -soundbufsize 200 -controlport1device 3 \
	-acia1 -acia1mode 2 -acia1base 0xde00 -acia1irq 1 -myaciadev 0 \
	-rsdev1 "${WINGS_SERIAL:-127.0.0.1:25232}" -rsdev1baud 230400 \
	-drive8type 1581 -drive8truedrive "$@"

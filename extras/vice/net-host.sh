#!/bin/sh
# Host side of WiNGs networking under VICE: serial bridge, pppd and
# netdrive serving wingsnet.img. Start this before run-vice.sh, then
# type "netup" at the WiNGs shell. Needs /etc/ppp/peers/wings
# (see ppp-peers-wings) and membership of the dip group.
cd "$(dirname "$0")/../.." || exit 1
[ -f wingsnet.img ] || jam wingsnet.img || exit 1
trap 'kill 0' INT TERM EXIT
extras/vice/serial-bridge.py &
sleep 1
pppd "$XDG_RUNTIME_DIR/wings-serial" call wings passive &
btools/netdrive wingsnet.img &
wait

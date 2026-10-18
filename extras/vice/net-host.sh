#!/bin/sh
# Host side of WiNGs networking under VICE: serial bridge, pppd and
# netdrive serving wingsnet.img. Start this before run-vice.sh, then
# type "netup" at the WiNGs shell. Needs /etc/ppp/peers/wings
# (see ppp-peers-wings) and membership of the dip group.
#
# It also NATs the PPP subnet out through the host so WiNGs can reach
# the internet. That needs sudo; set NAT=0 to skip it.
cd "$(dirname "$0")/../.." || exit 1
[ -f wingsnet.img ] || jam wingsnet.img || exit 1

# Must match ppp-peers-wings and extras/vice/netup.
SUBNET=192.168.65.0/24
NAT=${NAT:-1}

# Each rule as "table chain match...", added with -I and removed with -D.
RULES="nat POSTROUTING -s $SUBNET ! -o ppp0 -j MASQUERADE
filter FORWARD -s $SUBNET -j ACCEPT
filter FORWARD -d $SUBNET -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT"

rules() {
	echo "$RULES" | while read -r table chain match; do
		# Drop any copies left by an earlier run that didn't clean up.
		while sudo iptables -t "$table" -D "$chain" $match 2>/dev/null; do :; done
		[ "$1" = add ] && sudo iptables -t "$table" -I "$chain" $match
	done
}

# Stop our own processes first, so they don't linger holding the ports
# if the sudo below fails or is interrupted.
PIDS=
cleanup() {
	trap - INT TERM EXIT
	[ -n "$PIDS" ] && kill $PIDS 2>/dev/null
	if [ "$NAT" = 1 ]; then
		rules del
		sudo sysctl -qw net.ipv4.ip_forward="$OLDFWD"
	fi
}

OLDFWD=$(cat /proc/sys/net/ipv4/ip_forward)
trap cleanup INT TERM EXIT
if [ "$NAT" = 1 ]; then
	sudo sysctl -qw net.ipv4.ip_forward=1 || exit 1
	rules add || exit 1
	echo "NAT on for $SUBNET (ip_forward was $OLDFWD)"
fi

# DNS server offered to WiNGs over IPCP. Use the host's real upstream
# server, not systemd-resolved's 127.0.0.53 stub, which WiNGs can't reach.
if [ -z "$DNS" ]; then
	DNS=$(awk '$1 == "nameserver" && $2 ~ /^[0-9.]+$/ && $2 !~ /^127\./ \
		{ print $2; exit }' /run/systemd/resolve/resolv.conf /etc/resolv.conf 2>/dev/null)
	DNS=${DNS:-1.1.1.1}
fi
echo "Offering DNS $DNS"

extras/vice/serial-bridge.py &
PIDS=$!
sleep 1
kill -0 $PIDS 2>/dev/null || { echo "serial-bridge.py failed to start" >&2; exit 1; }
# persist: pppd exits when VICE quits (the link hangs up); keep it waiting
# for the next VICE instead.
pppd "$XDG_RUNTIME_DIR/wings-serial" call wings passive persist maxfail 0 \
	holdoff 1 ms-dns "$DNS" &
PIDS="$PIDS $!"
btools/netdrive wingsnet.img &
PIDS="$PIDS $!"
wait

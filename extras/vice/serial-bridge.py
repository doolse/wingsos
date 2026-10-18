#!/usr/bin/env python3
"""Bridge VICE's emulated SwiftLink to a pty that pppd can use.

VICE connects to the TCP port when uart.drv raises DTR, drops the
connection on the driver's programmed reset and then connects again,
so a plain socat would exit. This keeps one pty open (symlinked to
LINK) and shuttles bytes to whichever connection is current.

usage: serial-bridge.py [port] [link]

The default link lives in $XDG_RUNTIME_DIR: pppd runs setuid root, and
protected_symlinks stops root following another user's link in /tmp.
"""
import os
import select
import signal
import socket
import sys
import termios
import tty

port = int(sys.argv[1]) if len(sys.argv) > 1 else 25232
link = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.environ.get("XDG_RUNTIME_DIR", os.path.expanduser("~")), "wings-serial")

master, slave = os.openpty()
tty.setraw(slave)
attrs = termios.tcgetattr(slave)
attrs[4] = attrs[5] = termios.B38400
termios.tcsetattr(slave, termios.TCSANOW, attrs)
slave_name = os.ttyname(slave)
if os.path.islink(link):
    os.unlink(link)
os.symlink(slave_name, link)

listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(4)
print(f"{link} -> {slave_name}, listening on 127.0.0.1:{port}", flush=True)

signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
conn = None
try:
    while True:
        fds = [listener, master] + ([conn] if conn else [])
        ready, _, _ = select.select(fds, [], [])
        if listener in ready:
            if conn:
                conn.close()
            conn, addr = listener.accept()
            print("VICE connected", flush=True)
        if conn and conn in ready:
            try:
                data = conn.recv(4096)
            except OSError:
                data = b""
            if data:
                os.write(master, data)
            else:
                print("VICE disconnected", flush=True)
                conn.close()
                conn = None
        if master in ready:
            try:
                data = os.read(master, 4096)
            except OSError:
                # nothing has the pty open (e.g. pppd not started yet)
                data = b""
            if data and conn:
                try:
                    conn.sendall(data)
                except OSError:
                    pass
except (KeyboardInterrupt, SystemExit):
    pass
finally:
    if os.path.islink(link):
        os.unlink(link)

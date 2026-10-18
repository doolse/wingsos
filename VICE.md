# Running WiNGs in VICE (SuperCPU)

How to build WiNGs from source on a modern Linux machine and run it in
VICE's SuperCPU emulator (`xscpu64`), including sound, the mouse, and
networking with a disk served from the host.

Tested on Ubuntu with GCC 15, VICE 3.9 and PipeWire. Everything here
lives on the `vice-supercpu` branch.

The examples assume this layout. Adjust paths if you put things elsewhere:

```
~/astrolabe/wingsos     this repo
~/astrolabe/lcc         lcc 4.1 with the 65816 backend
~/astrolabe/pucrunch    pucrunch source
```

## 1. Packages

```bash
sudo apt install build-essential jam vice ppp python3 git zip
```

You need `ppp` only for networking (section 8).

## 2. VICE ROMs

Debian and Ubuntu ship VICE without its ROMs (the `+dfsg` package). Copy
them from the official source release. Use the version that matches
`xscpu64 --version`:

```bash
V=3.9
cd /tmp
curl -L -o vice-$V.tar.gz "https://sourceforge.net/projects/vice-emu/files/releases/vice-$V.tar.gz/download"
tar xzf vice-$V.tar.gz vice-$V/data/C64 vice-$V/data/SCPU64 vice-$V/data/DRIVES vice-$V/data/PRINTER
mkdir -p ~/.local/share/vice
cp -r vice-$V/data/{C64,SCPU64,DRIVES,PRINTER} ~/.local/share/vice/
```

To check it worked, run `xscpu64`. It should boot to `16M RAM SYSTEM`. The
log will complain about missing CMD 2000/4000/CMDHD ROMs, which you
can ignore.

Make sure `~/.local/bin` is on your `PATH`; the next two steps install there.

## 3. pucrunch

```bash
cd ~/astrolabe
git clone https://github.com/mist64/pucrunch.git
cd pucrunch && make
install -m755 pucrunch ~/.local/bin/
```

## 4. LCC 65816 compiler

The backend is an overlay on stock lcc **4.1** (not 4.2). The overlay
replaces `src/mips.md` with the 65816 backend and patches a few other files.

```bash
cd ~/astrolabe
git clone https://github.com/drh/lcc.git lcc-upstream
git clone https://github.com/lcc-816/lcc-wings.git
mkdir lcc
git -C lcc-upstream archive v4_1 | tar x -C lcc
cp -r lcc-wings/{makefile,etc,lburg,src} lcc/

cd lcc && mkdir -p output
make BUILDDIR=$HOME/astrolabe/lcc/output CC=gcc LD=gcc CFLAGS="-g -std=gnu89 -w" cpp rcc

cd etc
make CC="gcc -std=gnu89 -w" \
     LCCDIR=$HOME/astrolabe/lcc/output/ \
     WGSDIR=$HOME/astrolabe/wingsos \
     BINDIR=$HOME/.local/bin \
     scpu install
```

The trailing slash on `LCCDIR` matters. The driver has `WGSDIR` compiled
in: it uses `include/c`, `lib/` and `btools/` from there. If you move the
repo, rebuild the driver.

`-std=gnu89` is needed because GCC 15 defaults to C23, which rejects the
K&R-style code.

## 5. Assembler config (`~/.ja`)

`ja` and `jl65` read the system include dir and the library dir from
`~/.ja`. The Jam build doesn't create it, so create it yourself:

```bash
printf '%s\n%s\n' "$HOME/astrolabe/wingsos/include" "$HOME/astrolabe/wingsos/lib" > ~/.ja
```

## 6. Build WiNGs

Use Jam. The Makefiles are stale.

```bash
cd ~/astrolabe/wingsos
jam                 # everything, about 500 targets
jam wings.d81       # flat 1581 boot disk for VICE
jam wingsnet.img    # 8MB CMD native image for the network disk (section 8)
```

Images:

| Target | What it is |
|---|---|
| `wings.d81` | Boot disk for VICE. Flat (1581 DOS has no subdirectories), so `.app` bundles are left out. Uses `extras/vice/init` instead of the normal init. |
| `wingsnet.img` | Full system at the image root, served over PPP and mounted as `/wings/`. Needed for the GUI. |
| `wings.img` | The original CMD HD partition image for real hardware. Not usable in stock VICE. |

## 7. Run

```bash
./run-vice.sh
```

This rebuilds `wings.d81` and boots it. Boot takes about 20 seconds to
reach the `J-Shell` prompt. See the script for the full set of VICE options.
The ones that matter:

- `-sidmodel 0`: emulate a 6581. VICE's SuperCPU default is the 8580,
  which makes WiNGs's 4-bit SID digi playback nearly silent.
- `-userportdevice 12`: 8-bit user port DAC. The VICE init starts
  `digi.drv -u` to use it.
- `-soundbufsize 200`: with VICE's 32ms default buffer, VICE froze on
  PipeWire (Jabra USB speaker). It affects every VICE machine, so you may want
  to set it in VICE's Settings → Audio too.
- `-autostart-handle-tde`: autostart with true drive emulation on.
- `-controlport1device 3`: 1351 mouse in port 1.
- `-acia1 ...`: SwiftLink at `$DE00` for networking (section 8). It's
  harmless if nothing is listening.

### Things to try

- Play a mod with `josmod DASMODEL.MOD` (if it can't find the file, `ls`
  shows the name as stored on disk).
- Press **Alt+M** to grab or release the mouse.
- The Commodore key is **Tab**, and C64 CTRL is **Left Ctrl**.

## 8. Networking and the network disk

The original setup used an X1541 cable with `prmain` on the PC. VICE can't
put an external program on the IEC bus. Instead, an emulated SwiftLink
connects to a TCP port on the host, and you run PPP over that:

```
WiNGs ppp ─ /dev/ser0 (uart.drv) ─ VICE SwiftLink ─ TCP 25232
   ─ serial-bridge.py ─ pty ─ pppd (192.168.65.1 ⇄ 192.168.65.2)
WiNGs netdisk ─ TCP 5000 over PPP ─ btools/netdrive wingsnet.img
```

### One-time setup

Install the pppd peer file. You need to be in the `dip` group to run
pppd without root:

```bash
sudo cp extras/vice/ppp-peers-wings /etc/ppp/peers/wings
groups | grep -q dip || sudo usermod -aG dip $USER   # then log out and back in
jam wingsnet.img
```

### Each session

In one terminal, start the host side. It leaves the bridge, pppd and
netdrive running, and Ctrl+C stops them all:

```bash
extras/vice/net-host.sh
```

In another terminal:

```bash
./run-vice.sh
```

Then at the WiNGs shell:

```
netup
gui
```

`netup` runs `ppp /dev/ser0`, connects `netdisk` to `192.168.65.1`, and
mounts the image as `/wings/`. From the host you can then `ping 192.168.65.2`.

Notes:

- Start `net-host.sh` first. VICE connects to the bridge during boot,
  when `uart.drv` loads.
- VICE drops and reopens the TCP connection when `uart.drv` resets the
  chip. `serial-bridge.py` handles that, which is why it's used rather
  than plain `socat`.
- The bridge puts its pty link in `$XDG_RUNTIME_DIR`, not `/tmp`.
  pppd is setuid root, and the kernel's `protected_symlinks` stops root
  following another user's symlink in `/tmp`.
- `net-host.sh` only builds `wingsnet.img` if it's missing, so files
  written from WiNGs survive. Run `jam wingsnet.img` after rebuilding to
  refresh it (this discards those changes).
- `net-host.sh` also lets WiNGs reach the internet. It turns on
  `ip_forward` and adds iptables NAT/forward rules for the PPP subnet,
  then puts `ip_forward` back and removes the rules on exit. This uses
  `sudo`, so run it from a terminal where you can type your password, or
  set `NAT=0` to skip it.
- pppd offers WiNGs a DNS server (`ms-dns`), and WiNGs's `ppp` prints
  `DNS IP is ...` when it accepts it. The default is the host's upstream
  server from `/run/systemd/resolve/resolv.conf`, falling back to
  `1.1.1.1`. Set `DNS=` to override it.
- Only one VICE can use the serial port at a time.
- The PPP subnet is `192.168.65.x`. It must not overlap your LAN: the
  original cable setup used `192.168.0.1`/`.2`, and when that's your
  router's address, bringing up PPP breaks DNS and routing to it. To change
  it, edit `ppp-peers-wings` (and reinstall it) and `extras/vice/netup`.

## Changes made for VICE

These are all on the `vice-supercpu` branch:

- `Jamrules`: host tools are built with `-std=gnu89`.
- `programs/sysapps/initp.c`: uses the 40-column `con.drv`.
  `con80.drv` isn't in the initrd, so the console never came up (black
  screen after "All DOS's loaded!").
- `drivers/block/iec.a65`: RAMLink probe disabled (`RAMLINK = 0`). It
  tests bit 7 of `$D200`, which VICE returns as `$FF`. The bogus RAMLink
  took a device number from open bus and could clash with the boot drive,
  giving intermittent "IEC: Bad Read" / "Error loading init!".
- `drivers/misc/win.a65`: selects the paddle lines (`CIA1` PA6/PA7) right
  before reading the 1351. VICE returns the POT value for whichever port
  is selected at the moment of the read, so a stray `$FF` from the keyboard
  scan upset WiNGs's range learning and made the pointer jump.
- `btools/netdrive.c`: fixed for 64-bit hosts (its `uint32` was 8
  bytes) and now accepts reconnects.
- New: `run-vice.sh`, the `wings.d81` and `wingsnet.img` targets, and
  `extras/vice/` (`init`, `netup`, `net-host.sh`, `serial-bridge.py`,
  `ppp-peers-wings`).

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Black screen after "All DOS's loaded!" | An old build with `con80.drv` in `initp.c`. Rebuild from this branch. |
| "Error loading init!" after "IEC: Bad Read" | The RAMLink probe. Make sure `iec.a65` has `RAMLINK = 0` and rebuild. |
| VICE window freezes with sound on | Use `-soundbufsize 200`. |
| No sound from `josmod` | Use `-sidmodel 0` (6581), or the user port DAC (`-userportdevice 12` with `digi.drv -u`). |
| `ja` can't find includes | `~/.ja` is missing (section 5). |
| `pppd: Couldn't stat ...: Permission denied` | The pty link is in `/tmp`. Use the default link from `serial-bridge.py` (`$XDG_RUNTIME_DIR`). |
| `ppp` times out in WiNGs | `net-host.sh` wasn't running when VICE booted, or another VICE holds the port. |
| Mouse pointer jumps around | An old `win.drv`. Rebuild, then run `jam wingsnet.img` (the GUI loads `win.drv` from the network disk). |
| GUI says "Couldn't load font 'bsw.font'" | Run `netup` first. The GUI needs `/wings/` from the network disk. |

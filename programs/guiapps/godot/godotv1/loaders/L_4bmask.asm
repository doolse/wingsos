.petscii
.include "godotlib.lib"
.ob "4BitMask,p,w"

; ------------------------------------------------------------ 
;
; ldr.4Bit&Mask
; Lader für 4Bit mit der Möglichkeit, Flächen per (Hires-) Maske
; auszusparen und damit zu schützen
;
;    1.04, 18.12.93, bugfixes
;    1.05
;    1.06,   9.1.94, added: RAM support
;    1.07,  11.1.99, added: Clips (not successfully)
;    1.08,  20.1.02, finished: Clips
;
; ------------------------------------------------------------ 

            .ba $c000
;
            .eq filetype	=$30
            .eq counter		=$31
            .eq gbyte		=$33
            .eq ymerk		=$34
            .eq adcnt		=$35
            .eq offx		=$36
            .eq offy		=$37
            .eq readvek		=$38
            .eq pfvek		=$3a
            .eq brmerk		=$3c
            .eq dst		=$3d
            .eq bbreite         =$3f
            .eq rrand		=$40
            .eq skip		=$41
            .eq mask		=$43
            .eq maskcnt		=$44
            .eq byte1		=$45
            .eq byte2		=$46
            .eq masksrc		=$47
            .eq masksrcw	=$49
;
            .eq colors		=$0400
;
            .eq y2		=$d005
            .eq vdc		=$d600
            .eq vdcdata		=$d601
;
; ----------------------------------------- Header
;
header      jmp start
            .by $80
            .by 4
            .by 1
            .wo modend
            .wo 0
            .tx "4Bit+BmMask+Clip"
            .tx "1.08"
            .tx "20.01.02"
            .tx "A.Dettke/W.Kling"
;
; ----------------------------------------- Leave Loader

cancel      lda #$ff
            sta used
cn2         sec
            rts
;
; ----------------------------------------- Main

jrequ       jmp requ
jinst       jmp install
jmerge      jmp merge
jerror      jmp error
;
start       ldx used		; build Requester?
            bmi jrequ		; yes
            beq cancel
            cpx #1		; read mask
            beq jinst
            cpx #3		; add another mask
            beq jmerge
            cpx #4
            bcs cancel
;
st0         ldx #0		; read 4bit
            stx breite
            stx breite+1
            stx bbreite
            stx bbreite+1
            stx brmerk
            stx filetype
            jsr tcopy
;            jsr setin2clip
;            lda masksrc
;            ldy masksrc+1
;            sta masksrcw
;            sty masksrcw+1
            jsr setfull
            ldy #0
            sty $d015

setsource   lda flag4		; Which source? (0=disk)
            pha
            beq ob3
            cmp #3
            bne ob31

ob32        sei			; 3=VDC2
            ldx #$12
            lda #$83
            jsr setvdc
            jsr clrlo
            cli
            ldx #<(readvdc)
            ldy #>(readvdc)
            bne ob4

ob31        lda ramtype		; 1=Temp / 2=Undo
            cmp #4		; REU?
            bcs ob30		; yes
            cmp #3		; Pagefox?
            bne ob32		; no, VDC

            lda #$80		; on Pagefox
            ldx #0
            sta pfvek+1
            stx pfvek
            lda #8
            sta pfbank
            ldx #<(readpfox)
            ldy #>(readpfox)
            bne ob4

ob30        cmp #7		; SuperRAM?
            beq ob3		; yes, but not yet implemented

            ldx #<(readreu)	; on REU
            ldy #>(readreu)
            bne ob4

ob3         ldx #<(readdisk)	; on disk
            ldy #>(readdisk)

ob4         stx readvek
            sty readvek+1
            pla			; from disk?
            beq ob7		; yes

            cmp #1		; from Temp?
            bne ob6
            jsr srt2		; yes
            bcc ld61

ob6         cmp #3		; from VDC?
            beq ld61
            jsr srt7		; no, Undo
            bcc ld61
; ----------------------------------------- 
ob7         jsr getname		; save filename
            jsr gd_xopen	; open file
            jsr onebyte		; packed/raw 4bit?
            beq ld64
ld63        jmp error
ld64        tay
            beq ld60
            jsr onebyte
            bne ld63
            jsr onebyte
            bne ld63
ld60        jsr onebyte
            bne ld63
            sta filetype
            tya
            bne ld61
            sta filetype	; set type
; ----------------------------------------- 
ld61        asl filetype
            lda filetype	; 4Bit Clip?
            and #2
            beq ob50

            jsr getfclip	; yes, get clip data from file
            lda clpbr
            sta brmerk
            jsr gbh2

ob50        lda wclip		; which clipmode?
            beq ld62

ob51        cmp #1		; 1: To Clip
            bne ob52
            jsr set2clip
            bne ld62

ob52        cmp #2		; 2: Into Clip
            bne ob53
            jsr setin2clip
            bne ld62

ob53        jsr setorigin	; 3: To Origin

ld62        jsr gd_clrms	; 0: To Full
            stx offy
            stx counter
            stx counter+1
            lda #7
            sta offx

            inx
            stx adcnt
            lda masksrc
            ldy masksrc+1
            sta masksrcw
            sty masksrcw+1
; ----------------------------------------- 
ld0         ldx breite		; set counters
            stx ls_vekta8
            ldy breite+1
            sty ls_vekta8+1
            ldx bbreite
            stx skip
            ldy bbreite+1
            sty skip+1
            ldx #0
            stx maskcnt

ld01        ldy #0		; get mask for current position
            lda maskcnt
            bne ld1
            lda #4
            sta maskcnt
            lda (masksrcw),y
            sta mask
            inc masksrcw
            bne ld1
            inc masksrcw+1

ld1         jsr readfile	; get byte from image
            ldx $90
            bne ld5

            tax
            lda skip		; must be skipped?
            ora skip+1
            beq ld02		; yes

            txa			; no, split byte into 2 pixels:
            and #$f0		; ****....
            sta byte1
            txa
            and #$0f		; ....****
            sta byte2

            asl mask		; left mask pixel set?
            bcs sk1		; yes, skip
            lda (sc_texttab),y	; no, replace 4bit pixel
            and #$0f
            ora byte1		; with pixel from file
            sta (sc_texttab),y
sk1         asl mask		; right mask pixel set?
            bcs sk2		; yes, skip
            lda (sc_texttab),y	; no, replace 4bit pixel
            and #$f0
            ora byte2		; with pixel from file
            sta (sc_texttab),y

sk2         inc sc_texttab
            bne sk22
            inc sc_texttab+1

sk22        dec maskcnt
            lda skip		; decrement skip counter
            bne ld11
            dec skip+1
ld11        dec skip

ld02        jsr cou6		; inc image pointer/ dec byte counter

            jsr action		; activity display

            lda $90
            bne ld5

sk4         lda ls_vekta8	; byte counter finished?
            ora ls_vekta8+1
            bne ld01

            clc			; yes, proceed to next blockline
            lda dst+1
            adc #5
            sta dst+1
            sta sc_texttab+1
            lda dst
            sta sc_texttab
            clc
            lda masksrc
            adc #<320
            sta masksrc
            sta masksrcw
            lda masksrc+1
            adc #>320
            sta masksrc+1
            sta masksrcw+1
            dec hoehe		; and decrement height
            bne jld0

ld3         lda flag4		; from disk?
            bne ld6		; no

            ldx which
            bne ld5
            jsr setinfo

ld5         jsr gd_xclose	; close files
            jsr gd_xmess

ld7         jsr gd_spron	; mouse pointer on
            ldx #20		; re-init activity display
            lda #32
ld8         sta mess,x
            dex
            bpl ld8
            jmp cancel		; leave

jld0        jmp ld0

ld6         jsr srt4		; reset REU record
            jsr vw2		; generate main screen
            jmp ld7
;
; ----------------------------------------- Requester

requ        lda list+1
            bne rq0
            ldx sc_screenvek	; 1st time: store address
            ldy sc_screenvek+1
            stx list
            sty list+1
rq0         inc used
            ldx #0
            stx $90
            ldx #<(fbitlst)	; display requester
            ldy #>(fbitlst)
            jsr gd_xmloop	; wait...
            jmp start		; then start over
;
; ----------------------------------------- Gauge

action      dec adcnt
            bne ld40
            lda cntwert
            sta adcnt
            ldy offy
            ldx offx
            lda filltab,x
            sta mess,y
            jsr messout
            dec offx
            bpl ld40
            inc offy
            lda #7
            sta offx
ld40        rts
;
tcopy       ldy #0
tc0         lda txt,x
            beq clrmess
            sta message,y
            inx
            iny
            bne tc0
;
clrmess     ldx #20
            lda #32
cl0         sta mess,x
            dex
            bpl cl0
            ldy #0
            ldx #7
            sty offy
            stx offx
            rts
;
messout     ldx #<(message)
            ldy #>(message)
            jmp gd_xtxout2
;
; ----------------------------------------- 

error       jsr ld3
            clc
            rts
;
; ----------------------------------------- 

onebyte     jsr basin
            ldx $90
            rts
;
; ----------------------------------------- 

filltab     .by 160,93,103,127,126,124,105,109
cntwert     .by 200
;
txt         .ts " 4Bit   @"
            .ts " Mask   @"
message     .ts " Reading  "
mess        .tx "                     "
            .by 0
;
; ----------------------------------------- Gauge when in RAM mode

gaugelst    .by 0,22,3,34,3,$02,0,0,$80

; ----------------------------------------- Screenlist

fbitlst     .by 0
            .by 2,10,20,23,$91,0,0
            .ts "Select Operation@"
            .by 4,10,10,3,$c8,<(sevinst),>(sevinst)
            .ts "Get Mask@"
from        .by 4,24,6,3,$cf,<(sevfrom),>(sevfrom)
fromtx      .ts "Disk@"
            .by 7,10,20,3,$ca,<(sevview),>(sevview)
            .ts "View Mask@"
            .by 10,10,20,3,$ca,<(sevinv),>(sevinv)
            .ts "Invert Mask@"
            .by 13,10,20,3,$ca,<(sevmerge),>(sevmerge)
            .ts "Merge another Mask@"
            .by 16,10,10,3,$c8,<(sevget),>(sevget)
            .ts "Get 4Bit@"
from4       .by 16,24,6,3,$cf,<(sev4from),>(sev4from)
fromtx4     .ts "Disk@"
clipgad     .by 19,19,11,3,$8f,<(evclip),>(evclip)
cliptxt     .ts " to Full @"
            .by 22,10,20,3,$c9,<(cn2),>(cn2)
            .ts "Leave@"

            .by $c0,4,19,4
            .ts "from@"
            .by $c0,16,19,4
            .ts "from@"
            .by $c0,19,12,4
            .ts "Load@"
            .by $80
;
; ----------------------------------------- 

fromwhich   .ts "ClipDiskTempUndoVDC2"
used        .by $ff
which       .by 0
fflag       .by $00
list        .wo $00
clpzl       .by 0
clpsp       .by 0
clpbr       .by 0
clpho       .by 0
mclip       .by 0,0,40,25
full        .by 0,0,40,25
breite      .wo 0
hoehe       .by 25
wclip       .by 0
wcliptx     .ts " to Full  to Clip into Clipto Origin"
clipoffs    .by 8,17,26,35
ramtype     .by 9
banks       .by $ff,$ff,0,0,3,7,$ff,$ff,1,0
bank        .by 7
pfbank      .by 8
flag4       .by $00
reuinit     .by <(gbyte),>(gbyte),0,$83,7,1,0
            .by 0,$c0,0,0,0,0,$10
;
; ----------------------------------------- Event: Merge another Mask

sevmerge    inc used
            jsr srt1
            stx flag4
;
; ----------------------------------------- Event: Get 4Bit

sevget      inc used
            lda flag4		; from REU?
            beq sss
            jmp gaugeout	; yes, gauge
sss         jmp sin1		; no, requester
;
; ----------------------------------------- Event: View Mask

sevview     lda #$1b		; hires on
            sta $d018
            lda #$3b
            sta $d011
            lda #$f0		; lt gray/ black
            ldy #250
            ldx #0
            stx sc_stop
vw0         sta colors-1,y
            sta colors+249,y
            sta colors+499,y
            sta colors+749,y
            dey
            bne vw0

vw1         lda sc_keyprs	; wait for keypress
            ora sc_stop
            beq vw1

            ldx #$13		; text on
            lda #$1b
            stx $d018
            sta $d011
vw3         dec used
vw2         ldx list		; display main screen
            ldy list+1
            jsr gd_screen
            sec
            rts
;
; ----------------------------------------- Event: Invert Mask

sevinv      lda #200
            jsr initad
            ldy #0
            lda #<($2000)
            ldx #>($2000)
            sta sc_vekt20
            stx sc_vekt20+1
            lda #<(8000)
            ldx #>(8000)
            sta ls_vekta8
            stx ls_vekta8+1
sin0        lda (sc_vekt20),y
            eor #$ff
            sta (sc_vekt20),y
            dec adcnt		; activity bar
            bne sin2
            lda #200
            sta adcnt
            inc y2
            inc y2+2
sin2        jsr count
            bne sin0
            jsr clearad
            clc
            rts
;
; ----------------------------------------- Counters

count       inc sc_vekt20
            bne cou5
            inc sc_vekt20+1
cou5        inc sc_texttab
            bne cou6
            inc sc_texttab+1
cou6        lda ls_vekta8
            bne cou7
            dec ls_vekta8+1
cou7        dec ls_vekta8
            lda ls_vekta8
            ora ls_vekta8+1
            rts
;
; ----------------------------------------- Event: Set Source of Mask

sevfrom     lda fflag
            bmi mrt0
            lda #$fe
            sta fflag
mrt0        inc fflag
            beq mrt1
            ldx #3
            .by $2c
mrt1        ldx #7
            ldy #3
mrt3        lda fromwhich,x
            sta fromtx,y
            dex
            dey
            bpl mrt3
            jsr gettab
            ldy #6
            jmp gd_xtxout1
;
; ----------------------------------------- 

gettab      lda #<(from)
            sta sc_screentab
            lda #>(from)
gettab2     sta sc_screentab+1
            lda #0
            tax
            tay
gt0         jsr gd_setpos
            cpx #3
            bne gt0
            stx sc_ho
            jsr gd_trim
            jmp gd_initmove
;
; ----------------------------------------- Event: Get Mask

sevinst     lda fflag
            beq sin1
            jmp instclip
sin1        inc used
            ldx #1
            jsr gd_xload
            jsr vw2
            jmp cancel
;
; ----------------------------------------- display REU Gauge

gaugeout    inc used
            lda sc_screenvek
            pha
            lda sc_screenvek+1
            pha
            ldx #<(gaugelst)
            ldy #>(gaugelst)
            jsr gd_screen
            pla
            sta sc_screenvek+1
            sta gr_redisp
            pla
            sta sc_screenvek
            jmp st0
;
; ----------------------------------------- Open Mask file

openmask    ldx #9
            jsr tcopy
            jsr gd_xopen
            ldx #0
            ldy #$20
            stx sc_texttab
            sty sc_texttab+1
            jsr onebyte
            beq in0
in1         jmp ld66
in0         jsr onebyte
            bne in1
            jsr gd_clrms
            stx offy
            lda #7
            sta offx
            inx
            stx adcnt
            lda #<(8000)
            ldx #>(8000)
            sta ls_vekta8
            stx ls_vekta8+1
            rts
;
; ----------------------------------------- Read Mask

install     jsr openmask
in2         ldy #0
            jsr basin
            sta (sc_texttab),y
            jsr action
            jsr cou5
            bne in2
            jmp ld5
;
; ----------------------------------------- Merge Mask

merge       jsr openmask
in3         ldy #0
            jsr basin
            ora (sc_texttab),y
            sta (sc_texttab),y
            jsr action
            jsr cou5
            bne in3
            jmp ld5
;
; ----------------------------------------- Read Error

ld66        pla
            pla
            jmp ld5
;
; ----------------------------------------- Generate Mask from Clip

instclip    jsr initad
            jsr clrmap
            lda #$20
            sta dst+1
            ldy #0
            sty dst
            lda sc_clipzl
            beq scp1
            sta sc_zl
scp4        clc
            lda dst
            adc #$40
            sta dst
            lda dst+1
            adc #1
            sta dst+1
            dec sc_zl
            bne scp4
scp1        lda sc_clipsp
            beq scp5
            sta sc_sp
scp8        clc
            lda dst
            adc #8
            sta dst
            bcc scp6
            inc dst+1
scp6        dec sc_sp
            bne scp8
scp5        ldx sc_clipho
            ldy #0
cc2         sty ls_vekta8+1
            clc
            lda sc_clipbr
            asl
            asl
            asl
            bcc cc3
            inc ls_vekta8+1
cc3         sta ls_vekta8
            lda dst
            pha
            lda dst+1
            pha
cc0         tya
            sta (dst),y
            inc dst
            bne cc1
            inc dst+1
cc1         jsr cou6
            bne cc0
            pla
            sta dst+1
            pla
            clc
            adc #$40
            sta dst
            lda dst+1
            adc #1
            sta dst+1
            dex
            bne cc2
            jsr sevinv
            jmp cancel
;
; ----------------------------------------- Clear Hires area (Mask)

clrmap      ldy #0
            sty sc_texttab
            lda #$20
            sta sc_texttab+1
            lda #<(8000)
            ldx #>(8000)
            sta ls_vekta8
            stx ls_vekta8+1
clm0        lda #$ff
            sta (sc_texttab),y
            jsr cou5
            bne clm0
            rts
;
; ----------------------------------------- Activity Bar

initad      sta adcnt
            ldy #60
            lda #0
adl0        sta $3fc3,y
            dey
            bpl adl0
            sty $3fc0
            sty $3fc1
            sty $3fc2
            lda #15
            sta $d029
            sta $d02a
            lda $d01d
            ora #12
            sta $d01d
            lda $d010
            ora #12
            sta $d010
            lda #8
            sta $d004
            lda #32
            sta $d006
            lda #146
            sta y2
            sta y2+2
            lda #$ff
            sta $07fa
            sta $07fb
            lda $d015
            ora #12
            sta $d015
            rts
;
; ----------------------------------------- 

clearad     lda $d015
            and #243
            sta $d015
            lda $d010
            and #243
            sta $d010
            rts
;
; ----------------------------------------- distributor

readfile    jmp (readvek)
;
; ----------------------------------------- Read from VDC

readvdc     sei
            ldx #$1f
            stx vdc
tv2         bit vdc
            bpl tv2
            lda vdcdata
            cli
            rts
;
clrlo       inx
            lda #0
setvdc      stx vdc
tv5         bit vdc
            bpl tv5
            sta vdcdata
            rts
;
; ----------------------------------------- Read from Pagefox

readpfox    sei
            sty ymerk
            inc 1
            ldy #0
            lda pfbank
            sta $de80
            lda (pfvek),y
            sta gbyte
            lda #$ff
            sta $de80
            inc pfvek
            bne pw0
            inc pfvek+1
pw0         lda pfvek+1
            cmp #$c0
            bne pw1
            lda pfbank
            cmp #10
            beq pw1
            lda #10
            sta pfbank
            lda #$80
            sta pfvek+1
pw1         dec 1
            cli
            ldy ymerk
            lda gbyte
            rts
;
; ----------------------------------------- Read from REU

readreu     lda #$91
            jsr gd_reu
            inc rm_werte+2
            bne rr0
            inc rm_werte+3
rr0         iny
            lda gbyte
            rts
;
; ----------------------------------------- Read from Disk

readdisk    lda counter
            ora counter+1
            bne getit2
            inc counter
            jsr basin
            cmp #$ad
            bne getit1
            bit filetype
            bvc getit1
            jsr basin
            sta counter
            beq rb2
            lda #0
            beq rb3
rb2         lda #1
rb3         sta counter+1
            jsr basin
getit1      sta gbyte
getit2      lda counter
            bne rb4
            dec counter+1
rb4         dec counter
            lda gbyte
            rts
;
; ----------------------------------------- Event: Determine which RAM

sev4from    lda ls_drive+6
            and #15
            sta ramtype
            tay
            lda banks,y
            sta bank
            cpy #9
            beq ok

next        lda flag4		; toggle gadget
            cmp #3
            bne srt0
            lda #$ff
            sta flag4
srt0        inc flag4
            beq srt1

            lda flag4
            cmp #1
            beq srt2
            cmp #3
            beq srt8

            lda rm_ramundo
            and #1
            beq next

srt7        ldx #$10		; Undo
            lda #0
            jsr srt3
            ldx #15
            bne text4
;
srt3        stx reuinit+3	; init REU
            sta reuinit+4
            ldx #6
            .by $2c
srt4        ldx #13
            ldy #6
srt5        lda reuinit,x
            sta rm_werte,y
            dex
            dey
            bpl srt5
ok          clc
            rts
;
srt2        lda rm_tmpsaved	; Temp
            and #1
next1       beq next
            ldx #$83
            lda bank
            jsr srt3
            ldx #11
            bne text4
;
srt1        jsr srt4		; Disk
            ldx #7

text4       ldy #3
srt6        lda fromwhich,x
            sta fromtx4,y
            dex
            dey
            bpl srt6
            lda #<(from4)
            sta sc_screentab
            lda #>(from4)
srt9        jsr gettab2
            ldy #6
            jmp gd_xtxout1
;
srt8        lda rm_tmpsaved	; VDC2
            and #2
            beq next1
            ldx #19
            bne text4
;
; ----------------------------------------- Image Info

getname     ldx #0
si0         lda ls_lastname,x
            beq si1
            sta nbuf,x
            inx
            cpx #16
            bcc si0
si1         rts
;
getdatac    ldx #4
            .by $2c
getdatag    ldx #9
            ldy #4
sinfo0      lda dtextc,x
            sta datatype,y
            dex
            dey
            bpl sinfo0
            rts
;
setinfo     jsr getdatag
            jsr setname
            jsr setloader
            jsr setmode
            jmp setdata
;
setname     lda #0
            ldx #<(ls_picname)
            ldy #>(ls_picname)
            bne si3
setloader   lda #17
            ldx #<(ls_iloader)
            ldy #>(ls_iloader)
            bne si3
setmode     lda #25
            ldx #<(ls_imode)
            ldy #>(ls_imode)
            bne si3
setdata     lda #33
            ldx #<(ls_idrive)
            ldy #>(ls_idrive)
si3         stx dst
            sty dst+1
            tax
            ldy #0
si4         lda nbuf,x
            beq si5
            sta (dst),y
            inx
            iny
            bne si4
si5         rts
;
nbuf        .by 32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,0
            .ts "4BitGod@"
modetx      .ts "320x200@"
datatype    .ts "Color@"
ttext       .ts "Text   "
htext       .ts "320x200"
mtext       .ts "160x200"
dtextc      .ts "Color"
dtextg      .ts "Gray "

; ---------------------------------------- Event: Clip Settings

evclip      lda wclip
            cmp #3
            bne ecl0
            lda #$ff
            sta wclip
ecl0        inc wclip

            ldx wclip
            lda clipoffs,x
            tay
            ldx #8
ecl1        lda wcliptx,y
            sta cliptxt,x
            dey
            dex
            bpl ecl1
            lda #<clipgad
            sta sc_screentab
            lda #>clipgad
            jmp srt9

; ----------------------------------------- Read Clip data from File

jerror2	    jmp error
getfclip    jsr onebyte		; fetch Clip row
            bne jerror2		; on error: exit
            sta clpzl
            jsr onebyte		; fetch Clip column
            bne jerror2
            sta clpsp
            jsr onebyte		; get width of Clip
            bne jerror2
            sta clpbr
            jsr onebyte		; get height of Clip
            bne jerror2
            sta clpho
            rts

; ----------------------------------------- set Full

setfull	    lda filetype	; true Clip?
	    ldx full+2
	    and #2
	    beq gm9		; no
	    ldx brmerk		; else: save width
gm9  	    ldy #3
gm0         lda full,y
            sta mclip,y
            sta clpzl,y
            dey
            bpl gm0
	    jsr setclip
	    lda full+3		; height 25
            sta hoehe
	    txa
	    sta rrand		; right border 40/ width of Clip
	    jmp gbh2		; Width and Skip

; ---------------------------------------- Start address of Clip

setclip     lda #$40		; $4000 (4bit)
            sta sc_texttab+1
            lda #$20		; $2000 (Mask)
            sta masksrc+1
            ldy #0
            sty sc_texttab 
            sty masksrc
stcl        lda mclip		; plus Clip row * 1280
            beq scp11
            sta clpzl
scp41       clc
            lda sc_texttab+1
            adc #5
            sta sc_texttab+1
            clc			; * 320 in Mask
            lda masksrc
            adc #<320
            sta masksrc
            lda masksrc+1
            adc #1
            sta masksrc+1
            dec clpzl
            bne scp41

scp11       lda mclip+1		; plus Clip column * 32
            beq scp51
            sta clpsp
scp61       clc
            lda sc_texttab
            adc #$20
            sta sc_texttab
            bcc scp7
            inc sc_texttab+1
scp7        lda masksrc		; * 8 in Mask
            adc #8
            sta masksrc
            bne scp71
            inc masksrc+1
scp71       dec clpsp
            bne scp61		; is Start address

scp51       lda sc_texttab
	    ldy sc_texttab+1
	    sta dst
	    sty dst+1
	    rts 

; ----------------------------------------- Set Cliptop/File

set2clip    ldy #3		; width/height from File
gm8	    lda clpzl,y
	    sta mclip,y
	    dey
	    bpl gm8
	    lda full+2		; right border to 40
	    sta rrand
            ldy #1
gm1         lda sc_lastclpzl,y	; row/column from Clip
            sta mclip,y
            dey
            bpl gm1
gm4         lda clpho		; height matches?
gm5         clc
            adc sc_lastclpzl
            cmp #25
            bcc gm7
            lda #25		; too high, delimit
            sbc sc_lastclpzl
gm7         ldx clpbr		; width from File
	    sta hoehe
	    jsr getbbr		; compute hangover
gm6         jsr getbr
            jmp setclip

; ----------------------------------------- Set Clip from GoDot

setin2clip  ldy #3
gm2         lda sc_lastclpzl,y
            sta mclip,y
            dey
            bpl gm2
	    clc			; right border = sc_clipsp+sc_clipbr
	    lda sc_lastclpzl+1
	    adc sc_lastclpzl+2
	    sta rrand
            lda sc_lastclpzl+3	; height from Clip 
            bne gm7

; ----------------------------------------- Set Clip from File

setorigin   ldy #3		; row and column from File
gm3         lda clpzl,y
            sta mclip,y
            dey
            bpl gm3
	    lda full+2		; right border = 40
	    sta rrand
	    lda sc_lastclpzl+1	; GoDot clip of no interest
	    pha
	    lda clpsp		; overwrite with clip from File
	    sta sc_lastclpzl+1
            lda clpho
            jsr gm7		; width and height from File
	    pla			; GoDot clip back
	    sta sc_lastclpzl+1
	    rts

; ----------------------------------------- fetch width from File

getbr       ldy #0
	    sty breite
	    sty breite+1
gbh0	    clc			; width * 32
            lda breite
            adc #$20
            sta breite
            bcc gbh1
            inc breite+1
gbh1        dex
            bne gbh0		; is counter for width
	    rts

; ----------------------------------------- true width of Clip

getbbr	    stx brmerk		; save width
	    txa
	    clc
	    adc sc_lastclpzl+1	; width plus Start too wide?
	    cmp rrand
	    bcc gbh2
	    lda rrand		; then delimit
	    sec
	    sbc sc_lastclpzl+1

gbh2	    tax			; times 32
	    jsr getbr
	    lda breite
	    sta bbreite		; and store
	    lda breite+1
	    sta bbreite+1

	    ldx brmerk		; width back again
	    rts

modend      .en

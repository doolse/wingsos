.petscii
.include "godotlib.lib"
; ------------------------------------ 
; mod.Lexmark.drv for GoDot Image Processing
; (c) 2001 by Arndt Dettke, Germany
; 
; 1.00: 19.05.01, first release
; 1.01: 24.05.01, changed default value of COLORFL from 3 to 2 ( = 3 planes instead of 4 planes)
; 1.02: 27.05.01, changed CMY values for BLU, BWN, PUR, ORA, and LBL
;                 added balancing capability for color printout
;                 re-inserted gadget for print quality, also its functionality 
;                 changed timeout routine to raster beam driven one
; 1.03: 27.05.01, bug: negative printout in color (fixed)
; 1.04: 28.05.01, bug: change of balancing facility routines
;                 withdrawal of new timeout routine
; 1.05: 29.05.01, bug: balancing made darker instead of brighter and vv (fixed)
; 
; ------------------------------------ 
            .ob "mlexdrv,p,w"
            .ba $c000
;
            .eq dst		=$30
            .eq dst0		=$32
            .eq cnt80		=$34
            .eq vertfak		=$35
            .eq vertcnt		=$36
            .eq byte		=$37
            .eq bitcnt		=$38
            .eq bcnt1		=$39
            .eq src		=$3a
            .eq src0		=$3c
            .eq adcnt		=$3e
            .eq hori		=$3f
            .eq kbuf		=$40
            .eq cbuf		=$42
            .eq mbuf		=$44
            .eq ybuf		=$46
            .eq dbl		=$48
            .eq oneline		=$49
            .eq index		=$4a
            .eq horifak		=$4b
            .eq fullbyte	=$4c
            .eq pixposx		=$4d
            .eq pixposy		=$4f

            .eq status		=$90
            .eq device		=$9a

            .eq lcnt8		=$fa
            .eq lcnt		=$fb
            .eq bcnt		=$fc

            .eq basebuf		=$ca00

; ------------------------------------ Module header
header      jmp start		; module entry vector
            .by $20		; loader signature
            .by 0		; "own requester" flag
            .by 0		; "dirty" flag
            .wo modend
            .wo 0		; reserved
            .tx "Lexmark Optra 40"
            .tx "1.05"
            .tx "29.05.01"
            .tx "Arndt Dettke    "

; ------------------------------------ Main
start       jsr makech		; create listbox character
            bit type		; look for current interface driver
            bvc st0
            jsr initpar		; $40 is Parallel Cable

st0         ldx #<(pflst)	; output requester to screen
            ldy #>(pflst)
            jsr gd_screen
            ldy #3		; colorize requester texts
st01        lda maingad,y
            sta sc_zl,y
            dey
            bpl st01
            ldx #2
            jsr gd_fcol

            lda dpiflag		; which image size?
            asl
            tay
            jsr setpixpos	; set print positions appropriately
            jsr gd_eloop	; wait for mouse events (pass control over to GoDot)
            sec			; leave module
            rts
; ------------------------------------ Render image to printer
jdone       jmp done		; leave on error
;
sevpfox     lda #0		; init STOP key and addresses 
            sta sc_stop
            sta vertcnt
            sta index
            sta src0
            sta src
            lda #5		; init activity display bar
            jsr initad
            ldx rasfl		; init vector to dither routine
            lda dlo,x
            sta dmode
            lda dhi,x
            sta dmode+1
            lda dpiflag		; get size of image to print
            jsr setvecs		; init vectors
            jsr initprt		; init printer
spf1        bne jdone		; error condition?
;
            ldx #>($4000)	; start from $4000
            stx src0+1
            stx src+1
            lda vertfak		; init zoom factors (to "double" at least)
            sta dbl
            lda horifak
            sta hori

            lda #25		; count 25 block lines
            sta lcnt8
lnloop8     lda #0		; count 8 raster lines per block
            sta lcnt
;
lnloop      jsr adinc		; activity display (move bar)

ads0        lda #>(basebuf)	; clear buffer (4 pages)
            sta kloop+2		; self modification! (sorry)
            lda #0
            sta cnt80
            ldx #4
            tay
kloop       sta basebuf,y
            iny
            bne kloop
            inc kloop+2
            dex
            bne kloop

zloop       lda fullbyte	; 2,4, or 6 relating to zoom factor
            sta bcnt1
byloop      lda #$c0		; pixel mask: %11000000
            sta bitcnt
btloop      ldy index
            lda (src),y		; retrieve left pixel
            pha
            lsr
            lsr
            lsr
            lsr
            tax
            jsr colorize	; get color (dither pattern in color)
            lsr bitcnt		; move mask: $00110000
            lsr bitcnt
            pla			; retrieve right pixel
            and #$0f
            tax
            jsr colorize	; get color (like before)
            dec index		; offset to current byte minus 1
            dec hori		; horizontal zoom (1,2, or 3)
            bne s3
            lda horifak		; restore zoom factor when 0
            sta hori
            inc index		; offset to byte plus 2 (one ahead)
s3          inc index		; offset to byte plus 1 (none ahead)
            lsr bitcnt		; move mask: $00001100
            lsr bitcnt
            bcc btloop		; get next two pixels

            inc cnt80		; count printer data bytes
            dec bcnt1		; dec zoom factor
            bne byloop		; loop if not finished

            lda #0		; restore index to 0
            sta index
            lda src		; proceed to next tile of image data
            clc
            adc #32
            sta src
            bcc s4
            inc src+1
s4          ldx cnt80		; line finished?
            cpx oneline
            bne zloop		; no, loop

            ldx sc_stop		; STOP pressed?
            bne done		; yes, break

            jsr moveline	; no, print line
;
            ldx vertcnt		; all lines of a tile?
            inx
            cpx #8
            bne s411
            ldx #0		; yes, restore to 0
s411        stx vertcnt
            dec dbl		; dec vertical zoom factor
            beq newline
            lda src0		; not finished, restore data vector
            sta src
            lda src0+1
            sta src+1
            jmp ads0		; and process the same line again

newline     lda vertfak		; restore vert zoom factor
            sta dbl
            lda src0		; proceed to next line in image tile
            clc
            adc #4
            sta src0
            sta src
            bcc s5
            inc src0+1
s5          lda src0+1
            sta src+1
            inc lcnt		; all 8 lines processed?
            lda #8
            cmp lcnt
            beq s51
            jmp lnloop		; no, loop

s51         lda src0		; yes, proceed to next block line
            clc
            adc #<(1248)
            sta src0
            sta src
            lda src0+1
            adc #>(1248)
            sta src0+1
            sta src+1
            dec lcnt8		; all 25 block lines processed?
            beq done
            jmp lnloop8		; no, loop
;
done        jsr endoff		; finish printer activity
toutdone    jsr clearad		; switch off activity display bar
            jsr clrch		; close channels to printer
            lda #4
            jsr close
            jsr gd_spron	; switch on mouse pointer
            jmp sevpix		; increment print position, finished printing

; ------------------------------------ Dither routines
makebyte    jmp (dmode)		; distributor vector
;
dithrst     ldy vertcnt		; dith type "Ordered"
            ora offs,y
            tay
            lda gr_orderedpat,y
            and bitcnt
            rts
;
dithpat     asl			; dith type "Pattern"
            asl
            asl
            ora vertcnt
            tay
            lda gr_pattern,y
            and bitcnt
            rts

; ------------------------------------ Output to printer
moveline    ldx oneline		; get # of bytes per line (depends on picture size)
            stx cnt80
            lda colorfl		; get flag for # of colorplanes
            sta colcnt
            beq col0		; 0 = black&white
            cmp #2		; 1 = CMYK
            bne col1
            lda cbuf		; 2 = CMY
            ldy cbuf+1
            bne col2

col0        inc trans0		; modify PCL command accordingly (set color flag)
col1        lda #<(basebuf)	; set output buffer address accordingly
            ldy #>(basebuf)
col2        sta dst
            sty dst+1
coll        ldy #0		; output PCL command "Write graphics pixels"
col3        lda trans,y
            jsr bsout
            iny
            cpy #7
            bne col3
            ldy #0		; add graphics data to output command
colloop     lda (dst),y
            jsr bsout
            inc dst
            bne wl6
            inc dst+1
wl6         dex			; line finished?
            bne colloop		; no, continue
            ldx cnt80		; yes, re-get # of bytes
            dec colcnt		; distinguish # of planes
            bmi col4		; last color? finish line
            bne coll		; no, print next color on same line
            inc trans0		; modify color flag, if COLCNT was 0
            bne coll
col4        dec trans0		; proceed to next line (modify PCL command)
            rts
; ------------------------------------ Create color dither
colorize    lda colorfl		; no colors?
            beq rasterize	; yes, dither only

cz0         ldy cyan,x		; C (cyan)
            jsr weighted	; care for balancing settings
            jsr makebyte	; dither
            ldy cnt80
            sta byte
            lda (cbuf),y
            ora byte
            sta (cbuf),y

            ldy magenta,x	; M (magenta)
            jsr weighted
            jsr makebyte	; dither
            ldy cnt80
            sta byte
            lda (mbuf),y
            ora byte
            sta (mbuf),y

            ldy yellow,x	; Y (yellow)
            jsr weighted
            jsr makebyte	; dither
            ldy cnt80
            sta byte
            lda (ybuf),y
            ora byte
            sta (ybuf),y
cz1         rts

; ------------------------------------ Create B&W dither
rasterize   lda gr_btab,x	; gray can be balanced 
            tax
            lda invtab,x
            jsr makebyte	; dither
            ldy cnt80
            sta byte
            lda (kbuf),y
            ora byte
            sta (kbuf),y
            rts
; ------------------------------------ Give Weight for Balancing
weighted    lda invtab,y	; get weighted balancing value
            tay
            lda gr_btab,y
            tay
            lda invtab,y
            rts
; ------------------------------------ Disconnect Parallel driver
paroff      ldx oldbsout
            beq p0
            ldy oldbsout+1
            stx $0326
            sty $0327
            ldx oldchkout
            ldy oldchkout+1
            stx $0320
            sty $0321
p0          rts
; ------------------------------------ Event: Choose printer interface driver
sevtype     jsr paroff		; set parallel driver off
            lda tflag
            cmp #2
            bne et0
            lda #$ff
            sta tflag
et0         inc tflag
            ldx tflag
            lda ptypes,x
            sta type
            lda ptoffs,x
            tax
            ldy #6
et1         lda ptext,x
            sta pgadt,y
            dex
            dey
            bpl et1
            iny
            jsr settext
            ldy #6
            jsr gd_xtxout1
            bit type		; which driver?
            bmi wies
            bvs cent
merl        lda #5		; Merlin (Xetec) has 5 as secondary address
            .by $2c
wies        lda #1		; Wiesemann has 1
            sta sek
            clc
            rts
cent        jmp initpar		; parallel port must be initialized differently

; ------------------------------------ Output gadget text routines
tabigad     .wo (pgad)		; pointers to texts
            .wo (dpigad)
            .wo (maingad)
            .wo (hpgad)
            .wo (qualgad)
            .wo (hpgad)
            .wo (hpgad)
            .wo (hpgad)
            .wo (rasgad)
            .wo (picnogad)
;
settext     lda #<(tabigad)
            sta sc_texttab
            lda #>(tabigad)
            sta sc_texttab+1
;
gettext     lda (sc_texttab),y
            sta sc_screentab
            iny
            lda (sc_texttab),y
            sta sc_screentab+1
            lda #0
            tax
            tay
gt0         jsr gd_setpos
            cpx #3
            bne gt0
            stx sc_ho
            jsr gd_trim
            jmp gd_initmove
; ------------------------------------ Event: Choose size of printout
sevdpi      lda dpiflag
            cmp #2
            bne dp0
            lda #$ff
            sta dpiflag
dp0         inc dpiflag
            jsr setpixmx
            ldx dpiflag
            lda gadoffs,x
            tax
            ldy #3
dpi1        lda dpitext,x
            sta dpitx,y
            lda dpl,x
            sta dpi,y
            dex
            dey
            bpl dpi1
            ldy #2
dpi2        jsr settext
            ldy #6
            jmp gd_xtxout1
; ------------------------------------ Event: Choose HP type
sevhp       lda hpflag		; ...and set base options accordingly
            cmp #1
            bne hp0
            lda #$ff
            sta hpflag
hp0         inc hpflag
hp3         ldx #0
            ldx hpflag
            lda hpoffs,x
            tax
            ldy #20
hp1         lda hptext,x
            sta hptx,y
            dex
            dey
            bpl hp1
            ldx hpflag
            lda hpplns,x
            sta planes
            and #15
            tax
            dex
            stx colorfl
            ldy #6
hp5         jmp dpi2
; ------------------------------------ Event: Choose print quality
sevqual     lda qualfl
            cmp #2
            bne qa0
            lda #$ff
            sta qualfl
qa0         inc qualfl
            lda qualfl
            tax
            ora #$30
            sta qual
            lda gadoffs,x
            tax
            ldy #3
qa1         lda qualtext,x
            sta qualtx,y
            dex
            dey
            bpl qa1
            ldy #8
            bne hp5
; ------------------------------------ Event: Choose print dither type
sevras      lda rasfl
            beq ra0
            lda #$ff
            sta rasfl
ra0         inc rasfl
            ldx rasfl
            lda ptoffs,x
            tax
            ldy #6
ra1         lda rastext,x
            sta rastx,y
            dex
            dey
            bpl ra1
            ldy #16
hp8         bne hp5
; ------------------------------------ Determine max # of images per sheet
setpixmx    lda dpiflag
            asl
            tay
            bne spx0
            lda #17		; 18 tiny
            bne spx1
spx0        cmp #2		; 3 norm
            beq spx1
spx2        lda #1		; 2 lrge
spx1        sta pixmx
            sta pixfl		; current # of image to print
            jsr setpixpos	; set position of first image now:

; ------------------------------------ Event: Choose # of image to print (set position)
sevpix      lda pixfl
            cmp pixmx
            bcc spx3
            lda #$ff
            sta pixfl
spx3        inc pixfl
            ldx pixfl
            lda dpiflag
            beq spx31
            ldy pixoffsx,x
            bne spx32
spx31       ldy pixoffsy,x
spx32       ldx #3
spx4        lda (pixposy),y
            sta capy,x
            dey
            dex
            bpl spx4
            ldx pixfl
            ldy pixoffsx,x
            ldx #3
spx41       lda (pixposx),y
            sta capx,x
            dey
            dex
            bpl spx41
            ldx pixfl
            inx
            txa
            jsr gd_xcnvdez
            sta picnotx+1
            txa
            and #15
            bne spx5
            ldx #32
spx5        stx picnotx
            ldy #18
            jmp dpi2
; ------------------------------------ Set image print position
setpixpos   ldx #0
spx10       lda pixx,y
            sta pixposx,x
            lda pixy,y
            sta pixposy,x
            iny
            inx
            cpx #2
            bne spx10
            rts
; ------------------------------------ Event: Leave requester
sevcan      jsr paroff
            sec
            rts
; ------------------------------------ Create listbox character 223
makech      ldx #7
mc0         lda char,x
            sta sc_undochar,x
            dex
            bpl mc0
            rts
; ------------------------------------ 
char        .by $00,$3a,$46,$4e,$40,$40,$3c,$00
invtab      .by 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
maingad     .by 8,6,14,7
; ------------------------------------ Main Screenlist 
pflst       .by 0
            .by 1,5,24,17,$81,0,0
            .ts " Lexmark Optra 40  @"
hpgad       .by 5,5,24,3,$ca,<(sevhp),>(sevhp),223
hptx        .ts "        Color        @"
dpigad      .by 7,23,6,3,$ce,<(sevdpi),>(sevdpi)
dpitx       .ts "norm@"
qualgad     .by 9,23,6,3,$ce,<(sevqual),>(sevqual)
qualtx      .ts "high@"
picnogad    .by 11,23,6,3,$ce,<(sevpix),>(sevpix),32,32
picnotx     .ts " 1@"
rasgad      .by 13,20,9,3,$ce,<(sevras),>(sevras)
rastx       .ts "Pattern@"
pgad        .by 15,5,10,3,$c7,<(sevtype),>(sevtype),223
pgadt       .ts "Centron@"
            .by 15,15,7,3,$cc,<(sevpfox),>(sevpfox)
            .ts "Print@"
            .by 15,22,7,3,$ce,<(sevcan),>(sevcan)
            .ts "Leave@"
            .by $c0,3,5,22
            .ts "Select Options & Print@"
            .by $c0,7,5,6
            .ts "Format@"
            .by $c0,9,5,13
            .ts "Print Quality@"
            .by $c0,11,5,8
            .ts "Position@"
            .by $c0,13,5,6
            .ts "Raster@"
            .by $80
; ------------------------------------ Variables, values, texts, and constants
ptypes      .by $40,$80,$00	; printer type flag values
type        .by $40		; selected printer type 
tflag       .by 0
sek         .by 1		; secondary printer address
dpioffs     .by 2,5,8,11
gadoffs     .by 3,7,11,15,19
dpiflag     .by 1		; image size
colcnt      .by 0		; number of color planes
ptoffs      .by 6,13,20
hpoffs      .by 20,41
hpplns      .tx "13"
options     .by 60
colorfl     .by 2
qualfl      .by 2
hpflag      .by 1
rasfl       .by 1
pixfl       .by 0
pixmx       .by 2
pixx        .wo (tinytextx),(normtextx),(lrgetextx)
pixy        .wo (tinytexty),(normtexty),(lrgetexty)
ptext       .ts "Centron"
            .ts " Wiesem"
            .ts " Merlin"
dpitext     .ts "tinynormlrge"
hptext      .ts "   Black and White   "
            .ts "        Color        "
qualtext    .ts "KPad lowhigh"
rastext     .ts "OrderedPattern"
dpl         .tx "064012801920"
tinytexty   .tx "010006001100160021002600"
tinytextx   .tx "015009401730"
normtexty   .tx "010011002100"
normtextx   .tx "056005600560"
lrgetexty   .tx "01001600"
lrgetextx   .tx "02400240"
;
offs        .by $00,$10,$20,$30
            .by $00,$10,$20,$30
;
dlo         .by <(dithrst)
            .by <(dithpat)
;
dhi         .by >(dithrst)
            .by >(dithpat)
;
dmode       .wo 0
; ------------------------------------ Activity display routines
adinc       dec adcnt
            bne adskip
            inc $d005
            inc $d007
            lda #5
            sta adcnt
adskip      rts
;
initad      sta adcnt
            ldy #63
            lda #0
adl0        sta $3fc0,y
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
;
            lda #146
            sta $d005
            sta $d007
            lda #$ff
            sta $07fa
            sta $07fb
            lda $d015
            ora #12
            sta $d015
            rts
;
clearad     lda #3
            sta $d015
            lda $d010
            and #$f3
            sta $d010
            rts
; ------------------------------------ Initialize printer
initprt     lda #4		; OPEN 4,4,sek for output
            tax
            ldy sek
            jsr filpar
            ldy #0
            sty $b7
            jsr copen
            ldx #4
            jsr ckout
            lda status
            bne enderr
            ldx #0		; send init string
ini0        lda initp,x
            jsr bsout
            inx
            cpx options		; # of bytes is variable
            bne ini0
            lda status		; not zero if error
enderr      rts
; ------------------------------------ "close printer" sequence
endoff      ldx #0
off0        lda endsequ,x
            jsr bsout
            inx
            cpx #10
            bne off0
            rts
; ------------------------------------ PCL3 printer commands 
endsequ     .by $1b,$2a,$72,$62,$43,$1b,$26,$6c,$30,$48
;
initp       .by $1b,$45,$1b,$26,$6c,$30,$6f,$30,$4c
            .by $1b,$2a,$72,$62,$63,$2d
planes      .by $33,$55,$1b,$2a,$74,$33,$30,$30,$52,$1b,$2a,$72
dpi         .by $31,$32,$38,$30,$53,$1b,$2a,$72
qual        .by $32,$51,$1b,$2a,$70
capy        .by $30,$31,$30,$30,$79
capx        .by $30,$35,$36,$30,$58,$1b,$2a,$62,$30,$4d,$1b,$2a,$72,$31,$41
; number of bytes to send on init: 60 (see label "options")
trans       .by $1b,$2a,$62
transw      .by $30,$38,$30
trans0      .by $56
; ------------------------------------ PCL3 commands used with GoDot
; if the trailing character is lower case you can add more commands of the same start sequence, so: 
; "1b 2a 72 62 43" is short for "1b 2a 72 42 1b 2a 72 43".
; 
; 1b 45 			: ESC E			: Reset printer
; 1b 26 6c 30 48		: ESC & l 0 H		: Eject page
; 1b 26 6c 30 4f		: ESC & l 0 O		: Set orientation to Portrait mode 
; 1b 26 6c 30 4c		: ESC & l 0 L		: Set Skip Perforation to off
; 1b 2a 62 31 42		: ESC * b 1 B		: Set gray balancing (to on) (n/a on Lexmarks)
; 1b 2a 62 30 4d		: ESC * b 0 M		: Set compression mode (to off)
; 1b 2a 62 36 34 30 56 ...	: ESC * b 640 V		: transfer 640 pixels to printer, planes 1-3, data following
; 1b 2a 62 36 34 30 57 ...	: ESC * b 640 W		: transfer 640 pixels to printer, plane 4 (only 3 planes on Lexmarks)
; 1b 2a 6f 32 44		: ESC * o 2 D		: Set depletion (to 25%) (n/a on Lexmarks)
; 1b 2a 6f 32 51		: ESC * o 2 Q		: Set shingling (to 50%) (n/a on Lexmarks)
; 1b 2a 70 34 4e		: ESC * p 4 N		: Set print mode to Smart Bidirectional (n/a on Lexmarks)
; 1b 2a 70 30 35 36 30 58	: ESC * p 0560 X	: Set horizontal position to 560 
; 1b 2a 70 30 31 30 30 59	: ESC * p 0100 Y	: Set vertical position to 100
; 1b 2a 72 32 51		: ESC * r 2 Q		: Set print quality (to high) (n/a on Lexmarks)
; 1b 2a 72 30 41		: ESC * r 0 A		: Start graphics (leftmost)
; 1b 2a 72 62 43		: ESC * r b C		: End raster graphics
; 1b 2a 72 2d 34 55		: ESC * r  -4 U		: Set # of planes to -4 (=CMYK; only -3 on Lexmarks)
; 1b 2a 72 36 34 30 53		: ESC * r 640 S		: Set width of image to print to 640 pixels
; 1b 2a 74 33 30 30 52		: ESC * t 300 R		: Set resolution to 300 dpi
;
; ------------------------------------ Initialize parallel port driver
initpar     ldx $0326		; set new vectors for BSOUT and CHKOUT
            ldy $0327
            stx oldbsout
            sty oldbsout+1
            ldx #<(nbsout)
            ldy #>(nbsout)
            stx $0326
            sty $0327
            ldx $0320
            ldy $0321
            stx oldchkout
            sty oldchkout+1
            ldx #<(nchkout)
            ldy #>(nchkout)
            stx $0320
            sty $0321
            clc
            rts
; ------------------------------------ New BSOUT routine ($ffd2)
nbsout      pha			; this routine only when parallel driver activated
            sta outmerk
            lda device		; output device: printer?
            cmp #4
            beq parprint	; yes, parallel then
            jmp $f1cd		; back to standard BSOUT
; ------------------------------------ parallel cable driver
parprint    sei			; no interrupt
            txa			; save registers
            pha
            tya
            pha

            lda #$ff		; set registers to %11111111
            tax
            tay
            sta $dd03		; userport to output
            lda $dd02		; set PIN A2 to output
            ora #4
            sta $dd02
            lda $dd00		; send attention to printer
            ora #4
            sta $dd00
            lda #$10
            sta $dd0d
            lda $dd0d		; clear acknowledge
            cli
            lda outmerk		; byte to send
            sta $dd01		; send to printer
            lda $dd00		; send ready
            and #$fb
            sta $dd00		; send attention
            ora #4
            sta $dd00

; ------------------------------------ delay routine

busy    lda $dd0d
        dex
        bne bsy0
        dey		; second wait?
        beq timeout	; yes, timed out
bsy0    and #$10
        beq busy

; ------------------------------------ 

bsy1        lda #0		; clear status flag
            sta status
            pla			; restore registers
            tay
            pla
            tax
            pla
            clc
parend      rts

; ------------------------------------ Timeout handler

timeout     ldx #<(parend)	; if timed out, set driver to NO OP
            ldy #>(parend)
            stx $0326
            sty $0327
            bne bsy1

; ------------------------------------ New CHKOUT routine ($ffc9)

nchkout     jsr $f30f
            beq ffound
            jmp $f701
ffound      jsr $f31f
            lda $ba		; output device: printer?
            cmp #4
            beq parout		; yes, parallel then
            jmp $f25b		; back to standard CHKOUT
parout      jmp $f275		; set # of output device

; ------------------------------------ 

outmerk     .by 0
oldbsout    .wo 0
oldchkout   .wo 0

; ------------------------------------ set printer driver vectors

setvecs     pha
            asl
            tax
            lda bufs,x
            sta kbuf
            lda bufs+1,x
            sta kbuf+1
            ldy #0
svc0        lda (kbuf),y
            sta cbuf,y
            iny
            cpy #6
            bne svc0
            lda #<(basebuf)
            ldy #>(basebuf)
            sta kbuf
            sty kbuf+1
            pla
            tax
            lda faktor,x
            sta fullbyte
            sta vertfak
            lsr
            sta horifak
            lda counters,x
            sta oneline
            lda dpioffs,x
            tax
            ldy #2
svc1        lda trwtext,x
            sta transw,y
            dex
            dey
            bpl svc1
            rts

; ------------------------------------ 

bufs        .wo (b0640),(b1280),(b1920)
;
b0640       .wo basebuf+80,basebuf+160,basebuf+240
b1280       .wo basebuf+160,basebuf+320,basebuf+480
b1920       .wo basebuf+240,basebuf+480,basebuf+720
faktor      .by 2,4,6
counters    .by 80,160,240
trwtext     .tx "080160240"
;
pixoffsy    .by 3,3,3,7,7,7,11,11,11,15,15,15,19,19,19,23,23,23
pixoffsx    .by 3,7,11,3,7,11,3,7,11,3,7,11,3,7,11,3,7,11

; ------------------------------------ color parameters for printout

; offsets:       0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15

cyan        .by 15,14,02,10,00,03,00,07,05,00,15,03,04,00,04,00
magenta     .by 15,14,04,10,15,12,03,07,01,05,01,03,00,00,00,00
yellow      .by 15,00,13,10,15,00,09,07,00,05,15,03,00,09,04,00

modend      .en


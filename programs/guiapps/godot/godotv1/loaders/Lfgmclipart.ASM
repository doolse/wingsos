.petscii
.ob "lfgmclipart,p,w"
.include "godotlib.lib"

.ba $c000
; ----------------------------------- 
; Loader FGM (incl. ClipArt)
;        1.00, 22.04.01: first release
;        1.01, 24.04.01: added failure routines and error messages
;        1.02, 27.04.01: added handling of wide images (>40 tiles)
; ----------------------------------- 

; ----------------------------------- Equates

            .eq width		=$30	; width in tiles
            .eq col0		=$31	; paper color
            .eq col1		=$32	; ink colors
            .eq col2		=$33
            .eq col3		=$34

            .eq cntrle		=$35
            .eq rleflg		=$36
            .eq dst		=$37	; /$38
            .eq dst0		=$39	; /$3a
            .eq adcnt		=$3b	; activity counter
            .eq btcnt		=$3c	; counts 4 bytes
            .eq tilecnt		=$3d	; counts WIDTH tiles
            .eq offx		=$3e
            .eq offy		=$3f
            .eq lcnt		=$40
            .eq lines		=$41
            .eq gbyte		=$42
            .eq gbyte2		=$43
            .eq merky		=$44
            .eq comp		=$45
            .eq blcnt		=$46
            .eq skip		=$47

            .eq status		=$90	; floppy status byte

; ----------------------------------- Header

godheader   jmp start
            .by $80
            .by 0
            .by 0
            .wo modend
            .wo 0
            .tx "FGM ClipArt Pix "
            .tx "1.02"
            .tx "27.04.01"
            .tx "A.Dettke        "

; ----------------------------------- 

jerror      jmp error
fail        jmp faila

start       jsr getcols		; set color assignments
            jsr getname		; save name of file
            
            jsr gd_xopen	; open file for input
            ldx #0		; Start at $4000
            ldy #$40
            stx cntrle		; clear RLE-counter
            stx status		; clear status byte
            stx skip		; clear right skip
            stx rleflg		; init RLE-Flag
            stx height+1	; clear hibyte of height
            stx dst		; set destination address
            stx dst0
            sty dst+1
            sty dst0+1
            inx 		; activity to 1: start right off
            stx adcnt

            jsr onebyte		; get first byte
            bne jerror
            sta comp		; if $00: uncompressed "s." file
            jsr onebyte		; get second byte
            bne jerror
            sta header+1

            lda comp		; uncompressed if $00
            ldy #200		; height is 200 then (25 rows)
            sty height
            ldy #40		; width is 40 then
            tax
            beq st2

            ldy #2
st1         jsr onebyte		; get header
            sta header,y
            bne jerror
            iny
            cpy #7		; 7 bytes header
            bne st1
            lda header+1	; is second byte zero?
            beq st3
            jmp fail		; no, fail

st3         ldy comp		; first byte is width
            tya
            cmp #41		; greater than 40?
            bcc st2             ; no
            sbc #40		; subtract 40 from width
            sta skip		; store value
            ldy #40

st2         sty header		; set width according to filetype
            sty width
            lda #8		; set # of lines per tile (8)
            sta lines
            lda height+1	; if height less than 8...
            bne ld6
            lda height
            cmp #9
            bcs ld6
            sta lines		; then already last line, set value!

ld6         jsr gd_clrms	; clear gauge
            jsr tcopy

            lda #25		; set counter for screen height
            sta blcnt
            lda #4		; preset 4bit-Byte counter to 4
            sta btcnt
;
ld0         lda width		; WIDTH tiles per row
            sta tilecnt

ld03        ldy #0		; must be 0 for 1 tile
            lda lines		; 8 lines per tile (except last row)
            sta lcnt

ld02        jsr getbyte		; get 1 byte
ld01        jsr cnvbyte		; convert to 4 bytes
            sta (dst),y		; store to 4bit memory
            sty merky
            jsr action
            ldy merky
            iny
            dec btcnt
            bne ld01		; no, loop

            lda #4		; counter to 4 again
            sta btcnt

            dec lcnt		; all lines of a tile?
            bne ld02

            clc
            lda dst		; inc vector to 4bit
            adc #32
            sta dst
            bcc ld10
            inc dst+1

ld10        dec tilecnt		; all tiles of a row?
            bne ld03

            lda skip		; more than 40 tiles wide?
            beq ld13		; no
            sta tilecnt

sk01        lda lines		; skip right part of image
            sta lcnt
sk00        jsr getbyte
            dec lcnt
            bne sk00
            dec tilecnt
            bne sk01

ld13        sec			; yes, subtract 8 from height
            lda height
            sbc #8
            sta height
            tax
            bcs ld11
            dec height+1

ld11        lda height+1	; last row?
            bne ld12		; no, continue
            txa			; get remainder from last subtraction
            beq ld3		; finished if zero
            cmp #9		; >8?
            bcs ld12		; no, not last row yet
            sta lines		; yes, don't fill last row completely 

ld12        clc			; add 1280 to start address for next row
            lda dst0+1
            adc #5
            sta dst0+1
            sta dst+1
            lda dst0
            sta dst

            dec blcnt		; for safety: bottom of screen?
            beq ld3		; yes, finish

            lda height+1	; end of file?
            bpl ld0		; no, loop
;
ld3         jsr setinfo		; set fileinfo
ld7         jsr gd_xclose	; close file
            jsr gd_spron	; pointer on
            jsr gd_xmess	; display floppy message
cancel      sec			; leave loader
            rts

; ----------------------------------- 

getbyte     lda comp		; compressed?
            beq uncomp		; no

            ldx cntrle		; just decompressing?
            beq next		; no, get next byte

            ldx rleflg		; record or not?
            beq uncomp		; uncompressed record if zero
            lda gbyte2		; get old data byte
            jmp getdata		; compressed record 

next        jsr basin		; get byte from file then
            ldx #0		; clear flag
            stx rleflg
            tax			; save byte to .X 
            stx cntrle		; set as counter
            bpl uncomp		; positive: uncompressed record

decomp      and #$7f		; negative: compressed record
            sta cntrle		; set byte as a counter
            dec rleflg		; and set flag (negative)

uncomp      jsr basin		; uncompressed image: get byte
            eor #$ff		; invert byte
getdata     sta gbyte		; and store 
            sta gbyte2
            dec cntrle		; count bytes
            rts

; ----------------------------------- 

cnvbyte     lda #0		; convert:
            asl gbyte		; fetch 2 pixels
            rol
            asl gbyte
            rol
            tax			; assign color and return value
            lda col0,x
            rts

; ----------------------------------- File Error

error       jsr ld7
            clc
            rts

; ----------------------------------- Get 1 Byte for Header

onebyte     jsr basin
            ldx status
            rts

; ----------------------------------- 

header      .by 0,0
height      .wo 200
            .by 0,0,0
cntwert     .by 200

; ----------------------------------- 

faila       lda #0
            pha
            jsr ld7
            pla
            tax
            ldy #0
fl0         lda err1,x
            sta message,y
            inx
            iny
            cpy #32
            bne fl0
            jsr gd_clrms
           
messout     ldx #<(message)
            ldy #>(message)
            jmp gd_xtxout2
;
action      dec adcnt
            bne ld4
            lda cntwert
            sta adcnt
            ldy offy
            ldx offx
            lda filltab,x
            sta mess,y
            jsr messout
            dec offx
            bpl ld4
            inc offy
            lda #7
            sta offx
ld4         rts
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
filltab     .by 160,93,103,127,126,124,105,109
message     .ts " FGMClip   "
mess        .ts "                     @"
txt         .ts " FGMClip   @"
err1        .ts " ERROR: Not a valid FGM image.  "

; ----------------------------------- Assign colors

getcols     lda #0		; %00: black
            sta col0
            lda gr_picked	; colorize, if not black
            and #$0f
            tax
            bne gc0
            ldx #1		; (white; a value 15 would be lgrey)
gc0         lda gdcols,x	; %11: comes from PICKED 
            sta col3
            tax			; %01: same as %11
            and #$0f
            sta col1
            txa			; %10: same as %01
            and #$f0
            sta col2
            rts

; ----------------------------------- 
;
gdcols      .by $00,$ff,$44,$cc,$55,$aa,$11,$dd
            .by $66,$22,$99,$33,$77,$ee,$88,$bb
;
; ----------------------------------- 

getname     ldx #0
si0         lda ls_lastname,x
            beq si1
            sta nbuf,x
            inx
            cpx #16
            bcc si0
si1         rts
;
; ----------------------------------- 

setinfo     jsr setname
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
si3         stx sc_texttab
            sty sc_texttab+1
            tax
            ldy #0
si4         lda nbuf,x
            beq si5
            sta (sc_texttab),y
            inx
            iny
            bne si4
si5         rts
;
nbuf        .by 32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,0
            .ts "FGMClip@"
modetx      .ts "320x200@"
datatype    .ts "Color@"

modend      .en

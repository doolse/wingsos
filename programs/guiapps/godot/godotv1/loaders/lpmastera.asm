.petscii
.ob "lprintm,p,w"
.include "godotlib.lib"

.ba $c000
; ----------------------------------- 
; Loader Printmaster/Printshop A & B
;        1.00, 22.04.01: first release
;        1.01, 23.04.01: added ERROR messages
; ----------------------------------- 

; ----------------------------------- Equates

            .eq width		=$30	; width in tiles
            .eq height		=$31
            .eq col0		=$32	; paper color
            .eq col1		=$33	; ink colors
            .eq col2		=$34
            .eq col3		=$35

            .eq dst		=$36	; /$37
            .eq dst0		=$38	; /$39
            .eq dst00		=$3a	; /$3b
            .eq adcnt		=$3c	; activity counter
            .eq btcnt		=$3d	; counts 4 bytes
            .eq tilecnt		=$3e	; counts WIDTH tiles
            .eq offx		=$3f
            .eq offy		=$40
            .eq lcnt		=$41
            .eq lines		=$42
            .eq merky		=$43
            .eq blcnt		=$44
            .eq gbyte		=$45
            .eq ftype		=$46

            .eq status		=$90	; floppy status byte

; ----------------------------------- Header

godheader   jmp start
            .by $80
            .by 0
            .by 0
            .wo modend
            .wo 0
            .tx "PMaster/PShopA&B"
            .tx "1.01"
            .tx "24.04.01"
            .tx "A.Dettke        "

; ----------------------------------- 

jerror      jmp error
fail        jmp faila

start       jsr getcols		; set color assignments
            jsr getname		; save name of file
            
            jsr gd_xopen	; open file for input
            ldx #0		; Start at $4000
            ldy #$40
            stx status
            stx dst		; set destination address
            stx dst0
            stx dst00
            sty dst+1
            sty dst0+1
            sty dst00+1
            inx 		; activity to 1: start right off
            stx adcnt

            ldy #52		; height is 52 pixels
            sty height
            ldy #11		; width is 11 tiles

            jsr onebyte		; get first byte
            bne jerror
            tax			; Printshop?
            beq ps0		; yes
            cmp	#$50 		; Signature "P"?
            beq pm0		; yes

ps0         jsr onebyte		; get second byte
            bne jerror
            cmp #$58		; Printshop A?
            beq pm1
            ldy #45		; Printshop B is 45 pixels high
            cmp #$58		; Printshop B?
            bne fail		; no, no valid file
            sty height
            ldy #6		; B is 5.5 tiles wide
            ldx #0

pm1         stx ftype		; Printshop is $00
            beq st1

pm0         jsr onebyte		; get second byte
            bne jerror
            cmp #$77		; signature "M"?
            bne fail
            sta ftype		; Printmaster is $77
            ldx #12

st1         sty width		; width either 11 or 6
            jsr tcopy

            lda ftype
            beq st3
            ldy #2
st2         jsr onebyte		; get header and skip
            bne jerror
            iny
            cpy #7		; 7 bytes header
            bne st2

st3         lda #8		; set # of lines per tile (8)
            sta lines

ld6         jsr gd_clrms	; clear gauge

            lda #4		; preset 4bit-Byte counter to 4
            sta btcnt
            lda #7		; height is 6.5 tiles
            sta blcnt
;
ld00        lda lines		; 8 lines per tile (except last row)
            sta lcnt

ld0         lda ftype
            beq ld03
            jsr basin		; skip Printmaster line header ($8b)

ld03        lda width		; WIDTH tiles per row (11)
            sta tilecnt

ld02        ldy #0		; must be 0 for 1 tile
            jsr basin		; get 1 byte
            eor #$ff		; invert it
            sta gbyte
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

            clc
            lda dst		; next tile
            adc #32
            sta dst
            bcc ld10
            inc dst+1

ld10        dec tilecnt		; all tiles of a row (11)?
            bne ld02

            clc			; next line
            lda dst0
            adc #4
            sta dst0
            sta dst
            bcc ld09
            inc dst0+1
ld09        lda dst0+1
            sta dst+1

            dec lcnt		; all lines of a row?
            bne ld0

            sec			; yes, subtract 8 from height
            lda height
            sbc #8
            sta height
            bcc ld3		; finished if negative
            cmp #6		; last tile?
            bcs ld12		; no, don't change LINES
            sta lines		; fill last row half

ld12        clc			; add 1280 to start address for next row
            lda dst00+1
            adc #5
            sta dst00+1
            sta dst0+1
            sta dst+1
            lda dst00
            sta dst0
            sta dst

            dec blcnt
            bne ld00
;
ld3         jsr setinfo		; set fileinfo
ld7         jsr gd_xclose	; close file
            jsr gd_spron	; pointer on
            jsr gd_xmess	; display floppy message
cancel      sec			; leave loader
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

cntwert     .by 15

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
message     .ts " PrMaster  "
mess        .tx "                     "
            .by 0

txt         .ts " PrShopAB  @"
            .ts " PrMaster  @"
err1        .ts "   ERROR: Not a PM/PS image.    "
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
            .ts "PMaster@"
modetx      .ts "320x200@"
datatype    .ts "Color@"

modend      .en

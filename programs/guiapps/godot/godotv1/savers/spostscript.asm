.petscii
.include "godotlib.lib"
.ob "seps,p,w"

; ----------------------------------------- 
; svr.EPS
;     Saver for Encapsulated PostScript (language level 2)
;
; 1.00: 23.6.01, first release
; 1.01: 27.6.01, added:   - option to choose area to be saved
;                         - option to choose landscape or portrait
;                changed: - requester
;                         - PostScript programming
; 1.02: 9.7.01,  bug:     - saved clip only when activated in main screen, fixed
;
; ----------------------------------------- 

            .ba $c000
;
            .eq footlen		=hexchars-ffooter
            .eq src		=$30
            .eq src0		=$32
            .eq adcnt		=$34
            .eq offy		=$35
            .eq offx		=$36
            .eq mlen		=$37
            .eq cpos		=$38
            .eq lcnt8		=$39
            .eq lcnt		=$3a
            .eq tilecnt		=$3b
            .eq cntchars	=$3c
            .eq clpzl		=$3d
            .eq clpsp		=$3e
            .eq clpbr		=$3f
            .eq clpho		=$40
            .eq indent          =$41
;
; ----------------------------------------- 

header      jmp start
            .by $40
            .by 0
            .by 1
            .wo modend
            .wo 0
            .tx "Encapsulated PS"
version     .tx "1.02"
            .tx "09.07.01"
            .tx "A.Dettke        "
;
; ----------------------------------------- 

start       ldx gr_nr
            dex
            bne st0
            jmp initinp
;
st0         jsr requester	; display requester
            lda used		; cancelled?
            cmp #3
            bne st11
            jsr canc		; yes, leave
            bcs st12

st11        jsr gd_sproff
            lda gd_modswitch
            pha
            lda ls_flen
            beq st1

st3         sta wflen
            jsr addeps		; add ".eps" to name (length plus 4)
            lda #1
            sta adcnt

            jsr wopen		; open write file
            bne st2		; error?

            jsr getname		; no, save image file name
            ldx #12		; OPEN 12,drv,12,"file,p,w"
            jsr ckout
            jsr gd_clrms	; clear gauge bar
            ldx #0		; Text: "Bitmap"
            jsr tcopy
            jsr wrheader
            jsr writeps
            jsr wrfooter

st2         jsr clrch		; CLOSE 12
            lda #12
            jsr close
            jsr canc
            jsr err9
            bne st1
            jsr setname		; publish image name to GoDot

st1         pla
            sta gd_modswitch
st12        jsr gd_spron
            sec
            rts

; ----------------------------------------- Input Routines

cursor      ldy sc_br
            dey
c0          lda ls_nambuf,y
            sta sc_movetab,y
            dey
            bpl c0
            jsr gd_cnvbc
            sta sc_loop
            jsr gd_xtxout3
            stx sc_loop
            jsr gd_initmove
            ldy cpos
            lda (sc_vekt20),y
            eor #$80
            sta (sc_vekt20),y
            rts
;
setcursor   dec ls_len
            jsr gd_position
            lda sc_merk                    ; zeile
            cmp sc_zl
            bne sc0
            sec
            lda sc_merk+1                  ; spalte
            sbc sc_sp
            cmp ls_len
            bcc sc1
            ldy ls_len
            cpy sc_br
            bcc sc2
            dey
sc2         tya
sc1         sta cpos
sc0         inc ls_len
            rts
;
initinp     lda ls_flen
            sta mlen
            ldy sc_br
            jsr gd_blank
            lda #0
            sta cpos
            sta ls_len
            ldy sc_br
            sta sc_movetab,y
            dey
ii0         lda (sc_vekt20),y
            ldx ls_len
            bne ii1
            cmp #32
            beq ii1
            sty ls_len
            inc ls_len
            inc ls_len
ii1         sta ls_nambuf,y
            sta buff2,y
            dey
            bpl ii0
            ldy ls_len
            bne ii3
            iny
            sty ls_len
ii3         jsr gd_cnvasc
            jsr setcursor
;
input       ldx #0
            stx sy_tbuffer
            jsr cursor
iloop       lda sy_tbuffer
            beq iloop
            ldx #9
in0         cmp sonder1,x
            beq in1
            dex
            bpl in0
            ldy sc_iflag
            beq in2
in3         cmp (ls_vekta8),y
            beq in1
            dey
            bpl in3
            bmi input
in2         cmp #32
            bcc input
            cmp #219
            bcs input
            ldx #5
in4         cmp sonder2,x
            beq in1
            dex
            bpl in4
            cmp #91
            bcc in1
            cmp #193
            bcc input
in1         cmp #147                    ; clr
            beq c2end
            cmp #19                     ; home
            beq c2strt
            cmp #148                    ; inst
            beq insert
            cmp #20                     ; del
            beq delete
            cmp #3                      ; stop
            beq jcancel
            cmp #13                     ; cr
            beq ready
            cmp #189                    ; c=x
            beq clear
            cmp #29                     ; right
            beq right
            cmp #157                    ; left
            beq left
            cmp #160                    ; sh sp
            beq setcur
            cmp #64
            bne in5
            lda #186
in5         jmp string
;
c2end       jmp sclr
c2strt      jmp shome
insert      jmp sinsert
delete      jmp sdelete
clear       jmp sclear
right       jmp sright
left        jmp sleft
setcur      jsr setcursor
            jmp input
jcancel     jmp cancel
;
ready       ldy sc_br
            jsr gd_blank
r01         ldy ls_len
            cpy sc_br
            bne r00
            inc ls_len
            bne r01
r00         dey
            dey
            bmi jcancel
            tya
            tax
r0          lda ls_nambuf,y
            sta sc_movetab,y
            dey
            bpl r0
r02         lda sc_movetab,x
            cmp #32
            bne r03
            dec ls_len
            dex
            bne r02
r03         jsr gd_cnvbc
            ldy mlen
            ldx sc_zl
            cpx #20
            bne r1
            ldx ls_dirmask
            beq r4
            ldx #11
            ldy #15
r5          lda ls_nambuf,x
            sta ls_nambuf,y
            dey
            dex
            bpl r5
r6          inx
            lda $073c,x
            sta ls_nambuf,x
            cpx #3
            bne r6
            clc
            ldy ls_len
            dey
            sty ls_flen2
            tya
            adc #5
            sta ls_len
r4          ldy #15
r3          lda sc_movetab,y
            sta ls_lastname,y
            dey
            bpl r3
            ldy ls_len
            dey
r1          sty ls_flen
            clc
            lda #8
            adc sy_soffx
            sta $d002
            sta $d000
            clc
            lda #114
            adc sy_soffy
            sta $d003
            tay
            dey
            dey
            sty $d001
            ldy #3
            sty $d010
r2          jsr gd_xtxout3
            clc
            rts
;
cancel      ldy sc_br
            jsr gd_blank
            ldy mlen
            beq cc1
            dey
cc0         lda buff2,y
            sta sc_movetab,y
            sta ls_nambuf,y
            dey
            bpl cc0
cc1         jsr gd_cnvasc
            ldy mlen
            sty ls_flen
            bcc r2
;
string      ldx inst
            bne over
            pha
            ldx sc_br
            ldy sc_br
            dey
str0        lda ls_nambuf-1,y
            sta ls_nambuf-1,x
            dex
            dey
            cpx cpos
            bne str0
            pla
            sta ls_nambuf,x
str1        inx
            cpx sc_br
            beq s0
            stx cpos
            ldx ls_len
            dex
            cpx sc_br
            beq s0
            inc ls_len
            bne s0
;
over        ldx cpos
            sta ls_nambuf,x
            inx
            cpx sc_br
            beq s0
            stx cpos
            ldx ls_len
            cpx cpos
            bne s0
            inc ls_len
            bne s0
;
shome       ldx #0
            stx cpos
            beq s0
;
sclr        ldx ls_len
            dex
            cpx sc_br
            beq s2
            cpx #1
            bne s1
s2          dex
s1          stx cpos
s0          jmp input
;
sleft       ldx cpos
            dex
            bmi s0
            stx cpos
            bpl s0
;
sright      ldx cpos
            inx
            cpx ls_len
            bcs s0
            cpx sc_br
            beq s0
            stx cpos
            bcc s0
;
sclear      ldy ls_len
            dey
            dey
            bmi s0
            lda #32
scl1        sta ls_nambuf,y
            dey
            bpl scl1
            iny
            sty cpos
            iny
            sty ls_len
            bne s0
;
sinsert     lda inst
            eor #1
            sta inst
            jmp input
;
sdelete     lda cpos
            tax
            tay
            inx
            cpx sc_br
            beq sd0
            cpx ls_len
            beq s0
            dex
            iny
sd1         lda ls_nambuf,y
            sta ls_nambuf,x
            inx
            iny
            cpy sc_br
            bne sd1
            dey
sd0         lda #32
            sta ls_nambuf,y
            lda ls_len
            cmp #2
            beq s0
            dec ls_len
            bpl s0
;
inst        .by 0
buff2       .tx "                                        "
sonder1     .by 147,148,157,29,189,3,13
            .by 20,19,160
sonder2     .by 93,91,176,185,186,92

; ----------------------------------------- 

pw          .tx "w,p,"
drinit      .tx "i0"
wflen       .by 0
shptx       .tx "spe."
;
wopen       jsr inull
            bne err8
            ldy wflen
            ldx #3
wo1         lda pw,x
            sta ls_nambuf,y
            iny
            dex
            bpl wo1
            sty wflen
            lda #12
            tay
            ldx ls_drive
            jsr filpar
            lda wflen
            ldx #<(ls_nambuf)
            ldy #>(ls_nambuf)
            jsr filnam
            jsr copen
            jmp err9
;
; ----------------------------------------- Add ".eps" to filename

addeps		tay			; Name longer than 12 chars?
		cpy #12
		bcs ag0			; yes, treat it
;
		ldx #3			; no, add ".eps" 
ag1		lda shptx,x
ag3		sta ls_nambuf,y
		sta ls_lastname,y
		iny
		dex
		bpl ag1
		sty wflen		; store new length, finished
		sty ls_flen
		rts
;
ag0		dey			; get last char
		lda ls_nambuf,y
ag2		dey			; shorten to 12 
		cpy #11
		bne ag2
		ldx #04			; counter for chars
		bne ag3			; unconditional jump

; ----------------------------------------- 

inull       ldx #<(drinit)
            ldy #>(drinit)
            lda #2
            jsr gd_sendcom
err9        jsr gd_xmess
            lda ls_err2
            and #15
err8        sec
            rts
;

; ----------------------------------------- Activity gauge

messout     ldx #<(message)
            ldy #>(message)
            jmp gd_xtxout2
;
tcopy       ldy #0
tc0         lda txt,x
            beq clrmess
            sta message,y
            inx
            iny
            bne tc0
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
clrmess     ldx #23
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
;
cntwert     .by 50
;
txt         .ts " Bitmap@" ; 0
;
message     .tx "        "
mess        .tx "                        "
            .by 0

; ----------------------------------------- Store last filename

getname     ldx #0
si0         lda ls_lastname,x
            beq si1
            sta nbuf,x
            cmp #$20		; convert to standard ASCII
            bcs si2
            ora #$40		; lower case
si2         ora #$20		; upper case
            sta title,x
            inx
            cpx #16
            bcc si0
si1         rts
; ----------------------------------------- Publish new filename
setname     lda #0
            ldx #<(ls_picname)
            ldy #>(ls_picname)
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

; ----------------------------------------- Startup requester

requ        inc used
            ldx #<(lstarlst)
            ldy #>(lstarlst)
            jsr gd_xmloop

; ----------------------------------------- Requester management

requester   ldx used
            bmi requ
            rts

; ----------------------------------------- Leave saver

cn2         lda #3
            .by $2c
canc        lda #$ff
            sta used
            sec
            rts

; ------------------------------------ Event: Choose Palette
evpalette   lda palflag
            cmp #4
            bne dp0
            lda #$ff
            sta palflag
dp0         inc palflag
            jsr setpalette
            ldx palflag
            lda gadoffs,x
            tax
            ldy #9
dpi1        lda paltxts,x
            sta paltx,y
            dex
            dey
            bpl dpi1
            ldy #0
dpi2        jsr settext
            ldy #6
            jmp gd_xtxout1

; ------------------------------------ Event: Choose area

evarea      lda areaflag
            bmi ea0
            lda #$fe
            sta areaflag
ea0         inc areaflag
            beq ea1
            ldx #3
            .by $2c
ea1         ldx #7
            ldy #3
ea2         lda areatxts,x
            sta areatx,y
            dex
            dey
            bpl ea2
            ldy #2
            bne dpi2

; ------------------------------------ Event: Choose view

evview      lda viewflag
            bmi ev0
            lda #$fe
            sta viewflag
ev0         inc viewflag
            beq ev1
            ldx #8
            .by $2c
ev1         ldx #17
            ldy #8
ev2         lda viewtxts,x
            sta viewtx,y
            dex
            dey
            bpl ev2

            ldy #4
            jmp dpi2

; ------------------------------------ Output gadget text routines

tabigad     .by <palgad, >palgad
            .by <areagad,>areagad
            .by <viewgad,>viewgad


settext     lda #<tabigad
            sta ls_vekta8
            lda #>tabigad
            sta ls_vekta8+1

            lda (ls_vekta8),y
            sta sc_screentab
            iny
            lda (ls_vekta8),y
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

; ----------------------------------------- Set palette values

setpalette  ldx palflag
            bne spl0
            lda #<palgodot
            ldx #>palgodot
            bne spl4
spl0        dex
            bne spl1
            lda #<palpepto
            ldx #>palpepto
            bne spl4
spl1        dex
            bne spl2
            lda #<paldeekay
            ldx #>paldeekay
            bne spl4
spl2        dex
            bne spl3
            lda #<palbrix
            ldx #>palbrix
            bne spl4
spl3        lda #<palgray
            ldx #>palgray
spl4        sta sc_pos
            stx sc_pos+1
            ldy #119
spl5        lda (sc_pos),y
            sta palette,y
            dey
            bpl spl5
            rts

; ----------------------------------------- Set Save EPS data parameters
;
evsaveps    inc used
            jsr setversion
            jsr setclip
            jsr setvals
            sec
            rts

; ---------------------------------------- Set Version number

setversion  ldx #3
svs0        lda version,x
            sta cversion,x
            dex
            bpl svs0
            rts

; ---------------------------------------- Compute Start address of Clip

setclip     lda #$40		; 4Bit starting from $4000
            sta sc_texttab+1
            ldy #0
            sty sc_texttab
            sty clpzl
            sty clpsp
            lda #40
            sta clpbr
            lda #25 
            sta clpho
            lda areaflag 
            bmi scp8

stcl        lda sc_lastclpzl	; row
            beq scp1		; is zero: skip
            sta clpzl
scp4        clc
            lda sc_texttab+1	; else add 1280, "row" times
            adc #5
            sta sc_texttab+1
            dec clpzl
            bne scp4

scp1        lda sc_lastclpsp	; column
            beq scp5		; is zero: skip
            sta clpsp
scp6        clc
            lda sc_texttab	; else add 32, "column" times
            adc #$20
            sta sc_texttab
            bcc scp7
            inc sc_texttab+1
scp7        dec clpsp
            bne scp6
scp5        lda sc_lastclpbr
            sta clpbr
            lda sc_lastclpho
            sta clpho
scp8        rts

; ----------------------------------------- Set file values for width and height

setvals     ldy #0		; width
            ldx #2
            stx indent
            lda clpbr
            jsr convert		; convert to PetSCII and write to file header

            ldy #0		; height
            ldx #6
            stx indent
            lda clpho
            jsr convert		; convert to PetSCII and write to file header

            ldx #2		; write values to remaining locations in file header
svl2        lda width,x
            sta width2,x
            lda height,x
            sta height2,x
            sta height3,x
            dex
            bpl svl2

            lda viewflag	; landscape?
            bmi ev30		; yes, branch
            ldy #2		; no, exchange width and height
ev3         ldx width,y
            lda height,y
            sta width,y
            txa
            sta height,y
            dey
            bpl ev3

ev30        lda viewflag	; portrait?
            beq ev4		; yes, branch
            lda #$30		; no, reset translate and rotate
            sta transl
            sta transl+1
            sta transl+2
            sta rotate
            sta rotate+1
            bne ev5
ev4         ldx #2		; set translate to height...
ev6         lda height,x
            sta transl,x
            dex
            bpl ev6
            lda #$32		; and rotate to 270
            sta rotate
            lda #$37
            sta rotate+1

ev5         rts

; ----------------------------------------- Convert to PetASCII

convert     sta sc_pos		; value times 8
            sty sc_pos+1
            clc
            asl sc_pos
            rol sc_pos+1
            asl sc_pos
            rol sc_pos+1
            asl sc_pos
            rol sc_pos+1
            ldx sc_pos
            lda sc_pos+1
            sta $62		; convert to PetSCII
            stx $63
            inc 1
            ldx #$90
            sec
            jsr $bc49
            jsr $bddf
            dec 1
            ldx indent		; clear target
            ldy #3
            lda #32
di1         sta width,x
            dex
            dey
            bne di1
            ldy #6		; right adjust digits at target
di2         dey
            lda sy_numbers,y
            bne di2
            dey
            ldx indent
di3         lda sy_numbers,y
            sta width,x
            dex
            dey
            bpl di3
            rts

; ----------------------------------------- Screenlist

lstarlst    .by 0
            .by 4,11,18,16,$81,0,0
            .ts "Save EPS@"
palgad      .by 9,11,18,3,$ca,<(evpalette),>(evpalette)
paltx       .ts "  GoDot   @"
areagad     .by 11,23,6,3,$ce,<evarea,>evarea
areatx      .ts "Full@"
viewgad     .by 13,18,11,3,$ce,<evview,>evview
viewtx      .ts "Landscape@"
            .by 15,11,18,3,$cb,<(evsaveps),>(evsaveps)
            .ts "Save Image@"
            .by 17,11,18,3,$cb,<(cn2),>(cn2)
            .ts "Cancel Save@"
            .by $c0,7,12,15
            .ts "Select Palette:@"
            .by $c0,11,12,10
            .ts "Save Area:@"
            .by $c0,13,12,5
            .ts "View:@"
            .by $80

; ----------------------------------------- Variables

used        .by $ff
mode        .by 0
  
; ----------------------------------------- Write EPS
writeps     lda sc_texttab	; set startaddress
            ldx sc_texttab+1

we0         sta src0
            sta src
            stx src0+1
            stx src+1

            lda #10		; count # of ASCII characters per line in file (80)
            sta cntchars

            lda clpho		; count y blocklines (25 on full screen)
            sta lcnt8

lnloop8     lda #8		; count 8 rasterlines
            sta lcnt
tloop8      lda clpbr		; count x tiles (40 on full screen)
            sta tilecnt
tloop       ldx #4		; count # of pixels per tileline (8)
lnloop      ldy #0		; get value from image
            lda (src),y
            pha
            lsr
            lsr
            lsr
            lsr			; isolate left pixel
            tay			; create hexchar from pixel value
            lda hexchars,y
            jsr bsout		; write to file
            pla			; isolate right pixel
            and #15
            tay
            lda hexchars,y
            jsr bsout
            inc src
            bne lnl3
            inc src+1
lnl3        dex
            bne lnloop
            jsr action

            lda src
            clc
            adc #28
            sta src
            bcc lnl0
            inc src+1
lnl0        dec cntchars
            bne lnl4

            lda #13
            jsr bsout
            lda #10
            sta cntchars
            jsr bsout

lnl4        dec tilecnt
            bne tloop

            lda src0
            clc
            adc #4
            sta src0
            sta src
            lda src0+1
            adc #0
            sta src0+1
            sta src+1
            dec lcnt
            bne tloop8

            lda src0
            clc
            adc #<1248
            sta src0
            sta src
            lda #>1248
            adc src0+1
            sta src0+1
            sta src+1
lnl2        dec lcnt8
            bne lnloop8
            rts

; ----------------------------------------- File Header

.ascii

fheader 	.tx "%!PS-Adobe-3.0 EPSF-3.0"
		.by 13,10
		.tx "%%BoundingBox: 0 0 "
width        	.tx "320 "
height        	.tx "200"
		.by 13,10
		.tx "%%Creator: GoDot C64 Image Processing EPS-Saver v
cversion	.tx "1.00"
		.by 13,10
		.tx "%%Title: "
title		.tx "                "
		.by 13,10
		.tx "%%EndComments
		.by 13,10
		.tx "[/Indexed /DeviceRGB 15"
		.by 13,10
		.tx " <"
palette         .tx "000000 0000aa 664400 333333"
		.by 13,10,32,32
		.tx "880000 cc44cc dd8855 777777"
		.by 13,10,32,32
		.tx "0088ff ff7777 00cc55 bbbbbb"
		.by 13,10,32,32
		.tx "aaffee eeee77 aaff66 ffffff>]setcolorspace"
		.by 13,10
		.tx "0 "
transl		.tx "000 translate"
		.by 13,10
rotate		.tx "000 rotate"
		.by 13,10
		.tx "<<"
		.by 13,10
		.tx "/ImageType 1"
		.by 13,10
		.tx "/Width "
width2		.tx "320"
		.by 13,10
		.tx "/Height "
height2		.tx "200"
		.by 13,10
		.tx "/ImageMatrix [1 0 0 -1 0 "
height3		.tx "200]"
		.by 13,10
		.tx "/BitsPerComponent 4"
		.by 13,10
		.tx "/Decode [0 15]"
		.by 13,10
		.tx "/DataSource <"
		.by 13,10
		.by 0
; ----------------------------------------- File footer

ffooter		.tx ">"
		.by 13,10
		.tx ">> image"
		.by 13,10
		.tx "showpage"
		.by 13,10
                
; ----------------------------------------- 

hexchars	.tx "0123456789abcdef"

palgodot	.tx "000000 0000aa 664400 333333"
		.by 13,10,32,32
		.tx "880000 cc44cc dd8855 777777"
		.by 13,10,32,32
		.tx "0088ff ff7777 00cc55 bbbbbb"
		.by 13,10,32,32
		.tx "aaffee eeee77 aaff66 ffffff"	; 120
palpepto	.tx "000000 352879 433900 444444"
		.by 13,10,32,32
		.tx "68372b 6f3d86 6f4f25 6c6c6c"
		.by 13,10,32,32
		.tx "6c5eb5 9a6759 588d43 959595"
		.by 13,10,32,32
		.tx "70a4b2 b8c76f 9ad284 ffffff"
paldeekay	.tx "000000 181090 472b1b 484848"
		.by 13,10,32,32
		.tx "882000 a838a0 a04800 808080"
		.by 13,10,32,32
		.tx "5090d0 c87870 50b818 b8b8b8"
		.by 13,10,32,32
		.tx "68d0a8 f0e858 98ff98 ffffff"
palbrix		.tx "000000 4242c6 846339 7b7b7b"
		.by 13,10,32,32
		.tx "ad524a bd6bbd c88e2f 949494"
		.by 13,10,32,32
		.tx "9c9cf7 ff9c9c 73e773 cecece"
		.by 13,10,32,32
		.tx "73f7f7 ffff7b a5ffa5 ffffff"
palgray		.tx "000000 111111 222222 333333"
		.by 13,10,32,32
		.tx "444444 555555 666666 777777"
		.by 13,10,32,32
		.tx "888888 999999 aaaaaa bbbbbb"
		.by 13,10,32,32
		.tx "cccccc dddddd eeeeee ffffff"

.petscii

gadoffs		.by 9,19,29,39,49
gadoffs2        .by 3,7
palflag		.by 0
areaflag	.by $ff
viewflag	.by $ff

paltxts		.ts "  GoDot   "	; 0
		.ts "  Pepto   "	; 10
		.ts "  Deekay  "	; 20
		.ts "   Brix   "	; 30
		.ts "  Gray16  "	; 40
areatxts	.ts "Full" ; 0
		.ts "Clip" ; 4
viewtxts	.ts "Landscape" ; 0 
		.ts "Portrait " ; 8

; ----------------------------------------- Write file header

wrheader    ldy #0
            lda #<fheader
            sta src
            lda #>fheader
            sta src+1
wrh1        lda (src),y
            beq wrh0
            jsr bsout
            inc src
            bne wrh1
            inc src+1
            bne wrh1
wrh0        rts

; ----------------------------------------- Write file footer

wrfooter    ldx #footlen
            ldy #0
wrf0        lda ffooter,y
            jsr bsout
            iny
            dex
            bne wrf0
            rts

; ----------------------------------------- 

modend      .en

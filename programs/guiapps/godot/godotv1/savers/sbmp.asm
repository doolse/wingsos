.petscii
.include "godotlib.lib"
.ob "sbmp,p,w"

; ----------------------------------------- 
; svr.BMP
;     Saver for Windows Bitmap Files (BMP)
;
; 1.00: 8.12.01, first release
;
; ----------------------------------------- 

            .ba $c000
;
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
            .tx "Windows Bitmap  "
version     .tx "1.00"
            .tx "08.12.01"
            .tx "A.Dettke        "
;
; ----------------------------------------- 

start       ldx gr_nr
            dex
            bne st0
            jmp initinp
;
st0         ldx #0		; reset filelength
            stx flength
            stx flength+1

            jsr requester	; display requester
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
            jsr addbmp		; add ".bmp" to name (length plus 4)
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
            jsr writbmp

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
shptx       .tx "pmb."
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
; ----------------------------------------- Add ".bmp" to filename

addbmp		tay			; Name longer than 12 chars?
		cpy #12
		bcs ag0			; yes, treat it
;
		ldx #3			; no, add ".bmp" 
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
            ldx #<(bmplst)
            ldy #>(bmplst)
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

; ------------------------------------ Output gadget text routines

tabigad     .by <palgad, >palgad
            .by <areagad,>areagad


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
            ldy #63
spl5        lda (sc_pos),y
            sta palette,y
            dey
            bpl spl5
            rts

; ----------------------------------------- Set Save BMP data parameters
;
evsavbmp    inc used
            jsr setclip
            jsr setvals
            sec
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
            ldx #25 
            stx clpho
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

            ldx sc_lastclpho	; now move to the bottom of the clip
            stx clpho
scp8        dex 
            stx clpzl
scp51       clc
            lda sc_texttab+1	; add 1280, "height"-1 times
            adc #5
            sta sc_texttab+1
            dec clpzl
            bne scp51
            clc			; and add 1 block
            lda sc_texttab
            adc #28
            sta sc_texttab
            bcc scp9
            inc sc_texttab+1

scp9        rts

; ----------------------------------------- Set file values 

setvals     lda clpbr		; width
            jsr convert		; convert to Word and write to file header
            stx width
            sta width+1

            lda clpho		; height
            jsr convert		; convert to Word and write to file header
            stx height
            sta height+1

            lda clpbr		; compute filelength: width*4*height+header
            asl
            asl
            tax
ev54        clc
            lda sc_pos
            adc flength
            sta flength
            bcc ev55
            lda sc_pos+1
            adc flength+1
            sta flength+1
ev55        dex
            bne ev54
            clc			; add length of header (118 bytes)
            lda flength
            adc dstart
            sta flength
            bcc ev5
            inc flength+1

ev5         rts

; ----------------------------------------- Convert to Word

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
            rts

; ----------------------------------------- Screenlist

bmplst      .by 0
            .by 4,11,18,13,$81,0,0
            .ts "Save BMP@"
palgad      .by 9,11,18,3,$ca,<(evpalette),>(evpalette)
paltx       .ts "  GoDot   @"
areagad     .by 11,23,6,3,$ce,<evarea,>evarea
areatx      .ts "Full@"
            .by 13,11,18,3,$cb,<(evsavbmp),>(evsavbmp)
            .ts "Save Image@"
            .by 15,11,18,3,$cb,<(cn2),>(cn2)
            .ts "Cancel Save@"
            .by $c0,7,12,15
            .ts "Select Palette:@"
            .by $c0,11,12,10
            .ts "Save Area:@"
            .by $80

; ----------------------------------------- Variables

used        .by $ff
mode        .by 0

; ----------------------------------------- Write BMP

writbmp     lda sc_texttab	; set startaddress
            ldx sc_texttab+1

we0         sta src0
            sta src
            stx src0+1
            stx src+1

            lda clpho		; count y blocklines (25 on full screen)
            sta lcnt8

lnloop8     lda #8		; count 8 rasterlines
            sta lcnt
tloop8      lda clpbr		; count x tiles (40 on full screen)
            sta tilecnt
tloop       ldx #4		; count # of pixels per tileline (8)
lnloop      ldy #0		; get value from image
            lda (src),y
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
lnl0        dec tilecnt
            bne tloop

lnl01       lda src0
            sec
            sbc #4
            sta src0
            sta src
            lda src0+1
            sbc #0
            sta src0+1
            sta src+1
            dec lcnt
            bne tloop8

            lda src0
            sec
            sbc #<1248
            sta src0
            sta src
            lda src0+1
            sbc #>1248
            sta src0+1
            sta src+1
lnl2        dec lcnt8
            bne lnloop8
            rts

; ----------------------------------------- File Header

fheader 	.tx "bm"
flength        	.by 0,0,0,0
		.by 0,0,0,0
dstart        	.by $76,0,0,0
		.by $28,0,0,0
width		.by 0,0,0,0
height		.by 0,0,0,0
planes		.by $01,0
depth		.by $04,0
comp		.by 0,0,0,0
csize		.by 0,0,0,0
haspect		.by 0,0,0,0
vaspect		.by 0,0,0,0
numcols		.by 16,0,0,0
		.by 0,0,0,0
palette         .by $00,$00,$00,$00 	; black
		.by $aa,$00,$00,$00	; blue
		.by $00,$44,$66,$00	; brown
		.by $33,$33,$33,$00	; dark gray
		.by $00,$00,$88,$00	; red
		.by $cc,$44,$cc,$00	; purple
		.by $55,$88,$dd,$00	; orange
		.by $77,$77,$77,$00	; medium gray
		.by $ff,$88,$00,$00	; light blue
		.by $77,$77,$ff,$00	; light red
		.by $55,$cc,$00,$00	; green
		.by $bb,$bb,$bb,$00	; light gray
		.by $ee,$ff,$aa,$00	; cyan
		.by $77,$ee,$ee,$00	; yellow
		.by $66,$ff,$aa,$00	; light green
		.by $ff,$ff,$ff,$00	; white

; ----------------------------------------- 

palgodot        .by $00,$00,$00,$00 	; black
		.by $aa,$00,$00,$00	; blue
		.by $00,$44,$66,$00	; brown
		.by $33,$33,$33,$00	; dark gray
		.by $00,$00,$88,$00	; red
		.by $cc,$44,$cc,$00	; purple
		.by $55,$88,$dd,$00	; orange
		.by $77,$77,$77,$00	; medium gray
		.by $ff,$88,$00,$00	; light blue
		.by $77,$77,$ff,$00	; light red
		.by $55,$cc,$00,$00	; green
		.by $bb,$bb,$bb,$00	; light gray
		.by $ee,$ff,$aa,$00	; cyan
		.by $77,$ee,$ee,$00	; yellow
		.by $66,$ff,$aa,$00	; light green
		.by $ff,$ff,$ff,$00	; white

palpepto	.by $00,$00,$00,$00, $79,$28,$35,$00, $00,$39,$43,$00, $44,$44,$44,$00
		.by $2b,$37,$68,$00, $86,$3d,$6f,$00, $25,$4f,$6f,$00, $6c,$6c,$6c,$00
		.by $b5,$5e,$6c,$00, $59,$67,$9a,$00, $43,$8d,$58,$00, $95,$95,$95,$00
		.by $b2,$a4,$70,$00, $6f,$c7,$b8,$00, $84,$d2,$9a,$00, $ff,$ff,$ff,$00

paldeekay	.by $00,$00,$00,$00, $90,$10,$18,$00, $1b,$2b,$47,$00, $48,$48,$48,$00
		.by $00,$20,$88,$00, $a0,$38,$a8,$00, $00,$48,$a0,$00, $80,$80,$80,$00
		.by $d0,$90,$50,$00, $70,$78,$c8,$00, $18,$b8,$50,$00, $b8,$b8,$b8,$00
		.by $a8,$d0,$68,$00, $58,$e8,$f0,$00, $98,$ff,$98,$00, $ff,$ff,$ff,$00

palbrix		.by $00,$00,$00,$00, $c6,$42,$42,$00, $39,$63,$84,$00, $7b,$7b,$7b,$00
		.by $4a,$52,$ad,$00, $bd,$6b,$bd,$00, $2f,$8e,$c8,$00, $94,$94,$94,$00
		.by $f7,$9c,$9c,$00, $9c,$9c,$ff,$00, $73,$e7,$73,$00, $ce,$ce,$ce,$00
		.by $f7,$f7,$73,$00, $7b,$ff,$ff,$00, $a5,$ff,$a5,$00, $ff,$ff,$ff,$00

palgray		.by $00,$00,$00,$00, $11,$11,$11,$00, $22,$22,$22,$00, $33,$33,$33,$00
		.by $44,$44,$44,$00, $55,$55,$55,$00, $66,$66,$66,$00, $77,$77,$77,$00
		.by $88,$88,$88,$00, $99,$99,$99,$00, $aa,$aa,$aa,$00, $bb,$bb,$bb,$00
		.by $cc,$cc,$cc,$00, $dd,$dd,$dd,$00, $ee,$ee,$ee,$00, $ff,$ff,$ff,$00

gadoffs		.by 9,19,29,39,49
gadoffs2        .by 3,7
palflag		.by 0
areaflag	.by $ff

paltxts		.ts "  GoDot   "	; 0
		.ts "  Pepto   "	; 10
		.ts "  Deekay  "	; 20
		.ts "   Brix   "	; 30
		.ts "  Gray16  "	; 40
areatxts	.ts "Full" ; 0
		.ts "Clip" ; 4

; ----------------------------------------- Write file header

wrheader    ldy #0
            lda #<fheader
            sta src
            lda #>fheader
            sta src+1
wrh1        lda (src),y
            jsr bsout
            iny
            cpy dstart
            bne wrh1
            rts

; ----------------------------------------- 

modend      .en

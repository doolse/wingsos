.petscii
.include "godotlib.lib"
.ob "sGeoPnt,p,w"

; ----------------------------------------- 
; svr.GeoPaint
;     Saver for GEOS Bitmap Files 640x400
;
; 1.00: 06.05.94, first release
; 1.01: 22.12.01; more features:
;
; ----------------------------------------- 

            .ba $c000
;
            .eq pport		=$01

            .eq ycnt		=$37
            .eq bbuf		=$39
            .eq bitcnt		=$3a
            .eq ybuf		=$3b
            .eq data		=$3c
            .eq adcnt		=$3e
            .eq pixadd		=$45
            .eq packed		=$49
            .eq byte		=$4a
            .eq blockend	=$4b

            .eq status		=$90

            .eq offx		=$b2
            .eq offy		=$b3

            .eq hold		=$f9
            .eq rflg		=$fa
            .eq blcnt		=$fb
            .eq bycnt		=$fc
            .eq mlen		=$fd
            .eq cpos		=mlen+1
            .eq bmerk		=$fe

            .eq buffer		=$0200
            .eq cred		=gr_rgb
            .eq cgrn		=cred+16
            .eq cblu		=cgrn+16

            .eq buf16		=$3f40
            .eq byte1buf	=$3f80
            .eq byte2buf	=$3f81

            .eq blockbuf	=$cc00
            .eq blockbuf2	=$cd00

            .eq get		=$ffe4
;
; ----------------------------------------- 

header      jmp start
            .by $40
            .by 0
            .by 1
            .wo modend
            .wo 0
            .tx "GeoPaint        "
            .tx "1.01"
            .tx "22.12.01"
            .tx "A.Dettke/W.Kling"
;
; ----------------------------------------- Main Routine
;
start       ldx gr_nr		; Input or Save?
            dex
            bne st0
            jmp initinp		; it's "input"
;
st0         lda #0		; save: mouse pointer off
            sta $d015
            sta updown		; init variables
            lda gd_modswitch
            pha
            lda ls_flen
            beq st1
            sta wflen
            lda #1
            sta adcnt
            inc pport		; BASIC on
            jsr getname
            jsr gd_clrms
            ldx #0
            jsr tcopy
            jsr cnvinit		; init dithering
            jsr write		; save image to disk
st2         jsr clrch
            lda #12
            jsr close
            jsr err9
            bne st1
            jsr setname
st1         pla
            sta gd_modswitch
            lda #$03		; mouse pointer on
            sta $d015
            dec pport		; BASIC off
;
cn2         sec			; leave saver
            rts

; ----------------------------------------- 
; ----------------------------------------- Input Routines
; ----------------------------------------- 
;
cursor      ldy sc_br
            dey
c0          lda buffer,y
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
ii1         sta buffer,y
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
r0          lda buffer,y
            sta sc_movetab,y
            dey
            bpl r0
            jsr gd_cnvbc
            ldy mlen
            ldx sc_zl
            cpx #20
            bne r1
            ldx ls_dirmask
            beq r4
            ldx #11
            ldy #15
r5          lda buffer,x
            sta buffer,y
            dey
            dex
            bpl r5
r6          inx
            lda $073c,x
            sta buffer,x
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
            sta buffer,y
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
str0        lda buffer-1,y
            sta buffer-1,x
            dex
            dey
            cpx cpos
            bne str0
            pla
            sta buffer,x
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
            sta buffer,x
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
            lda #32
scl1        sta buffer,y
            dey
            bpl scl1
            iny
            sty cpos
            iny
            sty ls_len
            beq s0
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
sd1         lda buffer,y
            sta buffer,x
            inx
            iny
            cpy sc_br
            bne sd1
            dey
sd0         lda #32
            sta buffer,y
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
; ----------------------------------------- Disk Access Routines
; ----------------------------------------- 

pw          .tx "w,p,"
drinit      .tx "i0"
wflen       .by 0
;
wopen       jsr inull
            bne err8
            ldy wflen
            ldx #3
wo1         lda pw,x
            sta buffer,y
            iny
            dex
            bpl wo1
            sty wflen
            lda #12
            tay
            ldx ls_drive
            jsr filpar
            lda wflen
            ldx #<(buffer)
            ldy #>(buffer)
            jsr filnam
            jsr copen
            jmp err9
;
inull       ldx #<(drinit)
            ldy #>(drinit)
            lda #2
            jsr gd_sendcom
err9        jsr gd_xmess
            lda ls_err2
            and #15
err8        sec
            rts

; ----------------------------------------- Disk Error

dfehler     jsr clrch
            lda #$01
            jsr close
            lda #$02
            jsr close
            sec
            rts
;
; ----------------------------------------- Request Block

sendmess    pha
            txa
            pha
            ldx #$01
            jsr ckout
            ldy #$00
sm1         lda message3,y	; U1 2 0 TT SS
            beq sm2
            jsr out
            iny
            jmp sm1
;
sm2         lda #$20
            jsr out
            pla
            tax
            lda #$00
            jsr intout
            lda #$20
            jsr out
            pla
            tax
            lda #$00
            jsr intout
            lda #$0d
            jsr out
            jmp clrch
;
; ----------------------------------------- Set Pointer Position in Buffer

sendmess2   ldx #$01
            jsr ckout
            ldy #>(message4)	; B-P 2 0
            lda #<(message4)
            jsr $ab1e
            jmp clrch
;
; ----------------------------------------- Read Disk Block to 1st buffer

getblock    jsr sendmess
            jsr sendmess2
            ldx #$02
            jsr chkin
            ldy #$00
gbl1        jsr in
            sta blockbuf,y
            iny
            bne gbl1
            jmp clrch
;
; ----------------------------------------- Read Disk Block to 2nd buffer

getblock2   jsr sendmess
            jsr sendmess2
            ldx #$02
            jsr chkin
            ldy #$00
gbl2        jsr in
            sta blockbuf2,y
            iny
            bne gbl2
            jmp clrch
;
; ----------------------------------------- Read Byte from Disk
;
in          jsr basin
            pha
            lda status
            and #$83
            beq svg10
            lda #$00
            sta status
            pla
            jsr dfehler
            jmp gd_xmess
svg10       pla
            rts
;
; ----------------------------------------- Write Byte to Disk

out         jsr bsout
            pha
            lda status
            and #$83
            beq o1
            lda #$00
            sta status
            pla
            jsr dfehler
            jmp gd_xmess
o1          pla
            rts
;
; ----------------------------------------- Get Byte from Buffer2

getbyte2    inc blindex
            beq gby1
            ldy blindex
            lda blockbuf2,y
            rts
;
gby1        ldx blockbuf2
            lda blockbuf2+1
            jsr getblock2
            ldy #$01
            sty blindex
            jmp getbyte2
;
; ----------------------------------------- Write Block to Disk

sendmess3   pha
            txa
            pha
            ldx #$01
            jsr ckout
            ldy #>(message6)	; U2 2 0 TT SS
            lda #<(message6)
            jsr $ab1e
            jmp sm2
;
; ----------------------------------------- Retrieve 1st writeable Block on Disk

getfirst    lda #$00
            sta kommas
gfloop      ldx #$01
            jsr ckout
            ldy #>(message8)	; B-A 0 TT SS
            lda #<(message8)
            jsr $ab1e
            lda #$00
            ldx track
            jsr intout
            lda #$20
            jsr out
            lda #$00
            ldx sector
            jsr intout
            lda #$0d
            jsr out
            jsr clrch

            ldx #$01
            jsr chkin
            jsr in
            cmp #$30		; ok?
            bne gf1
            jmp clrch		; yes, return
;
gf1         inc kommas		; no, search next
            lda kommas
            cmp #$03
            bne gf2
            jsr dfehler
            jmp gd_xmess
gf2         jsr in		; skip error number
            cmp #$2c
            bne gf2
gf3         jsr in		; skip message
            cmp #$2c
            bne gf3
gf4         jsr in		; retrieve next TT
            cmp #$20
            beq gf4
            sec
            sbc #$30
            pha
            asl
            asl
            sta track
            pla
            clc
            adc track
            asl
            sta track
            jsr in
            sec
            sbc #$30
            clc
            adc track
            sta track
gf5         jsr in
            cmp #$2c
            bne gf5
gf6         jsr in		; retrieve next SS
            cmp #$20
            beq gf6
            sec
            sbc #$30
            pha
            asl
            asl
            sta sector
            pla
            clc
            adc sector
            asl
            sta sector
            jsr in
            sec
            sbc #$30
            clc
            adc sector
            sta sector
            jsr clrch
            lda track
            cmp ls_track		; skip directory track
            beq gf7
            jmp gfloop		; check found one
;
gf7         inc track
            lda #$00
            sta sector
            jmp gfloop
;
; ----------------------------------------- Activity Display Routines

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
            jsr gd_setmess
            jsr messout
            dec offx
            bpl ld4
            inc offy
            lda #7
            sta offx
ld4         rts
;
clrmess     ldx #25
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
cntwert     .by 200
;
txt         .ts " GEOS @"
;
message     .ts "      "
mess        .tx "                          "
            .by 0
;
; ----------------------------------------- Filename Routines

getname     ldx #0
si0         lda ls_lastname,x
            beq si1
            sta nbuf,x
            inx
            cpx #16
            bcc si0
si1         rts
;
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
;
; ----------------------------------------- Floppy Messages

message3    .tx "u1 2 0"
            .by 0
message4    .tx "b-p 2 0"
            .by 13,0
message6    .tx "u2 2 0"
            .by 0
message8    .tx "b-a 0 "
            .by 0

; ----------------------------------------- Variables

blindex     .by $00
picindex    .by $00
dirchan     .tx "#"
updown      .by $00
track       .by $00
strack      .by $00
sector      .by $00
ssector     .by $00
tlink1      .by $00
slink1      .by $00
count2      .by $00,$00
kommas      .by $00
wtrack      .by $00
wsector     .by $00
dirindex    .by $00

; ----------------------------------------- GEOS Info Record Block

info        .by $00,$ff,$03,$15,$bf,$ff,$ff,$ff; "....¿..." Icon Data
            .by $c0,$00,$03,$a0,$00,$05,$9f,$ff; "À.. ...."
            .by $f9,$95,$55,$59,$9a,$aa,$a9,$95; "..uy.ª©."
            .by $55,$59,$9a,$aa,$a9,$95,$55,$59; "uy.ª©.uy"
            .by $9a,$aa,$a9,$95,$55,$59,$9a,$aa; ".ª©.uy.ª"
            .by $a9,$9f,$ff,$f9,$a0,$00,$05,$c0; "©... ..À"
            .by $00,$03,$ff,$ff,$ff,$00,$00,$00; "........"
            .by $00,$00,$03,$7f,$ff,$b6,$80,$00; ".....¶.."
            .by $fe,$7f,$ff,$bc,$83,$07,$01,$00; "...¼...." USR, AppData, VLIR
            .by $00,$ff,$ff,$00,$00,$50,$61,$69; ".....p.." Class: GeoPaint 1.1
            .by $6e,$74,$20,$49,$6d,$61,$67,$65; ".. i...."
            .by $20,$56,$31,$2e,$31,$00,$00,$00; " v1.1..."
            .by $47,$6f,$44,$6f,$74,$20,$56,$31; "GoDot v1" Author: GoDot
            .by $2e,$32,$32,$20,$28,$57,$4b,$26; ".22 (WK&"
            .by $41,$44,$29,$00,$00,$67,$65,$6f; "AD)..geo" Application: GeoPaint 2.0
            .by $50,$61,$69,$6e,$74,$20,$20,$20; "paint   "
            .by $20,$56,$32,$2e,$30,$00,$00,$00; " v2.0..."
            .by $00,$20,$31,$41,$c9,$06,$d0,$02; ". 1A...."
            .by $a9,$12,$8d,$26,$40,$a9,$00,$8d; "........"
            .by $2b,$40,$20,$01,$40,$90,$05,$a9; "........"
            .by $43,$72,$65,$61,$74,$65,$64,$20; "Created " Application Data
            .by $62,$79,$20,$47,$6f,$44,$6f,$74; "by GoDot"
            .by $20,$56,$31,$2e,$32,$32,$20,$28; " V1.22 ("
            .by $57,$4b,$26,$41,$44,$29,$20,$2d; "WK&AD) -"
            .by $20,$56,$49,$56,$41,$20,$46,$2e; " VIVA F."
            .by $44,$69,$74,$72,$69,$63,$68,$21; "Ditrich!"
            .by $00

; ----------------------------------------- 

offs        .by $00,$10,$20,$30
            .by $00,$10,$20,$30
;
dlo         .by <(dithoff)
            .by <(dithrst)
            .by <(dithpat)
            .by <(dithnoi)
            .by <(dithrnd)
;
dhi         .by >(dithoff)
            .by >(dithrst)
            .by >(dithpat)
            .by >(dithnoi)
            .by >(dithrnd)
;
dmode       .by 0,0
;
; ----------------------------------------- 4Bit addresses

line8       .by $40,$45,$4a,$4f,$54	; Highbyte Blocklines
            .by $59,$5e,$63,$68,$6d
            .by $72,$77,$7c,$81,$86
            .by $8b,$90,$95,$9a,$9f
            .by $a4,$a9,$ae,$b3,$b8
;
bllo        .by $00,$20,$40,$60,$80,$a0,$c0,$e0	; Offset in Blocklines
            .by $00,$20,$40,$60,$80,$a0,$c0,$e0
            .by $00,$20,$40,$60,$80,$a0,$c0,$e0
            .by $00,$20,$40,$60,$80,$a0,$c0,$e0
            .by $00,$20,$40,$60,$80,$a0,$c0,$e0
;
blhi        .by 0,0,0,0,0,0,0,0
            .by 1,1,1,1,1,1,1,1
            .by 2,2,2,2,2,2,2,2
            .by 3,3,3,3,3,3,3,3
            .by 4,4,4,4,4,4,4,4
;
; ----------------------------------------- 
; ----------------------------------------- Disk Write Routines
; ----------------------------------------- 

writerle    inc blindex		; inc pointer in data buffer
            bne wr1		; zero?
            pha			; yes, save block:
            lda track		; set TT and SS
            sta tlink1
            lda sector
            sta slink1
            jsr getfirst	; search 1st available block on disk
            ldx track
            lda sector
            stx blockbuf2
            sta blockbuf2+1
            jsr sendmess2	; set pointer in disk buffer
            ldx #$02
            jsr ckout
            ldy #$00
wr2         lda blockbuf2,y	; write block to disk buffer
            jsr out
            iny
            bne wr2
            jsr clrch
            ldx tlink1
            lda slink1
            jsr sendmess3	; write buffer to disk
            inc count2
            bne wr3
            inc count2+1
wr3         ldx #$00		; re-init buffer
            lda #$ff
            stx blockbuf2
            sta blockbuf2+1
            ldy #$02
            sty blindex
            pla			; write current byte to new buffer
wr1         ldy blindex
            sta blockbuf2,y
            rts
;
; ----------------------------------------- Save GeoPaint VLIR image to Disk

write       ldy ls_flen		; fill filename w/ $a0 (Shift Space)
            lda #$a0
np1         cpy #$10
            beq np2
            sta ls_nambuf,y
            iny
            jmp np1
;
np2         lda #$01		; OPEN 1,drive,15
            ldx ls_drive
            ldy #$0f
            jsr filpar
            lda #$00
            jsr filnam
            jsr copen
            lda #$02		; OPEN 2,drive,2,"#"
            ldx ls_drive
            ldy #$02
            jsr filpar
            lda #$01
            ldx #<(dirchan)
            ldy #>(dirchan)
            jsr filnam
            jsr copen
            ldx ls_track	; retrieve BAM block
            lda #0
            jsr getblock
np3         ldx blockbuf
            lda blockbuf+1
            stx wtrack
            sta wsector
            cpx #$00
            bne np4
            jsr dfehler
            jmp gd_xmess
np4         jsr getblock
            ldy #$00
np5         lda blockbuf+2,y
            beq np6
            tya
            clc
            adc #$20
            beq np3
            tay
            bne np5
np6         sty dirindex
            ldx #$01		; search disk for 1st available empty block
            lda #$00
            stx track
            sta sector
            jsr getfirst
            ldx track		; set its T/S number
            lda sector
            stx strack
            sta ssector
            ldy #$00
np7         lda #$00		; fill buffer w/ $00 $ff
            sta blockbuf,y
            lda #$ff
            sta blockbuf+1,y
            iny
            iny
            cpy #$5c
            bne np7
            lda #$00
np8         sta blockbuf,y
            iny
            bne np8
            lda #$02		; set counter to 2 (filelength)
            sta count2
            lda #$00
            sta count2+1
            lda #$00		; set blockline counter to 0
            sta picindex
np11        jsr getfirst	; search for next available block
            lda updown		; get flag for upper or lower half 
            asl
            tay
            iny
            iny
            lda track		; store block address to buffer
            sta blockbuf,y
            lda sector
            sta blockbuf+1,y
            ldx #$00
            lda #$ff
            stx blockbuf2
            sta blockbuf2+1
            lda #$01
            sta blindex
            lda #$40		; data start at $4000
            clc			; (move in steps of $500)
            adc picindex	; -> in PIXADD
            adc picindex
            adc picindex
            adc picindex
            adc picindex
            sta pixadd+1
            adc #$05		; data end at $44ff (1280 bytes)
            sta blockend
            lda #$00
            sta pixadd
            lda #$81		; init RLE byte
            sta packed
            jsr getbyte		; retrieve one rendered byte
            sta byte
np12        jsr getbyte		; retrieve next rendered byte
            cmp byte
            bne notequal
            inc packed		; equal: compress byte
            jmp equal
;
notequal    pha			; not equal: save last byte
            lda packed
            cmp #$81
            bne pk1
            lda byte		; get first byte
            sta byte1buf
            pla			; get second byte
            sta byte2buf
            lda #$02		; compress 2 bytes
            jmp pk2
;
pk1         pla
            sta byte1buf	; store byte
            lda packed		; end compress sequence
            jsr writerle
            lda byte
            jsr writerle
            lda #$01		; restart w/ 1 byte

pk2         sta packed
pk3         lda pixadd+1	; end of blockline?
            cmp blockend
            bne pk7		; no
pk4         lda packed		; yes, break compressing
            jsr writerle
            dec packed
            lda #$00
pk5         pha
            tay
            lda byte1buf,y	; ...and write buffer
            jsr writerle
            pla
            cmp packed
            beq pk6
            clc
            adc #$01
            jmp pk5
;
pk6         lda pixadd+1
            cmp blockend	; end of blockline?
            beq pk14		; yes, add color information
            jmp pk12		; no, start over
;
pk7         lda packed		; $3f bytes compressed?
            cmp #$3f
            beq pk4		; yes, break
            jsr getbyte		; no, get next byte
            ldy packed
            cmp byte1buf-1,y
            bne pk10
            sta byte
            dec packed
            beq pk9
            lda packed
            jsr writerle
            dec packed
            lda #$00
pk8         pha
            tay
            lda byte1buf,y	; ...and write pattern
            jsr writerle
            pla
            cmp packed
            beq pk9
            clc
            adc #$01
            jmp pk8
;
pk9         lda #$82
            sta packed
            jmp equal
;
pk10        sta byte1buf,y
            inc packed
            jmp pk3
;
equal       lda pixadd+1	; end of blockline?
            cmp blockend
            beq pk13		; yes, break and add color information
            lda packed		; max number of compressed bytes?
            cmp #$ff
            beq pk11		; yes, write and loop over
            jmp np12
;
pk11        jsr writerle
            lda byte
            jsr writerle
pk12        lda #$81
            sta packed
            jsr getbyte
            sta byte
            jmp equal
;
pk13        lda packed
            jsr writerle
            lda byte
            jsr writerle
pk14        lda #$88		; add 8 bytes 0
            jsr writerle
            lda #$00
            jsr writerle
            lda #$ff		; add 160 bytes $bf (colors for 2 blocklines)
            jsr writerle
            lda #$bf
            jsr writerle
            lda #$a1
            jsr writerle
            lda #$bf
            jsr writerle
            lda #$00		; end w/ 0
            jsr writerle
            lda blindex		; write block
            sta blockbuf2+1
            jsr sendmess2
            ldx #$02
            jsr ckout
            ldy #$00
pk15        lda blockbuf2,y
            jsr out
            iny
            bne pk15
            jsr clrch
            ldx track
            lda sector
            jsr sendmess3
            inc count2
            bne pk16
            inc count2+1
pk16        inc picindex
            inc updown
            lda picindex
self3       cmp #$19		; all blocklines? (25)
            beq pk17		; yes, write file info block
            jmp np11		; start over
;
pk17        jsr sendmess2	; write file info block
            ldx #$02
            jsr ckout
            ldy #$00
pk18        lda blockbuf,y
            jsr out
            iny
            bne pk18
            jsr clrch
            ldx strack
            lda ssector
            jsr sendmess3
            jsr getfirst
            ldx #$02
            jsr ckout
            ldy #$00
pk19        lda info,y
            jsr out
            iny
            cpy #$d1
            bne pk19
            jsr clrch
            ldx track
            lda sector
            jsr sendmess3
            ldx wtrack
            lda wsector
            jsr getblock
            ldy dirindex
            lda #$83
            sta blockbuf+2,y
            lda strack
            sta blockbuf+3,y
            lda ssector
            sta blockbuf+4,y
            lda track
            sta blockbuf+21,y
            lda sector
            sta blockbuf+22,y
            lda #$01
            sta blockbuf+23,y
            lda #$07
            sta blockbuf+24,y

            lda #94		; Date and Time: 94-05-06, 6pm
            sta blockbuf+25,y
            lda #5
            sta blockbuf+26,y
            lda #6
            sta blockbuf+27,y
            lda #18
            sta blockbuf+28,y
            lda #0
            sta blockbuf+29,y

            lda count2
            sta blockbuf+30,y
            lda count2+1
            sta blockbuf+31,y
            tya
            clc
            adc #$05
            sta pixadd
            lda #>(blockbuf)
            sta pixadd+1
            ldy #$0f
pk20        lda ls_nambuf,y
            sta (pixadd),y
            dey
            bpl pk20
            jsr sendmess2
            ldx #$02
            jsr ckout
            ldy #$00
pk21        lda blockbuf,y
            jsr out
            iny
            bne pk21
            jsr clrch
            ldx wtrack
            lda wsector
            jsr sendmess3
            jmp dfehler		; finished
;
; ----------------------------------------- Init Dither Routines

cnvinit     lda gr_dither		; get dither type
            tax
            lda dlo,x		; set JMP address
            sta dmode
            lda dhi,x
            sta dmode+1
            lda #$80		; init random values
            sta $d418
            sta $d40e
            sta $d40f
            lda #0
            sta $d412
            lda #$81
            sta $d412
            lda #0		; init variables
            sta rflg
            sta blcnt
            lda #16
            sta bycnt
            rts
;
; ----------------------------------------- Get Data Byte
;
getbyte     ldx bycnt
            cpx #16
            bcc skip0
            jsr make16		; convert 4 4Bit pixels to 4 hires pixels
            ldx #0
            stx bycnt
skip0       lda buf16,x		; invert byte
            eor #$ff
            sta bmerk
            inc bycnt		; inc tile counter
            inc pixadd		; inc 4Bit address
            bne gb1
            inc pixadd+1
gb1         jsr action
            lda bmerk
            rts
;
; ----------------------------------------- Render 4Bit to Hires

make16      ldx picindex	; Blockline counter
            lda line8,x		; compute 4Bit data address
            clc
            ldx blcnt
            adc blhi,x
            sta data+1
            lda bllo,x
            bit rflg		; upper/lower half of tile?
            bpl skip1
            clc
            adc #16		; add 16 if lower half
skip1       sta data
            bcc skip2
            inc data+1
skip2       inx			; scanned one blockline?
            cpx #40
            bne skip3
            ldx #0		; yes, end of line:
            lda rflg		; toggle flag for upper/lower half
            eor #$ff
            sta rflg		; tile counter to 0 again
skip3       stx blcnt
            dec pport
;
            ldx #0		; .X   = pointer into BUF16
            ldy #0		; YCNT = pointer into dither pattern
            sty ycnt
theloop     lda #0		; clear buffer
            sta buf16,x
            sta buf16+1,x
            lda #$c0		; set mask (%11000000)
            sta bitcnt
            lda (data),y	; get 4bit data byte
            lsr			; isolate left pixel
            lsr
            lsr
            lsr
            jsr dith2		; dither (2 bits)
            lda (data),y
            and #$0f		; isolate right pixel
            jsr dith2		; dither (next 2 bits)
            iny
            lda (data),y	; get next data byte
            lsr			; isolate left pixel
            lsr
            lsr
            lsr
            jsr dith2		; dither (3rd 2 bits)
            lda (data),y
            and #$0f		; isolate right pixel
            jsr dith2		; dither (last 2 bits)
            iny			; increment to next data line
            iny			; pointing to $4004 (first time)
            iny
            inx			; increment in buffer
            inx
            inc ycnt		; increment pattern offset
            inc ycnt
            cpx #8		; got 8 lines? Then: right half of tile
            bne skip4
            ldy #2		; start over two bytes to the right
            lda #0		; pattern offset to 0
            sta ycnt
skip4       cpx #16		; result in BUF16
            bne theloop		; (upper left quarter of a tile in hires)
            inc pport
            rts
; ----------------------------------------- Description of MAKE16
;
; the whole operation works (4 times) on quarters of a tile: 
; 1st pass: upper left quarter
; 2nd pass: upper right quarter
; 3rd pass: lower left quarter
; 4th pass: lower right quarter
; resulting in a double pixel/double height hires tile (16x16 pixels)
;
; ----------------------------------------- Dither Routines

dith2       sty ybuf		; store line counter
            tay
            lda gr_btab,y	; apply balancing
            sta bbuf
            jsr makebits	; dither pixel
            ora buf16,x		; store data value to buffer
            sta buf16,x
            inc ycnt		; inc pattern pointer
            lda bbuf		; dither pixel
            jsr makebits
            ora buf16+1,x
            sta buf16+1,x
            dec ycnt		; dec pattern pointer
            lsr bitcnt		; move mask 2 bits to the right
            lsr bitcnt
            ldy ybuf		; restore line counter
            rts
;
; ----------------------------------------- Dither Subroutines

makebits    jmp (dmode)
;
dithoff     cmp #8
            bpl do0
            lda #0
            .by $2c
do0         lda bitcnt
            rts
;
dithrst     ldy ycnt
            ora offs,y
            tay
            lda gr_orderedpat,y
            and bitcnt
            rts
;
dithpat     asl
            asl
            asl
            ora ycnt
            tay
            lda gr_pattern,y
            and bitcnt
            rts
;
dithnoi     beq dn0
            cmp #15
            beq dn1
            ldy #0
            sta hold
            lda $d41b
            and #$0f
            cmp hold
            bcs dn3
            ldy #$aa
dn3         lda $d41b
            and #$0f
            cmp hold
            tya
            bcs dn2
            ora #$55
dn2         .by $2c
dn1         lda #$ff
            and bitcnt
dn0         rts
;
dithrnd     cmp #8
            beq dr0
            bcs dr1
            bcc dr2
dr0         lda $d41b
            .by $2c
dr1         lda bitcnt
            and bitcnt
            .by $2c
dr2         lda #0
ml4         rts
;
; ----------------------------------------- 

modend      .en

.petscii
.include "godotlib.lib"
.ob "l_Koala,p,w"
.ba $c000

; ---------------------------
;
;  ldr.koala
;  Loader for Koala Painter multicolor pictures
;
;    1.01, 03.19.91, bug fix: JiffyDOS compatibility
;    1.05, 16.07.98, 
;
; ---------------------------

;
; --------------------------- Equates

            .eq ftype		=$30
            .eq counter		=$31
            .eq gbyte		=$32
            .eq byte		=$33
            .eq cnt		=byte
            .eq vr		=$45
            .eq cr		=$47
            .eq status		=$90
            .eq offx		=$b2
            .eq offy		=offx+1

            .eq vrbuf		=$c600
            .eq crbuf		=vrbuf+1000
            .eq col0		=vrbuf+2000
            .eq sprite		=$d015

; --------------------------- Header

            jmp start
            .by $80
            .by $00,$00
            .by <(modend),>(modend)
            .by $00,$00
            .tx "Koala (even GG) "
            .tx "1.05"
            .tx "16.07.98"
            .tx "W.Kling/A.Dettke"

; --------------------------- Main

jerror      jmp error
;
start       lda #50		; activity counter to 50
            sta cntwert
            sta $ff
            ldx $20		; init counter and filetype to 0
            stx ftype
            stx counter
            ldy $21		; get start address
            stx sc_texttab
            sty sc_texttab+1
            lda ls_nambuf	; look for spades in name
            cmp #$c1
            bne stt1
            lda #$81		; replace (for GoDot)
stt1        sta ls_nambuf
            ldx $a7		; if namelength shorter than 16:
            cpx #16
            beq stt2
            lda #$2a		; add "*"
            sta ls_nambuf,x
            inc $a7
            jsr getname		; store filename
;
stt2        lda ls_nambuf	; name starts with "GG"?
            cmp #$47
            bne stt3
            cmp ls_nambuf+1
            bne stt3
            sta ftype		; then compressed Koala

stt3        jsr gd_xopen	; open file
            jsr onebyte		; skip start address
            bne jerror
            jsr onebyte
            bne jerror

            jsr gd_clrms	; clear message bar
            ldx #18		; "Bitmap"
            jsr tcopy

            lda #<(8000)	; count 8000 bytes (bitmap)
            sta ls_vekta8
            lda #>(8000)
            sta ls_vekta8+1
;
loop        jsr onebyte		; get bitmap
l1          bne jerror
            sta byte
            jsr action
            ldy #0
            ldx #4		; convert to 4bit indexes
bloop       lda #0
            asl byte
            rol
            asl byte
            rol
            sta (sc_texttab),y
            inc sc_texttab
            bne sk
            inc sc_texttab+1
sk          dex
            bne bloop
            lda ls_vekta8
            bne sk1
            dec ls_vekta8+1
sk1         dec ls_vekta8
            lda ls_vekta8
            ora ls_vekta8+1
            beq sk2
            lda status
            beq loop
;
sk2         lda #<(2000)	; count 2000 bytes (colors)
            sta ls_vekta8
            lda #>(2000)
            sta ls_vekta8+1
            lda #<(vrbuf)
            sta sc_texttab
            lda #>(vrbuf)
            sta sc_texttab+1
;
            lda #13		; out: "Colors"
            sta cntwert
            sta $ff
            ldx #0
            jsr tcopy
;
loop1       jsr onebyte		; get color RAMs
            sta (sc_texttab),y
            jsr action
            ldy #0
            inc sc_texttab
            bne sk3
            inc sc_texttab+1
sk3         lda ls_vekta8
            bne sk4
            dec ls_vekta8+1
sk4         dec ls_vekta8
            lda ls_vekta8
            ora ls_vekta8+1
            beq sk5
            jmp loop1
;
sk5         ldy #0		; get background color
            jsr onebyte
            and #15
            sta (sc_texttab),y

            jsr gd_xclose	; close file

            lda #<(vrbuf)	; convert to 4bit (set start addresses)
            sta vr
            lda #>(vrbuf)
            sta vr+1
            lda #<(crbuf)
            sta cr
            lda #>(crbuf)
            sta cr+1
            lda #<($4000)
            sta sc_texttab
            lda #>($4000)
            sta sc_texttab+1
            lda #<(1000)	; count 1000
            sta ls_vekta8
            lda #>(1000)
            sta ls_vekta8+1
;
            lda #200
            sta cntwert
            sta $ff
            ldx #9		; out: "Convert"
            jsr tcopy
;
loop2       lda (vr),y		; convert video RAM colors to 4bit
            pha
            lsr
            lsr
            lsr
            lsr
            sta col0+1
            pla
            and #$0f
            sta col0+2
            lda (cr),y		; convert color RAM colors to 4bit
            and #$0f
            sta col0+3
            lda #32
            sta cnt
bloop1      jsr action
            ldy #0
            lda (sc_texttab),y	; get values from 4bit (0-3)
            tax
            lda col0,x		; get color values from table
            tax
            lda dnib,x		; get conversion values to GoDot palette from table
            sta (sc_texttab),y	; write back to 4bit (double nibbles)
            inc sc_texttab	; advance
            bne sk6
            inc sc_texttab+1
sk6         dec cnt		; one tile
            bne bloop1
            inc vr		; next tile
            bne sk7
            inc vr+1
sk7         inc cr
            bne sk8
            inc cr+1
sk8         lda ls_vekta8	; 1000 tiles
            bne sk9
            dec ls_vekta8+1
sk9         dec ls_vekta8
            lda ls_vekta8
            ora ls_vekta8+1
            bne loop2

; --------------------------- Close File

            ldx #18		; reset text to default ("Bitmap")
            jsr tcopy
;
sk10        jsr gd_xmess
            jsr setinfo
            lda #$03
            sta sprite
            sec
            rts
; --------------------------- Subroutines

onebyte     lda counter
            bne getit2
            inc counter
            jsr basin
            cmp #$fe
            bne getit1
            bit ftype
            bvc getit1
            jsr basin
            pha
            jsr basin
            sta counter
            pla
getit1      sta gbyte
getit2      dec counter
            lda gbyte
            ldx $90
            rts
;
error       jsr gd_xclose
            jsr sk10
            clc
            rts

; --------------------------- Activity Display


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
action      dec $ff
            bne ld4
            lda cntwert
            sta $ff
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
dnib        .by $00,$ff,$44,$cc,$55,$aa,$11,$dd
            .by $66,$22,$99,$33,$77,$ee,$88,$bb
;
;
filltab     .by 160,93,103,127,126,124,105,109
;
cntwert     .by 50
;
txt         .by " Colors @"
            .by " Convert@"
            .ts " Bitmap @"
;
message     .ts " Bitmap    "
mess        .tx "                     "
            .by 0
;
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
setinfo     jsr getdatac
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
            ldx #<(iloader)
            ldy #>(iloader)
            bne si3
setmode     lda #25
            ldx #<(imode)
            ldy #>(imode)
            bne si3
setdata     lda #33
            ldx #<(idrive)
            ldy #>(idrive)
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
            .by 75,15,1,12,1,32,32,0
modetx      .by $31,$36,$30,24,$32,$30,$30,0
datatype    .by 67,15,12,15,18,0
ttext       .by 84,5,24,20,32,32,32
htext       .by 72,9,18,5,19,32,32
mtext       .by $31,$36,$30,24,$32,$30,$30
dtextc      .by 67,15,12,15,18
dtextg      .by 71,18,5,25,32
modend      .en

.petscii
.include "godotlib.lib"
.ob "mdrawmask,p,w"
.ba $c000

; ------------------------------------------------------------ 
;
; mod.DrawMask
; Module to paint a mask onto a picture
;
;   1.00, 28.03.02: first release
;   1.01, 30.03.02: changed the brush shapes
;                   removed brush shadow
;                   added brush size 1 (1 pixel)
;   1.02, 30.03.02: brush cut on right third fixed
;   1.03, 30.03.02: inelegant handling of switching on graphics fixed
;
; ------------------------------------------ 

            jmp start
            .by $20
            .by $00,$00
            .wo modend
            .by $00,$00
            .tx "Draw Mask on pic"
            .tx "1.03"
            .tx "30.03.02"
            .tx "A. Dettke       "
;
; ------------------------------------------ 

            .eq src		=$30	; /$31
            .eq dst		=$32	; /$33
            .eq dst0		=$34	; /$35
            .eq linenr		=$36
            .eq blcnt		=$37
            .eq bcnt		=$38
            .eq btcnt		=$39
            .eq lcnt		=$3a
            .eq xbuf		=$3b
            .eq byte1		=$3c
            .eq byte2		=$3d
            .eq byte3		=$3e
            .eq byte4		=$3f
            .eq sprx		=$40	; /$41
            .eq spry		=$42
            .eq stoa		=$43
            .eq bytev		=$44
            .eq byteh		=$45
            .eq offx		=$b2
            .eq offy		=$b3


            .eq spr11		=$02c0
            .eq sprx0		=$d000
            .eq spry0		=$d001
            .eq x2		=$d004
            .eq y2		=$d005
            .eq sprxmsb		=$d010
            .eq spren		=$d015
;
; --------------------------------------- Main

start       ldx sc_screenvek
            stx list
            ldx sc_screenvek+1
            stx list+1
            lda gr_redisp
            sta redisp
            lda $d027
            sta mscol
            jsr showreq
            jsr gd_eloop	; wait for clicks

; --------------------------------------- Event: Exit

evexit      sec
            rts

; --------------------------------------- Event: Select size

evsize: ldx bsize
        inx
        cpx #8
        bcc es0
        ldx #0
es0     stx bsize
        jsr setbrush
        inx
        txa
        ora #$30
        sta sizet+1
        lda #10
        ldx #24
        ldy #2
        jsr gd_setpar
        jsr gd_initmove
        ldx #<(sizet)
        ldy #>(sizet)
        jmp gd_xtxout2

; ----------------------------------------- Event: Switch Draw Mode

evmode: ldx pmode
        inx
        cpx #2
        bcc md0
        ldx #0
md0     stx pmode
        ldy drawoffs,x
        ldx #5
md1     lda drawtxt,y
        sta drawm,x
        dey
        dex
        bpl md1
        lda #12
        ldx #20
        ldy #6
        jsr gd_setpar
        jsr gd_initmove
        ldx #<(drawm)
        ldy #>(drawm)
        jmp gd_xtxout2

; ------------------------------------------ Event: Draw

ex1         jmp exit		; skip if not valid

evdraw      jsr swapspr
            lda #1
            sta spren
            ldx pmode
            lda brushcols,x
            sta $d027
            jsr view

drloop      jsr draw		; display graphics, get position
            ldx sc_stop
            bne ex1
            sty redisp

dr0         lda spry0		; Brush scanline minus 50 (to get graphics ordinate)
            cmp #250
            bcc dr3
            lda #250
            sta spry0
            bne ex1

dr3         sec
            sbc #50
            sta spry		; store
            bcs goon
            lda #50
            sta spry0
            bne ex1

goon        lda sprxmsb		; Brush x-position (hi)
            and #1
            sta sprx+1
            beq dr1
            ldx bsize
            lda rightborder,x
            clc
            adc sprx0
            cmp #91
            bcc dr1
            lda #91
            sbc rightborder,x
            sta sprx0
            bne ex1

dr1         lda sprx0		; and lo
            sec			; minus 24 (to get graphics ordinate)
            sbc #24
            sta sprx		; store
            bcs goon1
            dec sprx+1
            bpl goon1
            lda #24
            sta sprx0
            bne ex1
;
goon1       and #7		; bit in byte
            sta stoa
            lda sprx		; offset in line
            and #$f8
            sta sprx

            lda spry		; y-position of brush
            sta linenr
            lda #0
            sta xbuf
            lda #21
            sta lcnt
;
loop00      lda linenr		; compute bitmap address
            pha
            lsr
            lsr
            lsr
            tax
            pla
            and #$07
            clc
            adc line8lo,x
            tay
            lda line8,x
            pha

            tya
            clc
            adc sprx
            sta dst
            sta dst0
            pla
            adc sprx+1
            sta dst+1
            sta dst0+1

            ldx xbuf

            ldy #0
get0        lda (dst),y		; get bytes where brush was
            sta byte1,y
            clc
            lda dst
            adc #7		; next, take care for .y
            sta dst
            bcc skp0
            inc dst+1
skp0        iny
            cpy #4
            bne get0

            ldy stoa		; save first and last byte
            beq get8
            lda byte1
            and maskv,y
            sta bytev
            lda byte4
            and maskh,y
            sta byteh

get2        asl byte4		; shift 4 bytes left to edge
            rol byte3
            rol byte2
            rol byte1
            dey
            bne get2

get8        lda pmode
            bne get1

get6        lda spr11,x		; merge brush
            eor #$ff		; clear mode
            and byte1,y
            sta byte1,y
            inx
            iny
            cpy #3
            bne get6
            jmp get7

get1        lda spr11,x		; merge brush
            ora byte1,y		; draw mode
            sta byte1,y
            inx
            iny
            cpy #3
            bne get1

get7        ldy stoa		; shift back right
            beq get3
get4        lsr byte1
            ror byte2
            ror byte3
            ror byte4
            dey
            bne get4
            lda byte1
            ora bytev
            sta byte1
            lda byte4
            ora byteh
            sta byte4
            
get3        lda dst0
            sta dst
            lda dst0+1
            sta dst+1
get5        lda byte1,y		; write back to line
            sta (dst),y
            clc
            lda dst
            adc #7
            sta dst
            bcc skp1
            inc dst+1
skp1        iny
            cpy #4
            bne get5

            stx xbuf

            inc linenr		; next scanline
            lda linenr
            cmp #200
            beq exit
            dec lcnt
            beq exit
            jmp loop00
;
; ------------------------------------------ 

exit        lda sc_stop
            bne finish
            jmp drloop

finish      jsr swapspr
            lda #3
            sta spren
            jsr rp2
            lda redisp
            sta gr_redisp	; set auto render 
            lda mscol		; pointer white
            sta $d027

; --------------------------------------- Show Requester

showreq     ldx #<(pmasklst)	; display requester
            ldy #>(pmasklst)
            jsr gd_screen
            clc
            rts

; ------------------------------------------ 

line8       .by $20,$21,$22,$23,$25
            .by $26,$27,$28,$2a,$2b
            .by $2c,$2d,$2f,$30,$31
            .by $32,$34,$35,$36,$37
            .by $39,$3a,$3b,$3c,$3e

line8lo     .by $00,$40,$80,$c0,$00
            .by $40,$80,$c0,$00,$40
            .by $80,$c0,$00,$40,$80
            .by $c0,$00,$40,$80,$c0
            .by $00,$40,$80,$c0,$00

maskv       .by $00,$80,$c0,$e0,$f0,$f8,$fc,$fe
maskh       .by $ff,$7f,$3f,$1f,$0f,$07,$03,$01

; ------------------------------------------ 

spr7        .by $01,$fc,$00,$07,$ff,$00,$0f,$ff,$80,$1f,$ff,$c0,$3f,$ff,$e0
            .by $7f,$ff,$f0,$7f,$ff,$f0,$ff,$ff,$f8,$ff,$ff,$f8,$ff,$ff,$f8
            .by $ff,$ff,$f8,$ff,$ff,$f8,$ff,$ff,$f8,$ff,$ff,$f8,$7f,$ff,$f0
            .by $7f,$ff,$f0,$3f,$ff,$e0,$1f,$ff,$c0,$0f,$ff,$80,$07,$ff,$00
            .by $01,$fc,$00
;
; ------------------------------------------ 

spr6        .by $03,$e0,$00,$0f,$f8,$00,$3f,$fe,$00,$3f,$fe,$00,$7f,$ff,$00
            .by $7f,$ff,$00,$ff,$ff,$80,$ff,$ff,$80,$ff,$ff,$80,$ff,$ff,$80
            .by $ff,$ff,$80,$7f,$ff,$00,$7f,$ff,$00,$3f,$fe,$00,$3f,$fe,$00
            .by $0f,$f8,$00,$03,$e0,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00
;
; ------------------------------------------ 

spr5        .by $07,$80,$00,$1f,$e0,$00,$3f,$f0,$00,$7f,$f8,$00,$7f,$f8,$00
            .by $ff,$fc,$00,$ff,$fc,$00,$ff,$fc,$00,$ff,$fc,$00,$7f,$f8,$00
            .by $7f,$f8,$00,$3f,$f0,$00,$1f,$e0,$00,$07,$80,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00
;
; ------------------------------------------ 

spr4        .by $0e,$00,$00,$3f,$80,$00,$7f,$c0,$00,$7f,$c0,$00,$ff,$e0,$00
            .by $ff,$e0,$00,$ff,$e0,$00,$7f,$c0,$00,$7f,$c0,$00,$3f,$80,$00
            .by $0e,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00
;
; ------------------------------------------ 

spr3        .by $18,$00,$00,$7e,$00,$00,$7e,$00,$00,$ff,$00,$00,$ff,$00,$00
            .by $7e,$00,$00,$7e,$00,$00,$18,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00
;
; ------------------------------------------ 

spr2        .by $70,$00,$00,$f8,$00,$00,$f8,$00,$00,$f8,$00,$00,$70,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00
;
; ------------------------------------------ 

spr1        .by $e0,$00,$00,$e0,$00,$00,$e0,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00
;
; ------------------------------------------ 

spr0        .by $80,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
            .by $00,$00,$00

; ------------------------------------------ 

spr         .by $01,$fc,$00,$07,$ff,$00,$0f,$ff,$80,$1f,$ff,$c0,$3f,$ff,$e0
            .by $7f,$ff,$f0,$7f,$ff,$f0,$ff,$ff,$f8,$ff,$ff,$f8,$ff,$ff,$f8
            .by $ff,$ff,$f8,$ff,$ff,$f8,$ff,$ff,$f8,$ff,$ff,$f8,$7f,$ff,$f0
            .by $7f,$ff,$f0,$3f,$ff,$e0,$1f,$ff,$c0,$0f,$ff,$80,$07,$ff,$00
            .by $01,$fc,$00
;
; ------------------------------------------ 

list        .wo 0
redisp      .by 0
pmode       .by 1		; mode: 1=set 0=reset
drawoffs    .by 5,11
drawtxt     .ts "Clear  Draw "
brushcols   .by 13,10
mscol       .by 1
bsize       .by 7
rightborder .by 3,5,7,10,13,16,19,23
sprofflo    .by <spr0,<spr1,<spr2,<spr3,<spr4,<spr5,<spr6,<spr7
sproffhi    .by >spr0,>spr1,>spr2,>spr3,>spr4,>spr5,>spr6,>spr7

; ------------------------------------------ 

draw        lda #0
            sta sc_keyprs
            sta sc_stop
kl          lda sc_stop
            ora sc_keyprs	; wait for key
            beq kl
ok          rts
;
; ------------------------------------------ 

swapspr     ldy #62
ssl         ldx spr11,y
            lda spr,y
            sta spr11,y
            txa
            sta spr,y
            dey
            bpl ssl
            rts

; --------------------------------------- Setbrush

setbrush    lda sprofflo,x
            sta src
            lda sproffhi,x
            sta src+1
            ldy #62
sb0         lda (src),y
            sta spr,y
            dey
            bpl sb0
            rts

; ------------------------------------------ Redisplay routines

; ------------------------------------------ Switch Graphics/Text

view:   ldx #1
        stx gr_redisp

redis:  ldy #251
        lda #$f0
red0    sta $03ff,y
        sta $04f9,y
        sta $05f3,y
        sta $06ed,y
        dey
        bne red0
rp4:    lda #$1b
        sta $d018
        lda #$3b
        sta $d011
        lda gr_redisp    ; leave if flag set
        bne rp3

rp2:    jsr tmode
        ldx list
        ldy list+1
        jsr gd_screen
rp3:    clc
        rts

tmode:  ldx #$13
        lda #$1b
        stx $d018
        sta $d011
        lda #$08
        sta $d016
        rts

; --------------------------------------- Screenlist 

pmasklst    .by 0
            .by 7,13,14,11,$81,0,0
            .ts "DrawMask@"
sizeg       .by 9,23,4,3,$cf,<evsize,>evsize
sizet       .ts " 8@"
            .by 11,19,8,3,$ce,<evmode,>evmode
drawm       .ts " Draw @"
            .by 13,13,14,3,$cb,<(evdraw),>(evdraw)
            .ts " Draw Mask  @"
            .by 15,13,14,3,$cb,<(evexit),>(evexit)
            .ts "Leave@"
            .by $c0,9,13,9
            .ts "Brushsize@"
            .by $c0,11,13,4
            .ts "Mode@"
            .by $80

; --------------------------------------- 

modend         .en

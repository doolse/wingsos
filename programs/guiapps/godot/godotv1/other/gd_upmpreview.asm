.petscii

; ***---------------------------------------------------***
; ***                                                   ***
; ***  GoDot UpMem 1.14                                 ***
; ***  Preview Routines                                 ***
; ***                                                   ***
; ***---------------------------------------------------***
;

; Declarations

            .eq vekt20      =$20
            .eq pos         =$a4
            .eq loop        =$a6
            .eq lflag       =$a8
            .eq brttmp      =$a9
            .eq crttmp      =$aa
            .eq screentab   =$ab
            .eq keypressed  =$c6

            .eq sprptr      =$07fa

            .eq initmove    =$1090
            .eq invert      =$1125

            .eq spr2        =$3e80
            .eq spr5        =$3f40

            .eq dbuf	    =$bd00

            .eq sprxy       =$d004
            .eq xmsb        =$d010
            .eq sprreg      =$d015
            .eq yexp        =$d017
            .eq sprpri      =$d01b
            .eq sprmulti    =$d01c
            .eq xexp        =$d01d
            .eq sprcol2     =$d025
            .eq sprcol3     =$d026
            .eq sprcol1     =$d029


;
; All GoDot modules run at $c000 (Execution Area)
; but note that this is not the true location of Preview
;
            .ba $c000
;
; -------------------------------------------------
; When entering this event routine you have just clicked the 
; appropriate gadget. So, all values are still set here and now.
; The event routine first reverts the gadget's background.
;

evprviu     jsr initmove	; compute screen address
            pha			; A-reg contains vekt20+1
            lda vekt20
            pha
            jsr invert		; reverting to black

            ldx #$c0		; make space for three sprites...
            ldy #$00
loop1       lda spr2-1,x
            sta dbuf-1,x
            tya
            sta spr2-1,x	; and initialize to $00
            dex
            bne loop1

            ldx #$c0		; initialize three more sprites
loop2       sta spr5-1,x
            dex
            bne loop2

            sta sprpri		; priority
            sta xexp		; no expansion
            sta yexp

            lda #$fc		; sprites 2 to 7: multi mode
            sta sprmulti

            ldx #$05		; colorize: mid gray
            lda #$0c
loop3       sta sprcol1,x
            dex
            bpl loop3
            lda #$0b		; dark gray
            sta sprcol2
            lda #$0f		; light gray
            sta sprcol3

            ldx #$0b		; define positions (in gadget)
loop4       lda xytab,x
            sta sprxy,x
            dex
            bpl loop4
            lda xmsb		; beyond 255
            ora #$fc
            sta xmsb

            ldx #$05		; activate sprite pointers
            ldy #$ff
ploop       tya
            sta sprptr,x
            dey
            dex
            bpl ploop

            lda #$ff		; switch sprites on
            sta sprreg
;
; Now rendering the 4Bit Area to the sprites. Will be done as three
; bands of 12 tiles down from left to right.
; 
            ldx #$02		; three times 12 tiles (20 rows down)
m0loop      txa
            pha
            lda srcl,x		; set source address (4Bit)
            sta pos
            lda srch,x
            sta pos+1
            lda dstl,x		; set destination address (sprites)
            sta loop
            lda dsth,x
            sta loop+1
            lda #$00		; counter to skip byte 63 in sprite
            sta screentab
            jsr makeit		; render one band
            pla
            tax
            dex
            bpl m0loop		; and the next two
;
; now rendered, wait for click to shut down sprites
; 
            inx			; wait for click
            stx keypressed
wait        lda keypressed
            beq wait
;
            lda #3		; only sprites 0+1 remain (mouse)
            sta sprreg
            lda xmsb		; force to lower 256 position
            and #$03		; (sometimes flickers, can't imagine why)
            sta xmsb

            pla			; restore gadget address
            sta vekt20
            pla
            sta vekt20+1
            jsr invert		; revert to blue again

            ldx #$c0		; restore rendered image from buffer
piu0        lda dbuf-1,x
            sta spr2-1,x
            dex
            bne piu0

            clc			; don't leave screenlist
            rts
;------------------------------------------------------------------
;
; rendering 4Bit Area to 6 sprites
;
;------------------------------------------------------------------
makeit      ldx #$00		; MUST be $00!
            ldy #$14		; 20 rows of tiles down

prloop      tya			; start preview loop
            pha
            lda #$40		; flag: %0100 0000, 2 passes per tile
            sta lflag

dzloop      lda #$03		; flag: 3 passes across
            sta brttmp

zloop       ldy #$60		; 4 tiles from right to left
            lda lflag		; on 2nd pass (tile): 
            bpl bloop
            ldy #$70		; indent to 4 pixels below

bloop       lda (pos),y		; get 4bit data
            lsr
            lsr
            lsr
            ror crttmp
            lsr
            ror crttmp

            tya			; next tile (to the left)
            sec
            sbc #$20
            tay
            bpl bloop

            lda crttmp		; byte complete, write to sprites
            sta (loop,x)

skip64      inc loop		; increment destination pointer
            bne s1
            inc loop+1

s1          inc screentab	; skip byte 63
            lda screentab
            cmp #$3f
            beq skip64

            lda pos		; add to 4 tiles ahead
            clc			; (summing up to 12 tiles at last)
            adc #$80
            sta pos
            bcc skip
            inc pos+1

skip        ror brttmp		; loop for next two passes across
            bcs zloop

            lda pos		; 3 times 4 tiles backwards
            sec
            sbc #$80
            sta pos
            lda pos+1
            sbc #$01
            sta pos+1

            rol lflag		; loop for second pass within tile
            bcc dzloop

            lda pos		; add $500 (1280): next row of tiles
            clc
            adc #$00
            sta pos
            lda pos+1
            adc #$05
            sta pos+1

            pla			; count 20 times
            tay
            dey
            bpl prloop
            rts			; until finished
;-------------------------------------------------------------
; Data (screen positions and so on)
;-------------------------------------------------------------
srcl        .by $40,$c0,$40	; not from leftmost position!
srch        .by $4d,$4b,$4a

dstl        .by $80,$00,$80	; sprite block addresses
dsth        .by $3f,$3f,$3e

xytab       .by $07,$91,$07,$a6,$1f,$91,$1f,$a6
            .by $37,$91,$37,$a6
            .by $00

modend      .en

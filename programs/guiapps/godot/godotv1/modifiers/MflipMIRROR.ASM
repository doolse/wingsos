.petscii
.ob "mflipMir,p,w"
.ba $c000
.include "godotlib.lib"
;
; ----------------------------------- 
; Module Flip'n'Mirror
; 1.00: 13.04.01 - first release
; 1.01: 15.04.01 - added automatical clip accommodation
; ----------------------------------- 
;

            .eq src		=$30	; /$31
            .eq bcnt		=$32
            .eq bcnt0		=$33
            .eq lcnt8		=$34
            .eq dst		=$35	; /$36
            .eq src0		=$37	; /$38
            .eq src00		=$39	; /$3a
            .eq dst0		=$3b	; /$3c
            .eq dst00		=$3d	; /$3e
            .eq clpzl		=$3f
            .eq clpsp		=$40
            .eq clpwd		=$41	; /$42
            .eq bytecnt		=$43	; /$44
            .eq pass		=$45
            .eq adcnt		=$46
            .eq lcnt		=$47

            .eq spriteptr	=$07f8
            .eq sprite255	=$3fc0

            .eq swaptable	=$c500
            .eq mirrorbuf	=swaptable+256

            .eq spritex2	=$d004
            .eq spritey2	=spritex2+1
            .eq spriteen	=$d015
            .eq spritexe	=$d01d
            .eq spritecol2	=$d029

; ----------------------------------- 

header      jmp start
            .by $20
            .by 0
            .by 0
            .wo modend
            .wo 0
            .tx "4Bit Flip&Mirror"
            .tx "1.01"
            .tx "15.04.01"
            .tx "A.Dettke        "

; ----------------------------------- 
;
start       jsr makeswap	; init mirroring
            dex
            stx nclipzl		; init new clip position values
            stx nclipsp
            ldx #<fliplst	; activate requester
            ldy #>fliplst
            jsr gd_screen
            jsr clipfull	; determine which mode
            jsr gd_eloop	; wait for events
endmod      lda #$ff		; graphics must be rendered
            sta gr_redisp
leave       sec			; module finished
            rts

; ----------------------------------- 

initad      ldy #60		; create bright bar
            lda #0
adl0        sta sprite255+3,y
            dey
            bpl adl0
            sty sprite255
            sty sprite255+1
            sty sprite255+2

            lda #15		; light gray
            sta spritecol2
            sta spritecol2+1
            lda spritexe	; expand x
            ora #12
            sta spritexe

            lda spritehi	; sprites 2 & 3 beyond 255
            ora #12
            sta spritehi
            lda #8		; x position: 33 & 36
            sta spritex2
            lda #32
            sta spritex2+2
            lda #144		; y position: 18
            sta spritey2
            sta spritey2+2

            lda #$ff		; sprite 2 & 3 at area 255 ($3fc0)
            sta spriteptr+2
            sta spriteptr+3

            lda spriteen	; activate sprites
            ora #12
            sta spriteen
            rts

; ----------------------------------- 

clearad     lda spriteen	; display mouse pointer only
            and #$f3
            sta spriteen
            lda spritehi
            and #$f3
            sta spritehi

            lda clipfl		; any clip?
            bne hf2		; yes, finish

            lda nclipzl		; Flip executed?
            bmi hf3		; no
            sta sc_lastclpzl
hf3         lda nclipsp		; Mirror executed?
            bmi hf2		; no
            sta sc_lastclpsp
hf2         jsr toclip
            sec
            rts

; ----------------------------------- Create Mirror Table

makeswap    ldx #0		; count 0 to 255
swl0        txa
            sta swaptable,x
            ldy #4		; exchange nibbles (eg. $01 to $10)
swl1        asl
            rol swaptable,x
            dey
            bne swl1
            inx
            bne swl0
            rts
; ---------------------------------------- Compute Start address of Clip

setclip     lda #>sy_4bit	; 4Bit starting from $4000
            sta src0+1
            ldy #<sy_4bit
            sty src0

            lda clipfl		; any clip set?
            bne stcn		; yes, change values

            jsr tofull
            bne stcl

stcn        lda sc_lastclpzl
            sta sc_clipzl
            lda sc_lastclpsp
            sta sc_clipsp
            lda sc_lastclpho
            sta sc_clipho
            lda sc_lastclpbr
            sta sc_clipbr

stcl        lda sc_clipzl	; row of current clip
            beq scp1		; is zero: skip
            sta clpzl
scp4        clc
            lda src0+1		; else add 1280, "row" times
            adc #5
            sta src0+1
            dec clpzl
            bne scp4

scp1        lda sc_clipsp	; column
            beq scp5		; is zero: skip
            sta clpsp
scp6        clc
            lda src0		; else add 32, "column" times
            adc #$20
            sta src0
            bcc scp7
            inc src0+1
scp7        dec clpsp
            bne scp6
scp5        rts

; ----------------------------------- Compute destination

compdst     ldx sc_clipho	; HO minus 1
            lda src0+1
            dex
            beq cd0
cd1         clc			; times 1280
            adc #5
            dex
            bne cd1
cd0         sta dst0+1
            lda src0
            clc
            adc #28		; plus 28
            sta dst0		; result in vector dst0
            rts

; ----------------------------------- Compute passes

passes      lda sc_clipho	; passes are half the height times 8
            asl
            asl
            rts

; ---------------------------------------- Compute Width of Clip

compclip    sty ls_vekta8+1	; Hibyte to zero
            clc			; Lobyte times 32
            lda sc_clipbr	; get width of current clip
            sta ls_vekta8
            asl ls_vekta8
            rol ls_vekta8+1
            asl ls_vekta8
            rol ls_vekta8+1
            asl ls_vekta8
            rol ls_vekta8+1
            asl ls_vekta8
            rol ls_vekta8+1
            asl ls_vekta8
            rol ls_vekta8+1
            rts

; ----------------------------------- Count write bytes

cou6        lda bytecnt		; number of bytes
            bne cou7
            dec bytecnt+1
cou7        dec bytecnt
            lda bytecnt
            ora bytecnt+1
            rts

; ----------------------------------- Set Clip values to Full

tofull      lda #0
            sta sc_clipzl	; values to 0,0,40,25
            sta sc_clipsp
            lda #25
            sta sc_clipho
            lda #40
            sta sc_clipbr
            rts

; ----------------------------------- Set Clip values back to Clip

toclip      lda sc_lastclpzl
            sta sc_clipzl
            lda sc_lastclpsp
            sta sc_clipsp
            lda sc_lastclpho
            sta sc_clipho
            lda sc_lastclpbr
            sta sc_clipbr
            rts

; ----------------------------------- Determine which Mode

clipfull    lda sc_clipped
            sta clipfl
            bne cf0
            jmp setfullm
cf0         lda #1
            sta clipfl
            jmp setclipm

; ----------------------------------- Event Mirror Image horizontally

evmirror    lda #5		; set progression counter
            sta adcnt
            jsr initad		; init activity display
            jsr setclip		; source: start of clip
            lda src0
            ldx src0+1
            sta src00
            stx src00+1
            sta src
            stx src+1
            jsr compclip	; compute width of clip
            lda ls_vekta8
            ldx ls_vekta8+1
            sta bytecnt
            stx bytecnt+1
            sec
            sbc #29
            sta clpwd
            bcs swi0
            dex
swi0        stx clpwd+1

            ldy #0		; .y is index into data
            lda sc_clipho	; count tile rows (height of clip)
            sta lcnt8

l0          clc			; destination: end of tile row
            lda #<mirrorbuf
            adc clpwd
            sta dst0
            sta dst
            lda #>mirrorbuf
            adc clpwd+1
            sta dst0+1
            sta dst+1

            lda #8		; count 8 lines per row
            sta lcnt
l1          lda sc_clipbr	; count tiles (width of clip)
            sta bcnt0
l2          lda #4		; count 4 bytes (eq 8 pixel or 1 tile line)
            sta bcnt

l3          lda (src),y		; get first 2 pixels
            tax
            lda swaptable,x	; mirror them
            sta (dst),y		; and store to destination position

            inc src		; proceed in data: source
            bne s0
            inc src+1
s0          lda dst
            bne s1
            dec dst+1		; backwards proceed in data: destination
s1          dec dst
            dec bcnt		; 1 tile line?
            bne l3

            lda src		; yes, proceed to next tile
            clc
            adc #28
            sta src
            bcc s2
            inc src+1
s2          lda dst		; backwards at destination
            sec
            sbc #28
            sta dst
            bcs s3
            dec dst+1
s3          dec bcnt0		; all tiles of a line processed?
            bne l2

            lda src0		; yes, proceed to next tile line
            clc
            adc #4
            sta src0
            bcc s4
            inc src0+1
s4          lda dst0		; same at destination
            clc
            adc #4
            sta dst0
            bcc s5
            inc dst0+1
s5          lda src0		; set new vectors
            ldx src0+1
            sta src
            stx src+1
            lda dst0
            ldx dst0+1
            sta dst
            stx dst+1

            dec adcnt		; display activity counter
            bne s6
            lda #5		; reset counter if down
            sta adcnt
            inc spritey2	; then advance bar
            inc spritey2+2
s6          dec lcnt		; all tile lines processed?
            bne l1

            lda #<mirrorbuf	; yes, write processed buffer back to 4bit
            ldx #>mirrorbuf
            sta src
            stx src+1
            lda src00
            ldx src00+1
            sta dst
            stx dst+1

s7          lda (src),y
            sta (dst),y
            inc src
            bne s8
            inc src+1
s8          inc dst
            bne s9
            inc dst+1
s9          jsr cou6
            bne s7

            lda ls_vekta8
            ldx ls_vekta8+1
            sta bytecnt
            stx bytecnt+1

            lda src00		; and proceed to next tile row
            clc
            adc #<(1280)
            sta src
            sta src0
            sta src00
            lda src00+1
            adc #>(1280)
            sta src+1
            sta src0+1
            sta src00+1

            dec lcnt8		; all tile rows processed?
            beq hfok		; yes, finished
            jmp l0		; no, loop

hfok        lda clipfl		; any clip?
            bne hf0		; yes, finish

            sec			; else: 
            lda #40		; compute new clip column
            sbc sc_lastclpsp
            sbc sc_lastclpbr
            sta nclipsp
hf0         jmp clearad		; finish

; ----------------------------------- Event Flip vertically

evflip      lda #3		; set progression counter
            sta adcnt
            jsr initad		; init activity display
            jsr setclip
            lda src0
            ldx src0+1
            sta src
            stx src+1
            sta src00
            stx src00+1
            jsr compdst
            lda dst0
            ldx dst0+1
            sta dst00
            stx dst00+1
            sta dst
            stx dst+1
            jsr passes
            sta pass

            ldy #0
sil5        lda #8
            sta lcnt8

sil2        lda sc_clipbr	; count width of clip (# of tiles)
            sta bcnt0

sil1        lda #4		; count bytes per tile line 
            sta bcnt

sil0        lda (src),y		; exchange data
            tax
            lda (dst),y
            sta (src),y
            txa
            sta (dst),y

            inc src		; increment source
            bne sis0
            inc src+1
sis0        inc dst		; increment destination
            bne sis1
            inc dst+1
sis1        dec bcnt		; 1 tile
            bne sil0

            lda src		; proceed to next tile
            clc
            adc #28
            sta src
            bcc sf0
            inc src+1
sf0         lda dst
            clc
            adc #28
            sta dst
            bcc sf1
            inc dst+1
sf1         dec bcnt0		; decrement tile counter
            bne sil1		; 1 line finished

            dec adcnt		; display activity counter
            bne sis2
            lda #3		; reset counter if down
            sta adcnt
            inc spritey2	; then advance bar
            inc spritey2+2

sis2        lda src0		; then proceed to next line in tile
            clc
            adc #4
            sta src0
            sta src
            bcc sf2
            inc src0+1
sf2         lda dst0		; destination either (backwards)
            sec
            sbc #4
            sta dst0
            sta dst
            bcs sil3
            inc dst0+1
sil3        lda src0+1
            sta src+1
            lda dst0+1
            sta dst+1

            dec pass		; bis alle durch
            beq finish

            dec lcnt8
            beq sil4
            jmp sil2

sil4        lda src00+1
            clc
            adc #5
            sta src00+1
            sta src0+1
            sta src+1
            lda src00
            sta src0
            sta src
            lda dst00+1
            sec
            sbc #5
            sta dst00+1
            sta dst0+1
            sta dst+1
            lda dst00
            sta dst0
            sta dst
            jmp sil5

finish      lda clipfl		; any clip?
            bne hf1		; yes, finish

            sec			; else: 
            lda #25		; compute new clip row
            sbc sc_lastclpzl
            sbc sc_lastclpho
            sta nclipzl
hf1         jmp clearad		; finish

; ----------------------------------- Event UpsideDown

evboth      jsr evmirror
            jsr evflip
            jmp clearad

; ----------------------------------- Event Select Mode

evselmode   lda clipfl
            eor #1
            sta clipfl
            beq setfullm
setclipm    ldx #3
            .by $2c
setfullm    ldx #7
            ldy #3
mx1         lda cliptxt,x
            sta cliptx,y
            dex
            dey
            bpl mx1
            ldx #<(clip)
            ldy #>(clip)

; --------------------------------------- Textout

settext     stx sc_screentab
            sty sc_screentab+1
            lda #0
            tax
            tay
gt0         jsr gd_setpos
            cpx #3
            bne gt0
            jsr gd_trim
            jsr gd_initmove
            ldy #6
            jmp gd_xtxout1


; ----------------------------------- Variables and Constants

cliptxt     .ts "ClipFull"
clipfl      .by 0
nclipzl     .by 0
nclipsp     .by 0

; ----------------------------------- 

fliplst     .by 0
            .by 5,10,19,14,$81
            .wo 0
            .ts "Flip'n'Mirror@"

clip        .by 7,23,6,3,$cf
            .wo evselmode
cliptx      .ts "Clip@"

            .by 10,10,19,3,$ca
            .wo evmirror
            .ts "Mirror@"
            .by 12,10,19,3,$cb
            .wo evflip
            .ts "Flip@"
            .by 14,10,19,3,$cb
            .wo evboth
            .ts "UpsideDown@"
            .by 16,10,19,3,$cb
            .wo leave
            .ts "Leave@"

            .by $c0,7,10,12
            .ts "Select Area:@"
            
            .by $80

; ----------------------------------- 

modend      .en

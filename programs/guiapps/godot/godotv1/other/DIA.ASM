.petscii
; IFLI-Diashow - 27.07.94
;
; .li 1,4,7
;         .ob "diashow,p,w"
;
;
            .ba $0801
sysline     .by 1,1,$ad,0,$9e,$32,$30,$36,$35,58,$24,$41,$44,0,0,0
;
;  performs a "JMP DIASHOW"
;
; ------------------------------------ definitions
            .eq piclist=$9400
            .eq setnam=$ffbd
            .eq setlfs=$ffba
            .eq open=$ffc0
            .eq close=$ffc3
            .eq chkin=$ffc6
            .eq clrch=$ffcc
            .eq basin=$ffcf
            .eq bsout=$ffd2
            .eq talk=$ffb4
            .eq sektlk=$ff96
            .eq getin=$ffa5
            .eq untalk=$ffab
            .eq strout=$ab1e
            .eq plot=$fff0
            .eq rahmen=$d020
            .eq backgr=$d021
;
            .eq vekt=$59
            .eq sekadr=$b9
            .eq drive=$ba
            .eq status=$90
            .eq qflag=$5b
            .eq index=$5c
            .eq cnt8=$5d
            .eq mbyte=$5e
            .eq mvekt=$5f
            .eq nambuf=$0100
            .eq flen=$f8
            .eq cnt=$f9
            .eq index=$fa
            .eq formok=$fb
            .eq mlen=$fd
            .eq pport=$01
; ------------------------------------ 
; ------------------------------------ Main program loop
; ------------------------------------ 
diashow     jsr logodis			; display logo ("GoDotGraphics")
            lda #0			; init list of pictures
            sta piclist
            lda #$30			; init error display
            sta errnum
            sta errnum+1
            jsr dirlist			; get list of pictures

            lda piclist			; empty?
            beq nextdisk		; yes, prompt for next disk

            jsr setplist		; set pointer to liststart
show        jsr logodis			; display logo again
            jsr getfname		; get next filename
            bne ok			; end of list?

nextdisk    jsr next			; yes, prompt for next disk...
            jmp diashow			; and loop
; ------------------------------------ 
ok          jsr fname			; show filename ("LOADING: ...")
            jsr prepact			; prepare activity display
            jsr loadf			; load picture file
            lda formok			; correct format?
            bne show			; no, skip to next file

            jsr display			; otherwise display picture...
            jmp show			; and loop
; ------------------------------------ 
; ------------------------------------ Subroutines
; ------------------------------------ 
next        jsr logodis			; display logo
            clc				; display message ("Next disk...")
            ldx #3
            ldy #9
            jsr plot
            lda #<(ndisk1)
            ldy #>(ndisk1)
            jsr strout
            clc
            ldx #5
            ldy #14
            jsr plot
            lda #<(ndisk2)
            ldy #>(ndisk2)
            jsr strout
            clc
            ldx #8
            ldy #8
            jsr plot
            lda #<(ndisk3)
            ldy #>(ndisk3)
            jsr strout
; ------------------------------------ Waitkey
wait        lda $91			; wait for:
            cmp #$7f			; STOP
            beq reset
            cmp #$ef			; SPACE
            bne wait
            rts
; ------------------------------------ STOP 
reset       jmp $fce2			; reset C64
; ------------------------------------ Init vector
setplist    ldy #<(piclist)		; set vector to list of pictures
            lda #>(piclist)
            sty vekt
            sta vekt+1
            rts
; ------------------------------------ Get Filename
getfname    ldx #15			; clear last name
            lda #32
gf1         sta namout,x
            dex
            bpl gf1

            ldy #0			; get length of name
            lda (vekt),y
            beq gfend			; if $00, finished
            sta flen			; store

            inc vekt			; get filename
            bne gf0
            inc vekt+1
gf0         lda (vekt),y
            sta nambuf,y		; store
            sta namout,y
            iny
            cpy flen
            bne gf0

            clc				; point vector to next name
            lda vekt
            adc #16
            sta vekt
            bcc gfend
            inc vekt+1
gfend       rts
; ------------------------------------ LOADING
fname       clc
            ldx #17
            ldy #8
            jsr plot
            lda #<(name)
            ldy #>(name)
            jmp strout
; ------------------------------------ Activity display
action      lda $91			; check for STOP
            cmp #$7f
            bne ac2

            lda #$37 			; if STOP:
            sta pport
            pla
            pla
            jmp wrong			; set flag, leave

ac2         dec cnt			; skipcounter finished?
            bne noaction		; no, no action

            lda mlen			; maskcounter finished?
            beq noaction		; yes, no action

            lda cntwert			; re-set skipcounter
            sta cnt

            lsr cnt8			; bitcounter finished?
            bcc ac0			; no, skip

            ror cnt8			; re-set bitcounter
            dec mlen			; (decrement maskcounter)
            beq noaction		; (if $00, skip)
            inc index			; increment mask index

            ldy index			; get next byte from mask
            lda (mvekt),y
            sta mbyte			; and store

ac0         lda pport			; switch kernelrom on
            pha
            lda #$36
            sta pport
            asl mbyte			; what to display?
            bcs ac1
            lda #$a3			; a) underscore
            .by $2c
ac1         lda #$b8			; b) other character
            jsr bsout
            sei				; switch kernelrom off again
            pla
            sta pport
            ldy #0			; must be zero
noaction    rts
; ------------------------------------ Prepare activity display
prepact     ldx #<(maske)		; point vector to mask definition
            ldy #>(maske)
            stx mvekt
            sty mvekt+1

            ldy #0			; get counter
            lda (mvekt),y
            sta mlen			; and store (+1)
            inc mlen

            inc mvekt			; point to 1. byte
            bne pa0
            inc mvekt+1
pa0         lda (mvekt),y		; and get it
            sta mbyte

            lda cntwert			; set skipcounter
            sta cnt
            dey				; init index into mask
            sty index
            lda #1			; init bitcounter
            sta cnt8

            clc
            ldx #1
            ldy #0
            jsr plot
            lda #<(home)		; init display
            ldy #>(home)
            jmp strout
; ------------------------------------ Create list of pictures
dirlist     jsr setplist		; set pointer to list
            lda #13			; OPEN 13,drive,1,"$,r"
            ldx drive
            jsr setlfs
            lda #1
            ldx #<(dollar)
            ldy #>(dollar)
            jsr setnam
            jsr open
            ldx #13
            jsr chkin

            lda #0			; successful?
            sta status
            jsr basin
            lda status
            beq dir

            jsr enddir			; no, end with message
            jmp meldung
;
dir         jsr basin			; read directory of disk
            jsr basin
            jsr basin
loop        jsr basin
            jsr basin
d0          jsr basin
            cmp #32
            beq d0
            ldy #0
            sty qflag
            iny
            ldx #16
            bne d2
d3          eor qflag
            beq d4
            sta qflag
d1          jsr basin

d2          cmp #$22			; store what is between quotes
            beq d3
            cmp #$12
            beq d1
            sta (vekt),y
            iny
            dex
            bne d1
d4          dey
            tya
            ldy #0
            sta (vekt),y
            iny
            lda (vekt),y
            cmp #$5e			; double up-arrow? 
            beq d6
            dey
            tya
            sta (vekt),y
            beq d5
d6          iny				;(This must be quite different now)
            eor (vekt),y
            bne d5

            lda vekt			; move vector to next entry
            clc
            adc #17
            sta vekt
            bcc d5
            inc vekt+1

d5          jsr basin			; skip till zero
            tax
            bne d5
            jsr basin			; skip length entry
            jsr basin
            lda status			; still ok?
            bne enddir
            beq loop			; yes, next entry

enddir      jsr clrch			; CLOSE 13
            lda #13
            jmp close
; ------------------------------------ Display "GoDotGraphics"
logodis     lda #0
            sta rahmen
            sta backgr
            lda #$93			; Clear screen
            jsr bsout
            clc
            ldy #0
            ldx #19
            jsr plot
            lda #<(logo)
            ldy #>(logo)
            jmp strout
; ------------------------------------ Logo data
; ------------------------------------ characters for "GoDotGraphics"
logo        .by $8e,$08,$96,$92
            .by $d5,$c0,$c0,$c9,$20,$20,$20,$b0
            .by $c0,$c0,$c9,$20,$20,$20,$20,$20
            .by $d5,$c0,$c0,$c9,$20,$20,$20,$20
            .by $20,$20,$20,$20,$20,$c9,$20,$20
            .by $20,$2e,$20,$20,$20,$20,$20,$20
;
            .by $dd,$c0,$c0,$ae,$d5,$c0,$c9,$dd
            .by $20,$20,$dd,$d5,$c0,$c9,$db,$c0
            .by $dd,$c0,$c0,$ae,$b0,$c0,$c9,$d5
            .by $c0,$ae,$b0,$c0,$c9,$ab,$c0,$c9
            .by $c0,$ae,$d5,$c0,$c9,$d5,$c0,$c9
;
            .by $dd,$20,$20,$dd,$dd,$20,$dd,$dd
            .by $20,$20,$dd,$dd,$20,$dd,$dd,$20
            .by $dd,$20,$20,$dd,$dd,$20,$20,$dd
            .by $20,$dd,$dd,$20,$dd,$dd,$20,$dd
            .by $20,$dd,$dd,$20,$20,$ca,$c0,$c9
;
            .by $ca,$c0,$c0,$cb,$ca,$c0,$cb,$ad
            .by $c0,$c0,$cb,$ca,$c0,$cb,$ca,$c0
            .by $ca,$c0,$c0,$cb,$b1,$20,$20,$ca
            .by $c0,$b1,$ab,$c0,$cb,$b1,$20,$b1
            .by $20,$b1,$ca,$c0,$cb,$ca,$c0,$cb,0
;
            .by $20,$20,$20,$dd,$ad,$c0,$c0,$bd
            .by $ad,$c0,$bd,$b1,$c0,$bd,$ad,$c0
            .by $bd,$ad,$c0,$ad,$c0,$c0,$bd,$dd
            .by $ad,$c0,$b1,$db,$c0,$b1,$bd,$20
            .by $dd,$dd,$20,$20,$dd,$20,$20,$20,0
; ------------------------------------ Mask data
; ------------------------------------ for "running line"
maske       .by 80				; number of mask bytes
            .by $00,$00,$00,$00,$00		; every bit set indicates
            .by $00,$00,$00,$00,$00		; to display a character
            .by $00,$00,$00,$00,$00		; different from "underscore"
            .by $01,$e1,$ef,$07,$80		; 40x16 bits: 16 complete 
            .by $01,$e7,$8f,$07,$80		; screen lines
            .by $01,$ef,$0f,$07,$80
            .by $01,$ef,$0f,$07,$80
            .by $01,$e7,$8f,$27,$80
            .by $01,$e3,$cf,$77,$80
            .by $01,$e1,$ef,$ff,$80
            .by $01,$e1,$ef,$df,$80
            .by $01,$e3,$cf,$8f,$80
            .by $01,$ef,$0f,$07,$80
            .by $00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00
            .by $00,$00,$00,$00,$00
; ------------------------------------ 
cntwert     .by 51
dollar      .by "$"
home        .by 19,155,18,0
;
ndisk1      .by 155
            .tx "neue diskette einlegen"
            .by 0
ndisk2      .tx "und <space>"
            .by 0
ndisk3      .tx "oder mit <stop> beenden"
            .by 0
;
derr        .by 18,28
            .tx " fehler #"
errnum      .tx "00"
            .tx " beim laden! "
            .by 146,0
;
name        .by 152
            .tx "loading: "
namout      .tx "                "
            .by 0
; ------------------------------------ 
; ------------------------------------ IFLI display routines
; ------------------------------------ 
            .eq vekt02=$02
            .eq pcnt=$57
            .eq pbyte=$58
            .eq ld3=$d3
            .eq mpport=$fc
            .eq header=$3ff0
            .eq packed=$3ffe
            .eq packbyte=$3fff
            .eq icolor=$8000
            .eq ifli=$8400
            .eq ld011=$d011
            .eq ld012=$d012
            .eq ld016=$d016
            .eq ld018=$d018
            .eq ld019=$d019
            .eq ld01a=$d01a
            .eq colorram=$d800
            .eq ldc00=$dc00
            .eq ldc01=$dc01
            .eq ldc0d=$dc0d
            .eq ldd00=$dd00
            .eq ramirq=$fffe
;
display     jsr prepifli
            jsr iflion
l9040       lda ldc00				; changed (for timer)
            and ldc01
            and #$10
            bne l9040
l904a       lda ld011
            bpl l904a
            sei
            lda #$6b
            sta ld011
            lda #$00
            sta ld01a
            lda #$81
            sta ldc0d
            lda #$37
            sta pport
l907b       lda ld011
            bpl l907b
            lda #$03
            sta ldd00
            lda #$15
            sta ld018
            lda #$08
            sta ld016
            lda #$1b
            sta ld011
            cli
reinit      lda #$37
            sta pport
            rts
; ------------------------------------ Load file
loadf       lda #$00
            sta formok
            lda flen			; OPEN 13,drive,0,"^^name,r"
            ldx #<(nambuf)
            ldy #>(nambuf)
            jsr setnam
            lda #13
            ldx drive
            ldy #$00
            jsr setlfs
            jsr open
            ldx #13
            jsr chkin

            jsr basin
            jsr basin
            lda status
            beq l9138
            jmp l91c0			; message if not ok
;
l9138       lda #$40
            sta vekt02+1
            ldy #$00
            sty vekt02
l9142       jsr basin
            sta header,y
            iny
            cpy #$10
            bne l9142
            ldy #$00
            lda status
            bne l91c0
            ldx #$0d			; Fun Painter?
l9157       lda header,x
            cmp hcheck,x
            bne wrong
            dex
            bpl l9157

            lda packed			; compressed?
            bne depack

l9167       jsr basin			; it's uncompressed
            sei
            sty pport
            sta (vekt02),y
            ldx #$36
            stx pport
            jsr action			; activity display
            cli
            jsr increm
            bcs l91c0
            lda status
            beq l9167
            bne l91c0

depack      jsr basin			; it's compressed
            cmp packbyte
            bne l919c
            jsr basin
            beq l91bb
            tax
            jsr basin
            jmp l919e
l919c       ldx #$01
l919e       sta pbyte
            sei
            sty pport
            stx pcnt
l91a8       lda pbyte
            sta (vekt02),y
            jsr action			; activity display
            jsr increm
            dec pcnt
            bne l91a8
            ldx #$36
            stx pport
            lda status
            beq depack

l91bb       lda #$36			; finished
            sta pport
            cli
l91c0       jsr enddir
            lda status			; changed (speeder probs)
            cmp #$40
            bne meldung
            rts
; ------------------------------------ Wrong format/STOPped
wrong       lda #$01
            sta formok
            jmp enddir
; ------------------------------------ Message
meldung     lda #$02			; red border
            sta rahmen

            lda drive			; get floppy message
            jsr talk
            lda #$6f
            sta sekadr
            jsr sektlk
            jsr getin
            sta errnum
            jsr getin
            sta errnum+1
ml0         jsr getin
            cmp #$0d
            bne ml0
            jsr untalk

            jsr reinit			; display
            jsr logodis
            clc
            ldx #7
            ldy #8
            jsr plot
            lda #<(derr)
            ldy #>(derr)
            jsr strout
            jsr wait
            pla
            pla
            jmp show
; ------------------------------------ Sub to read routine
increm      inc vekt02
            bne l9211
            inc vekt02+1
l9211       lda vekt02
            ldx vekt02+1
            cpx #$83
            bne l9225
            cmp #$e8
            bne l9223
            sty vekt02
            lda #$c0
            sta vekt02+1
l9223       clc
            rts
;
l9225       cpx #$ff
            bne l9223
            cmp #$a4
            bne l9223
            sec
            rts
; ------------------------------------ Prepare IFLI mode
prepifli    lda #$3b
            sta iflirout+1
            lda #$08
            sta smod2+1
            lda #$40
            sta smod3+1
            lda #$7f
            sta smod3+2
            lda #$00
            sta vekt02
            lda #$84
            sta vekt02+1
            ldx #$d0
l924d       ldy #$11
l924f       lda iflirout,y
            sta (vekt02),y
            dey
            bpl l924f
            clc
            lda vekt02
            adc #$12
            sta vekt02
            bcc l9262
            inc vekt02+1
l9262       clc
            lda iflirout+1
            adc #$01
            and #$07
            ora #$38
            sta iflirout+1
            lda smod2+1
            adc #$10
            and #$7f
            sta smod2+1
            inc smod3+1
            dex
            beq l928f
            cpx #$64
            bne l924d
            lda #$40
            sta smod3+1
            lda #$ff
            sta smod3+2
            bne l924d
l928f       ldy #$00
            lda #$60
            sta (vekt02),y
            rts
; ------------------------------------ IFLI routines
iflirout    lda #$ff			; stored to $8400
            sta ld011
smod2       lda #$ff
            sta ld018
smod3       lda $ffff
            sta backgr
            bit ld3

iflion      lda ld011			; display IFLI
            bpl iflion
            sei
            lda #$6b
            sta ld011
            lda #$35
            sta pport
            ldx #$00
l92b9       lda icolor,x
            sta colorram,x
            lda icolor+$0100,x
            sta colorram+$0100,x
            lda icolor+$0200,x
            sta colorram+$0200,x
            lda icolor+$0300,x
            sta colorram+$0300,x
            inx
            bne l92b9
            lda #$00
            sta ldd00
            lda #$38
            sta ld018
            lda #$19
            sta ld016
            lda #$7f
            sta ldc0d
            lda ldc0d
            lda #$01
            sta ld01a
            sta ld019
            lda #<(rirq)
            sta ramirq
            lda #>(rirq)
            sta ramirq+1
            lda #$2f
            sta ld012
l9302       lda ld011
            bpl l9302
            lda #$3b
            sta ld011
            cli
            rts
;
            .by 0,0,0,0,0,0,0,0		; padding
            .by 0,0,0,0,0,0,0,0
            .by 0,0,0,0,0
; ------------------------------------ End of IRQ
eoirq       pla
            sta pport
            pla
            tay
            pla
            tax
            pla
            rti
; ------------------------------------ Raster IRQ
rirq        pha
            lda pport
            sta mpport
            lda #$35
            sta pport
            lda #<(rirq2)
            sta ramirq
            lda #$30
            sta ld012
            dec ld019
            cli
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
rirq2       nop
            nop
            nop
            nop
            nop
            pla
            pla
            pla
            txa
            pha
            tya
            pha
            lda mpport
            pha
            jsr ifli
            dec ld019
            lda ld016
            eor #$01
            sta ld016
            lda ldd00
            eor #$02
            sta ldd00
            lda #$2f
            sta ld012
            lda #<(rirq)
            sta ramirq
            lda #>(rirq)
            sta ramirq+1
            jmp eoirq
; ------------------------------------ Header data
hcheck      .by $46,$55,$4e,$50,$41,$49,$4e,$54	; "Funpaint (MT) "
            .by $20,$28,$4d,$54,$29,$20
; ------------------------------------ Copyright information
cright      .by $20,$20,$20,$20,$43,$4f,$44,$45
            .by $44,$20,$49,$4e,$20,$31,$39,$39
            .by $31,$20,$42,$59,$20,$4d,$41,$4e
            .by $46,$52,$45,$44,$20,$54,$52,$45
            .by $4e,$5a,$20,$20,$20,$20
l9457       .tx "show 1994 by arndt dettke"
            .by 0
; "coded in 1991 by manfred trenz"
; "(dia) show 1994 by arndt dettke"
modend      .en

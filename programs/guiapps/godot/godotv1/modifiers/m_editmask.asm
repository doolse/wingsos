.petscii
.include "godotlib.lib"
.ob "editmask,p,w"

; ----------------------------------------- 
; mod.MaskEd
;     Module to edit hires monochrome bitmaps used as masks
;     for ldr.4Bit&Mask
;
; 1.00: 12.01.01, first release
; 1.01: 07.01.02, added clear visible screen feature
;                 added mouse access to gadgets when in edit mode
;
; ----------------------------------------- 

* =  $c000

        .eq zsp 	= $30
        .eq zzl 	= $31
        .eq spi 	= $32
        .eq zli 	= $33
        .eq cnt 	= $34
        .eq screenpnt 	= $35
        .eq col 	= $37
        .eq bitcnt	= $38
        .eq storespx	= $39
        .eq storespy	= $3a
	.eq storesph	= $3b
        .eq mkx		= $3c
        .eq mky		= $3d
        .eq vpos	= $3e

        .eq lastkey 	= $c5       ; current key
        .eq mac 	= $fb

        .eq commoflag 	= $028d
        .eq shiftvekt 	= $028f
        .eq sprites 	= $07f8     ; Sprite definition pointers

        .eq spr255 	= $3fc0     ; Spriteblock 255

        .eq sprp4 	= $d008     ; spr #4 x pos
        .eq sprp5 	= $d00a     ; spr #5 x pos
        .eq sprp6 	= $d00c     ; spr #6 x pos
        .eq sprp7 	= $d00e     ; spr #7 xpos
        .eq sprenable 	= $d015     ; spr display
        .eq sprxy 	= $d017     ; expand spr y
        .eq sprmulti 	= $d01c     ; multicolr
        .eq sprxx 	= $d01d     ; exp spr x
        .eq sprcols 	= $d02b

        .eq vram2 	= $ce0b
        .eq vram1 	= $de0b
        .eq cstart 	= $ee0b

; ----------------------------------------- Header

	jmp start
        .byte $20
        .byte $00,$00
        .word (modend)
        .byte 0,0
        .byte "Edit Mask pixels"
        .byte "1.01"
        .byte "07.01.02"
        .byte "A.Dettke/Mac    "

; ----------------------------------------- Main

start:  jsr makech          ; define new character
        jsr makespr         ; define spritebox
        jsr savecols        ; save screen colors
        jsr bestcols        ; set best colors for black
        ldx #<(pixlst)      ; build requesters
        ldy #>(pixlst)
        jsr gd_screen       ; returns with A = 0
        jsr messoff         ; hide Edit Mode message
        lda #0
        sta vertical
        sta horizont
        sta sc_loop         ; init mem flag (to: textmem)
        jsr display         ; show Mask enlarged
        jsr sprbox          ; show spritebox
;        jsr sptr
        jsr gd_eloop        ; wait... (what else within GoDot?)

cancel: jsr restcols        ; restore screen colors
        ldy #0
        lda sc_merk
        sta vertical
        lda sc_merk+1
        sta horizont
        jsr dp13            ; sprites and so on
        sec                 ; leave module
        rts

; ----------------------------------------- Values

palzl:   	.byte 0
palsp:   	.byte 0
hold:    	.byte 0
vert:    	.byte 0
plast:   	.byte 0
cntx:    	.byte 0
cnty:    	.byte 0
storesv: 	.word 0
c64pal:  	.byte $00,$ff,$44,$cc,$55,$aa,$11,$dd
         	.byte $66,$22,$99,$33,$77,$ee,$88,$bb

; ----------------------------------------- Fill screen with new character

fill: ldy #0                ; set textscreen as destination
        sty sc_loop
        sty sc_zl           ; start at row 0, column 0
        sty sc_sp
        lda #32             ; width 32 (4x8)
        sta sc_br
        lda #24             ; height 24 (3x8)
        sta sc_ho
        jsr gd_initmove     ; compute screenaddress
        lda #223            ; get fill character
        jmp gd_fi0          ; fill now

; ----------------------------------------- Make new character

makech: ldx #7
mc0:    lda char,x
        sta sc_undochar,x
        dex
        bpl mc0
        rts

; ----------------------------------------- Make Sprites for grid

makespr: ldx #0
masp:   lda sprstore,x ; load sprite
        sta spr255,x
        inx
        cpx #64
        bne masp
        lda #0
        sta sprmulti        ; multi off
        sta $d010
        sta $d01d
        sta $d017
        jsr gridcl
        lda #$ff            ; all sprites
        sta sprites+2       ; point to $ff
        sta sprites+3
        sta sprites+4
        sta sprites+5
        sta sprites+6
        sta sprites+7
        jsr sprbox   
; ----------------------------------------- set vert loc
        lda #111
        sta $d005
        sta $d007
        sta $d009
        lda #175
        sta $d00b
        sta $d00d
        sta $d00f
; ----------------------------------------- set horiz loc
        lda #85
        sta $d004
        sta $d00a
        lda #149
        sta $d006
        sta $d00c
        lda #213
        sta $d008
        sta $d00e
        rts

; ----------------------------------------- grid color

gridcl: lda grclr           ; colors for grid
        sta $d029
        sta $d02a
        sta $d02b
        sta $d02c
        sta $d02d
        sta $d02e
        rts

; ----------------------------------------- Show grid

sprbox: lda sprenable
        and #3
        ora #$ff
        sta sprenable
        rts

; ----------------------------------------- Data

char: 	  .byte $7e,$ff,$ff,$ff,$ff,$ff,$ff,$7e
tab:      .byte 0,8,16,24   ; horizontal tab positions
tabspr:   .byte 0,64,128,192
cols      .byte 0,0,0,0,0
bestcolor .byte 6,2,5,8	    ; schwarz
bits:     .byte $80,$40,$20,$10,$08,$04,$02,$01
vertical: .byte 0
horizont: .byte 0
oldv:     .byte 0
oldh:     .byte 0
grclr:    .byte 1
pmode:    .byte 1           ; mode: 1=set 0=reset
pcols:    .byte 0,15
drawoffs  .byte 3,7
drawtxt   .ts "ErasDraw"

grstore:  .byte $aa,$55,$aa,$55,$aa,$55,$aa,$55
pmove     .byte 0,0,0,0,0,0,0,0


sprstore:       .byte 16,0,0,16,0,0,16,0
                .byte 0,254,0,0,16,0,0,16
                .byte 0,0,16,0,0,0,0,0
                .byte 0,0,0,0,0,0,0,0
                .byte 0,0,0,0,0,0,0,0
                .byte 0,0,0,0,0,0,0,0
                .byte 0,0,0,0,0,0,0,0
                .byte 0,0,0,0,0,0,0,0

; ------------------------------------------ Check for Gadgets when in Edit Mode

ckgads  lda sc_merk+1	; horizontal in Gadgetgebiet?
        cmp gdbr
        bcs ok1		; nein, zu weit rechts
        cmp gdsp
        bcc ok1		; nein, zu weit links

        lda sc_merk	; vertikal darin?
        cmp gdho3
        bcs ok1		; nein, zu weit unten
        cmp gdzl1
        bcc ok1		; nein, zu weit oben
        cmp gdho1	; ist im Mover?
        bcc jended	; ja, Edit beenden
        cmp gdzl2	; ist im Chooser?
        bcc ok1		; nein, dazwischen
        cmp gdho2       ; ist in Wipe?
        bcc jended      ; ja, visible Screen löschen
        cmp gdzl3       ; ist in Wipe?
        bcc ok1         ; nein, zwischen Wipe und Draw
jended  pla
        pla
        jmp endedit
ok1     rts
; ----------------------------------------- Switch Draw Mode

evmode: ldx pmode
        inx
        cpx #2
        bcc md0
        ldx #0
md0     stx pmode
        ldy drawoffs,x
        ldx #3
md1     lda drawtxt,y
        sta drawm,x
        dey
        dex
        bpl md1
        lda #17
        ldx #34
        ldy #4
        jsr gd_setpar
        jsr gd_initmove
        ldx #<(drawm)
        ldy #>(drawm)
        jmp gd_xtxout2

; ----------------------------------------- Save colors

savecols    lda sc_maincolor
            ldx #4
            bne sav1
sav0        lda sc_shadow,x
sav1        sta cols,x
            dex
            bpl sav0
            rts

; ----------------------------------------- Restore colors

restcols    ldx #4
            lda cols,x
            sta sc_maincolor
            jmp sav3
sav2        lda cols,x
            sta sc_shadow,x
sav3        dex
            bpl sav2
            rts

; ----------------------------------------- Best colors

bestcols    ldy #0		; background color
            ldx #0		; index to cols
bc0         lda bestcolor,y	; get new color
            sta sc_shadow,x	; set it
            iny
            inx
            cpx #4
            bne bc0
            rts

; ----------------------------------------- Edit pixels

evedit: jsr messon        ; show message
        lda #0            ; init break with STOP key
        sta sc_stop
        sta bigbox        ; set function inactive
        jsr hiedi         ; hilite Edit gadget
        lda vertical      ; save current position
        sta oldv
        lda horizont
        sta oldh

; ----------------------------------------- Edit Routines

ed0:    jsr ckgads        ; in einem der Auswahlgadgets?
        lda sc_merk+1     ; look for too wide
        cmp #$20
        bcs edx1          ; too far rt
        lda sc_merk
        cmp #$18          ; too far dwn
        bcc edx3
edx1:   ldx #2            ; border red
        stx $d020
edx2:   inx               ; delay
        bne edx2
        jsr gd_position
        lda #0            ; border black
        sta $d020
        beq ed0		  ; loop until in box again
edx3:   lda sc_keyprs     ; FIRE/RETURN/STOP?
        ora commoflag     ; CTRL key?
        ora sc_stop
        beq ed0           ; no, wait

        tay               ; hold key
        jsr gd_position   ; where is the mouse?
        lda commoflag     ; check for CTRL/Shift
        beq edit          ; no, other key
        ldx #0
        stx sc_keyprs
        stx commoflag
        tax
        and #5
        beq ed0
        jmp keymove       ; keyboard if CTRL or Shift pressed

edit:   lda sc_stop       ; if STOP: exit from edit
        beq ed1	          ; no STOP

endedit: lda sc_normtext  ; STOP: exit, restore Edit gadget
        sta sc_chgcol
        ldy #3
ed3:    lda editgad,y
        sta sc_zl,y
        dey
        bpl ed3
        jsr gd_trim
        ldx #4
        jsr gd_fcol
        jsr messoff       ; hide message
        lda #$40
        sta bigbox
        lda oldv          ; restore last position
        sta vertical
        lda oldh
        sta horizont
sl5:    clc               ; leave function
        rts

ed1:    sta sc_loop       ; init memflag
        lda screenpnt     ; save current Bitmap pointer
        pha
        lda screenpnt+1
        pha
        lda vertical
        pha
        lda horizont
        pha

        lda sc_merk       ; row of mouse pointer (0-23)
        cmp #24           ; valid?
        bcs sl3           ; no, leave
        pha               ; save value (y indent)
        lsr               ; / 8
        lsr
        lsr
        sta vertical
        lda sc_merk+1     ; column of mouse pointer
        cmp #32           ; valid?
        bcc sl4
        pla               ; if no, clear stack...
        jmp sl3           ; ...and leave
sl4:    pha               ; save value (x indent)
        lsr               ; / 8
        lsr
        lsr
        sta horizont
        jsr set4bit       ; add to Bitmap address
                          ; (now pointing to Bitmap tile)
        pla               ; bit number (horizontally)
        and #7
        tax               ; bit 0 indicates which nibble
        pla               ; pixel row in tile
        and #7
        clc
        adc screenpnt     ; now pointing to correct byte
        sta screenpnt
        bcc sl0
        inc screenpnt+1
sl0     lda bits,x        ; now decide which pixel
        ldy #0
        ldx pmode         ; and set/reset
        beq sl1
        ora (screenpnt),y
        jmp sl2
sl1     eor #$ff
        and (screenpnt),y
sl2     sta (screenpnt),y

        lda sc_merk       ; set new color to display
        sta sc_zl
        lda sc_merk+1
        sta sc_sp
        lda #1
        sta sc_br
        sta sc_ho
        sta gr_redisp
        lda pcols,x
        sta sc_chgcol
        ldx #4
        jsr gd_fcol
sl3:    pla               ; restore indents
        sta horizont
        pla
        sta vertical
        pla               ; restore vector to 4Bit
        sta screenpnt+1
        pla
        sta screenpnt
        jmp ed0           ; edit loop

; ----------------------------------------- Move by key

keymove: txa              ; Commo-Flag (1 (Shift), 4 (CTRL) or 5 (both))
        ldx lastkey
        cpx #40           ; "+" key
        bne pe2
        jsr f1            ; fill buffer
pe2: cpx #17              ; "r" key
        bne pe2a
        jsr rotf          ; rotate block
pe2a: cpx #43             ; "-" key
        bne pe3
        jsr f3            ; buffer to screen
        jsr f7
pe3: cpx #48              ; "pound" key
        bne pe4
        jsr f5            ; col to screen
pe4: cpx #62              ; "q" key
        bne pe5
        inc grclr         ; change grid color
        jsr tdelay
        jsr gridcl
pe5: cpx #37              ; "k" key
        bne pete
        jsr tdelay
        lda sprenable 
        eor #$fc          ; flip sprites
        sta sprenable 
pete: cpx #60             ; left mouse button?
        beq shifted
        cmp #5
        beq shifted
        cmp #4
        bne kmb
unshifted: cpx #2         ; CRSR right
        bne km0
        jsr right
        bne km1
km0: cpx #7               ; CRSR down
        bne km4
        jsr down
        bne km1
km4: cpx #29              ; "h" for Home
        bne km5
        jsr evhome
        bcc km6
km5: cpx #28              ; "b" for Bottom Right
        bne km7
        jsr evbotrght
        bcc km6
km7: cpx #49              ; "*" for Center
        bne km8
        jsr evcenter
        bcc km6
km8: cpx #38              ; "o" for Overview
        bne kma
        jsr evdisplay
        jsr messon
        jsr hiedi
        beq km6

kma     cpx #36           ; "m" for Mode
kmb     bne kmc
        jsr tdelay
        jsr evmode
        bcc km6
kmc     cpx #9		  ; "w" for Wipe
        bne km3
        jsr tdelay
        jsr evwipe
        bcc km3
shifted: cpx #60          ; Shift Space (left mouse button)?
        bne km9
        cmp #1            ; (Shift?)
        bne km3
        jmp edit          ; yes, back to normal edit routine
km9: cpx #2               ; CRSR left
        bne km2
        jsr left
        bne km1
km2: cpx #7               ; CRSR up
        bne km3
        jsr up
km1: jsr home             ; display section
km6: lda vertical
        pha
        ldy horizont
        lda oldv
        sta vertical
        lda oldh
        sta horizont
        jsr setspr        ; don't move mouse pointer!
        pla
        sta vertical
        sty horizont
km3: jmp ed0              ; back to edit loop

; ----------------------------------------- delay loop

tdelay: txa               ; save x val
        pha
        ldx #32
td1     lda $d012
        cmp #64
        bne td1
        dex
        bne td1
        pla               ; recall x val
        tax
        rts

; ----------------------------------------- Message handling

messon: lda sc_hilite     ; show message
        jmp ms0
messoff: lda pcols        ; hide message
ms0:    sta sc_chgcol
        lda #24           ; on row 24
        sta sc_zl
        ldx #0            ; column 0
        stx sc_sp
        lda #26           ; width 26 letters
        sta sc_br
        inx               ; one line high
        stx sc_ho
        stx sc_loop
        ldx #4
        jmp gd_fcol

; ----------------------------------------- 

hiedi: lda sc_hilite      ; hilite Edit gadget
        sta sc_chgcol
        ldy #3
ed2: lda editgad,y
        sta sc_zl,y
        dey
        bpl ed2
        jsr gd_trim
        ldx #4
        jmp gd_fcol

; ----------------------------------------- Display Bitmap 8x enlarged

display: jsr fill        ; fill with new character
        lda pcols        ; set background color
        sta $d021
        ldx #0           ; black border
        stx $d020
home:   jsr setstart     ; compute address in Bitmap
dp00:   ldx screenpnt    ; get address
        stx sc_texttab   ; store to work vector
        ldy #1           ; width/height: 1
        sty sc_br
        sty sc_ho
        dey
        sty sy_tbuffer
        sty sc_zl        ; row/column: 0
        sty sc_sp
        sty zsp          ; textscreen: row
        sty zzl          ; column
        sty spi          ; counters (up to 8)
        sty zli
        lda screenpnt+1
        sta sc_texttab+1
dp3:    ldx zzl          ; get row tab value
        lda tab,x
        sta sc_zl
dp2:    ldx zsp          ; get column tab value
        lda tab,x
        sta sc_sp
dp0:    lda (sc_texttab),y  ; get pixels
dp01:   asl
        pha
        bcc dp02
        ldx #1
        .byte $2c
dp02:   ldx #0
        lda pcols,x
        sta sc_chgcol
        ldx #4           ; colorize character on screen
        jsr gd_fcol
        inc sc_sp        ; next column
        inc spi
dp1:    ldy #0           ; restore destination
        sty sc_loop
        ldx spi          ; 8 columns?
        pla
        cpx #8
        bcc dp01         ; no, next
        inc sc_texttab
        bne dp11
        inc sc_texttab+1
dp11:   sty spi          ; counter to 0 again
        inc zli          ; count rows
        inc sc_zl        ; next row
        lda zli          ; 8 rows?
        cmp #8
        bcc dp2          ; no, next
        inc zsp          ; next horizontal tab
        sty zli          ; counter to 0 again
        lda zsp          ; 4 tabs?
        cmp #4
        bcc dp3          ; no, next
        sty zsp          ; start far left again
        clc              ; proceed to next Bitmap row
        lda sc_texttab
        adc #$20
        sta sc_texttab
        lda sc_texttab+1
        adc #1
        sta sc_texttab+1
        inc zzl          ; next vertical tab
        lda zzl
        cmp #3           ; 3 tabs?
        bcc dp3          ; no, next
dummy:  clc
        rts

; ----------------------------------------- Scroll events (Navigation Window)

evmove: lda sc_merk      ; row
        ldx sc_merk+1    ; column
        cmp #4           ; topmost row of gadget?
        bne em0
        cpx #34          ; four columns within
        beq evupleft
        cpx #35
        beq evup
        cpx #36
        beq evuprght
        cpx #37
        beq evhome
em0: cmp #5              ; middle row?
        bne em1
        cpx #34
        beq evleft
        cpx #37
        beq evcenter
        cpx #36
        beq evright
        cpx #35
        bne em1
        jmp evdisplay
em1: cmp #6              ; bottom row?
        bne em2
        cpx #34
        beq evdwnleft
        cpx #35
        beq evdown
        cpx #36
        beq evdwnrght
        cpx #37
        beq evbotrght
em2:    clc
        rts

; ----------------------------------------- Scroll event routines

evdown: jsr down         ; scroll down
        jmp dp00         ; display
; ----------------------------------------- 
evdwnrght: jsr evdown
        jmp evright
; ----------------------------------------- 
evdwnleft: jsr evdown
        jmp evleft
; ----------------------------------------- 
evup: jsr up             ; scroll up
        jmp dp00         ; display
; ----------------------------------------- 
evuprght: jsr evup
        jmp evright
; ----------------------------------------- 
evupleft: jsr evup
        jmp evleft
; ----------------------------------------- 
evright: jsr right       ; scroll right
        jmp dp00         ; display
; ----------------------------------------- 
evleft: jsr left         ; scroll left
        jmp dp00         ; display

; ----------------------------------------- Position events

evhome: ldy #0
        sty vertical     ; row/column: 0
        sty horizont
        jmp home         ; display
; ----------------------------------------- 
evcenter: lda #11        ; center position
        sta vertical
        lda #18
        sta horizont
        jmp home         ; display
; ----------------------------------------- 
evbotrght: lda #22       ; bottom right position
        sta vertical
        lda #36
        sta horizont
        jmp home         ; display

; ----------------------------------------- Scroll down

down: ldx vertical
        inx
        cpx #23
        bcs dw0
        stx vertical
        lda screenpnt
        adc #$40
        sta screenpnt
        lda screenpnt+1  ; next Bitmap row
        adc #1
        sta screenpnt+1
dw0: rts

; ----------------------------------------- Scroll up

up: ldx vertical
        dex
        bmi dw0
        stx vertical
        sec
        lda screenpnt
        sbc #$40
        sta screenpnt
        lda screenpnt+1  ; previous Bitmap row
        sbc #1
        sta screenpnt+1
        rts

; ----------------------------------------- Scroll right

right: ldx horizont
        inx
        cpx #37
        bcs ri0
        stx horizont
        lda screenpnt    ; next Bitmap column
        adc #8
        sta screenpnt
        bcc ri0
        inc screenpnt+1
ri0: rts

; ----------------------------------------- Scroll left

left: ldx horizont
        dex
        bmi ri0
        stx horizont
        sec              ; previous Bitmap column
        lda screenpnt
        sbc #8
        sta screenpnt
        lda screenpnt+1
        sbc #0
        sta screenpnt+1
        rts

; ----------------------------------------- Display graphics event

evdisplay: nop           ; dummy (don't know anymore why)
dp11b:  jsr disp2        ; show graphics
        jsr sprbox
        jmp display      ; redisplay magnification
; ----------------------------------------- 
dp13:   sty sc_stop      ; re-init stop flag
        lda sc_maincolor ; restore colors
        sta $d020
        sta $d021
        jsr setspr       ; set pointer to last tile
        jsr gd_spron     ; switch sprites on
        sec              ; leave module
        rts

; ----------------------------------------- Define Bitmap address

setstart: lda #0         ; base address: $2000
        sta screenpnt
        lda #$20
        sta screenpnt+1

set4bit: lda vertical    ; row (of tiles)
        beq ss0
        sta cnt          ; compute row address
ss1:    clc
        lda screenpnt
        adc #$40
        sta screenpnt
        lda screenpnt+1
        adc #1
        sta screenpnt+1
        dec cnt
        bne ss1
ss0:    lda horizont     ; column (of tiles)
        beq ss2
        sta cnt
ss4:    clc              ; compute indent into row
        lda screenpnt
        adc #8
        sta screenpnt
        bcc ss3
        inc screenpnt+1
ss3:    dec cnt
        bne ss4
ss2:    rts              ; result in screenpnt (byte in bitmap)

; ----------------------------------------- Display graphics

disp2:  jsr setspr       ; set pointer
        jsr gd_spron     ; switch on
        jsr redisplay    ; redisplay graphics
sptr    jsr gd_position  ; get pointer position
        lda sc_merk
        pha
        lda #5
        sta vertical
        lda sc_merk+1
        pha
        lda #35
        sta horizont
        jsr setspr
        pla
        sta horizont
        pla
        sta vertical
        rts

; ----------------------------------------- Set pointer to last tile

setspr: lda $d010        ; X-Hi off
        and #$fc
        sta $d010
        lda vertical     ; set row
        asl
        asl
        asl
        clc
        adc sy_soffy
        tax
        stx $d003
        dex
        dex
        stx $d001
        lda horizont     ; set column
        tax
        cmp #29
        bcc ssp0
        and #31
        tax
        lda $d010        ; X-Hi on
        ora #3
        sta $d010
ssp0:   txa
        asl
        asl
        asl
        clc
        adc sy_soffx
        tax
        stx $d002
        stx $d000
        rts

; ----------------------------------------- Redisplay routines

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
rp1:    lda sc_keyprs    ; wait for stop key
        ora sc_stop
        beq rp1
rp2:    jsr tmode
        ldx sc_screenvek
        ldy sc_screenvek+1
        jsr gd_screen
rp3:    clc
        rts

; ----------------------------------------- 

tmode:  ldx #$13
        lda #$1b
        stx $d018
        sta $d011
        lda #$08
        sta $d016
        rts

; ----------------------------------------- 

redisplay: lda gr_redisp
        pha
        ldx #1
        stx gr_redisp
        dex
        stx sc_stop
        jsr redis
        jsr rp1
        pla
        sta gr_redisp    ; auto render on
        jmp messoff      ; hide message after graphics

; ----------------------------------------- Event Wipe Screen

evwipe  lda spritex	; save current position
        sta storespx
        lda spritey
        sta storespy
	lda spritehi
        sta storesph
	lda #50
	sta vpos
        lda #0
        sta spritehi
        ldy #3		; 3 rows
ew1     ldx #0		; 4 columns
ew0     lda tabspr,x	; get x-position
        sta spritex
        lda vpos	; get vpos
        sta spritey
        stx mkx
        sty mky
        jsr f3		; fill 1 tile
        ldy mky
        ldx mkx
        inx		; for 4 columns
        cpx #4
        bne ew0
        clc		; and 3 rows
        lda vpos
        adc #64
        sta vpos
        dey
        bne ew1
	lda storespy	; restore current position
	sta spritey
	lda storespx
	sta spritex
	lda storesph
	sta spritehi
        jmp dp00

; ----------------------------------------- Fill Tile Routines

f1:     jsr hv1          ; get block address
        ldy #00          ; fill buffer
f2:     lda (sc_texttab),y
        sta grstore,y
        iny
        cpy #8
        bne f2
        rts

; ----------------------------------------- paste buffer on screen

f3:     jsr hv1          ; get block address
        ldy #00
f4:     lda grstore,y    ; buffer to screen
        sta (sc_texttab),y
        iny
        cpy #8
        bne f4
        rts

; ----------------------------------------- fill color

f5:     jsr hv1          ; get block address
        ldy #00
        ldx pmode
        bne f55
        lda #0
        .by $2c
f55     lda #$ff         ; fill block with
f6:     sta (sc_texttab),y     ; pixels
        iny
        cpy #8
        bne f6
f7      sty gr_redisp
        jmp home

; ----------------------------------------- 

rotf:   jsr hv1          ; get block address   
cloop:  lda #$80
        sta bitcnt
        lda #0
        sta ls_temp
srt2    ldy #7
        ldx #0
        stx sc_merk+1
        ldx #$80
        stx sc_merk
srt1    lda (sc_texttab),y
        and bitcnt
        beq srt0
        lda sc_merk+1
        ora sc_merk
        sta sc_merk+1
srt0    lsr sc_merk
        dey
        bpl srt1
        ldy ls_temp
        lda sc_merk+1
        sta pmove,y
        inc ls_temp
        lsr bitcnt
        bcc srt2
        ldy #7
stu1    lda pmove,y
        sta (sc_texttab),y
        dey
        bpl stu1
        jmp f7

; ----------------------------------------- 

hv1:    lda screenpnt+1  ; lda msb
        sta sc_texttab+1
        inc $d020        ; flash border
        lda screenpnt
        sta sc_texttab
        jsr gd_position
        lda sc_merk      ; lda how many rows down
        lsr              ; / 2
        lsr              ; / 2
        lsr              ; / 2
        tay              ; put in y
        beq hv3          ; if 0 skip
hv2:    clc
        lda sc_texttab
        adc #$40
        sta sc_texttab
        lda sc_texttab+1
        adc #1
        sta sc_texttab+1 
hv20    dey
        bne hv2
hv3:    lda sc_merk+1    ; get horz rows
        lsr              ; /2
        lsr              ; /2
        lsr              ; /2
        tay              ; put in y
        beq hh3
        clc
hh2:    lda sc_texttab
        adc #8           ; add column
        sta sc_texttab
        bcc hh5
        inc sc_texttab+1
hh5:    dey
        bne hh2
hh3:    dec $d020        ; restore border
        rts

; ------------------------------------------ 

gdzl1   .by 4
gdzl2   .by 14
gdzl3   .by 17
gdsp    .by 34
gdbr    .by 38
gdho1   .by 7
gdho2   .by 15
gdho3   .by 18


; ----------------------------------------- Screenlist

pixlst: .byte $93        ; Clear screen
        .byte 0,0,32,24  ; Edit area
bigbox: .byte $40        ; active
        .word (evedit)

        .byte 3,33,6,5   ; mover
        .byte $d0
        .word (evmove)
        .byte 223,30,223,"h",0

wipegad: .byte 13,33,6,3
        .byte $c0	 ; wipe screen
        .word evwipe
        .ts "Wipe@"

modegad: .byte 16,33,6,3
        .byte $c0        ; show draw mode
        .word evmode
drawm   .ts "Draw@"

editgad: .byte 19,33,6,3
        .byte $c0        ; Edit
        .word evedit
        .ts "Edit@"

        .byte 22,33,6,3
        .byte $c0        ; Exit
        .word cancel
        .ts "Exit@"

        .byte $c0,1,33,5 ; extra texts (mover)
        .ts "Move:@"

        .byte $c0,4,33,4
        .byte 91,"o",94,"*",0

        .byte $c0,5,33,4
        .byte 223,31,223,"b",0

        .byte $c0,23,255,26; Message: Edit Mode
        .ts "STOP exits from Edit Mode.@"
        .byte $80

modend: .en


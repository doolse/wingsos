.petscii
.include "godotlib.lib"
.ob "sloadst,p,w"

; ----------------------------------------- 
; svr.Loadstar
;     Saver for Loadstar SHP splash screen files
;
; 1.00: 10.6.01, first release
; 1.01: 14.6.01, bug: "Cancel Save" didn't work (fixed)
;
; ----------------------------------------- 

            .ba $c000
;
            .eq gbyte		=$30
            .eq cnt		=$31
            .eq adcnt		=$33
            .eq offy		=$34
            .eq offx		=$35
            .eq mlen		=$36
            .eq cpos		=$37
            .eq comprfl		=$38

            .eq colors		=$c82f
            .eq vid1		=colors
            .eq vid2		=$ca23
            .eq colram		=$cc17
            .eq back		=$cfff
;
; ----------------------------------------- 

header      jmp start
            .by $40
            .by 0
            .by 1
            .wo modend
            .wo 0
            .tx "Loadstar SHP    "
            .tx "1.01"
            .tx "14.06.01"
            .tx "A.Dettke        "
;
; ----------------------------------------- 

start       ldx gr_nr		; *every* saver must start with this check!
            dex
            bne st0
            jmp initinp		; INPUT if .X was 1
;
st0         jsr setmode		; savemode: hires or multi?
            lda used		; cancelled?
            cmp #3
            bne st11
            jsr canc		; yes, leave
            bcs st12

st11        lda #0		; reset compression counter
            sta cnt
            sta cnt+1
            jsr gd_sproff
            lda gd_modswitch
            pha
            lda #$ad
            sta comprfl
            lda ls_flen
            beq st1

st3         sta wflen
            jsr addshp		; add ".shp" to name (length plus 4)
            lda #1
            sta adcnt
            lda #50
            sta cntwert

            jsr getpic		; move rendered image data to write buffer
            jsr wopen		; open write file
            bne st2		; error?

            jsr getname		; no, save image file name
            ldx #12		; OPEN 12,drv,12,"file,p,w"
            jsr ckout
            jsr gd_clrms	; clear gauge bar
            ldx #0		; Text: "Bitmap"
            jsr tcopy

            lda mode		; hires?
            bmi st21

            jsr writem		; write multicolor file to disk
            jmp st2

st21        jsr writeh		; write hires file to disk

st2         jsr clrch		; CLOSE 12
            lda #12
            jsr close
            jsr err9
            bne st1
            jsr setname		; publish image name to GoDot

st1         pla
            sta gd_modswitch
st12        jsr gd_spron
            sec			; return to mainloop
            rts

; ----------------------------------------- 
; ----------------------------------------- Input Routines (don't change!)
; ----------------------------------------- 

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
; ----------------------------------------- End of Input Routines
; ----------------------------------------- 

pw          .tx "w,p,"
drinit      .tx "i0"
wflen       .by 0
shptx       .tx "phs."
;

; ----------------------------------------- Open Write File

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
; ----------------------------------------- Add ".shp" to filename

addshp		tay			; Name longer than 12 chars?
		cpy #12
		bcs ag0			; yes, treat it
;
		ldx #3			; no, add ".shp" 
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

; ----------------------------------------- Initialize Disk

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

; ----------------------------------------- Activity gauge Routines

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
            .ts " Colors@" ; 8
            .ts " Multi @" ; 16
            .ts " Hires @" ; 24
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

; ----------------------------------------- Display Startup requester

requ        inc used
            ldx #<(lstarlst)
            ldy #>(lstarlst)
            jsr gd_xmloop

; ----------------------------------------- Requester management

setmode     ldx used
            bmi requ
            rts

; ----------------------------------------- Leave saver

cn2         lda #3
            .by $2c
canc        lda #$ff
            sta used
            sec
            rts

; ----------------------------------------- Set mode
;
evhires     inc used
evmulti     inc used
            ldx used
            dex
            beq ec01
            ldx #$80
ec01        stx mode
            sec
            rts

; ----------------------------------------- Screenlist

lstarlst    .by 0
            .by 4,11,18,16,$81,0,0
            .ts "Save Loadstar@"
            .by 9,11,18,3,$ca,<(evhires),>(evhires)
            .ts " as Hires @"
            .by 12,11,18,3,$ca,<(evmulti),>(evmulti)
            .ts " as Multi @"
            .by 15,11,18,3,$ca,<(cn2),>(cn2)
            .ts "Cancel Save@"
            .by $c0,6,14,11
            .ts "Select Mode@"
            .by $80

; ----------------------------------------- Variables
used        .by $ff
mode        .by 0

; ----------------------------------------- 
; ----------------------------------------- Move image data to buffers
; ----------------------------------------- 

getpic      lda #$0b		; from $ce0b
            sta sc_texttab
            lda #<(vid1)
            ldx #>(vid1)
            ldy #$ce
            jsr move		; move 500 bytes

            lda #$0b		; from $ee0b
            sta sc_texttab
            lda #<(vid2)
            ldx #>(vid2)
            ldy #$ee
            jsr move		; move 500 bytes

            lda #$0b		; from $de0b
            sta sc_texttab
            lda #<(colram)
            ldx #>(colram)
            ldy #$de
            jsr movec		; move 1000 bytes
            lda gr_bkcol	; set background color
            and #15
            sta back
            rts
; ----------------------------------------- Move GoDot's video RAM
move        sty sc_texttab+1
            sta sc_vekt20
            stx sc_vekt20+1
            lda #<(500)
            ldx #>(500)
            sta ls_vekta8
            stx ls_vekta8+1
            sei
            lda #$30
            sta 1
            ldy #0
mv0         lda (sc_texttab),y
            sta (sc_vekt20),y
            jsr count
            bne mv0
            lda #$36
            sta 1
            cli
            rts
; ----------------------------------------- Move GoDot's color RAM
movec       sty sc_texttab+1
            sta sc_vekt20
            stx sc_vekt20+1
            lda #<(500)
            ldx #>(500)
            sta ls_vekta8
            stx ls_vekta8+1
            sei
            lda #$30
            sta 1
            ldy #0
mv1         sty sc_merk
            lda (sc_texttab),y
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            sta (sc_vekt20),y
            inc sc_vekt20
            bne mv2
            inc sc_vekt20+1
mv2         lda sc_merk
            sta (sc_vekt20),y
            jsr count
            bne mv1
            lda #$36
            sta 1
            cli
            rts
; ----------------------------------------- Counters
count       inc sc_vekt20
            bne cou5
            inc sc_vekt20+1
cou5        inc sc_texttab
            bne cou6
            inc sc_texttab+1
cou6        lda ls_vekta8
            bne cou7
            dec ls_vekta8+1
cou7        dec ls_vekta8
            lda ls_vekta8
            ora ls_vekta8+1
            rts
; ----------------------------------------- Write SHP file header

wrheader    lda #0		; write startaddress ($4000)
            jsr bsout
            lda #$40
            jsr bsout
            lda mode		; write graphics mode ($80 or $00)
            jsr bsout
            lda #$ad		; write compression indicator byte ($ad)
            jmp bsout

; ----------------------------------------- Write Hires

writeh      jsr wrheader
            jmp wm0

; ----------------------------------------- Write Multi

writem      jsr wrheader	; Header: $00 $40 $00 $ad
            lda back		; write background color
            jsr bsout

wm0         lda #<(8000)	; count bitmap (8000 bytes)
            ldx #>(8000)
            sta ls_vekta8
            stx ls_vekta8+1
            lda #<($2000)	; start from $2000
            ldx #>($2000)
            sta sc_texttab
            stx sc_texttab+1
            jsr writeit		; write section 

            ldx mode
            bmi wm1

            ldx #16		; Text on screen: "Multi"
            .by $2c
wm1         ldx #24		; Text on screen: "Hires"
            jsr tcopy

            sty comprfl		; .Y is $00: compress indicator is 0 now
            lda #7
            sta cntwert
            lda #<(1000)	; count 1000 bytes video RAM
            ldx #>(1000)
            sta ls_vekta8
            stx ls_vekta8+1
            lda #<(colors)	; start from color buffer
            ldx #>(colors)
            sta sc_texttab
            stx sc_texttab+1
            jsr writeit		; write section

            ldx mode
            bmi wm2

            ldx #8		; Text on screen: "Colors"
            jsr tcopy

            dey
            sty comprfl		; .Y is $ff: compress indicator is 255 now
            lda #7
            sta cntwert
            lda #<(1000)	; count 1000 bytes color RAM
            ldx #>(1000)
            sta ls_vekta8
            stx ls_vekta8+1
            lda #<(colram)	; start from color buffer
            ldx #>(colram)
            sta sc_texttab
            stx sc_texttab+1
            jsr writeit		; write section

wm2         lda #<(vid1)	; restore colors to their buffer
            sta sc_texttab
            lda #$0b
            ldx #$ce
            ldy #>(vid1)
            jsr move
;
            jmp canc		; finish

; ----------------------------------------- 
;
; Compress sequence: indicator byte, counter, compressed byte; $00 counts 256
;
; ----------------------------------------- 

lastb       lda gbyte
            eor #$ff
;
pack        cmp gbyte		; current byte equals last byte?
            beq incr		; yes, inc counter

p256        pha			; no, save current byte
            lda cnt+1		; counter is 256?
            bne wl0
            lda cnt		; counter is zero?
            beq wl1		; yes, store byte and start new counter

            cmp #4		; counter is below 4?
            bcs wl0		; no, write compressed sequence to disk
            lda gbyte		; yes, byte equals compression indicator byte?
            cmp comprfl
            bne wl2		; no, continue counting

wl0         lda comprfl		; write compression indicator
            jsr bsout
            lda cnt		; write counter
            jsr bsout
            lda #1
            sta cnt
            lda #0
            sta cnt+1

wl2         lda gbyte		; write uncompressed (single) byte to disk
            jsr bsout
            dec cnt		; dec counter (to $00)
            bne wl2		; if not $00, write again (values 1 to 3 are possible)

wl1         pla			; re-get current byte
            sta gbyte		; store

incr        inc cnt		; inc compression counter
            bne wl3
            inc cnt+1
wl4         jsr p256
            dec cnt		; dec counter to $00

wl3         rts
;
; ----------------------------------------- Write file

writeit     ldy #0		; get byte to write to file
            lda (sc_texttab),y
            jsr pack		; compress if necessary
            jsr action		; display gauge bar on screen
            jsr cou5		; count bytes 
            bne writeit		; loop if not zero
            jsr lastb		; care for last compress sequence
            dec cnt		; clear counter (start new sequence)
            rts

modend      .en

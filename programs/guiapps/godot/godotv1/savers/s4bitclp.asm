.petscii
.include "godotlib.lib"
.ob "s4bclip,p,w"
.ba $c000

; ----------------------------------------- 
; svr.4BitClip
;     Saver for GoDot 4Bit Clips
;
; 1.00: 05.08.97, first release
;       17.11.00, completely rewrote comment
;
; ----------------------------------------- 

            .eq cnt		=$31
            .eq byte		=$33
            .eq clpzl		=$34
            .eq clpsp		=$35
            .eq clpbr		=$36
            .eq clpho		=$37
            .eq adcnt		=$3e

            .eq packbyte	=$ad	; constant
            .eq offx		=$b2
            .eq offy		=$b3
            .eq mlen		=$fd
            .eq cpos		=$fe
;
; ---------------------------------------- Header
;
header      jmp start
            .by $40
            .by 0
            .by 1
            .wo modend
            .wo 0
            .tx "4Bit Godot Clip "
            .tx "1.00"
            .tx "05.08.97"
            .tx "A.Dettke        "
;
; ---------------------------------------- Main
;
start       ldx gr_nr		; get function number
            dex			; "Input" if 1
            bne st0
            jmp initinp

; ---------------------------------------- Save Clip

st0         lda #0		; else: "Save"
            sta $d015		; sprites off
            lda gd_modswitch	; save module managment flag
            pha
            lda ls_flen		; any file selected?
            beq st1		; no, finished

            sta wflen		; store name length
            lda #1		; set Activity Display to "Start immediately"
            sta adcnt
            lda #200		; activity counter
            sta cntwert

            jsr wopen		; open write file
            bne st2		; branch if error
            jsr getname		; save name
            ldx #12		; open file for output
            jsr ckout
            jsr gd_clrms	; clear message gadget
            ldx #0		; enter first message
            jsr tcopy
            jsr write		; save image
st2         jsr clrch		; close file
            lda #12
            jsr close
            jsr err9		; floppy message
            bne st1		; error?
            jsr setname		; yes, don't overwrite filename

st1         pla			; restore module manager
            sta gd_modswitch
            lda #$03		; sprites on
            sta $d015
            sec
            rts
;
; ---------------------------------------- Input Routines
;
; *each* saver transports GoDot's input routines -
; I didn't strip them (for information purposes)
; but didn't comment them, first entry is at INITINP

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
            lda sc_merk                    ; row
            cmp sc_zl
            bne sc0
            sec
            lda sc_merk+1                  ; column
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
; ---------------------------------------- Entry
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
            stx gd_tbuffer
            jsr cursor
iloop       lda gd_tbuffer
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
            cmp #160                    ; sh space
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
            adc gd_soffx
            sta $d002
            sta $d000
            clc
            lda #114
            adc gd_soffy
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
;
; ---------------------------------------- Diskaccess
;
pw          .tx "w,p,"
drinit      .tx "i0"
wflen       .by 0

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
; ---------------------------------------- Activity Display
;
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
filltab     .by 160,93,103,127,126,124,105,109
;
cntwert     .by 50
;
txt         .ts " Clip  @"
;
message     .tx "          "		; 10 spaces
mess        .tx "                     "	; 21 spaces
            .by 0
;
; ---------------------------------------- Image Info
            .eq picname=$1fa8
            .eq iloader=picname+$16
            .eq imode=picname+$22
            .eq idrive=picname+$2e
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
setname     lda #0
            ldx #<(picname)
            ldy #>(picname)
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


; ---------------------------------------- 
; ---------------------------------------- Save Clip Routines
; ---------------------------------------- 

; first entry at WRITE

; ---------------------------------------- file sign "GOD1"
magic       .tx "1dog"
; ---------------------------------------- Compute Start address of Clip

setclip     lda #$40		; 4Bit starting from $4000
            sta sc_texttab+1
            ldy #0
            sty sc_texttab

stcl        lda sc_clipzl	; row
            beq scp1		; is zero: skip
            sta clpzl
scp4        clc
            lda sc_texttab+1	; else add 1280, "row" times
            adc #5
            sta sc_texttab+1
            dec clpzl
            bne scp4

scp1        lda sc_clipzl+1	; column
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
scp5        rts

; ---------------------------------------- Compute Width of Clip

compclip    sty ls_vekta8+1	; Hibyte to zero
            clc			; Lobyte times 32
            lda sc_clipzl+2
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
; ---------------------------------------- Counters
count       inc sc_vekt20
            bne cou5
            inc sc_vekt20+1
cou5        inc sc_texttab
            bne cou6
            inc sc_texttab+1
cou6        lda ls_vekta8	; number of bytes
            bne cou7
            dec ls_vekta8+1
cou7        dec ls_vekta8
            lda ls_vekta8
            ora ls_vekta8+1
            rts

; ---------------------------------------- 
; ---------------------------------------- Save Clip
; ---------------------------------------- 

write       ldx #3		; save "GOD1" first (no start address)
wr00        lda magic,x		; (standard sign is "GOD0")
            jsr bsout
            dex
            bpl wr00

            inx
wr01        lda sc_clipzl,x	; followed by row, col, wid, hgt of Clip
            jsr bsout		; (not contained in "GOD0"-files)
            inx
            cpx #4
            bne wr01

            jsr setclip		; compute start address of data to save
            lda #0		; init values:
            sta byte		; output data byte
            sta cnt		; data byte counter
            sta cnt+1
            lda sc_clipzl+3	; height of Clip (# of tile rows to save)
            sta clpho

wr1         ldy #0		; compute width (times 32: # of 4Bit tiles)
            jsr compclip
            lda sc_texttab+1	; save current vector to start of clip
            pha
            lda sc_texttab
            pha

wr0         ldy #0		; compress bytes and save
            lda (sc_texttab),y
            jsr pack
            jsr action		; while Activity Display works
            jsr cou5		; count amount of data bytes to save
            bne wr0

            pla			; re-get start address
            sta sc_texttab
            pla
            clc			; add 1280 (1 tile row)
            adc #5
            sta sc_texttab+1
            dec clpho		; count rows (height)
            bne wr1		; until finished

            lda byte		; Finish: last compress
            eor #$ff
            jsr pack
            lda #packbyte	; attach pack indicator byte
            jsr bsout
            sec			; finished, leave module
            rts
;
; ------------------------------------------ GoDot's compression algorithm
;
pack        cmp byte		; current byte equals previous one?
            beq incr		; yes, count

p256        pha			; else save
            lda cnt+1		; counted 256 times?
            bne wl0		; yes, output

            lda cnt		; new data byte?
            beq wl1		; yes, count

            cmp #4		; more than three?
            bcs wl0		; yes, output

            lda byte		; byte equals pack indicator byte?
            cmp #packbyte
            bne wl2		; no, output data byte

wl0         lda #packbyte	; else output indicator byte
            jsr bsout
            lda cnt		; output counter
            jsr bsout
            lda #1		; and set to 1 for next
            sta cnt
            lda #0
            sta cnt+1

wl2         lda byte		; output data byte
            jsr bsout
            dec cnt		; decrement counter downto zero
            bne wl2		; (when 3 identical)

wl1         pla			; re-get current byte
            sta byte		; and save as previous byte

incr        inc cnt		; count
            bne wl3
            inc cnt+1		; up to 256
				; (and now recursion: P256)
            jsr p256		; then output (counter 256 becomes 0)
            dec cnt		; synchronize counter
				; (which got incremented in INCR)
wl3         rts
;
modend      .en


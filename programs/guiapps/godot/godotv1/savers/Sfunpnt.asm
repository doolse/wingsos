.petscii
.include "godotlib.lib"
.ob "s_FunPt2,p,w"
.ba $c000

; ----------------------------------------- 
; Saver Fun Painter II IFLI format
;       1.00: 16.06.94, first release
;       1.01: 23.07.94, requester inserted (compressed/uncompressed)
; ----------------------------------------- 
;
            .eq gbyte		=$30
            .eq cnt		=$31
            .eq src		=$32
            .eq src3		=$34
            .eq dst		=src3
            .eq adcnt		=$3e

            .eq offx		=$b2
            .eq offy		=$b3
            .eq mlen		=$fd
            .eq cpos		=mlen+1

            .eq hist		=$3f40
            .eq hist0		=hist+16
            .eq htab		=hist0+32
            .eq cram		=$ca00
            .eq c64		=$bd00
            .eq hist3		=$be00

; ----------------------------------------- 
header      jmp start
            .by $40
            .by 0
            .by 1
            .wo modend
            .wo 0
            .tx "FunPaintII  bg=0"
ver         .tx "1.01"
            .tx "23.07.94"
            .tx "A.Dettke/W.Kling"
; ----------------------------------------- 
addtxt      .tx "< GoDot FP2 Saver "
ver1        .tx "x.xx >"
            .by 0
; ----------------------------------------- 
; ----------------------------------------- Saver Funpaint II
; ----------------------------------------- 
start       ldx gr_nr		; Input? (if value=1)
            dex
            bne st3
            jmp initinp
; ----------------------------------------- Save
st3         ldx used		; first call?
            bmi requ		; yes, requester

            cpx #3		; finished?
            bcs canc		; yes, leave saver

st0         lda #0		; init values
            sta gbyte
            sta cnt
            sta $d015
            lda gd_modswitch
            pha

            lda ls_flen		; any filename?
            beq st1		; no, leave saver

            sta wflen		; yes, save image
            lda #1
            sta adcnt
            jsr wopen
            bne st2
            jsr getname
            ldx #12
            jsr ckout
            jsr gd_clrms
            jsr writefp
st2         jsr clrch
            lda #12
            jsr close
            jsr err9
            bne st1
            jsr setname

st1         pla			; restore system values
            sta gd_modswitch
canc        lda #$ff
            sta used
            lda #$03
            sta $d015
cn2         sec			; leave saver
            rts

; ----------------------------------------- Saver requester

requ        inc used			; set flag for next call
            ldx #<(fplist)		; output requester to screen
            ldy #>(fplist)
            jsr gd_xmloop		; and wait for clicks
            jmp st3			; leave saver

; ----------------------------------------- 
; ----------------------------------------- GoDot Input Routines (mandatory to every saver)
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
            sta ls_len
            sta cpos
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
            beq ss0
            stx cpos
            ldx ls_len
            dex
            cpx sc_br
            beq ss0
            inc ls_len
            bne ss0
;
over        ldx cpos
            sta ls_nambuf,x
            inx
            cpx sc_br
            beq ss0
            stx cpos
            ldx ls_len
            cpx cpos
            bne ss0
            inc ls_len
            bne ss0
;
shome       ldx #0
            stx cpos
            beq ss0
;
sclr        ldx ls_len
            dex
            cpx sc_br
            beq ss2
            cpx #1
            bne ss1
ss2         dex
ss1         stx cpos
ss0         jmp input
;
sleft       ldx cpos
            dex
            bmi ss0
            stx cpos
            bpl ss0
;
sright      ldx cpos
            inx
            cpx ls_len
            bcs ss0
            cpx sc_br
            beq ss0
            stx cpos
            bcc ss0
;
sclear      ldy ls_len
            dey
            dey
            bmi ss0
            lda #32
scl1        sta ls_nambuf,y
            dey
            bpl scl1
            iny
            sty cpos
            iny
            sty ls_len
            bne ss0
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
            beq ss0
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
            beq ss0
            dec ls_len
            bpl ss0
;
inst        .by 0
buff2       .tx "                                        "
sonder1     .by 147,148,157,29,189,3,13
            .by 20,19,160
sonder2     .by 93,91,176,185,186,92

; ----------------------------------------- Open File

pw          .tx "w,p,"
drinit      .tx "i0"
wflen       .by 0
;
wopen       jsr inull
            bne err8
            ldy wflen
            cpy #15
            bcc wo2
            ldy #15
wo2         ldx #3
wo1         lda pw,x
            sta ls_nambuf,y
            iny
            dex
            bpl wo1
            sty wflen
            ldx wflen
            inx
            dey
wo3         lda ls_nambuf,y
            sta ls_nambuf,x
            dex
            dey
            bpl wo3
            lda #94
            sta ls_nambuf+1
            sta ls_nambuf
            inc wflen
            inc wflen
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
; ----------------------------------------- Activity Display
messout     ldx #<(message)
            ldy #>(message)
            jmp gd_xtxout2
; ----------------------------------------- 
tcopy       ldy #0
tc0         lda txt,x
            beq clrmess
            sta message,y
            inx
            iny
            bne tc0
; ----------------------------------------- 
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
; ----------------------------------------- 
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
; ----------------------------------------- 
filltab     .by 160,93,103,127,126,124,105,109
;
cntwert     .by 210                     ;109
;
txt         .by $20,23,18,9,20,9,14,7,46,46,$20,0
making      .by 77,1,11,9,14,7,32,8,9,19,20,15,7,18,1,13,19,46,0
;
message     .by $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
mess        .tx "                     "
            .by 0
; ----------------------------------------- Create Fileinfo
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

; ----------------------------------------- 
; ----------------------------------------- 
;
;   write interlace fli
;
; ----------------------------------------- 
; ----------------------------------------- 

writefp     ldx #<(making)		; first: histograms and tables
            ldy #>(making)
            jsr gd_xtxout2
            jsr makec64
;
            ldy #0
whdl        lda fpheader,y		; create file header
            jsr bsout
            iny
            cpy #18
            bne whdl
;
            ldy #3
prepl       lda mask1,y
            sta mask,y
            lda ver,y
            sta ver1,y
            dey
            bpl prepl
;
            lda #32
            sta col0+4
            sta maxcol
;
            lda #<($4000)
            ldx #>($4000)
            sta src
            stx src+1
            lda #<(cram)
            ldx #>(cram)
            sta dst
            stx dst+1
loop3       ldy #15
            lda #0
ch1         sta hist,y
            dey
            bpl ch1
            lda #8
            sta blcnt
blloop      ldx #0
            ldy #0
            lda (src),y                 ;0
            sta hmbuf
            beq hms1
            inx
hms1        iny
            lda (src),y                 ;1
            sta hmbuf+1
            beq hms2
            cmp hmbuf
            beq hms2
            inx
hms2        iny
            lda (src),y                 ;2
            sta hmbuf+2
            beq hms3
            cmp hmbuf
            beq hms3
            cmp hmbuf+1
            beq hms3
            inx
hms3        iny
            lda (src),y                 ;3
            beq hms4
            cmp hmbuf
            beq hms4
            cmp hmbuf+1
            beq hms4
            cmp hmbuf+2
            beq hms4
            inx
hms4        cpx #3
            bcc s8
            ldy #3
hl3         lda (src),y
            and #$0f
            tax
            lda (src),y
            cmp dnib,x
            bne s71
            inc hist0,x
s71         dey
            bpl hl3
s8          lda src
            clc
            adc #4
            sta src
            bcc s9
            inc src+1
s9          dec blcnt
            bne blloop
            lda #0
            ldx #32
            sta max
            stx maxcol
            ldy #15
m3          lda hist0,y
            beq s10
            cmp max
            beq s10
            bcc s10
            sta max
            sty maxcol
s10         dey
            bpl m3
            lda maxcol
            ldy #0
            sta (dst),y
            inc dst
            bne s11
            inc dst+1
s11         lda src+1
            cmp #$bd
            beq ok3
            jmp loop3
;
ok3         jsr gd_clrms
            ldx #0
            jsr tcopy
;
            lda #0			; now write 1st image
            sta nibflg
            jsr writefli
;
            ldy #192			; write bg 1
            jsr write0
;
            jsr writecram		; write color ram
;
            lda #$ff			; write 2nd image
            sta nibflg
            jsr writefli
;
            ldy #100			; write bg 2
            jsr write0
            jsr lastbyte		; close file
;
ende        sec				; leave saver
            rts

; ----------------------------------------- Care for proper end of file

lastbyte    lda packflag
            beq ende
            lda gbyte
            eor #$ff
            jsr pack
            dec cnt
;
verylast    lda #$ad
            jsr bsout
            lda #0
            jmp bsout

; ----------------------------------------- Save color ram

writecram   lda #<(cram)
            ldx #>(cram)
            sta src3
            stx src3+1
            lda #4
            sta crcnt
wcrl0       ldy #0
wcrl        lda (src3),y
            tax
            lda c64,x
            and #15
            jsr write
            iny
            cpy #250
            bne wcrl
            lda src3
            clc
            adc #250
            sta src3
            bcc wcrs
            inc src3+1
wcrs        dec crcnt
            bne wcrl0
            rts

; ----------------------------------------- Save 1 FLI image

writefli    lda #0
            sta data
;
            sta vrcnt
vrloop      lda vrcnt
            ldx #>($4000)
            sta src
            stx src+1
            lda #<(cram)
            ldx #>(cram)
            sta src3
            stx src3+1
loop12      ldy #15
            lda #0
ch3         sta hist,y
            dey
            bpl ch3
            ldy #3
hl4         lda (src),y
            bit nibflg		; determine which image
            bmi lonib1		; first or second?
            lsr
            lsr
            lsr
            lsr
lonib1      and #$0f
            tax
            inc hist,x
            dey
            bpl hl4
            lda #0
            tax
            sta hist,x
            tay
            lda (src3),y
            tax
            cpx #32
            beq s12
            lda #0
            sta hist,x
s12         sty max
            stx col0+3
            lda #32
            sta col0+1
            ldy #15
m1          lda hist,y
            beq s13
            cmp max
            beq s13
            bcc s13
            sta max
            sty col0+1
s13         dey
            bpl m1
            lda col0+1
            cmp #32
            beq s14
            tax
            lda #0
            sta hist,x
            sta max
            ldy #15
m2          lda hist,y
            beq s15
            cmp max
            beq s15
            bcc s15
            sta max
            sty col0+2
s15         dey
            bpl m2
s14         lda col0+1
            asl
            asl
            asl
            asl
            ora col0+2
            tax
            lda c64,x
;
            jsr write
;
            inc src3
            bne s16
            inc src3+1
s16         lda src
            clc
            adc #32
            sta src
            bcc s17
            inc src+1
s17         lda src+1
            cmp #$bd
            beq s18
            jmp loop12
;
s18         ldx #0		; write signature
l24         lda addtxt,x
            jsr write
            inx
            cpx #24
            bne l24
;
            lda vrcnt
            clc
            adc #4
            sta vrcnt
            cmp #32
            beq ok12
            jmp vrloop
;
ok12        lda #<($4000)
            ldx #>($4000)
            sta src
            stx src+1
            lda #<(cram)
            ldx #>(cram)
            sta src3
            stx src3+1
bmloop      lda #8
            sta zlcnt
zlloop      ldy #15
            lda #0
ch4         sta hist,y
            dey
            bpl ch4
            ldy #3
hl5         lda (src),y
            bit nibflg
            bmi lonib2
            lsr
            lsr
            lsr
            lsr
lonib2      and #$0f
            tax
            inc hist,x
            lda bits,y
            sta nr0,y
            lda #32
            sta col0,y
            dey
            bpl hl5
            lda #0
            tax
            sta hist,x
            stx col0
            ldy #0
            lda (src3),y
            cmp #32
            beq s19
            tax
            tya
            sta hist,x
            stx col0+3
s19         sty max
            ldy #15
m11         lda hist,y
            beq s20
            cmp max
            beq s20
            bcc s20
            sta max
            sty col0+1
s20         dey
            bpl m11
            lda col0+1
            cmp #32
            beq s21
            tax
            lda #0
            sta hist,x
            sta max
            ldy #15
m21         lda hist,y
            beq s22
            cmp max
            beq s22
            bcc s22
            sta max
            sty col0+2
s22         dey
            bpl m21
s21         ldx #0
lx          stx xbuf
            ldy #3
ly          lda col0,x
            cmp col0,y
            bcc s23
            beq s23
            pha
            lda col0,y
            sta col0,x
            pla
            sta col0,y
            lda nr0,x
            pha
            lda nr0,y
            sta nr0,x
            pla
            sta nr0,y
s23         dey
            cpy xbuf
            bne ly
            inx
            cpx #3
            bne lx
            ldy #$ff
            sty hstart
htl         iny
            lda col0,y
            clc
            adc col0+1,y
            lsr
            cmp #16
            bcc s24
            lda #15
s24         sta hend
            lda nr0,y
            ldx hstart
htl1        inx
            sta htab,x
            cpx hend
            bcc htl1
            cpx #15
            bcs s25
            stx hstart
            cpy #3
            bne htl
s25         lda #0
            sta byte
            ldy #3
rl          lda (src),y
            bit nibflg
            bmi lonib3
            lsr
            lsr
            lsr
            lsr
lonib3      and #$0f
            tax
            lda htab,x
            and mask,y
            ora byte
            sta byte
            dey
            bpl rl
;
            jsr write
;
            lda src
            clc
            adc #4
            sta src
            bne s26
            inc src+1
s26         dec zlcnt
            beq s27
            jmp zlloop
s27         inc src3
            bne s28
            inc src3+1
s28         lda src+1
            cmp #$bd
            beq ok121
            jmp bmloop
ok121       rts

; ----------------------------------------- Write .Y zero bytes

write0      lda #0
            jsr pack
            dey
            bne write0
            rts
;
; ----------------------------------------- Fileheader, Tables
;
fpheader    .by $f0,$3f
            .tx "funpaint (mt) "
packflag    .by $00,$ad
used        .by $ff
;
dnib        .by $00,$11,$22,$33,$44,$55,$66,$77
            .by $88,$99,$aa,$bb,$cc,$dd,$ee,$ff
;
bits        .by $00,$55,$aa,$ff
;
mask1       .by $c0                     ;11000000
            .by $30                     ;00110000
            .by $0c                     ;00001100
            .by $03                     ;00000011

; ----------------------------------------- Write 1 byte with activity display

write       pha
            txa
            pha
            tya
            pha
            jsr action
            pla
            tay
            pla
            tax
            pla
            jsr pack
            rts

; ----------------------------------------- Create C64 indexed color table

makec64     ldy #0
mc64        tya
            lsr
            lsr
            lsr
            lsr
            tax
            lda cols64,x
            and #$f0
            sta bbuf
            tya
            and #$0f
            tax
            lda cols64,x
            and #$0f
            ora bbuf
            sta c64,y
            iny
            bne mc64
            rts
;
cols64      .by $00,$66,$99,$bb		; conversion table GoDot<->C64
            .by $22,$44,$88,$cc
            .by $ee,$aa,$55,$ff
            .by $33,$77,$dd,$11
;
; ----------------------------------------- Variables
;
comp0       .by 0
col0        .by 0,0,0,0,0
nr0         .by 0,0,0,0
mask        .by 0,0,0,0
max         .by 0,0
maxcol      .by 0
bbuf        .by 0
xbuf        .by 0
blcnt       .by 0
vrcnt       .by 0
zlcnt       .by 0
hstart      .by 0
hend        .by 0
byte        .by 0
data        .by 0
crcnt       .by 0
nibflg      .by 0
hmbuf       .by 0,0,0

; ----------------------------------------- Compress algorithm

pack        bit packflag
            bmi wl4
            jmp bsout
wl4         cmp gbyte
            beq incr
p255        pha
            lda cnt
            beq wl1
            cmp #$ff
            beq wl0
            cmp #4
            bcs wl0
            lda gbyte
            cmp #$ad
            bne wl2
wl0         lda #$ad
            jsr bsout
            lda cnt
            jsr bsout
            lda #1
            sta cnt
wl2         lda gbyte
            jsr bsout
            dec cnt
            bne wl2
wl1         pla
            sta gbyte
incr        inc cnt
            lda cnt
            cmp #$ff
            bne wl3
            lda gbyte
            jsr p255
            dec cnt
wl3         rts

; ----------------------------------------- Requester: choose compress

sevpack     inc used
            inc used
            lda #$80
            sta packflag
            sec
            rts

; ----------------------------------------- Requester: choose no compress

sevraw      inc used
            lda #0
            sta packflag
            sec
            rts

; ----------------------------------------- Requester: choose cancel

sevcanc     lda #3
            sta used
            sec
            rts

; ----------------------------------------- Screenlist for Requester

fplist      .by 0
            .by 5,11,18,15,$81,0,0
            .by 83,1,22,5,32,70,21,14,80,1,9,14,20,32,73,73,0
            .by 10,11,18,3,$ca,<(sevpack),>(sevpack)
            .by 80,1,3,11,5,4,32,68,1,20,1,0
            .by 13,11,18,3,$ca,<(sevraw),>(sevraw)
            .by 82,1,23,32,68,1,20,1,0
            .by 16,11,18,3,$ca,<(sevcanc),>(sevcanc)
            .by 67,1,14,3,5,12,32,83,1,22,5,0
            .by $c0,7,13,11,83,5,12,5,3,20,32,77,15,4,5,0
            .by $80

; ----------------------------------------- 

modend      .en

.petscii
.include "godotlib.lib"
.ob "mchgpart,p,w"

; ------------------------------------------------------------ 
;
; mod..ChangeParts
; Modul zum Wechseln zwischen Partitionen auf CMD-Drives
; fuer die GO64! aufbereitet als Kurs
;
;    1.00, 10.4.97: first release
;    1.01, 14.1.02, added: highlight name of current partition
;
; ------------------------------------------------------------ 
            .ba $c000
; ------------------------------------------------------------ 
;
            .eq parts		=$30
            .eq lo		=$32
            .eq hi		=lo+1
            .eq cpart		=$34

            .eq status		=$90
            .eq zeile		=$b0

            .eq sel3		=$1597
            .eq shunits		=$1630
            .eq hilite		=$17b9
            .eq fil3		=$19ba
            .eq decpt		=$19c7
            .eq reqpt		=$19d4
            .eq cmdpt		=$1a63
            .eq filetype	=$1d29

            .eq scrollback	=$bf00
            .eq pbuffer		=drives+32
            .eq modend		=drives+32
            .eq basin2		=$f1ad
            .eq untalk		=$ffab

; ------------------------------------------------------------ 

header      jmp start
            .by $20
            .by 0
            .by 0
            .wo (modend)
            .wo 0
            .tx "ChangePartitions"
            .tx "1.01"
            .tx "15.01.02"
            .tx "A. Dettke       "
;
; ------------------------------------------------------------ Main

start       jsr checkram	; active drive is RAM?
            jsr setunits	; display drive numbers
            jsr svlname		; save ls_lastname
            ldx #<(changelst)	; display requester
            ldy #>(changelst)
            jsr gd_screen
            jsr shdrives	; display drive types
            lda #255		; get # of current partition
            jsr getparts
            lda gpbuffer+2
            sta cpart		; and store
            jsr shall		; display partitions
            jsr gd_eloop	; wait...
            jsr svlname		; restore ls_lastname
evleave     sec			; leave module
            rts

; ------------------------------------------------------------ 

setunits    ldx #30		; get information from GoDot's kernel
            lda #$80
            bne su1
su0         lda filetype+29,x	; add to screenlist
su1         sta drives,x	; last byte $80 (end of screenlist) 
            dex
            bpl su0
            rts

; ------------------------------------------------------------ 

shdrives    lda #$60		; patch GoDot's kernel
            sta cmdpt		; (don't show "RAM")
            sta reqpt
            dec decpt
            jsr fil3		; display drive types
            lda #$20		; unpatch
            sta reqpt
            lda #$d0
            sta cmdpt
            inc decpt
            rts

; ------------------------------------------------------------ 

checkram    lda ls_drive	; get current drive number
            cmp #12
            bne cr0
            lda ls_bootdrive	; if RAM: change to boot drive
            sta ls_drive
cr0         rts

; ------------------------------------------------------------ 

svlname     ldx #15		; exchange ls_lastname and buffer
sl0         ldy ls_lastname,x
            lda pnbuf,x
            sta ls_lastname,x
            tya
            sta pnbuf,x
            dex
            bpl sl0
            rts

; ------------------------------------------------------------ Scroll back

evscrlrw    jsr checkpt		; CMD partition? (no return if not)
            ldx previous	; start of list?
            beq noscroll
            dec previous	; no, scroll back
            lda scrollback,x
            bne scr0

; ------------------------------------------------------------ Scroll ahead

evscrlvw    jsr checkpt		; CMD partition? (no return if not)
            ldx boxline		; window full?
            cpx #17
            bcc noscroll
            ldx #8		; 8: second part of dirlist

; ------------------------------------------------------------ 

scroll      jsr setbuffer
            ldy #1
            lda (parts),y
scr0        sta allparts	; get partition number...
            jsr gd_prepdir
            jsr shnext		; and display
noscroll    clc
            rts

; ------------------------------------------------------------ Init buffer for Partition data

setbuffer   jsr mal18		; one entry: 18 bytes, multiply .X by 18
            clc
            lda parts		; add to base address
            adc #<(pbuffer)
            sta parts
            lda parts+1
            adc #>(pbuffer)
            sta parts+1
            rts

; ------------------------------------------------------------ .X times 18

mal18       stx lo
            ldy #0
            sty hi
            stx sc_pos
            sty sc_pos+1
            asl lo
            rol hi
            jsr gd_mal20
            sec
            lda sc_pos
            sbc lo
            sta parts
            lda sc_pos+1
            sbc hi
            sta parts+1
            rts

; ------------------------------------------------------------ Show Partition Names

shparts     jsr gd_sproff	; mouse pointer off
            sta counter		; counter to zero

gp2         lda allparts	; current pertition number
            cmp #255		; skip 255 (# for current part)
            beq gp4
            jsr getparts	; get partition info (current # in .A)
            lda gpbuffer	; get partition type
            beq gp3		; 0   : not created
            cmp #8		; gt 7: not allowed
            bcs gp3

            ldx boxline		; number of current row in dir window
            dex
            cpx #16		; window exceeded?
            beq gp5		; yes, skip

            jsr setbuffer	; compute buffer address
            jsr boxprep		; clear screen row
            jsr mkbuffer	; create buffer and output data
            lda gpbuffer+2	; get current part #
            cmp cpart		; is active part?
            bne gp31
            lda #16 		; yes, hilite name
            sta hilite
gp31        jsr namout		; display partition name
            stx hilite		; clear hilite flag
            inc boxline		; next row in window
gp3         inc counter		; inc counter
            inc allparts	; inc partition number
            lda counter		; got info of 16 partitions?
            cmp #17
            bcc gp2		; no, loop

            lda boxline		; window full?
            cmp #17
            bcc gp4		; no, then finish

gp5         inc previous	; yes, store first part # of window
            ldx previous
            lda pbuffer+1
            sta scrollback,x

gp4         jsr gd_xmess	; floppy message
            jmp gd_spron	; mouse pointer on

; ------------------------------------------------------------ Init buffer

mkbuffer    lda gpbuffer	; get partition type
            ldy #0
            ldx #0
            sta (parts),y	; write to buffer
            iny
            lda gpbuffer+2	; get partition number
gp1         sta (parts),y	; write to buffer
            sta sc_movetab,x	; and to convert buffer
            inx
            iny
            lda gpbuffer+1,y	; get partition name
            cpy #18		; end of buffer?
            bne gp1		; no, loop
            jmp gd_cnvbc	; convert buffer to screencode

; ------------------------------------------------------------ Error Messages

mess1       ldx #0              ; no cmd
            .by $2c
mess2       ldx #32             ; cp/m
            .by $2c
mess3       ldx #64             ; print
            .by $2c
mess4       ldx #96             ; forgn
            .by $2c
mess5       ldx #128            ; no sel
            .by $2c
mess6       ldx #160            ; again
            ldy #0
ms0         lda mslist,x
            sta message,y
            inx
            iny
            cpy #32
            bne ms0
            jsr gd_clrms
messout     ldx #<(message)
            ldy #>(message)
            jmp gd_xtxout2

; ------------------------------------------------------------ Clear row

boxprep     clc
            lda boxline
            adc box
            ldx box+1
            inx
            ldy #16
            jmp clrname

; ------------------------------------------------------------ Display partition name

namout      ldx #0		; copy name to output buffer
np0         lda sc_movetab+1,x	; up to $00
            beq np1
            sta pname,x
            inx
            bne np0
np1         lda #32		; fill buffer with spaces
np3         cpx #16
            bcs np2
            sta pname,x
            inx
            bne np3
np2         ldx #<(pname)	; output name to screen
            ldy #>(pname)
            jmp gd_xtxout2

; ------------------------------------------------------------ Drive check

checkpt     ldx ls_drive	; drive number
            lda cmdoffs-8,x
            tax
            lda filetype,x	; drive is a CMD device?
            cmp #$3e
            beq cp0
            jsr mess1		; no, error message ("No CMD")
            pla
            pla
cp0         rts

; ------------------------------------------------------------ Select drive

evunits     jsr gd_prepdir	; clear dir window
            jsr shunits		; display drives
shall       jsr shinfo		; display device types
            jsr checkpt		; CMD? (doesn't return if not)
            ldx #01		; yes, start at #1
            stx allparts
            dex			; and set previous to zero
            stx previous
shnext      lda #1		; first row in window
            sta boxline
            jsr shparts		; display partition names
            clc
            rts

; ------------------------------------------------------------ Show Device types

shinfo      ldx ls_drive	; drive number
            lda ls_cmdtypes-8,x	; get drive type
            cmp #$46		; F (= FD)
            beq info1
            cmp #$48		; H (= HD)
            beq info2
            cmp #$52		; R (= RD)
            beq info3

info5       ldx #36		; then: no CMD ("Commodore")
            .by $2c
info4       ldx #27		; Partition info
            .by $2c
info3       ldx #18		; drive type infos
            .by $2c
info2       ldx #9
            .by $2c
info1       ldx #0

            ldy #0
ms1         lda infolist,x
            sta info,y
            inx
            iny
            cpy #9		; width of output text (=br)
            bne ms1

            jsr clrinfo		; output info to screen
infout      ldx #<(info)
            ldy #>(info)
            jmp gd_xtxout2

; ------------------------------------------------------------ clear output

clrinfo     lda #20		; =zl
            ldx #26		; =sp
clrname     jsr gd_setpar
            jmp gd_clrline

; ------------------------------------------------------------ Select Partition

evselect    lda sc_clicked	; double click?
            beq sel0
            jmp evchange	; yes, activate

sel0        sta ls_dirmask	; clear dirmask
            ldx zeile		; get mouse position (vertically)
            dex
            dex
            stx currline
            jsr sel3
            lda ls_flen		; filename clicked?
            bne sel1
            jmp shinfo		; no, show drive type instead
;
sel1        ldx #8		; yes, clear info buffer
            lda #32
si0         sta pinfo,x
            dex
            bpl si0
            ldx currline	; process info:
            jsr setbuffer	; clear buffer
            lda (parts),y	; get partition type...
            sta ptype
            pha

            iny
            lda (parts),y	; and number
            sta pnum
            jsr mknum
            ldy #6
            ldx #0
si1         lda sy_numbers,x
            beq si2
            sta pinfo,y		; write number to buffer
            inx
            iny
            bne si1

si2         pla
            tax
            dex
            lda ptoffs,x
            tax
            ldy #0
si3         lda ptnames,x	; add name
            sta pinfo,y
            inx
            iny
            cpy #4
            bne si3

            lda #$3a		; add colon
            sta pinfo,y
            iny
            lda #35		; add "#"
            sta pinfo,y
            jsr info4		; output info
            clc
            rts

; ------------------------------------------------------------ Convert to digits

mknum       sta $63
            lda #0
            sta $62
            inc 1
            ldx #$90
            sec
            jsr $bc49
            jsr $bddf
            dec 1
            rts

; ------------------------------------------------------------ Change Partition

evchange    lda ls_flen		; Partition selected?
            bne ec4
            jmp mess5		; no, error message

ec4         lda ptype		; get patition type
            cmp #7		; check if legal partition
            bne ec0
            jmp mess4
ec0         cmp #6
            bne ec1
            jmp mess3
ec1         cmp #5
            bne ec2
            jmp mess2

ec2         lda pnum		; get Part. number
            sta cpcom+2
            ldx #<(cpcom)	; send command to drive
            ldy #>(cpcom)
            lda #3
            jsr gd_sendcom

            jsr gd_xmess	; evaluate reply
            lda ls_err1
            and #15
            beq ec3		; error?
            jmp mess6

ec3         ldx ptype		; no, set parameters for new partition
            ldy dtracks-1,x
            lda dsectrs-1,x
            pha
            lda dtypes-1,x
            ldx ls_drive
            sta ls_units-8,x
            pla
            sta ls_ftabs-8,x
            tya
            sta ls_ftabt-8,x
            sec
            rts

; ------------------------------------------------------------ Retrieve Partition data

getparts    sta gpcom+3
            ldx #<(gpcom)
            ldy #>(gpcom)
            lda #5
            jsr gd_sendcom
            lda ls_drive
            ldy #$6f
            jsr gd_talk
            ldx #0
            stx status
gp0         jsr basin2
            sta gpbuffer,x
            inx
            cpx #31
            bne gp0
            jsr untalk
            jmp gd_xmess

; ------------------------------------------------------------ Variables

pnbuf       .tx "0123456789012345"
message     .tx "0123456789012345"
pname       .tx "6789012"
info        .tx "345678901"
            .by 0
infolist    .by 70,68,32,70,12,15,16,16,25	; FD Floppy
            .by 72,1,18,4,4,9,19,11,32  	; Harddisk
            .by 32,82,1,13,76,9,14,11,32	; RamLink
pinfo       .tx "012345678"             	; PInfo
            .by 67,15,13,13,15,4,15,18,5	; Commodore

; ------------------------------------------------------------ Messages

mslist      .by 32,32,84,8,9,19,32,4,18,9,22,5,32
            .by 14,15,20,32,16,1,18,20,9,20,9,15,14,5,4,46,32,32,32
            .by 32,67,80,47,77,32,16,1,18,20,9,20,9,15,14,32
            .by 14,15,20,32,1,3,3,5,19,19,9,2,12,5,46,32
            .by 32,32,80,18,9,14,20,32,66,21,6,6,5,18,32
            .by 14,15,20,32,1,3,3,5,19,19,9,2,12,5,46,32,32
            .by 70,15,18,5,9,7,14,32,77,15,4,5,32,65,18,5,1,32
            .by 14,15,20,32,1,3,3,5,19,19,9,2,12,5
            .by 32,32,32,32,32,78,15,32,16,1,18,20,9,20,9,15,14,32
            .by 19,5,12,5,3,20,5,4,46,32,32,32,32,32
            .by 32,32,32,32,78,15,20,32,3,8,1,14,7,5,4,46,32
            .by 84,18,25,32,1,7,1,9,14,46,32,32,32,32,32

; ------------------------------------------------------------ Variables

cmdoffs     .by 34,41,49,57
dtracks     .by 1,18,18,40
dsectrs     .by 34,1,1,3
dtypes      .by $20,$41,$10,$80
ptype       .by 0
pnum        .by 0
;
gpbuffer    .tx "0123456789012345678901234567890"
;
previous    .by 0
boxline     .by 1
currline    .by 0
counter     .by 0
allparts    .by 1
gpcom       .tx "g-p"
            .by 1,13
cpcom       .tx "cP."
;
ptoffs      .by 0,4,8,12,16,20,24
;
ptnames     .by 78,1,20,22,$31,$35,$34,$31
            .by $31,$35,$37,$31,$31,$35,$38,$31
            .by 67,80,47,77,80,82,78,32
            .by 70,15,18,77

; ------------------------------------------------------------ Screenlist

changelst   .by 0,0,3,34,25,$01,0,0
box         .by 1,4,18,18,$60
            .wo (evselect)
            .by 19,4,18,3,$20,0,0
            .by 2,23,13,6,$60
            .wo (evunits)
            .by 11,22,3,4,$d0
            .wo (evscrlrw)
            .by 30,0
            .by 15,22,3,4,$40
            .wo (evscrlvw)
            .by 13,25,11,3,$d0
            .wo (evchange)
            .by 67,8,1,14,7,5,80,0
            .by 16,25,11,3,$d0
            .wo (evleave)
            .by 76,5,1,22,5,0
            .by 19,25,11,3,$20,0,0
            .by 22,3,34,3,$49
            .wo (gd_xmess)
            .by $c0,0,26,5,85,14,9,20,19,0
            .by $c0,7,26,6,67,8,1,14,7,5,0
            .by $c0,8,24,10,80,1,18,20,9,20,9,15,14,19,0
            .by $c0,16,22,1,31,0
drives      .by $80

            .en


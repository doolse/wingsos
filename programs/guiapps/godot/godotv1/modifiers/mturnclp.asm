.petscii
.include "godotlib.lib"
.ob "mturnclp,p,w"
; --------------------------------------- 
;
;       mod.TurnClip
; 1.01: bugfix: 'Leave on start' crashed
;       new   : 'NewClip' at any time
;             : 'Warning' automatically off
;             : added Activity Display
;
; --------------------------------------- 
            .ba $c000
; --------------------------------------- 
            jmp start
            .by $20
            .by $00,$00
            .wo (modend)
            .by $00,$00
            .tx "TurnClip 90 deg."
            .tx "1.01"
            .tx "14.04.01"
            .tx "A.Dettke        "
; --------------------------------------- 

            .eq buf		=$30
            .eq dst		=$32
            .eq hcnt		=$34
            .eq tilecnt		=$35
            .eq cntbr		=$36
            .eq cntx		=$37
            .eq breite		=$38
            .eq hoehe		=$39
            .eq toggle		=$3a
            .eq adcnt		=$3b

            .eq lutable		=$cd00
            .eq vram2		=$ce0b
            .eq vram1		=$de0b
            .eq cstart		=$ee0b

; --------------------------------------- 
start 		lda gd_modswitch	; care for module management
		pha
		lda sc_screenvek	; save vector to mainlst
		sta mainlst
		lda sc_screenvek+1
		sta mainlst+1
		jsr warning		; display warning
		bcs scr02		; left by "No", leave module

		jsr setdata
		ldx #<(turnlst)
		ldy #>(turnlst)
scr0		jsr gd_screen
		jsr lolite
		ldx #11
		jsr lolite2
		lda warnflag
		bne scr01
		jsr warn
scr01		jsr gd_eloop		; wait for events

scr02		pla 			; restore module management flag
		sta gd_modswitch
		sec			; finished
		rts
; ----------------------------------- Init Activity Display

initad      ldy #60		; create bright bar
            lda #0
adl0        sta $0100+3,y
            dey
            bpl adl0
            sty $0100
            sty $0100+1
            sty $0100+2

            lda #15		; light gray
            sta $d029
            sta $d029+1
            lda $d01d	; expand x
            ora #12
            sta $d01d

            lda spritehi	; sprites 2 & 3 beyond 255
            ora #12
            sta spritehi
            lda #8		; x position: 33 & 36
            sta $d004
            lda #32
            sta $d004+2
            lda #144		; y position: 18
            sta $d005
            sta $d005+2

            lda #4		; sprite 2 & 3 at area 4 ($0100)
            sta $07f8+2
            sta $07f8+3

            lda $d015	; activate sprites
            ora #12
            sta $d015
            lda #5		; set progression counter
            sta adcnt
            rts
; ----------------------------------- 

clearad     lda $d015	; display mouse pointer only
            and #$f3
            sta $d015
            lda spritehi
            and #$f3
            sta spritehi
            sec
            rts

; --------------------------------------- Warning
warning		lda warnflag		; warn-requester disabled?
		bne wn1			; yes, leave
		
		lda #0			; set result flag
		sta exflag
		ldx #<(warnlst)		; display requester
		ldy #>(warnlst)
		jsr gd_screen
		ldx #7
		jsr lolite2
		jsr gd_eloop		; wait for events
		lda exflag		; result
		beq wn0
wn1		clc
		.by $24
wn0 		sec
		rts
; --------------------------------------- Warning anyway
evanyway	inc exflag		; enable warning
		sec 
		rts
; --------------------------------------- Turn the Clip
evexec		jsr setstart		; Start address of Clip
		jsr setbr		; width of Clip
		lda sc_lastclpho	; and its height
		sta hoehe
		lda sc_lastclpbr
		sta breite
		sta gr_redisp
		jsr initad
		jsr tobuf		; write clip to buffer

		lda emptyflag		; erase 4Bit?
		bne ex3
		ldy #0			; yes
		sty sc_texttab
		ldx #$40
		stx sc_texttab+1
		ldx #125
ex4		sta (sc_texttab),y
		iny
		bne ex4
		inc sc_texttab+1
		dex
		bne ex4

ex3		lda dirflag		; direction to turn to?
		beq ex0
		jsr backl24		; left
		beq ex1

ex0		jsr lookup		; right
		jsr backr24

ex1		lda adjflag		; take over Clip values?
		bne ex2
		lda #0			; yes
		sta sc_lastclpzl
		sta sc_lastclpzl+1
		sta sc_clipzl
		sta sc_clipzl+1
		ldx sc_lastclpbr
		lda sc_lastclpho
		sta sc_lastclpbr
		sta sc_clipzl+2
		stx sc_lastclpho
		stx sc_clipzl+3
ex2		jmp clearad		; finished, clip turned

; --------------------------------------- Set a new Clip
evnewclip	jsr setdata3
		jsr gd_eloop
		jsr rp5
		clc
		rts
; --------------------------------------- 
setdata3	jsr setdata2
		ldx #<(newlst)
		ldy #>(newlst)
		jmp gd_screen		
; --------------------------------------- Input of new Clip values
evinpzl     	ldx #0			; Flag for ZEILE (row)
		.by $2c
evinpsp		ldx #1			; Flag for SPALTE (column)
		.by $2c
evinpbr		ldx #2			; Flag for BREITE (width)
		.by $2c
evinpho		ldx #3			; Flag for HOEHE (height)
		stx inmerk
		ldx #10			; Init Special Input 
		stx sc_iflag
ar1		lda ziff,x
		sta sy_global,x
		dex
		bpl ar1
		lda #<(sy_global)
		sta ls_vekta8
		lda #>(sy_global)
		sta ls_vekta8+1
		jsr gd_xinput		; Input

		lda #>(ls_nambuf)	; evaluate Init
		sta ls_vekta8+1
		lda #<(ls_nambuf)
		sta ls_vekta8
		ldy #2
		sta ls_nambuf,y
		ldy #$ff
		sta $14
		sta $14+1
; --------------------------------------- 
getword     iny				; convert to digits
            lda (ls_vekta8),y
            cmp #32
            beq getword
            cmp #$3a
            bcs gw0
            sec
            sbc #$30
            sec
            sbc #$d0
            bcs gw0
            sbc #$2f
            sta $07
            lda $14+1
            sta ls_temp
            cmp #$19
            bcs gw0
            lda $14
            asl
            rol ls_temp
            asl
            rol ls_temp
            adc $14
            sta $14
            lda ls_temp
            adc $14+1
            sta $14+1
            asl $14
            rol $14+1
            lda $14
            adc $07
            sta $14
            bcc getword
            inc $14+1
            bne getword

gw0         lda inmerk			; Limit check: Zeile+Höhe, Spalte+Breite
            tax
            eor #3
            tay
            txa
            beq gw1			; Zeile? (y=3 for Hoehe)
            cpx #1			; Spalte? (y=2 for Breite)
            beq gw1

            lda $14			; Breite/Hoehe: get value
            beq gw2			; when zero: no change
            bne gw4

gw1         lda sc_lastclpzl,y		; get associated value
gw3         clc				; add input value
            adc $14
            sta $14+1			; save
            sec
            lda max-2,y			; compare to Maximum 
            sbc $14+1
            bcc gw2			; ok?
            lda $14			; yes, set new value
            sta sc_lastclpzl,x

gw2         lda #0			; finish
	    sta sc_iflag
	    jsr repairlst		; repair Exec-Gadget 
	    jsr setdata			; change digits in main requester
	    jsr setdata3		; accommodate secondary requester
	    clc
	    rts
;
gw4         lda sc_lastclpzl,y		; Breite: y=1/Höhe: y=0
            pha
            tya				; turn value again
            eor #3
            tay
            pla
            jmp gw3			;  and countercheck

; ---------------------------------------- restore List 

repairlst	ldy #13			; "Execute" instead of "NewClip"
rpl0		lda oldexgad,y
		sta execgad,y
		dey
		bpl rpl0
		rts

; --------------------------------------- Set direction

evdir		ldx dirflag		; 0: right/1: left
		beq ed0
		ldx #$ff
ed0		inx
		stx dirflag
		bne ed1
		ldy #1
		.by $2c
ed1		ldy #3
		ldx #1
ed2		lda direction,y
		sta dirtxt,x
		dey
		dex
		bpl ed2
            	jsr gettab
ed3         	ldy #6
            	jmp gd_xtxout1
; ----------------------------------------- Take over new Clip 
evadjust	lda adjflag
		beq adj0
		ldx #$ff
adj0		inx
		stx adjflag
		bne adj1
		ldy #2
		.by $2c
adj1		ldy #5
		ldx #2
adj2		lda yesno,y
		sta adjtxt,x
		dey
		dex
		bpl adj2
		lda #<(adjgad)
		sta sc_screentab
		lda #>(adjgad)
adj3        	jsr gettab2
		ldy #6
		jmp gd_xtxout1
; ----------------------------------------- Erase 4Bit 
evempty		lda emptyflag
		beq em0
		ldx #$ff
em0		inx
		stx emptyflag
		bne em1
		ldy #2
		.by $2c
em1		ldy #5
		ldx #2
em2		lda yesno,y
		sta emptytxt,x
		dey
		dex
		bpl em2
		lda #<(emptygad)
		sta sc_screentab
		lda #>(emptygad)
            	bne adj3
; ----------------------------------------- disable Warnscreen 
warn		ldx warnflag
		beq ew0
		ldx #$ff
ew0		inx
		stx warnflag
		rts
; ----------------------------------------- Hilite off
lolite		ldx #3
lolite2		ldy #3
scr1		lda carea,x
		sta sc_zl,y
		dex
		dey
		bpl scr1
		ldx #2
		jmp gd_fcol
; ----------------------------------------- Output Text
gettab      lda #<(dirgad)
            sta sc_screentab
            lda #>(dirgad)
gettab2     sta sc_screentab+1
            lda #0
            tax
            tay
gt0         jsr gd_setpos
            cpx #3
            bne gt0
            stx sc_ho
            jsr gd_trim
            jmp gd_initmove
; --------------------------------------- Set data NewClip
setdata2	ldx #1
sdt0		lda toptxt,x
		sta zltxt,x
		lda toptxt+3,x
		sta sptxt,x
		lda widthtxt,x
		sta brtxt,x
		lda hitetxt,x
		sta hotxt,x
		dex
		bpl sdt0
		rts
; --------------------------------------- Set data values
setdata		lda sc_lastclpzl	; topleft corner position: row
		jsr gd_xcnvdez
		sta toptxt+1
		cpx #$30
		bne sd0
		ldx #32
sd0		stx toptxt

		lda sc_lastclpzl+1	; and column
		jsr gd_xcnvdez
		sta toptxt+4
		cpx #$30
		bne sd1
 		ldx #32
sd1		stx toptxt+3

		lda sc_lastclpbr	; width
		pha
		jsr gd_xcnvdez
		sta widthtxt+1
		cpx #$30
		bne sd2
		ldx #32
sd2		stx widthtxt

		lda sc_lastclpho	; height
		tay
		jsr gd_xcnvdez
		sta hitetxt+1
		cpx #$30
		bne sd3
		ldx #32
sd3		stx hitetxt

		pla
		cmp #25			; width exceeding 25?
		beq sd41
		bcs seterrwid		; yes, error message

sd41		cpy sc_lastclpbr	; height exceeding width?
		bcc sd42
		tax			; yes, height matching width?
		tya
		cmp limits,x
		beq sd4
		bcc sd4
		bcs seterrh		; no, error

sd42		cmp limits,y		; width matching height?
		beq sd4
		bcs seterrhit		; no, error

sd4		ldy #15			; "Ready..."
sd6		lda readytxt1,y 
		sta proctxt1,y
		dey
		bpl sd6
		ldy #10
sd7 		lda readytxt2,y
		sta proctxt2,y
		dey
		bpl sd7
		rts
; --------------------------------------- 
seterrwid	lda #25			; maximum width: 25
		bne sd8
; --------------------------------------- 
seterrh		lda limits,x		; wrong height
		jsr sd8			; set correct value
		ldy #5
sd13		lda height,y		; set "Height" 
		sta proctxt1,y
		dey
		bpl sd13
		rts
; ---------------------------------------  
seterrhit	lda limits,y		; get Limit 
sd8		jsr gd_xcnvdez		; "Width exceeding..."
		sta wlimtxt+1
		cpx #$30
		bne sd10
		ldx #32
sd10		stx wlimtxt
		ldy #15
sd11 		lda wexctxt1,y
		sta proctxt1,y
		dey
		bpl sd11
		ldy #10
sd12		lda wexctxt2,y
		sta proctxt2,y
		dey
		bpl sd12

		ldy #13			; "NewClip" instead of "Execute"
sd9		lda setclgad,y
		sta execgad,y
		dey
		bpl sd9
		rts
; ---------------------------------------- Start address of Clip
setstart    lda #$40		; 4Bit from $4000
            sta sc_texttab+1
            ldy #0
            sty sc_texttab
stcl        lda sc_lastclpzl	; row * 1280
            beq scp1
            sta clpzl
scp4        clc
            lda sc_texttab+1
            adc #5
            sta sc_texttab+1
            dec clpzl
            bne scp4
scp1        lda sc_lastclpzl+1	; plus column * 32
            beq scp5
            sta clpsp
scp6        clc
            lda sc_texttab
            adc #$20
            sta sc_texttab
            bcc scp7
            inc sc_texttab+1
scp7        dec clpsp
            bne scp6
scp5        rts
; ---------------------------------------- compute width of 4bit Clip
compclip    sty ls_vekta8+1	; Hibyte to zero
            clc			; Lobyte times 32
            lda sc_lastclpbr
mal32       sta ls_vekta8
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
; ---------------------------------------- compute width of clip
setbr       sty ls_vekta8+1	; Hibyte to zero
            clc			; Lobyte times 8
            lda sc_lastclpbr
mal8        sta ls_vekta8
            asl ls_vekta8
            rol ls_vekta8+1
            asl ls_vekta8
            rol ls_vekta8+1
            asl ls_vekta8
            rol ls_vekta8+1
	    rts
; --------------------------------------- create Lookup Table for mirroring
lookup		ldx #0
		txa
lu1		sta lutable,x
		inx
		bne lu1
lu0		txa
		lsr
		ror lutable,x
		lsr
		ror lutable,x
		lsr
		ror lutable,x
		lsr
		ror lutable,x
		ora lutable,x
		sta lutable,x
		inx
		bne lu0
		rts
; --------------------------------------- buffer data in $2000
tobuf		ldy #0
		sty lire
		sty nibble
		sty buf
		ldx #4
		stx tilecnt
		ldx #$20
		stx buf+1
tr01		tya		; erase Buffer ($2000-$3fff)
		sta (buf),y
		jsr incbuf
		lda buf+1
		cmp #$40
		bne tr01
		lsr buf+1	; Buffer to $2000 again
		
		lda sc_texttab	; save Start adresse 
		sta dst
		lda sc_texttab+1
		sta dst+1

tr4		lda hoehe	; counter for vertical tiles
		sta hcnt

tr3		ldx #8		; counter for 1 tile
		bit lire
		bmi tr02
		jsr first	; left Pixels
		beq tr03
tr02		jsr second	; right Pixels

tr03		clc		; point to next below
		lda sc_texttab
		adc #$e0
		sta sc_texttab
		lda sc_texttab+1
		adc #4
		sta sc_texttab+1
		dec hcnt	; reached height?
		bne tr3

tr05		lda lire	; yes, choose right nibble
		eor #$80
		sta lire
		bmi tr6

		inc dst		; when left: new Start address
		bne tr5
		inc dst+1

tr5		dec tilecnt	; counter for horizontal tiles
		bne tr6
		ldx #4
		stx tilecnt
		clc		; down, next tile
		lda dst
		adc #28
		sta dst
		bne tr6
		inc dst+1

tr6		dec adcnt	; display activity counter
		bne s8
		lda #5		; reset counter if down
		sta adcnt
		inc $d005	; then advance bar
		inc $d005+2

s8		lda dst		; set Start address
		sta sc_texttab
		lda dst+1
		sta sc_texttab+1
		jsr cou6	; count width
		bne tr4		; finished?
		rts 		; yes
; --------------------------------------- Inc Buffer pointer
incbuf		inc buf
		bne ib0
		inc buf+1
ib0		rts
; --------------------------------------- Dec Destination
decdst		lda dst
		bne dd0
		dec dst+1
dd0		dec dst
		rts
; --------------------------------------- get first column
first		lda (sc_texttab),y
		and #$f0	; left
		bit nibble
		bpl tr11
		lsr
		lsr
		lsr
		lsr
tr11		ora (buf),y	; write to Buffer 
		sta (buf),y
		clc		; point to next pixel below
		lda sc_texttab
		adc #4
		sta sc_texttab
		bcc tr2
		inc sc_texttab+1
tr2		lda nibble	; target byte finished?
		eor #$80
		sta nibble
		bne tr22
		jsr incbuf	; yes, continue counting
tr22		dex		; 1 tile finished?
		bne first
		rts
; --------------------------------------- get second column
second		lda (sc_texttab),y
		and #$0f	; right
		bit nibble
		bmi tr110
		asl
		asl
		asl
		asl
tr110		ora (buf),y	; write to Buffer 
		sta (buf),y
		clc		; point to Pixel below
		lda sc_texttab
		adc #4
		sta sc_texttab
		bcc tr200
		inc sc_texttab+1
tr200		lda nibble	; target byte finished?
		eor #$80
		sta nibble
		bne tr220
		jsr incbuf	; yes, continue counting
tr220		dex		; 1 tile finished?
		bne second
		rts
; --------------------------------------- Back to 4Bit left
backl24		lda #$40	; Start 4Bit at $4000
		sta sc_texttab+1
		sta dst+1
		lsr		; Start Buffer at $2000
		sta buf+1
		ldy #0
		sty sc_texttab
		sty buf
		sty dst

		clc		; width*height*32+BUF: BUF end
		tya
		sta ls_vekta8+1
		ldy hoehe
bc6		adc breite
		bcc bc60
		clc
		inc ls_vekta8+1
bc60		dey
		bne bc6
		sta ls_vekta8
		jsr mal32
		clc
		lda buf
		adc ls_vekta8
		sta buf
		lda buf+1
		adc ls_vekta8+1
		sta buf+1

		sty ls_vekta8+1	; height*32+DST: target right border
		ldx hoehe
		dex
		txa
		jsr mal32
		clc
		lda sc_texttab
		adc ls_vekta8
		sta sc_texttab
		sta dst
		lda sc_texttab+1
		adc ls_vekta8+1
		sta sc_texttab+1
		sta dst+1

bc4		lda hoehe	; height is now width
		sta hcnt
		sty ls_vekta8+1
		jsr mal8	; width times 8 as counter

bc3		ldx ls_vekta8	; move one tile line
bc1		ldy #0
		sec		; source minus 4
		lda buf
		sbc #4
		sta buf
		bcs bc0
		dec buf+1
bc0		lda (buf),y	; move 4 Bytes 
		sta (dst),y
		iny
		cpy #4
		bne bc0

		dec hcnt	; at left border?
		bne bc01
		lda hoehe	; yes, start over right hand
		sta hcnt
		clc		; target plus 4
		lda sc_texttab
		adc #4
		sta dst
		sta sc_texttab
		lda sc_texttab+1
		adc #0
		sta dst+1
		sta sc_texttab+1
		bne bc5

bc01		sec		; no, move left
		lda dst
		sbc #32
		sta dst
		bcs bc5
		dec dst+1

bc5		dec adcnt	; display activity counter
		bne s6
		lda #201	; reset counter if down
		sta adcnt
		inc $d005	; then advance bar
		inc $d005+2

s6		dex		; 32 Bytes per tile
		bne bc1

		clc		; next line then
		lda sc_texttab
		adc #$e0
		sta sc_texttab
		sta dst
		lda sc_texttab+1
		adc #4
		sta dst+1
		sta sc_texttab+1

		dec breite	; as many times as height
		bne bc3
		rts
; --------------------------------------- Back to 4Bit right
backr24		lda #$40	; Start 4Bit $4000
		sta sc_texttab+1
		sta dst+1
		lsr		; Start Buffer $2000
		sta buf+1
		ldy #0
		sty sc_texttab
		sty buf
		sty dst

		sty ls_vekta8+1	; (height-1)*32+DST+3: target right border
		ldx hoehe
		dex
		txa
		jsr mal32
		clc
		lda ls_vekta8
		adc #3
		sta ls_vekta8
		bcc rc40
		inc ls_vekta8+1 
rc40		clc
		lda sc_texttab
		adc ls_vekta8
		sta sc_texttab
		sta dst
		lda sc_texttab+1
		adc ls_vekta8+1
		sta sc_texttab+1
		sta dst+1

rc4		lda hoehe	; height is now width
		sta hcnt
		sty ls_vekta8+1
		jsr mal8	; width times 8 as counter

rc3		ldx ls_vekta8	; move one tile line
		stx cntbr

rc1		ldx #4
		stx cntx
rc0		lda (buf),y	; move 4 Bytes 
		tax
		lda lutable,x
		sta (dst),y
		jsr incbuf
		jsr decdst
		dec cntx
		bne rc0

		dec hcnt	; at left border?
		bne rc01
		lda hoehe	; yes, start over right hand
		sta hcnt
		clc		; target plus 4
		lda sc_texttab
		adc #4
		sta dst
		sta sc_texttab
		lda sc_texttab+1
		adc #0
		sta dst+1
		sta sc_texttab+1
		bne rc5

rc01		sec		; no, move left
		lda dst
		sbc #28
		sta dst
		bcs rc5
		dec dst+1

rc5		dec adcnt	; display activity counter
		bne s7
		lda #201	; reset counter if down
		sta adcnt
		inc $d005	; then advance bar
		inc $d005+2

s7		dec cntbr	; 32 Bytes per tile
		bne rc1

		clc		; next line then
		lda sc_texttab
		adc #$e0
		sta sc_texttab
		sta dst
		lda sc_texttab+1
		adc #4
		sta dst+1
		sta sc_texttab+1

		dec breite	; as many times as height
		bne rc3
		rts
; --------------------------------------- 
redis       jsr setcolors
            lda gr_cmode
            beq rp4
            lda #$18
            sta $d016
rp4         lda #$1b
            sta $d018
            lda #$3b
            sta $d011
            lda gr_redisp
            bne rp3
rp1         lda sc_keyprs
            ora sc_stop
            beq rp1
;
rp2         jsr getcolors
            jsr tmode
rp5	    lda sc_maincolor
	    sta $d021
	    ldx mainlst
	    ldy mainlst+1
	    jsr gd_screen
            ldx turnlstbk
            ldy turnlstbk+1
            jsr gd_screen
	    jsr lolite
	    ldx #11
	    jsr lolite2
rp3         clc
            rts
;
tmode       ldx #$13
            lda #$1b
            stx $d018
            sta $d011
            lda #$08
            sta $d016
            rts
;
setcveks    sei
            lda #$35
            sta 1
            lda #>(cstart)
            ldx #$d8
            bne scv0
setbveks    lda #>(vram1)
            ldx #4
            dec 1
scv0        stx $b3
            ldy #0
            sty sc_merk
            sty $b2
            dey
setlast     sty gr_bkcol
            ldy #<(cstart)
            sty $20
            sta $21
            lda #<(500)
            sta $a8
            lda #>(500)
            sta $a9
            ldy #0
            rts
;
count       inc $20
            bne cou5
            inc $21
cou5        inc $b2
            bne cou6
            inc $b3
cou6        lda $a8
            bne cou7
            dec $a9
cou7        dec $a8
            lda $a8
            ora $a9
            rts
;
getcolors   jsr setcveks
stco        lda ($b2),y
            sta sc_merk
            inc $b2
            bne stc0
            inc $b3
stc0        lda ($b2),y
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lda sc_merk
            sta ($20),y
            jsr count
            bne stco
            jsr setbveks
stc1        lda ($b2),y
            sta ($20),y
            jsr count
            bne stc1
            ldy gr_bkcol
            bpl scv1
            ldy #0
            lda #>(vram2)
            jsr setlast
            beq stc1
scv1        lda #$36
            sta 1
            cli
            lda $d021
            and #15
            sta gr_bkcol
            rts
;
setcolors   lda gr_bkcol
            sta $d021
            pha
            jsr setcveks
stc2        sty sc_merk
            lda ($20),y
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            lsr
            rol sc_merk
            sta ($b2),y
            inc $b2
            bne stc3
            inc $b3
stc3        lda sc_merk
            sta ($b2),y
            jsr count
            bne stc2
            jsr setbveks
stc4        lda ($20),y
            sta ($b2),y
            jsr count
            bne stc4
            ldy gr_bkcol
            bpl scv1
            pla
            tay
            lda #>(vram2)
            jsr setlast
            beq stc4
;
redisplay   lda gr_redisp
            pha
            ldx #1
            stx gr_redisp
            dex
            stx sc_stop
            jsr redis
            pla
            sta gr_redisp
            rts
; --------------------------------------- Show Clip
showclip	lda sc_screenvek
		pha
		lda sc_screenvek+1
		pha
		ldx #0			; empty keyboard buffer 
                stx sy_tbuffer
		jsr redisplay		; Graphics on
		lda sc_lastclpzl	; set Clip data
		sta sc_zl
		lda sc_lastclpzl+1
		sta sc_sp
		lda #0			; Blinkflag off
		sta toggle
sh0		jsr invertcl		; invert area
		ldx #250		; wait
		jsr gd_dl2
		lda toggle		; toggle Blinkflag 
		eor #1
		sta toggle
		lda $dc00		; Joystick?
		and #16
		beq sh1			; yes, finish
		lda sy_tbuffer		; key?
		beq sh0			; no, continue

sh1		lda toggle		; Blink Phase is ON?
		and #1
		beq sh0			; no, wait
		jsr invertcl		; then OFF
		jsr rp2			; restore Screen 
		pla
		tay
		sta sc_screenvek+1
		pla
		tax
		sta sc_screenvek
		jsr gd_screen
		clc
		rts
; --------------------------------------- invert Clip 
invertcl	ldx #0			; Screenflag to Text mode
		stx sc_loop
		jsr gd_initmove		; compute Address
inv0		ldy #0			; reached Clip width?
inv1		cpy sc_lastclpbr
		beq inv2		; yes
		lda sc_vekt20+1		; otherwise: invert
		pha
		lda (sc_vekt20),y
		eor #$ff
		sta (sc_vekt20),y
		lda sc_vekt20+1		; Colorram too
		clc
		adc #$d4
		sta sc_vekt20+1
		lda (sc_vekt20),y
		eor #$ff
		sta (sc_vekt20),y
		pla
		sta sc_vekt20+1
		iny
		bne inv1
inv2		inx			; reached Clip height?
		cpx sc_lastclpho
		beq inv3		; yes
		jsr gd_plus40		; no, next line
		bne inv0
inv3		rts
; --------------------------------------- Variables
exflag		.by 0
dirflag		.by 1
adjflag		.by 0
warnflag	.by 0
emptyflag	.by 0
direction	.by 94,94,91,91
limits		.by 25,25,25,25,25,25,25,25,25,25
		.by 25,23,21,19,18,17,16,15,14,13
		.by 12,12,11,11,10,10
ziff        	.tx " 0123456789"
max         	.by 40,25
inmerk		.by 0
yesno		.ts "yes no"
height		.ts "Height"
wexctxt1	.ts " Width exceeding"
wexctxt2	.ts "limit ("
wlimtxt		.ts "25)."
readytxt1	.ts "Ready to process"
readytxt2	.ts "  image!   "
oldexgad	.by 21,10,9,3,$c7
		.wo evexec
		.ts "Execute"
setclgad	.by 21,10,9,3,$c7
		.wo evnewclip
		.ts "NewClip"
mainlst		.wo 0
turnlstbk	.wo turnlst
clpzl		.by 0
clpsp		.by 0
lire 		.by 0
nibble		.by 0
carea		.by 7,12,11,9,11,15,7,1,5,12,8,1
; --------------------------------------- Screenlist Warning
warnlst		.by 0
		.by 5,10,18,15,$91,0,0
		.ts "TurnClip@"

		.by 17,10,8,3,$c7
		.wo evanyway
		.ts "Anyway@"

		.by 17,18,10,3,$cd
		.wo wn0
		.ts "No,leave@"

		.by $c0,7,11,14
		.ts "Executing this@"
		.by $c0,8,12,11
		.ts "module will@"
		.by $c0,10,14,7
		.ts "destroy@"
		.by $c0,12,10,16
		.ts "your image data!@"
		.by $c0,14,14,8
		.ts "Proceed?@"
		.by $80
; --------------------------------------- Screenlist TurnClip
turnlst		.by 0
		.by 2,10,18,22,$91,0,0
		.ts "TurnClip@"

topgad		.by 4,21,7,3,$8f,0,0
toptxt		.ts " 0, 0@"

widthgad	.by 6,24,4,3,$8e,0,0
widthtxt	.ts " 1@"

hitegad		.by 8,24,4,3,$8e,0,0
hitetxt		.ts " 1@"

dirgad		.by 10,24,4,3,$ce
		.wo evdir
dirtxt		.by 91,91,0

adjgad		.by 12,23,5,3,$ce
		.wo evadjust
adjtxt		.ts "yes@"

emptygad	.by 14,23,5,3,$ce
		.wo evempty
emptytxt	.ts "yes@"

warngad		.by 16,19,9,3,$cf
		.wo evnewclip
warntxt		.ts "NewClip@"

execgad		.by 21,10,9,3,$c7
		.wo evexec
		.ts "Execute"

sublst		.by 0
		.by 21,19,9,3,$cd
		.wo wn0
		.ts " Leave @"

		.by $c0,4,11,7
		.ts "Clip at@"
		.by $c0,6,11,8
		.ts "Width is@"
		.by $c0,8,11,9
		.ts "Height is@"
		.by $c0,10,11,7
		.ts "Turn to@"
		.by $c0,12,11,9
		.ts "Take over@"
		.by $c0,14,11,10
		.ts "Empty 4Bit@"

		.by $c0,18,10,16
proctxt1	.ts "Ready to process@"
		.by $c0,19,13,11
proctxt2	.ts "  image!   @"
		.by $80
; --------------------------------------- Screenlist NewClip 
newlst		.by 0
		.by 9,6,16,10,$01,0,0

inpzl		.by 10,10,4,3,$e0
		.wo evinpzl
zltxt		.ts "00@

inpsp		.by 10,17,4,3,$e0
		.wo evinpsp
sptxt		.ts "00@"

inpbr		.by 13,10,4,3,$e0
		.wo evinpbr
brtxt		.ts "00@"

inpho		.by 13,17,4,3,$e0
		.wo evinpho
hotxt		.ts "00@"

		.by 16,6,8,3,$c7
		.wo showclip
		.ts " Show @"

		.by 16,14,8,3,$cd
		.wo wn0
		.ts "Accept@"

		.by $c0,10,6,3
		.ts "Row@"
		.by $c0,10,13,3
		.ts "Col@"
		.by $c0,13,6,3
		.ts "Wid@"
		.by $c0,13,13,3
		.ts "Hgh@"
		.by $80
; --------------------------------------- 
modend      	.en

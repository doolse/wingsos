.petscii
.include "godotlib.lib"
; ------------------------------------------------------------ 
;
;  mod.ShowAll
;
;  Module to toggle the flag for showing all files or specific files
;  in a directory other than in mod..FileCopy
;
;  1.00: 09.08.01, first release
;
; ------------------------------------------------------------ 
            .ob "mshowall,p,w"
            .ba $c000
; ------------------------------------------------------------ 
            .eq dst		=$30
; ------------------------------------------------------------ Header

modul       jmp start
            .by $20
            .by $00,$00
            .wo modend
            .by $00,$00
            .tx "Toggle ShowAll  "
            .tx "1.00"
            .tx "09.08.01"
            .tx "A.Dettke        "

; ------------------------------------------------------------ Main
start       lda #21
            ldx #27
            ldy #12
            jsr gd_setpar
            jsr gd_clrline

            lda ls_showfiles
            beq showall

            lda #0
            sta ls_showfiles
            beq txout2

showall     inc ls_showfiles

txout1      lda #<txall
            ldx #>txall
            bne txout3

txout2      lda #<txsome
            ldx #>txsome

txout3      sta dst
            stx dst+1
            ldy #12
to0         lda (dst),y
            sta sc_movetab,y
            dey
            bpl to0
            jsr gd_xtxout3
            clc
            rts

txall       .ts " All Files  @"
txsome      .ts "Def.Settings@"

modend      .en

;
; (c) 2024 v0.1 Jouni 'Mr.Spiv' korhonen
;
; Z80 decompressor for zxpac4
;
; Use e.g. following to assemble:
;  pasmo --tapbas --alocal -1 z80/z80dec.asm z80/z80.tap z80/z80.map   
;
;

ASCII_LITERALS  equ 1

        org     $8000



; Inputs:
;  DE = destination address
;  HL = start of compressed file
;
; Returns:
;
;
; Trashes:
;
;
;
;
main:


    IF  ASCII_LITERALS
        ld      a,0
    ELSE
        ld      a,1
    ENDIF


        call    z80dec
        ret

z80dec: ;
        xor     a
        ld      b,a
        ld      a,0x7f
        and     (hl)
        ld      c,a
        ld      (_pmr_value),bc
        inc     hl
        ;
        ; (SP) contains the (initial) PMR offset
        ;
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      a,0x80
        ;
        ; BC = length of the original file
        ;  A = empty bitbuffer
        ; 
        ; PMR is on the top of stack..
        ;

_main_loop:
        add     a,a
        jr nz,  _not_empty1
        ld      a,(hl)
        inc     hl
        adc     a,a
_not_empty1:
        jr c,   _tag_match_or_pmr
_tag_literal:
        push    de
        ldi
        jp po,  _done
        ex      (sp),hl
        srl     (hl)
        pop     hl
        jr nc,  _tag_literal
_tag_match_or_pmr:
        add     a,a
        jr nz,  _not_empty2
        ld      a,(hl)
        inc     hl
        adc     a,a
_not_empty2:
        jr c,   _tag_pmr
_tag_match:
        ;
        ; Match found:
        ;  mininum match = 2
        ;  offset > 0
        ;
        ex      af,af'
        ld      a,(hl)
        inc     hl


        ret
_pmr_value:
        dw      0
_tag_pmr:


_done:  pop     af      ; Excess word
        ret








        END main

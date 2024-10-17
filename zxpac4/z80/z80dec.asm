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
MAX32K_WIN      equ 1


GETBIT  MACRO
        local   not_empty
        add     a,a
        jr nz,  not_empty
        ld      a,(hl)
        inc     hl
        adc     a,a
not_empty:
        ENDM


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
        ld      hl,file
        ld      de,0xc000
        call    z80dec
        exx
        ret

z80dec: ;
        push    de
        xor     a
        ld      d,a
        ld      a,0x7f
        and     (hl)
        ld      e,a
        
        inc     hl          ; With Z80 only 16bit file length supported
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      a,0x80
        push    bc
        exx
        pop     bc
        pop     de
        exx

        ;
        ; DE' = PTR to destination
        ; DE  = last offset/PMR offset
        ; HL = PTR to compressed data
        ; BC' = length of the original file
        ;  A = empty bitbuffer
        ;
        ; The maximum offset supported by 8bit decoder is 32767 not 131071!!
        ; This fact allows us to cut some bytes from the offset decorer.
        ;
        ; Every compressed file starts with an implicit literal, thus there
        ; is no tag for it.
        jr      _tag_literal
        ;
_main_loop:
        GETBIT
        jr c,   _tag_match_or_pmr
        ;
_tag_literal:
        ;
        ; If all literals are ASCII then the byte is in format 'LLLLLLLx',
        ; where 'LLLLLLL' is 7 bits of the literal and
        ; 'x' is the next tag bit allowing to skip the code between the
        ; _main_loop and _tag_literal labels.
        ;
        ; The last byte of the compressed file, if a literal ASCII, must
        ; be in format '0LLLLLLL'.
        ;
        push    hl
        exx
        pop     hl
_tag_literal_loop:
        ldi
        ret po
    IF ASCII_LITERALS
        ex      de,hl
        dec     hl
        srl     (hl)
        inc     hl
        ex      de,hl   ; 9b / 56t
        jr nc,  _tag_literal_loop    ;  
        push    hl
        exx
        pop     hl
    ENDIF
    IF !ASCII_LITERALS
        exx
        inc     hl
        jr      _main_loop
    ENDIF
_tag_match_or_pmr:
        GETBIT
    IF MAX32K_WIN
        ld      bc,0x0300
    ELSE
        ld      bc,0x0400
    ENDIF
        jr c,   _tag_pmr_matchlen
        ;
        ; Match found:
        ;  mininum match = 2
        ;  offset > 0
        ;
        ld      d,c
        ld      e,(hl)
        inc     hl
        bit     7,e
        jr z,   _get_offset_done
_get_offset_tag_loop:
        GETBIT
        jr nc,  _get_offset_tag_term
        inc     c
        djnz    _get_offset_tag_loop
_get_offset_tag_term:
        GETBIT
        rl      c           ; Note, sets C=0
        jr z,   _get_offset_done
_get_offset_bits_loop:
        GETBIT
        rl      e
        rl      d
        dec     c
        jr nz,  _get_offset_bits_loop
_get_offset_done:
        db      0xfe
_tag_pmr_matchlen:  
        dec     c
        ex      af,af'
        ld      a,c
        ex      af,af'
        ; DE = offset
        ; C = 0 if normal match
        ; C = -1 if PMR
        ld      bc,0x0701
_get_matchlen_loop:
        GETBIT
        jr nc,  _get_matchlen_exit
        GETBIT
        rl      c
        djnz    _get_matchlen_loop
_get_matchlen_exit:
        ex      af,af'
        add     a,c
        push    de
        exx
        pop     hl
        ex      de,hl
        ; DE = offset
        ; HL = PTR to destination
        push    hl
        and     a
        sbc     hl,de
        pop     de
        and     a
        jr z,   _last_byte_copy
_copy_loop:
        ldi
        dec     a
        jr nz,  _copy_loop
_last_byte_copy:
        ex      af,af'
        ldi
        ret po
        exx
        jp      _main_loop
        ;

file:
        incbin  "../32k.pac"

        END main

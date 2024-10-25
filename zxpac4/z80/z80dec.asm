;
; (c) 2024 v0.1 Jouni 'Mr.Spiv' korhonen
;
; Z80 decompressor for zxpac4
;
; Use e.g. following to assemble:
;  pasmo --tapbas --alocal -1 z80/z80dec.asm z80/tap.tap
;
; Notes:
;  This version of the decompressor is for files that must
;  decompressed from lower memory to higher.
;  During compression none of the 'reverse' options were 
;  used.
;

ASCII_LITERALS  equ 1   ; 1 assumes 7bit ascii
MAX32K_WIN      equ 1   ; 1 assumes max 32768 bytes sliding window
INPLACE         equ 1   ; 1 will change the source compressed file
                        ; during decompression. The file can be
                        ; used for decomporession only once!!


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
;   DE = destination address
;   HL = start of compressed file
;
; Returns:
;   Nothing meaningful..
;   BC = 0
;   DE = PTR to the first memory location after the decompressed file
;   
; Trashes:
;   A, A', BC, DE, HL, IX
;
;
;
main:
        ld      hl,file
        ld      de,0xc000
        call    z80dec
        ret

z80dec: ;
        xor     a
        ld      ixh,a
        ld      a,0x7f
        and     (hl)
        ld      ixl,a
        
        inc     hl          ; With Z80 only 16bit file length supported
        inc     hl
        ld      b,(hl)
        inc     hl
        ld      c,(hl)
        inc     hl
        ld      a,0x80

        ;
        ; DE = PTR to destination
        ; IX = last offset/PMR offset
        ; HL = PTR to compressed data
        ; BC = length of the original file
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
        ; 'x' is the next tag bit allowing to skip GETBIT macro..
        ;
        ; The last byte of the compressed file, if a literal ASCII, must
        ; be in a preshifted format '0LLLLLLL'.
        ;
    IF INPLACE && ASCII_LITERALS
        ; This solution alters the source file by doing the shift before
        ; moving the data. Another side effect is that the preshifting
        ; of the last literal shall not be done in this case.
        srl     (hl)
    ENDIF
        ldi
        ret po
    IF ASCII_LITERALS
        ; This is a sad piece of code but could not figure out any
        ; better solution that would not a) use IX/IY or b) put anything
        ; into the stack before the conditional return.
    IF !INPLACE
        ex      de,hl
        dec     hl
        srl     (hl)
        inc     hl
        ex      de,hl   ; 9b / 56t
    ENDIF
        jr nc,  _tag_literal
    ELSE
        jr      _main_loop
    ENDIF
_tag_match_or_pmr:
        push    bc
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
        push    de
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
        push    de
        pop     ix
        pop     de
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
        rl      c       ; clears C-flags
        djnz    _get_matchlen_loop
_get_matchlen_exit:
        ; C-flag = 0
        ; DE = PTR to destination
        ; HL = PTR to source
        ; IX = offset
        ; Stack 0: remainig bytes count

        ex      (sp),hl
        ; Stack 0: PTR to source
        ; HL = remaining bytes count
        push    hl
        ; Stack 2: PTR to source
        ; Stack 0: remaining bytes count
        push    de
        ; Stack 4: PTR to source
        ; Stack 2: remaining bytes count
        ; Stack 0: PTR to destination
        ex      de,hl
        ; HL = PTR to destination
        push    ix
        pop     de
        sbc     hl,de
        ; HL = PTR to history
        pop     de
        ; DE = PTR to destination

        ex      af,af'
        add     a,c
        pop     bc
        jr z,   _last_byte_copy
_copy_loop:
        ldi
        dec     a
        jr nz,  _copy_loop
_last_byte_copy:
        ex      af,af'
        ldi
        pop     hl
        ret po
    IF ASCII_LITERALS && !INPLACE
        jp      _main_loop
    ELSE
        jr      _main_loop
    ENDIF
        ;

file:
        incbin  "../32k.pac"

        END main

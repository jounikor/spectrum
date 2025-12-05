;
;
;
;
;
;
;
;
;
;

TANS_NUM_FREQS  equ     8+8+10
INITIAL_STATE   equ     3
SPREAD_STEP     equ     5




        org     $8000


main:
        

        ld      de,$8000
        ld      a,INITIAL_STATE
        ld      hl,test_data_end-1
        ld      L_table,ix

        ld      b,8
        call    decode_tans_freqs
        ld      b,8
        call    decode_tans_freqs
        ld      b,10
        call    decode_tans_freqs

        ret

;
; Inouts:
;  C = bitbuffer
;  HL = ptr to current source byte
;  DE = ptr to destination
;
; Returns:
;  DE = ptr to end of array
;  HL = ptr to next source byte
;   C = bit buffer
;
; Trashes:
;  A,B
;
decode_tans_freqs:
        ; get number of leading 1-bits
_rice_loop:
        xor     a
_get_num_ones
        adc     a,0
        sla     c
        jr nz,  _no_reload1
        ld      c,(hl)
        dec     hl
        rl      c
_no_reload1:
        jr c,   _get_num_ones

        ; get 2 lowest bits
        ld      b,2
_get_two_bits:
        sla     c
        jr nz,  _no_reload2
        ld      c,(hl)
        dec     hl
        rl      c
_no_reload2:
        adc     a,a
        djnz    _get_two_bits

        ; store frequency.. DE must be 256 bytes aligned and
        ; arrays start from a 256 byte boundary
        ld      (de),a
        dec     e
        jr nz,  _rice_loop
        
        ret


test_data:
        db      $01,$00,$08,$22,$a6,$0e,$64,$a9,$f8
        db      $00,$40,$72,$f7
test_data_end:

        ; Must be 256 aligned
        org     ($ + 255) & $ff00
tans_arrays:
        ds      TANS_NUM_FREQS
tans_arrays_end:


        END     main



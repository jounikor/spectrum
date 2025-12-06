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

TANS_NUM_FREQS  equ     10+10+10
INITIAL_STATE   equ     3
SPREAD_STEP     equ     5

M_              equ     32
LS_LEN          equ     10
Y_              equ     M_
L_              equ     0

EXTRACT_SYMBOL_FREQ     MACRO
        local   rice_loop
        local   get_num_ones
        local   no_reload1
        local   no_reload2
        local   get_two_bits

rice_loop:
        xor     a
get_num_ones
        adc     a,0
        sla     d
        jr nz,  no_reload1
        dec     hl
        ld      d,(hl)
        rl      d
no_reload1:
        jr c,   get_num_ones

        ; get 2 lowest bits
        ld      b,2
get_two_bits:
        sla     d
        jr nz,  no_reload2
        dec     hl
        ld      d,(hl)
        rl      d
no_reload2:
        adc     a,a
        djnz    get_two_bits

        ENDM

EXTRACT_BYTE    MACRO   reg,ptr
        dec     ptr
        ld      reg,(ptr)
        ENDM

        org     $8000


main:
        

        ld      hl,test_data_end
        ld      ix,L_table_offset
        ld      a,INITIAL_STATE
        ld      de,0x8000


        ld      b,10
        call    tans_build_decoding_tables
        ld      b,10
        call    tans_build_decoding_tables
        ld      b,10
        call    tans_build_decoding_tables

        ret

tans_build_decoding_tables:
        ; A = xp
        ; B = M i.e. the size of Ls i.e. Ls_len, which is fixed in this case.
        ; E = s(ymbol)
_main_loop:
        push    bc
        push    af

        ; Bit extract trashes B, which is OK in this case. Bits are returned
        ; in A and Z-flag is set accordingly to what A contains.
        EXTRACT_SYMBOL_FREQ hl
        jr z,   _zero_symbol
        
        ld      b,a     ; B = counter
        ld      c,a     ; C = p
        pop     af
_per_symbol_loop:
        push    ix
        push    af
        
        add     a,ixl
        ld      ixl,a
        
        ; self.y[xp] = p
        ld      (ix+Y_),c
        inc     c
        
        ; self.L[xp] = s
        ld      (ix+L_),e
       
        ; spread step
        pop     af
        pop     ix
        add     a,SPREAD_STEP
        and     M_-1

        ;
        djnz    _per_symbol_loop

        ; Old trick to avoid a jump..
        db      $fe
_zero_symbol:
        pop     af
        pop     bc
        inc     e
        djnz    _main_loop

        ret

;----------------------------------------------------------------------------
;
; Inputs:
;  IX = ptr to Y_ and L_ tables. Tables must fit into 256 bytes aligned page.
;  A  = state
;  C  = bit buffer
;  HL = ptr to source
;
; Returns:
;
;  A  = next state
;  B  = symbol
;
; Changes:
;  C,HL,IXL
;
; Trashes:
;  
; Note:
;  The bit stream extractor expects the following:
;   * bits are left-shifted out
;   * a 1-bit is added as the least significant bit as the end marker
;   * Bytes are read from the stream in a reverse order
;

tans_decode_symbol:
        add     a,ixl
        ld      ixl,a

        ; get a symbol
        ld      b,(ix+L_)
        ld      a,(ix+Y_)

        ; k-tableless adding of bits for the next state
_do_while:
        sla     c
        jr nz,  _not_empty
        EXTRACT_BYTE c,hl
        rl      c
_not_empty:
        adc     a,a
        cp      M_
        jr c,   _do_while

        ; Scale state between [0..M) or rather [0..L)
        and     M_-1
        ret




test_data_end:





        org     ($+255) & 0xff00

tmp_Ls_table:
        ds      M_
tmp_Ls_table_end:

L_table_literal_run:
        ds      M_
Y_table_literal_run:
        ds      M_

L_table_match:
        ds      M_
Y_table_match:
        ds      M_

L_table_offset:
        ds      M_
Y_table_offset:
        ds      M_


        END     main



;-----------------------------------------------------------------------------
; 
; v0.1 (c) 2025 Jouni 'Mr.Spiv' Korhonen
;
;

        org     $8000

;----------------------------------------------------------------------------
;
; Notes concerning this particular tANS decoder Z80 implementation.
; Some of the properties are configurable compile time:
;  * Size of the alphabet (no power of two requirement) -> LS_LEN
;  * Total sum of symbol frequencies must be a power of two -> M_
;  * Maximum bits for a symbol frequency is 8 -> SYMBOL_FREQ_BITS
;  * Two decoding tables size of M_ entries are built:
;    - L_ state to symbol mapping 
;    - Y_ next state y-values  
;  * There is no k-table, since with Z80 we can shift one bit at time from an
;    input stream anyway, the k-value becomes implicitly known.
;  * Indices to both Y_ and L_ tables must be arranged such that given
;    a 256 bytes page pointed by IX all table access must be within
;    that page. This means IXL+M_+M_ is always <= 256.
;
; The decoding is done from the end to the start, thus there is no need to
; encode the original stream of symbols in a reverse order. This arrangement
; fits well for the intended inplace decompression use case.
;

INITIAL_STATE   equ     3
SPREAD_STEP     equ     5

SYMBOL_FREQ_BITS    equ 5

M_              equ     32
LS_LEN          equ     8
Y_              equ     M_
L_              equ     0


EXTRACT_SYMBOL_FREQ     MACRO
        local   not_empty_bitbuf
        local   bitbuf_loop

        xor     a
        ld      b,SYMBOL_FREQ_BITS
bitbuf_loop:
        sla     d
        jr nz,  not_empty_bitbuf
        dec     hl
        ld      d,(hl)
        rl      d
not_empty_bitbuf:
        adc     a,a
        djnz    bitbuf_loop
        ENDM

EXTRACT_BYTE    MACRO   reg,ptr
        dec     ptr
        ld      reg,(ptr)
        ENDM


; The main function & loop is just for testing purposes..

main:
        ld      hl,temp_ls_table_end
        ld      ix,L_table
        ld      b,LS_LEN
        call    tans_build_decoding_tables
     

        ld      hl,temp_packed_array_end
        ld      de,temp_dest_end
        ld      c,0x80
        ld      a,8
        ex      af,af'
        ld      a,34
loop:
        ex      af,af'
        ld      ix,L_table
        call    tans_decode_symbol
        ex      de,hl
        dec     hl
        ld      (hl),b
        ex      de,hl
        ex      af,af'
        dec     a
        jr nz,  loop

        ret



;----------------------------------------------------------------------------
;
; Build decoding tables
;
; Inputs:
;  HL = a ptr to the end of the Ls array. The array MUST have
;       symbold frequencies in a reverse order, i.e. the last
;       frequency is for the symbol 0 etc.
;       The array has a predefined length LS_LEN.
;  IX = a ptr to generated tables. HL must be such that
;       the generated table fits into 256 bytes aligned
;       memory area. Start of the table can be shifted
;       by M_ bytes.
;  B  = the number of entries in the Ls array.
;  
; Returns:
;  D (reminder of the bit buffer)
;
; Changes:
;  HL
;
; Trashes:
;  AF,B,D,E
;
;
; Note: The sum of symbol frequencies must be a power of two!
;       The bit buffer is reset on each time this function is called,
;       thus plan accordingly i.e. the new set of symbol frequencies
;       must start from a byte boundary.
;

tans_build_decoding_tables:
        ld      a,INITIAL_STATE
        ld      de,0x8000

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

;;
;;
; The rest are just for testing purposes..
;
; The symbol freq table must be in reverse order !
temp_ls_table:
        ;   7 6 5 4 3 2 1 9 and total sum of 32
        ;db  2,6,9,0,0,2,6,7
        ;db  00010001b,10010010b,00000000b,00001000b,11000111b
        db  11000010b,00100100b,00000000b,10000100b,00111001b
temp_ls_table_end:

;
; Original 34 symbols
;  [0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0]
;
; Final state: 8
; 
;   k  b
; [(2, 1), (2, 0), (4, 12), (4, 9), (2, 0), (4, 13), (2, 0), (4, 5),
;  (2, 1), (4, 10), (2, 1), (3, 2), (2, 3), (3, 2), (2, 3), (3, 2),
;  (2, 3), (2, 0), (4, 12), (2, 1), (4, 10), (4, 9), (4, 4), (2, 0),
; (4, 5), (2, 1), (4, 2), (2, 1), (4, 2), (2, 1), (2, 2), (3, 4),
; (2, 3)]


; Symbols must be decoded from the end to start.
temp_packed_array:
    IF 0
        .01
        .00
        .1100
        .1001
        .00
        .1101
        .00
        .0101
        .01
        .1010
        .01
        .010
        .11
        .010
        .11
        .010
        .11
        .00
        .1100
        .01
        .1010
        .1001
        .0100
        .00
        .0101
        .01
        .0010
        .01
        .0010
        .01
        .10
        .100
        .11
    ENDIF
        db  00000100b,00100111b,01001101b,10100101b,01101001b,10101101b
        db  11100001b,00110100b,10001001b,01001010b,10010010b,11100100b
temp_packed_array_end:



        org     ($+255) & 0xff00

; L table for state to symbol mapping
; [6,0,5,0,2,6,1,5,0,2,6,1,5,0,5,6,1,5,0,5,6,1,5,0,5,7,1,6,0,5,7,1]
L_table:
        ds      M_
; y:
; [7,D,D,7,2,8,6,E,8,3,9,7,F,9,9,A,8,10,A,A,B,9,11,B,B,2,A,6,C,C,3,B]
Y_table:
        ds      M_

        org     ($+255) & 0xff00
        db      0xff
temp_dest:
        ds      34
temp_dest_end:
        db      0xff




        END main

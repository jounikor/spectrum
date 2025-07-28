;-----------------------------------------------------------------------------
; 
; v0.1 (c) 2025 Jouni 'Mr.Spiv' Korhonen
;
; NOTE: still untested code!
;

        org     $8000
        

INITIAL_STATE   equ     3
M_              equ     32
LS_LEN          equ     8
SYMBOL_FREQ_BITS    equ 5
SPREAD_STEP     equ     5

; Indices to both Y and L tables.. Tables must be arranged such that
; give a 256 bytes page pointed by IX all table access must be within
; that page. That means IXL+M+M is always < 256.
;
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
        dec     hl
        ld      reg,(ptr)
        ENDM



main:
        ld      hl,temp_ls_table_end
        ld      ix,L_table
        call    tans_build_decoding_tables
        
        ret

; Build decoding tables
;
; Inputs:
;  HL = a ptr to the end of the Ls array. The array MUST have
;       symbold frequencies in a reverse order, i.e. the last
;       frequency is for the symbol 0 etc.
;       The array has a predefined length LS_LEN.
;  IX = a ptr to generated tables. HL must be such that
;       the geenrated table fits into 256 bytes aligned
;       memory area. Start of the table can be shiftes
;       by L bytes.
;  
; Returns:
;  D (bitbuffer)
;
; Changes:
;  HL, IX
;
; Trashes:
;  A,AF',B,D,E
;
;
; Note: The sum of symbol frequencies must be a power of two!
;

tans_build_decoding_tables:
        ld      a,INITIAL_STATE
        ld      b,LS_LEN
        ld      de,0x8000

        ; A = xp
        ; B = M i.e. the size of Ls i.e. Ls_len, which is fixed in this case.
        ; E = s
_main_loop:
        push    bc
        ex      af,af'

        ; Bit extract trashes B, which is OK in this case. Bits are returned
        ; in A and Z-flag is set accordingly to what A contains.
        EXTRACT_SYMBOL_FREQ hl
        jr z,   _zero_symbol
        
        ld      b,a     ; B = counter
        ld      c,a     ; C = p
        ex      af,af'
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

        db      $fe
_zero_symbol:
        ex      af,af'
        pop     bc
        inc     e
        djnz    _main_loop

        ret

;
;
; Inputs:
;  IX  = ptr to Y_ and L_ tables. Tables must fit into 256 bytes aligned page.
;  AF' = state
;  C   = bit buffer
;  HL  = ptr to source
;
; Returns:
;
;  AF' = next state
;  B   = symbol
;
; Changes:
;  C,HL,IXL
;
; Trashes:
;  
;

tans_decode_symbol:
        ex      af,af'
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
        ex      af,af'
        ret

; The symbol freq table must be in reverse order !
temp_ls_table:
        ;   7 6 5 4 3 2 1 9 and total sum of 32
        ;db  2,6,9,0,0,2,6,7
        ;db  00010001b,10010010b,00000000b,00001000b,11000111b
        db  11000010b,00100100b,00000000b,10000100b,00111001b
temp_ls_table_end:


        org     ($+255) & 0xff00

; L table for state to symbol mapping
; [6,0,5,0,2,6,1,5,0,2,6,1,5,0,5,6,1,5,0,5,6,1,5,0,5,7,1,6,0,5,7,1]
L_table:
        ds      M_
; y:
; [7,D,D,7,2,8,6,E,8,3,9,7,F,9,9,A,8,10,A,A,B,9,11,B,B,2,A,6,C,C,3,B]
Y_table:
        ds      M_




        END main

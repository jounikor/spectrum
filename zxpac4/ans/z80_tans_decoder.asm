;-----------------------------------------------------------------------------
; 
; v0.1 (c) 2025 Jouni 'Mr.Spiv' Korhonen
;
; NOTE: still untested code!
;

        org     $8000
        

INITIAL_STATE   equ     3
M_              equ     32
SPREAD_STEP     equ     5

; Indices to both Y and L tables.. Tables must be arranged such that
; give a 256 bytes page pointed by IX all table access must be within
; that page. That means IXL+M+M is always < 256.
;
Y_              equ     M_
L_              equ     0


EXTRACT_SYMBOL_FREQ     MACRO   ptr
        dec     ptr
        ld      a,(ptr)
        ENDM

EXTRACT_BYTE    MACRO   reg,ptr
        dec     ptr
        ld      reg,(ptr)
        ENDM



main:


; Build decoding tables
;
; Inputs:
;  HL = a ptr to Ls
;  IX = a ptr to generated tables. HL must be such that
;       the geenrated table fits into 256 bytes aligned
;       memory area. Start of the table can be shiftes
;       by L bytes.
;  
; Returns:
;  Nothing
;
; Changes:
;  HL, IX
;
; Trashes:
;  A,B,C,E
;
;
; Note: The sum of symbol frequencies must be a power of two!
;

tans_build_decoding_tables:
        ld      a,INITIAL_STATE
        ld      b,M_
        ld      e,0

        ; A = xp
        ; B = M i.e. the size of Ls i.e. Ls_len, which is fixed in this case.
        ; E = s
_main_loop:
        push    bc
        push    af

        EXTRACT_SYMBOL_FREQ hl
        and     a
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

_zero_symbol:
        inc     e
        pop     bc
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
;  A
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




        END main

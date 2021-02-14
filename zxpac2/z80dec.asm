;
; ZXPac v3 (c) 2014/21 Jouni 'Mr.Spiv' Korhonen
; Done for fun.. ;-) And definitely not the greatest.
;
; ZXPac v3 bit tag encoding is as follows:
;
; [n]    means an arbitrary bit vector of length n
; [B]    means a complete byte in byte boundary
; 11100  means a bit vector, MSB 1 and LSB 0
; 1xx11  means a bit vector with two varying bits 'xx'
; +      means a concatenation
;
; 0        + [B] -> RAW literal 
; 1 + OM + Offset + MatchLen
;
; The OM bits are precalculated and "optimally selected" offset and
; matchlen bit vector lengths. OM bits are encoded as 4 bits pairs
; at the end of file. There are 16 OM pairs thus 8 octets are used
; for them in each file. One OM octet is:
;  oommoomm  where 
;
;  oo 00 -> offset of 5 bits     Base 1 + [5]
;     01 -> offset 0f 8 bits     Base 33 + [8]
;     10 -> offset of 11 bits    Base 289 + [11]
;     11 -> offset of 14 bits    Base 2337 + [14]
;
;  mm 00 -> matchlen of 1 bit    Base 2 + [1]
;     01 -> matchlen of 3 bits   Base 4 + [3]
;     10 -> matchlen of 5 bits   Base 12 + [5]
;     11 -> matchlen of 7 bits   Base 44 + [7]
;
; In the compressed bit stream OMs are indexed as:
; 
;  100       -> OM index 0
;  101       -> OM index 1
;  1100      -> ..
;  1101
;  11100
;  11101
;  111100
;  111101
;  1111100
;  1111101
;  11111100
;  11111101 
;  111111100 
;  111111101 
;  111111110 
;  111111111 -> OM index 15
;
; The actul lengths OM index points at are precalculated before
; decompression into a table.
;
; Encoding of bit tags in the file:
;  - bit buffer (hold in A register) is stored in octet boundary
;  - literal run length is stored in an octet boundary
;
; Offsets are encoded as:
;
;  length 5  -> [5]
;  length 8  -> [B]
;  length 11 -> [B] + [3]
;  length 14 -> [B] + [6]
;
; The compressed file layout is roughly the following:
;
; [data] + [optional 2 octets inplace length] + [2 octets original length] + [8 OM octets]
;

INPLACE     equ 1       ; MUST be 1 if the file was crunched with --inplace

		org		$8000


main:

        ;ld      de,$4000
        ld      de,data
        ld      hl,dataend
        ld      b,$ff
        di
        exx
        push    hl
        exx
        call    decompress
		exx
        pop     hl
        exx
        ei
        ret
;
; DE = destination
; HL = end of compressed data
;  B = ptr to 256 octets aligned memory area for 32 octets
;
; Uses:
;  A,BC,DE,HL,IX
;  A',BC',DE',HL,
;
; On return AF or AF' is selected.
;
;

decompress:
        push    de
        ld      c,32
createomloop:
        dec     hl
        ld      a,(hl)
        ;
        ;        0    1
        ; A = mmoo|mmoo
        ;
        and     00000011b
        ld      e,a
        add     a,a
        add     a,e
        add     a,5
        dec     c
        ld      (bc),a  ; A = oo*3 + 5 -> offset bits
        ;
        ld      a,00001100b
        and     (hl)
        rrca    
        inc     a
        dec     c
        ld      (bc),a  ; A = mm*2 + 1
        ;
        ld      a,00110000b
        and     (hl)
        rrca
        rrca
        rrca
        ld      e,a
        rrca
        add     a,e
        add     a,5
        dec     c
        ld      (bc),a  ; A = oo*3 +5

        ld      a,11000000b
        and     (hl)
        rlca
        rlca
        rlca            ; Clears C-flag
        inc     a
        dec     c
        ld      (bc),a
        jr      nz,createomloop
        ;
        dec     c
        push    bc
        pop     ix
        dec     hl
        ld      b,(hl)
        dec     hl
        ld      c,(hl)
        ;
        ex      (sp),hl     ; HL   = destination
                            ; (sp) = end of compressed data
        add     hl,bc       ; HL   = end of destination
        ex      (sp),hl     ; HL   = end of compressed data
                            ; (sp) = end of destination
        ; inplace length
    IF INPLACE
        dec     hl
        ld      b,(hl)
        dec     hl
        ld      c,(hl)      ; inplace "original size"
    ENDIF
        pop     de          ; DE = end of destination
        ;
        ; HL = end of compressed data
        ; DE = end of destination 
        ; BC = original size
        ; IX = pointer to OM table (32 octets)
        ;
        dec     hl
        ld      a,(hl)
        dec     hl
        dec     de
        jr      decompressmain
decompressliteral:
        ;
        ; tag 0 + [8]
        ;
        ldd
decompressexit:
        ret     po
decompressmain:
        add     a,a
        jr      nz,mainNZ
        ld      a,(hl)
        adc     a,a
        dec     hl
mainNZ:
        jr      nc,decompressliteral
        ;
decompresstag:
        exx
        push    ix
        pop     hl
        ;
        ld      b,7 ;;
getomtag:
        inc     l
        add     a,a
        jr      nz,omNZ
        exx                 ; 4
        ld      a,(hl)      ; 7
        dec     hl          ; 6
        exx                 ; 4 -> 21 (4 octets)
        adc     a,a
omNZ:
        jr      nc,omtagend
        djnz    getomtag
        inc     l
omtagend:
        add     a,a
        jr      nz,omNZ2
        exx
        ld      a,(hl)
        dec     hl
        exx
        adc     a,a
omNZ2:
        rl      l
        sla     l           ; Clears C-flag
        ld      c,(hl)      ; C = 0000mmmm
        inc     l
        ld      b,(hl)      ; B = 0000oooo
        sbc     hl,hl       ; HL = 0, clears C-flag
        ex      af,af'
        ld      a,b
        ;
        ld      de,0+1
        sub     8
        jr      c,ooStart1
        ; and delete 8 from the bits to input..
        ; make sure nothing changes flags now..
        ld      e,32+1
        ld      b,a
        ; need to fetch a full octet into L
        exx
        ld      a,(hl)
        dec     hl
        exx
        ld      l,a
        jr      z,ooDone
        bit     0,b
        jr      nz,ooStart
        ld      d,00001000b
ooStart:
        inc     d
ooStart1:
        ex      af,af'
        ;
ooLoop:
        add     a,a
        jr      nz,ooNZ3
        exx
        ld      a,(hl)
        dec     hl
        exx
        adc     a,a
ooNZ3:
        adc     hl,hl
        djnz    ooLoop
        db      $fe     ; CP n
ooDone:
        ex      af,af'
        add     hl,de
        push    hl
        ;
        ; HL = inputbits + base 
        ; Now extract macthlen
        ;
        ld      e,b         ; E = 0
        ld      b,c         ; B = num of bits
        ;
        ; 1 ->  2 + [1]     00000010 + 0000000x
        ; 3 ->  4 + [3]     00000100 + 00000xxx
        ; 5 -> 12 + [5]     00001100 + 000xxxxx
        ; 7 -> 44 + [7]     00101100 + 0xxxxxxx
        ;
        ld      l,01010101b
getmatchlen:
        sla     l           ; add hl,hl would be shorter but slower
        ;add     hl,hl
mmX:
        add     a,a
        jr      nz,mmNZ
        exx
        ld      a,(hl)
        dec     hl
        exx
        adc     a,a
mmNZ:
        rl      e
        djnz    getmatchlen
        ex      af,af'
        ld      a,10101011b ; Adds base-1
        xor     l
        add     a,e
        ;
        exx
        ex      (sp),hl
        add     hl,de

mmCopy:
        ldd
        dec     a
        jr      nz,mmCopy
        ldd
        pop     hl
        ret     po
        ex      af,af'
        jr      decompressmain

data:
        incbin  "lzy.pac"
        ;
        ;
dataend:




		END main


/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

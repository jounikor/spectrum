;APS00000019000000190000001900000019000000190000001900000019000000190000001900000019
;
; @file zxpac4_data.asm
; @brief Data file decompressor for ZXPAC4
;        Decompression from the higher to the lower memory.
; @author Jouni 'Mr.Spiv' Korhonen
; @version 0.4
; @copyright The Unlicense
; 
; 20250109 0.1 - Initial version
; 20250109 0.2 - Fixed offset decoding bug
; 20250122 0.3 - Added 65535 bytes max match
; 20250124 0.4 - Small code changes and added comments
;
; Note:
;  - Reversed file
;  - Reversed encoding
;  - Selectable between 32K and 128K window
;  - Selectable between 255 or 65535 match length
;  - 8bit bytes
;
; @note There are two assembler defines the change the behaviour of the
;       decompression code.
;       MAX32K_WIN If set the sliding window size is 32768 bytes
;                  If not set the window size is 131072 bytes.
;       LARGE_MAX_MATCH If set the maximum match length is 65535 bytes.
;                   If not set the maximum match length is 255 bytes.
;


GETBIT  MACRO
        add.b   d6,d6
        bne.b   \@notempty
        move.b  -(a1),d6
        addx.b  d6,d6
\@notempty:
        ENDM


;-----------------------------------------------------------------------------
;
; @param[in] A0 A ptr to destination memory.
; @param[in] A1 A ptr to the first memory address after the
;               compressed file.
; 
; @return none
;
; @note Trashes d0-d3/d6-d7/a0-a3
;
start:  moveq   #$3f,d7
	and.b	-(a1),d7
	moveq	#0,d6
	move.b	-(a1),d6
	lsl.w	#8,d6
	move.b	-(a1),d6
	lsl.l	#8,d6
	move.b	-(a1),d6
        lea     0(a0,d6.l),a3
        moveq   #-128,d6
        bra.b   tag_literal
        ;
main_loop:
        ;
        ; D6 = bit buffer
        ; D7 = pmr offset
        ; A5 = ptr to the first segment
        ; A0 = decompressed
        ; A1 = compressed data end
        ; A3 = destination
        ; A2 = tmp regisrer
        GETBIT
        bcc.b   tag_literal
        ;
tag_match_or_literal:
    IFD MAX32K_WIN
        moveq   #3-1,d2
    ELSE
        moveq   #4-1,d2
    ENDIF
        moveq   #-1,d1
        ;
        GETBIT
        bcs.b   tag_pmr_matchlen 
        moveq   #0,d0
        move.b  -(a1),d0
        bpl.b   get_offset_done
get_offset_tag_loop:
        addq.w  #1,d1
        GETBIT
        dbcc    d2,get_offset_tag_loop
	bcc.b	get_offset_tag_term
	addq.w	#1,d1
get_offset_tag_term:
        GETBIT
	roxl.w	#1,d1
	beq.b   get_offset_done
get_offset_bits_loop:
        GETBIT
        addx.l  d0,d0
        subq.w  #1,d1
        bne.b   get_offset_bits_loop
get_offset_done:
        move.l  d0,d7
        moveq   #0,d1
tag_pmr_matchlen:
        ;
        ; D0 = offset
        ; D1 = 0 if a normal match
        ; D1 = -1 if a PMR
        ;
        moveq   #1,d2
    IFD LARGE_MAX_MATCH
        moveq	#15-1,d3
    ELSE
        moveq   #7-1,d3
    ENDIF
get_matchlen_loop:
        GETBIT
        bcc.b   get_matchlen_exit
        GETBIT
        addx.w  d2,d2
        dbra    d3,get_matchlen_loop
get_matchlen_exit:
        add.w   d2,d1
        ;
        ; A0 = decompressed data start
        ; a1 = compressed data end
        ; A3 = destination
        ; D7 = offset
        ; D1 = adjusted match lenght
        ;
        lea     0(a3,d7.l),a2
copy_loop:
        move.b  -(a2),-(a3)
        dbra    d1,copy_loop
        dc.w    $b07c           ; CMP.W #$xxxx,D0
tag_literal:
        move.b  -(a1),-(a3) 
        cmp.l   a0,a3
        bhi.b   main_loop
	rts

compressed_data:
compressed_data_end:

;
; Executabke file decompressor for ZXPAC4 
; Version 0.1
;
; ..just some sketch code.. never assembled..
;

GETBIT  MACRO
        add.b       d6,d6
        bne.b       \@notempty
        move.b      -(a2),d6
        addx.b      d6,d6
\@notempty:
        ENDM
        
MAX32K_WIN          EQU     0

__LVOForbid		    equ	    -132
__LVOPermit         equ     -138
__LVOFreeMem        equ     -210
__LVOCacheClearU    equ     -636
__LIB_VERSION       equ     20

; The compressed size of the file is in the 4 first bytes
file_size:
        dc.l        0

;
;  Each segment in memory is as follows:
;
;  start_address: dc.l size_in_bytes
;                 dc.l BPTR_to_next_segment
;                 ... code starts for this segment
;
; We were called with JSR so stack has the address of the
; first segment + 6
j:
        subq.l      #6,(sp)
        movem.l     d0-a5,-(sp)         ; 15*4 + 4
        move.l      14*4(sp),a1
        subq.l      #4,a1
        ; A1 = A ptr to the first segment
        lea         compressed_data(pc),a0
        move.l      a0,a2
        add.l       file_size(pc),a2
        
        ; parse compressed file header
        moveq       #0,d6
        moveq       #0,d7
        move.b      -(a2),d7

        move.b      -(a2),d6
        lsl.w       #8,d6
        move.b      -(a2),d6
        lsl.l       #8,d6
        move.b      -(a2),d6
        lea         64(a0,d6.l),a3
        lea         64(a0),a0
        moveq       #-128,d6
        bra.b       .tag_literal
        ;
.main_loop:
        ;
        ; D6 = bit buffer
        ; D7 = pmr offset
        ; A5 = ptr to the first segment
        ; A0 = decompressed data start
        ; A2 = compressed data end
        ; A3 = destination
        ; A4 = tmp regisrer
        GETBIT
        bcc.b       .tag_literal
        ;
.tag_match_or_literal:
    IF MAX32K_WIN
        moveq       #3-1,d2
    ELSE
        moveq       #4-1,d2
    ENDIF
        moveq       #-1,d1
        ;
        GETBIT
        bcs.b       .tag_pmr_matchlen 
        ;
        ; Match found:
        ;  min = 2
        ;  offset > 0
        ;
        moveq       #0,d0
        move.b      -(a2),d0
        bpl.b       .get_offset_done
.get_offset_tag_loop:
        addq.w      #1,d1
        GETBIT
        ;bcc.b       .get_offset_tag_term
        dbcc        d2,get_offset_tag_loop
.get_offset_tag_term:
        GETBIT
        addx.b      d1,d1
        beq.b       ._get_offset_done
.get_offset_bits_loop:
        GETBIT
        addx.l      d0,d0
        subq.w      #1,d1
        bne.b       .get_offset_bits_loop
.get_offset_done:
        move.l      d0,d7
.tag_pmr_matchlen:
        ;; solve the matchlength 1 vs 2 here.. later
        ;
        ; D0 = offset
        ; D1 = 0 if a normal match
        ; D1 = -1 if a PMR
        ;
        moveq       #1,d2
        moveq       #7-1,d3
.get_matchlen_loop:
        GETBIT
        bcc.b       .get_matchlen_exit
        GETBIT
        addx.b      d2,d2
        dbra        d3,.get_matchlen_loop
.get_matchlen_exit:
        add.w       d2,d1
        ;
        ; A0 = decompressed data start
        ; A2 = compressed data end
        ; A3 = destination
        ; D7 = offset
        ; D1 = adjusted match lenght
        ;
        lea         0(a3,d7.l),a4
.copy_loop:
        move.b      -(a4),-(a3)
        dbra        d1,.copy_loop
        dc.w        $b07c           ; CMP.W #$xxxx,D0
.tag_literal:
        move.b      -(a2),-(a3) 
        cmp.l       a0,a3
        bhi.b       .main_loop
        ;
        ; A1 = ptr to the first segment
        ; A3 = start of the decompressed data
        ; A4 = work register
        ;
reloc_main_or_bss_hunk:
        move.w      (a3)+,d0
        beq.b       reloc_main_loop
        bmi.b       .code_or_data_hunk
        and.w       #$3fff,d0
        beq.b       .bss_hunk
        bne.b       do_all_relocs
.code_or_data_hunk:
        swap        d0
        move.w      (a3)+,d0
        add.l       d0,d0
        add.l       d0,d0
        lea         4(a1),a4
.copy_code_or_data:
        move.l      (a3)+,(a4)+
        subq.l      #4,d0
        bne.b       .copy_code_or_data
.bss_hunk:
        move.l      a1,a5
        move.l      (a1),a1
        add.l       a1,a1
        add.l       a1,a1
        bra.b       reloc_main_or_bss_hunk
reloc_check_alignment:
        move.w      a3,d0
        lsr.w       #1,d0
        bcc.b       reloc_main_loop
        addq.l      #1,a3
reloc_main_loop:
        move.w      (a3)+,d0
        beq.b       reloc_done_exit
        bsr.b       get_segment_address
        move.l      a0,d1
        move.w      (a3)+,d0
        bsr.b       get_segment_address
        move.l      (a3)+,d3
        ;
        ; D1 = destination hunk address
        ; D3 = base for delta relocs
        ; A0 = source hunk address
        ; A1 = a ptr to the last segment
        ; A5 = a ptr to the second last segment
.reloc_loop:
        moveq       #0,d0
.reloc_delta:
        lsl.l       #7,d0
        move.b      (a3)+,d2
        beq.b       reloc_check_alignment
        bclr        #7,d2
        beq.b       .delta_done
        or.b        d2,d0
        bra.b       .reloc_delta
.delta_done:
        or.b        d2,d0
        add.l       d0,d0
        add.l       d0,d3
        add.l       d1,0(a0,d3.l)
        bra.b       .reloc_loop
reloc_done_exit:
        move.l      $4.w,a6
        cmp.b       #37,__LIB_VERSION(a6)
        blo.b       .too_low_kick
        jsr         __LVOCacheClearU(a6)
.too_low_kick:
        jsr         __LVOForbid(a6)     ; Dirty but we know A1 is preserved.
        clr.l       (a5)
        move.l      -(a1),d0
        jsr         __LVOFreeMem(a6)
        movem.l     (sp)+,d0-a5
        jmp         __LVOPermit(a6)

; D0 = segment number + 1
; return A0 segment address
;
get_segment_address:
        move.l      15*4(sp),a0
        subq.l      #4,a0

.loop:  move.l      (a0),a0
        add.l       a0,a0
        add.l       a0,a0
        subq.w      #1,d0
        bne.b       .loop
        addq.l      #4,a0
        ret

compressed_data:

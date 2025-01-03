;
; Executabke file decompressor for ZXPAC4 
; Version 0.1
;
;
;

GETBIT  MACRO
        add.b       d6,d6
        bne.b       \@notempty
        move.b      -(a2),d6
        addx.b      d6,d6
\@notempty:
        ENDM
        




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

        ; eventually return to the first bytes of the first
        ; segment.. 
        move.l      a0,(sp)

        ; Find the second last segment number..
.loop:  move.l      -(a0),d1
        lsl.l       #2,d1
        beq.b       .exit
        move.l      d1,a0
        bra.b       .loop
.exit:  ;
        ; A0 = A ptr to the second last segment
        lea         compressed_data(pc),a1
        move.l      a1,a2
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
        lea         64(a1,d6.l),a3
        moveq       #-128,d6
        bra.b       .tag_literal
        ;
.main_loop:
        ;
        ; D6 = bit buffer
        ; D7 = pmr offset
        ; A0 = second last segment
        ; A1 = compressed data start
        ; A2 = compressed data end
        ; A3 = destination
        ;
        GETBIT
        bcc.b       .tag_literal
        ;
.tag_match_or_literal:
    IF MAX32K_WIN
        moveq       #3-1,d1
    ELSE
        moveq       #4-1,d1
    ENDIF
        moveq       #0,d2
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
        GETBIT
        bcc.b       .get_offset_tag_term
        addq.w      #1,d2
        dbra        d1,get_offset_tag_loop
.get_offset_tag_term:
        GETBIT
        addx.b      d2,d2




.tag_literal:
        move.b      -(a2),-(a3) 
        cmp.l       a1,a3
        bhi.b       .main_loop

; Do relocations..




; Exit decompressor and run the code. 
;
        move.l      $4.w,a6
        cmp.b       #37,__LIB_VERSION(a6)
        blo.b       .too_low_kick
        jsr         __LVOCacheClearU()
.too_low_kick:
        jsr         __LVOForbid(a6)
        move.l      (a0),a1
        clr.l       (a0)
        add.l       a1,a1
        add.l       a1,a1
        move.l      -(a1),d0
        jsr         __LVOFreeMem(a6)
        movem.l     (sp)+,d0-a5
        jmp         __LVOPermit(a6)

; D0 = segment number..
; return A0 segment address
;
get_segment_address:
        move.l      60(sp),a0
        subq.l      #4,a0

.loop:  move.l      (a0),d1
        lsl.l       #2,d1
        beq.b       .exit
        move.l      d1,a0
        dbf         d0,.loop
        ret

compressed_data:

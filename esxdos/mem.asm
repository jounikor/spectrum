


; These are coming from somewhere else I guess.. here just for testing.
SRAM_CTRL       equ 227
SYS_PAGE        equ 0
;
;
MEM_PAGE_ADDR   equ $c000
;MEM_PAGE_ADDR   equ $2000
MEM_PAGE_SIZE   equ $2000
MEM_PAGE_HIGH    equ MEM_PAGE_ADDR+MEM_PAGE_SIZE

BLCK_BITS       equ 8
MIN_BLCK_SIZE   equ $0100
MAX_BLCK_SIZE   equ $2000

MEM_BLCK_CNT    equ 32       ; 32*256 = 8192
MEM_BLCK_MASK   equ MEM_BLCK_CNT-1

MEM_FREE_ADDR   equ $dd80
;MEM_FREE_ADDR   equ $3d80
MEM_FREE_BLCK   equ ((MEM_FREE_ADDR - MEM_PAGE_ADDR) >> BLCK_BITS)
SYS_VARS_ADDR   equ $de00
;SYS_VARS_ADDR   equ $3e00
SYS_VARS_BLCK   equ ((SYS_VARS_ADDR - MEM_PAGE_ADDR) >> BLCK_BITS)

; These are on a SYS_PAGE
NUM_PAGES       equ SYS_VARS_ADDR+255
RAM_PAGE        equ SYS_VARS_ADDR+254
NEXT_PAGE       equ SYS_VARS_ADDR+253


; PID 0 is for "unused memory blocks"
PID_FREE        equ $00
; User PIDs are between 1 and 62
PID_MIN         equ 1
PID_MAX         equ 62
; PID 63 is reserved for system use
PID_ROOT        equ 63

; Current block flags are unused..
BLCK_PID_MASK   equ 00111111b
BLCK_FLG_MASK   equ 11000000b

;-----------------------------------------------------------------------------
; Mutex macros for entering and leaving critical sections. These macros
; use HL as the mutex address. TAKE_MUTEX macro also trashes A register.
; A mutex needs one byte of RAM. There is no limit of the number of mutexes.
;
; INIT_MUTEX mutex - initialize mutex. If the mutex name (address) is not
;                    given as a macro parameter HL is expected to contain
;                    the mutex address.
;
; TAKE_MUTEX mutex - busy wait until the caller manages to take the mutex.
;                    If the mutex name (address) is not given as a macro
;                    parameter HL is expected to contain mutex address.
;                    A register gets trashed.
;
; GIVE_MUTEX mutex - release the mutex. Before using this macro the caller
;                    must have taken the mutex. If the mutex name (address)
;                    is not given as a macro parameter HL is expected to
;                    contain mutex address.
;

TAKE_MUTEX  MACRO  mtx
        LOCAL   busywait
        
        ; If mutex address is given load it into HL, otherwise
        ; the assumption is that HL already points at the mutex.
        IF !NUL mtx
        
        ld      hl,mtx
        ENDIF
busywait:
        ld      a,$0f
        rrd
        jr nz,  busywait
        ENDM

GIVE_MUTEX  MACRO   mtx
        ; If mutex address is given load it into HL, otherwise
        ; the assumption is that HL already points at the mutex.
        IF !NUL mtx 
        ld      hl,mtx
        ENDIF
        
        ld      (hl),$f0
        ENDM

INIT_MUTEX  MACRO   mtx
        ; If mutex address is given load it into HL, otherwise
        ; the assumption is that HL already points at the mutex.
        IF !NUL  mtx  
        ld      hl,mtx
        ENDIF
        
        ld      (hl),$f0
        ENDM

;----------------------------------------------------------------------------

;
;
;
;

PAGE    MACRO
       ; out     (SRAM_CTRL),a
        ENDM

;----------------------------------------------------------------------------
        
        org     $8000


main:
        di
        push    iy

        ld      a,1
        call    init_mem_system

        ;
        ;

        ld      b,3
        ld      c,7
        call    bmalloc

        ld      b,6
        ld      c,8
        call    bmalloc

        ld      b,2
        ld      c,9
        call    bmalloc

        ld      b,1
        ld      c,10
        call    bmalloc


        ld      b,5
        ld      c,11
        ld      hl,$d100
        ld      l,0
        call    ballocabs

        ld      b,1
        ld      c,11
        ld      hl,$c100
        ld      l,0
        call    ballocabs
        
        ; hl=ptr
        ; a=page
        ld      b,3
        ;call    bfree





        pop     iy
        ei
        ret


mutex:  db    0



;----------------------------------------------------------------------------
;
; Initialized the memory manager. The following assumptions must hold:
;  - The number of pages must be a power of two.
;  - The location of memory manager system variables are on a SYS_PAGE and
;    the value of the SYS_PAGE must be 0.
;  - There are 1x 256 bytes preallocated on each page to hold the
;    free blocks map (located as MEM_FREE_ADDR).
;  - The SYS_PAGE has additional 1x 256 bytes preallocated for variables
;    (located at SYS_VARS_ADDR).
;  - The maximum number of 256 bytes blocks on a page, based on the free
;    block table at MEM_FREE_ADDR location, is 128 i.e. 32768 bytes.
;  - The maximum number of pages is 255.
;  - The minimum allocation size is 256 bytes and all allocations are
;    multiple of 256 bytes.
;  - At exit of memory manage functions the caller page is paged in,
;    expect during init the SYS_PAGE is paged in.
;
; Inputs:
;  A [in] num pages, must be power of two.
;
; Returns:
;  C_flag=0 if error, C_flag=1 if OK.
;
; Trashes:
;  A,B,C,H,L
;

init_mem_system:
        ; First init the SYS_PAGE
        and     a
        ret z

        ; Chech number of bits in A.. must be 1 i.e. the number of
        ; pages must be a power of two.
        ld      c,a
        ld      b,a
        dec     b
        and     b
        ret nz

_power_of_two:
        ; A=0
        PAGE
        ld      (RAM_PAGE),a
        ld      (NEXT_PAGE),a
        ld      a,c
        ld      (NUM_PAGES),a

        ld      hl,MEM_FREE_ADDR
        ld      b,MEM_BLCK_CNT
_sys_page_init:
        ld      (hl),PID_FREE
        inc     hl
        djnz    _sys_page_init

        ; On a SYS_PAGE reserve $3Dxx and $3Exx
        ld      a,PID_ROOT
        ld      (MEM_FREE_ADDR+SYS_VARS_BLCK),a
        ld      (MEM_FREE_ADDR+MEM_FREE_BLCK),a

        ; And the rest of the pages if any
        ; C_flag=1
        scf
        dec     c
        ret z

        ; Page in a non SYS_PAGE page (!=0) and reserve the
        ; MEM_FREE_BLCK for free block map ($3Dxx).
_init_loop:
        ld      a,b
        PAGE
        ld      hl,MEM_FREE_ADDR
        ld      a,MEM_BLCK_CNT
_free_loop:
        ld      (hl),PID_FREE
        inc     hl
        dec     a
        jr nz,  _free_loop

        ld      a,PID_ROOT
        ld      (MEM_FREE_ADDR+MEM_FREE_BLCK),a
        djnz    _init_loop

        ; return SYS_PAGE as the active page
        xor     a
        PAGE

        scf
        ret


;----------------------------------------------------------------------------
;
; Allocate more then one 256 bytes blocks on a given page and at a predifed
; address. The address must be a multiple of 256 bytes.
;
; The function checks for:
;  - The page is valid.
;  - The address is within a page limits
;  - The block count is 0 < number <= maximum block count per page
;
; Input:
;  B  [in] number of blocks to allocate
;  C  [in] PID
;  H  [in] high byte of the address
;  L  [in] page
;
; Returns:
;  C_flag=0 in a case of an error.
;
; Trashes:
;  A, BC, E, HL
;
ballocabs:
        ; Requesting 0 blocks? this could part of the
        ; while loop as well but there is no size advantage..
        ; AND also sets C_flag=0
        ld      e,b
        ld      a,b
        and     a
        ret z

        ; Requesting more than a page size? 
        cp      MEM_BLCK_CNT+1
        ret nc

        ; Check address range, low 8 bits an insignificant
        ld      a,HIGH(MEM_PAGE_ADDR-1)
        cp      h
        ret nc
        
        ld      a,h
        cp      HIGH(MEM_PAGE_HIGH)
        ret nc

        ; Check if the requested page is valid
        xor     a
        PAGE
        ld      a,(NUM_PAGES)
        ld      e,a
        ld      a,l
        cp      e
        ; C_flag=0 in case of an error.
        jr nc,  _page_error

        ;
        PAGE

        ld      a,h
        sub     HIGH(MEM_PAGE_ADDR)
        add     a,LOW(MEM_FREE_ADDR)
        ld      l,a
        ld      h,HIGH(MEM_FREE_ADDR)
        
        ld      e,b
        xor     a
_blcks_free_loop:
        or      (hl)
        inc     hl
        jr nz,  _no_free_mem
        djnz    _blcks_free_loop 

        ;
_mark:  
        dec     hl
        ld      (hl),c
        dec     e
        jr nz,  _mark

_no_free_mem:
        xor     a
        PAGE
        cp      b
        ccf
_page_error:
        ; Page in the original caller page
        ld      a,(RAM_PAGE)
        PAGE
        ret

;----------------------------------------------------------------------------
;
; Allocated more than one 256 bytes blocks from any available page. The
; memory manage desides the appropriate page. The memory is always allocated
; at the "natural alignment" based on the block count. For example, 1 block
; is allocated using 256 bytes alignment, 2 block using 512 bytes alignment,
; 3-4 blocks using 1024 bytes alignment, and 5-8 blocks using 2048 bytes
; alignment.
;
; The function checks for:
;  - The block count is 0 < number <= maximum block count per page
;
; Input:
;  B  [in] number of blocks to allocate.
;  C  [in] PID
;
; Returns:
;  H The high byte of the pointer to the allocated memory.
;  L page where the allocation took place.
;  C_flag=1 if OK. In case of an error C_flag=0 and other registers have
;  undefined value.
;
; Trashes:
;  A, BC, DE, HL
;

bmalloc:
        ; Requesting 0 blocks? this could part of the
        ; while loop as well but there is no size advantage..
        ; AND also sets C_flag=0
        ld      a,b
        and     a
        ret z

        ; Adjust size to start from 0
        dec     a

        ; Requesting more than a page size? The maximum supported
        ; page size is 128*256 i.e. 32768 bytes.
        cp      MEM_BLCK_CNT
        ret nc

        ; C_flag=1
        ld      e,0
_while_level:
        rl      e       ; during the first shift C_flag=1 due the CP
        cp      e
        jr nc,  _while_level

        ; E=1<<level i.e. step, B=number of blocks to allocate, C=PID
        ;
        ; We are now in "caller page context" and that's where the
        ; stack is valid. So all stack operations must be done while
        ; this page is paged in. The page number is located in
        ; RAM_PAGE variable located in SYS_PAGE (=0)..
        ;
        ; IX and IY are used as 4 bytes "temp vars"..
        ;

        push    ix
        push    iy
        push    bc
        pop     iy
        
        dec     e
        ld      a,e
        cpl
        ld      d,a
        
        ; Swap to SYS_PAGE
        xor     a 
        PAGE

        ; Find a candidatre page that has enough RAM available..
        ; The assumption is that (NUM_PAGES) is a power of two.
        ld      a,(NUM_PAGES)
        ld      c,a
        dec     a
        ld      ixh,a
        ld      a,(NEXT_PAGE)
        ld      ixl,a

        ; A=(NEXT_PAGE) i.e. where to start
        ; IYH=number of pages to allocate
        ; IYL=PID
        ; IXH=(NUM_PAGES)-1
        ; IXL=(NEXT_PAGE)
        ; C=NUM_PAGES (power of two) - counter
        ; D=~(block_step-1) 
        ; E=block_step-1

_for_blck_loop:
        PAGE
        ; Each page has its own free block area (32 bytes). Note that the
        ; block free table shall not cross the 256 bytes boundary.
        ld      hl,MEM_FREE_ADDR

_while_blck_loop:
        xor     a
        ld      b,iyh
_blcks_free:
        or      (hl)
        inc     hl          ; Tne INC here is important. L shall not be 0..
        jr nz,  _blcks_allocated
        djnz    _blcks_free
        jr z,   _free_blcks_found
       
        ; Not enough blocks found.. advance to the next segment within a page
_blcks_allocated:
        ld      a,l
        add     a,e
        and     d
        ld      l,a
        cp      LOW(MEM_FREE_ADDR)+MEM_BLCK_CNT
        jr nz,  _while_blck_loop

        ld      a,ixl
        inc     a
        and     ixh
        ld      ixl,a

        dec     c
        jr nz,  _for_blck_loop
        
        ; Error.. no mem found. XOR sets C_flag=0 to indicate an error
        xor     a
        PAGE
        jr      _bmalloc_exit

_free_blcks_found:
        ; IXL=page where allocation succeeded
        ; IYH=num blocks
        ; IYL=PID
        ; HL=ptr to free pos+1
        ld      b,iyh
        ld      a,iyl
_mark:
        ; Reserve pages..
        dec     hl
        ld      (hl),a
        djnz    _mark

        ; Update the system RAM with the page the allocation was done in
        ; and page in the page what was active when calling bmalloc()
        xor     a
        PAGE
        ld      a,ixl
        ld      (NEXT_PAGE),a
        
        ; Calculate the ptr within the page
        ld      a,l
        sub     LOW(MEM_FREE_ADDR)
        ; Segment_number<<8
        add     a,HIGH(MEM_PAGE_ADDR)
        ld      h,a
        scf
        
        ; L=page where RAM was allocated in a successful case
        ; H=high byte of the ptr to allocated memory within the page
        ; C_flag=1
        ld      l,ixl

_bmalloc_exit:
        ; Page in the page where the caller was..
        ld      a,(RAM_PAGE)
        PAGE

        pop     iy
        pop     ix
        ret

;----------------------------------------------------------------------------
;
; Free an allocated memory block in a given page for a specific PID.
; The function errors if memory not belonging to another PID is attempted to
; be freed.
; It is not possible to free memory belonging to PID_ROOT i.e. system.
;
; The function checks for:
;  - The block count is 0 < number <= maximum block count per page
;  - The page is valid.
;  - The ptr is within the page.
;  - PID is not PID_ROOT i.e. system.
;
; Input:
;  L  [in] page
;  B  [in] number of allocated blocks
;  C  [in] PID
;  H  [in] High byte of thr ptr to allocated memory
;
; Returns:
;  If C_flag=1 then OK.
;  If C_flag=0 then error and:
;    B=number of allocated blocks -> error in input parameters
;    B<number of allocated blocks -> trying to free memory for wrong PID.
;
; Trashes:
;  A,B,C,E,HL
;

bfree:  ; Cannot free system memory areas..
        ld      a,PID_ROOT
        xor     c
        ret z

        ; Check B is not 0
        xor     a
        cp      b
        ret z
        
        ; Check address range, low 8 bits an insignificant
        ld      a,HIGH(MEM_PAGE_ADDR-1)
        cp      h
        ret nc
        
        ld      a,h
        cp      HIGH(MEM_PAGE_HIGH)
        ret nc

        ; Is the page to be freed valid?
        xor     a
        PAGE
        ld      a,(NUM_PAGES)
        ld      e,a
        ld      a,l
        cp      e
        jr nc,  _page_error

        ; Page in the page where allocation was done..
        PAGE
        
        ; Get the freemap position within the page based on
        ; the allocated memory ptr in HL
        ld      a,h
        sub     HIGH(MEM_PAGE_ADDR)
        add     a,LOW(MEM_FREE_ADDR)
        ld      l,a
        ld      h,HIGH(MEM_FREE_ADDR)
        
        ; Mark mem free..
_bfree_mem:
        ld      a,BLCK_PID_MASK
        and     (hl)
        xor     c
        jr nz,  _page_error

        ld      (hl),a
        inc     hl
        djnz    _bfree_mem
        
        ; A=0 i.e. same as the SYS_PAGE
        PAGE
        scf

        ; Page in the original page..
_page_error:
        ld      a,(RAM_PAGE)
        PAGE

        ret

;----------------------------------------------------------------------------
;
; Scan through all pages and free all allocated memory for a given PID.
;
; The function checks for:
;  - PID is not PID_ROOT i.e. system.
;
; Input:
;  C  [in] PID
;
; Returns:
;  C_flag=1 if OK. C_flag=0 if error.
;
; Trashes:
;  A,B,C,E,HL
;

bfree_by_pid:
        ; Cannot free system memory
        ld      a,PID_ROOT
        xor     c
        ret z

        xor     a
        PAGE
        ld      a,(NUM_PAGES)
        ld      e,a
_page_loop:
        dec     e
        ld      a,e
        PAGE
        ld      hl,MEM_FREE_ADDR
        ld      b,MEM_BLCK_CNT
_pid_free_loop:
        ld      a,BLCK_PID_MASK
        and     (hl)
        xor     c
        jr nz,  _not_my_pid
        ld      (hl),a
_not_my_pid:
        inc     l
        djnz    _pid_free_loop
        ld      a,e
        and     a
        jr nz,  _page_loop

        scf
        ld      a,(RAM_PAGE)
        PAGE
        ret

        END     main



;
; Scoopex DeCruncher v0.1 - for ZXPac
; (c) 2009 Jouni 'Mr.Spiv' Korhonen
; Originally coded for the 'ScoopZX' intro. 
;
; The routine has the following properties:
;  - allows inplace decrunching
;  - 72 bytes long decruncher
;  - runtime stack usage is 2 bytes
;  - position independent (code is reloctable as-is)
;  - could be considered as 'fast'
;  - uses only AF,BC,DE and HL
;  - it is a quick hack.. not meant to compete against
;    the other better crunchers out there ;)
;





;
;
; When calling the decruncher subroutine set:
;   DE = destination
;   HL = crunched data + crunched length
;
decrunch:
		dec		hl
		ld		b,(hl)
		dec		hl
		ld		c,(hl)
		ex		de,hl
		add		hl,bc
		ex		de,hl
		dec		de
		ld		b,0
		;
		; DE = end of decrunched memory area
		; HL = crunched data end
		;
loopHL:	dec		hl
loop:	ld		a,(hl)
		cp		$40
		jr		c,tag00
		cp		$80
		jr		nc,tag1
		;
tag01:	push	hl
		;
		; tag: 01xnnnnn
		;  A = 01xnnnnn, CF=1
		ld		c,a
		and		00011111b
		ld		l,a
		ld		h,b
		xor		c
		rlca
		rlca
		rlca
		ld		c,a
copy:	;out		(254),a		; If you want a fancy color
								; effect on the border during
								; decrunching, then uncomment
								; this line..
		add		hl,de
		inc		hl
		lddr
saveHL:	pop		hl
		jr		loopHL
		;
		; tag: nnnnnn*[8] + 00nnnnnn
		; A = 00nnnnnn, CF=0
tag00:	and		a
		ret		z
		ld		c,a
		dec		hl
		lddr
		jr		loop
		;
		; [8] + ([8]) + 1xxxxxnn
		; A=1xxxxxnn, CF=1
tag1:	ld		(de),a		; was PUSH AF
		; A = 0xxxxnnn
		and		01111000b
		rrca
		rrca
		rrca
		add		a,2
		cp		17
		jr		nz,noOvl
		dec		hl
		add		a,(hl)
		jr		nc,noOvl
		inc		b			; B=1
noOvl:	ld		c,a
		ld		a,(de)		; was POP AF
		;
		and		00000111b
		dec		hl
		push	hl
		ld		l,(hl)
		ld		h,a
		jr		copy

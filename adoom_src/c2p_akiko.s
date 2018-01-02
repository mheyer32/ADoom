		mc68020
		multipass
	if (_eval(DEBUG)&$8000)
		debug	on,lattice4
	endc

;void __asm c2p_akiko (register __a0 UBYTE *chunky_data,
;                      register __a1 PLANEPTR raster,
;                      register __a2 UBYTE *dirty_list,
;                      register __d1 ULONG plsiz,
;                      register __a5 UBYTE *akiko_address);

; a0 -> width*height chunky pixels in fastmem
; a1 -> contiguous bitplanes in chipmem
; a2 -> dirty list (1-byte flag for whether each 32 pixel "unit" needs updating)
; d1 = width*height/8   (width*height must be a multiple of 32)

	ifeq	depth-8
		xdef	_c2p_8_akiko
_c2p_8_akiko:
	else
	ifeq	depth-6
		xdef	_c2p_6_akiko
_c2p_6_akiko:
	else
		fail	"unsupported depth!"
	endc
	endc

		xref	_GfxBase

		movem.l	a2/a3/a6,-(sp)

		move.l	d1,d0		; plsiz
		lsl.l	#3,d0		; 8*plsiz
		lea	(a0,d0.l),a3	; a3 -> end of chunky data
		sub.l	d1,d0		; d0 = 7*plsiz
	ifle depth-6
		sub.l	d1,d0
		sub.l	d1,d0		; d0 = 5*plsiz if depth=6
	endc

		movem.l	d0/d1/a0/a1,-(sp)
		movea.l	(_GfxBase).l,a6
		jsr	(_LVOOwnBlitter,a6) ; gain exclusive use of Akiko
		movem.l	(sp)+,d0/d1/a0/a1

loop:		tst.b	(a2)+		; does next 32 pixel unit need updating?
		bne.b	c2p		; branch if yes

		adda.w	#32,a0		; skip 32 pixels on input
		addq.l	#4,a1		; skip 32 pixels on output

		cmpa.l	a3,a0
		bne.b	loop
		bra.b	exit		; exit if no changes

c2p:		move.l	(a0)+,(a5)	; write 32 pixels to akiko
		move.l	(a0)+,(a5)
		move.l	(a0)+,(a5)
		move.l	(a0)+,(a5)
		move.l	(a0)+,(a5)
		move.l	(a0)+,(a5)
		move.l	(a0)+,(a5)
		move.l	(a0)+,(a5)

		move.l	(a5),(a1)	; plane 0
		adda.l	d1,a1
		move.l	(a5),(a1)	; plane 1
		adda.l	d1,a1
		move.l	(a5),(a1)	; plane 2
		adda.l	d1,a1
		move.l	(a5),(a1)	; plane 3
		adda.l	d1,a1
		move.l	(a5),(a1)	; plane 4
		adda.l	d1,a1
	ifgt depth-6
		move.l	(a5),(a1)	; plane 5
		adda.l	d1,a1
		move.l	(a5),(a1)	; plane 6
		adda.l	d1,a1
	endc
		move.l	(a5),(a1)+	; last plane

		suba.l	d0,a1		; -7*plsiz (or 5*plsiz) (or 3*plsiz)

		cmpa.l	a3,a0
		bne.b	loop

exit:		jsr	(_LVODisownBlitter,a6) ; free Akiko

		movem.l	(sp)+,a2/a3/a6
		rts

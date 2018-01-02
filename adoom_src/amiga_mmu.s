		mc68040

		section	text,code

*
*   mmu.s - 68040/68060 MMU control functions
*   Copyright (C) 1997 Aki Laukkanen
*

		XDEF    _mmu_mark

		include "funcdef.i"
		include "exec/types.i"
		include "exec/memory.i"
		include "exec/exec_lib.i"
		include "amiga_mmu.i"

	IFND    TRUE
TRUE EQU    1
	ENDC

	IFND    FALSE
FALSE EQU   0
	ENDC

*
*   FUNCTION
*       UBYTE __asm mmu_mark (register __a0 UBYTE *start,
*                             register __d0 ULONG length,
*                             register __d1 ULONG cm,
*                             register __a6 struct ExecBase *SysBase);
*
*   SYNOPSIS
*       Changes the cache mode for the specified memory area. This
*       area  must be aligned by 4kB and be multiple of 4kB in size.
*
*   RESULT
*       Returns the old cache mode for the memory area.
*
*   NOTES
*       Works only after setpatch has been issued and in such
*       systems where 68040.library/68060.library is correctly
*       installed.
*

_mmu_mark
		movem.l	d2/d3/d7/a2/a4/a6,-(sp)

		move.l	a1,a4
		movem.l	d0/d1/a0,-(sp)
		jsr	(_LVOSuperState,a6)
		movec	tc,d3			; translation code register
		movec	urp,d2			; user root pointer
		jsr	(_LVOUserState,a6)
		movem.l	(sp)+,d0/d1/a0

		btst	#TCB_E,d3
		beq	.error
		btst	#TCB_P,d3
		bne	.error

		move.l	d1,-(sp)
		move.l	d0,d1

		lsr.l	#8,d0
		lsr.l	#4,d0

		move.l	a0,a1

		and.w	#$fff,d1
		beq.s	.skip_a
		addq.l	#1,d0
.skip_a
		move.l	(sp)+,d1
		subq.l	#1,d0
		move.l	d0,d7

; a1 - chunkybuffer
; d7 - counter
; d2 - urp
; d1 - cache mode

.loop
		move.l	d2,-(sp)
		move.l	a1,d0
		rol.l	#8,d0
		lsl.l	#1,d0
		and.w	#%1111111000000000,d2
		and.w	#%0000000111111100,d0
		or.w	d0,d2
		move.l	d2,a2
		move.l	(a2),d2
		btst	#TDB_UDT0,d2
		beq	.skip			; if 0
		btst	#TDB_UDT1,d2		; if 1
		beq	.end
		bra	.skip2
.skip
		btst	#TDB_UDT1,d2
		bne	.end
.skip2
		move.l	a1,d0
		lsr.l	#8,d0
		lsr.l	#8,d0
		and.w	#%1111111000000000,d2
		and.w	#%0000000111111100,d0
		or.w	d0,d2
		move.l	d2,a2
		move.l	(a2),d2
		btst	#TDB_UDT0,d2
		beq	.skip1			; if 0
		btst	#TDB_UDT1,d2		; if 1
		beq	.end
		bra	.skip3
.skip1
		btst	#TDB_UDT1,d2
		bne	.end
.skip3
		move.l	a1,d0
		lsr.l	#8,d0
		lsr.l	#2,d0
		and.w	#%1111111100000000,d2
		and.w	#%0000000011111100,d0
		or.w	d0,d2

		move.l	d2,a2
		btst	#PDB_PDT1,(3,a2)
		bne	.skip4
		btst	#PDB_PDT0,(3,a2)
		beq	.end
		bra	.skip5
.skip4
		btst	#PDB_PDT0,(3,a2)
		beq	.indirect
.skip5
		move.b	(3,a2),d3
		and.b	#~CM_MASK,(3,a2)
		or.b	d1,(3,a2)

.indirect
		lea	(4096,a1),a1

		move.l	(sp)+,d2
		dbf	d7,.loop

		and.b	#CM_MASK,d3
		jsr	(_LVOSuperState,a6)
		pflusha
		jsr	(_LVOUserState,a6)

		moveq	#0,d0
		move.b	d3,d0

		movem.l	(sp)+,d2/d3/d7/a2/a4/a6
		rts
.end
		move.l	(sp)+,d2
.error
		movem.l	(sp)+,d2/d3/d7/a2/a4/a6
		moveq	#0,d0
		rts

		end

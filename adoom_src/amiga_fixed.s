		mc68030
		fpu

;-----------------------------------------------------------------------
		xdef	_FixedMul
		xdef	_FixedMul_040
		xdef	_FixedMul_060fpu
		xdef	_FixedMul_060
		xdef	_FixedDiv
		xdef	_FixedDiv_040
		xdef	_FixedDiv_060fpu
		xdef	_SetFPMode

		section	text,code

;-----------------------------------------------------------------------
; set 68060 FPU rounding mode to "truncate towards minus infinity"
; otherwise demos get out of step and play wrong

		cnop	0,4

_SetFPMode	fmove.l	fpcr,d0
		or.b	#$20,d0
		and.b	#~$10,d0
		fmove.l	d0,fpcr
		rts

;-----------------------------------------------------------------------
		cnop	0,4

; fixed_t FixedMul (fixed_t a, fixed_t b)

_FixedMul_040	muls.l	d1,d1:d0
		move.w	d1,d0
		swap	d0
		rts

;-----------------------------------------------------------------------
		cnop	0,4

; special version for 68060 which otherwise traps and emulates 64-bit muls.l

_FixedMul_060fpu
		fmove.l	d0,fp0
		fmul.l	d1,fp0
		fmul.s	#0.0000152587890625,fp0	; d0 * d1 / 65536 (* reciprocal)
		fmove.l	fp0,d0
		rts

;-----------------------------------------------------------------------
		cnop	0,4

; I'm told all Amiga 68060 accelerators have FPUs, but just in case...

_FixedMul_060	movem.l d2-d5,-(sp)

		clr.b	d5          ; clear sign tag
		tst.l	d0          ; multiplier negative?
		bge	.not1
		neg.l	d0
		or.b	#1,d5
.not1
		tst.l	d1          ; multiplicand negative?
		bge	.not2
		neg.l	d1
		eor.b	#1,d5
.not2
		move.l	d0,d2       ; mr
		move.l	d0,d3       ; mr
		move.l	d1,d4       ; md
		swap	d3          ; hi_mr in lo d3
		swap	d4          ; hi_md in lo d4

		mulu.w	d1,d0       ; [1] lo_mr * lo_md
		mulu.w	d3,d1       ; [2] hi_mr * lo_md
		mulu.w	d4,d2       ; [3] lo_mr * hi_md
		mulu.w	d4,d3       ; [4] hi_mr * hi_md

		clr.l	d4
		swap	d0
		add.w	d1,d0
		addx.l	d4,d3
		add.w	d2,d0
		addx.l	d4,d3
		swap	d0

		clr.w	d1
		clr.w	d2

		swap	d1
		swap	d2
		add.l	d2,d1
		add.l	d3,d1

		tst.b	d5          ; check sign of result
		beq	.skip

		not.l	d0
		not.l	d1
		addq.l	#1,d0
		addx.l	d4,d1

.skip
		move.w	d1,d0
		swap	d0

		movem.l	(sp)+,d2-d5
		rts

;-----------------------------------------------------------------------
		cnop	0,4

; fixed_t FixedDiv (fixed_t a, fixed_t b)

_FixedDiv_040	movem.l	d2/d3,-(sp)
		move.l	d0,d3
		swap	d0
		move.w	d0,d2
		ext.l	d2
		clr.w	d0
		tst.l	d1
		beq.b	3$		; check for divide by 0 !
		divs.l	d1,d2:d0
		bvc.b	1$
3$		eor.l	d1,d3
		bmi.b	2$
		move.l	#$7fffffff,d0
		bra.b	1$
2$		move.l	#$80000000,d0
1$		movem.l	(sp)+,d2/d3
		rts

;-----------------------------------------------------------------------
		cnop	0,4

; m68060fpu fixed division

_FixedDiv_060fpu
		tst.l	d1
		beq.b	3$		; check for divide by 0 !
		fmove.l	d0,fp0
		fmove.l	d1,fp1
		fdiv.x	fp1,fp0
		fmul.s	#65536.0,fp0
		fmove.l	fp0,d0
		rts

3$		eor.l	d1,d0
		bmi.b	2$
		move.l	#$7fffffff,d0
		rts

2$		move.l	#$80000000,d0
		rts

;-----------------------------------------------------------------------
		section	__MERGED,bss

; pointers to CPU-specific FixedMul and FixedDiv routine

;_FixedMul	ds.l	1
;_FixedDiv	ds.l	1

;-----------------------------------------------------------------------
		end

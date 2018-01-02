		mc68020

		XDEF	_Sega6
		XDEF	_Sega3

		section	text,code

; returns - xxSAxxxxxxCBMXYZxxSAxxDUxxCBRLDU

		cnop	0,4

_Sega6		bsr.b	GetSega			; xxSAxxDUxxCBRLDU
		swap	d0
		bsr.b	GetSega			; xxSAxxDUxxCBRLDU
		bsr.b	GetSega			; xxSAxxxxxxCBRLDU
		bsr.b	GetSega			; xxSAxxxxxxCBMXYZ
		move.w	#$FF00,$DFF034		; SEL=1
		swap	d0
		rts

; returns - 0000000000000000xxSAxxDUxxCBRLDU

		cnop	0,4

_Sega3		moveq	#0,d0
		bsr.b	GetSega
		move.w	#$FF00,$DFF034		; SEL=1
		rts

		cnop	0,4

GetSega		movem.l	d1-d3,-(a7)
		clr.w	d0

		move.w	#$FF01,$DFF034		; SEL=1, dump caps
		moveq	#22,d3
.hl		tst.b	$BFE001
		dbra	d3,.hl

		btst	#7,$BFE001
		bne.b	.1
		bset	#4,d0			; B/0
.1		move.w	$DFF016,d1
		btst	#14,d1
		bne.b	.2
		bset	#5,d0			; C/0
.2		move.w	$DFF00C,d1
		btst	#1,d1
		beq.b	.3
		bset	#3,d0			; R/M
.3		btst	#9,d1
		beq.b	.4
		bset	#2,d0			; L/X
.4		move.w	d1,d2
		lsl.w	#1,d2
		eor.w	d2,d1
		btst	#1,d1
		beq.b	.5
		bset	#1,d0			; D/Y
.5		btst	#9,d1
		beq.b	.6
		bset	#0,d0			; U/Z

.6		lsl.w	#8,d0

		move.w	#$EF01,$DFF034		; SEL=0, dump caps
		moveq	#22,d3
.ll		tst.b	$BFE001
		dbra	d3,.ll

		btst	#7,$BFE001
		bne.b	.11
		bset	#4,d0			; A/0
.11		move.w	$DFF016,d1
		btst	#14,d1
		bne.b	.12
		bset	#5,d0			; S/0
.12		move.w	$DFF00C,d1
		btst	#1,d1
		beq.b	.13
		bset	#3,d0			; 0/1
.13		btst	#9,d1
		beq.b	.14
		bset	#2,d0			; 0/1
.14		move.w	d1,d2
		lsl.w	#1,d2
		eor.w	d2,d1
		btst	#1,d1
		beq.b	.15
		bset	#1,d0			; D/1
.15		btst	#9,d1
		beq.b	.16
		bset	#0,d0			; U/1

.16		ror.w	#8,d0
		movem.l	(a7)+,d1-d3
		rts

		END

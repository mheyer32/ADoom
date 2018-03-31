		mc68020

		xdef	_P_DivlineSide

		section	.text,code

		near	a4,-2

;-----------------------------------------------------------------------
; P_DivLineSide (in p_sight.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

;	STRUCTURE	divline,0
;	 LONG		x	;all are actually fixed_t values..
;	 LONG		y
;	 LONG		dx
;	 LONG		dy
;	LABEL		divline_size

x		equ	0
y		equ	4
dx		equ	8
dy		equ	12
divline_size	equ	16

_P_DivlineSide:
		;I bet this could be a little faster if someone profiled the input
		;and found which case all in all happens most often...

		tst.l	dx(a0)
		bne.b	.pd_DXOK
		cmp.l	(a0),d0
		bgt.b	.pd_2	;x>node->x
		bne.b	.pd_1
		moveq	#2,d0
		rts
.pd_1:
		move.l	dy(a0),d0	;return node->dy >0
		beq.b	.pd1_Exit	;=0 => FALSE, no need to set D0
		bmi.b	.pd1_False	;<0 => FALSE
.pd1_True:
		moveq	#1,d0	;node->dy > 0 => TRUE
		rts
.pd1_False:
		moveq	#0,d0
.pd1_Exit:
		rts
.pd_2:
		move.l	dy(a0),d0
		beq.b	.pd1_Exit
		bmi.b	.pd1_True
		moveq	#0,d0	;node->dy >0 =>FALSE
		rts

.pd_DXOK:
		tst.l	dy(a0)
		bne.b	.pd_DYOK
		cmp.l	y(a0),d0
		bne.b	.pd_3
		moveq	#2,d0
		rts
.pd_3:
		cmp.l	y(a0),d1
		bgt.b	.pd_4
		move.l	dx(a0),d0
		bmi.b	.pd3_True
.pd3_False:
		moveq	#0,d0
.pd3_Exit:
		rts
.pd3_True:
		moveq	#1,d0
		rts
.pd_4:
		move.l	dx(a0),d0
		bmi.b	.pd3_False
		moveq	#1,d0
		rts

.pd_DYOK:
		sub.l	(a0),d0		;x-node->x
		sub.l	y(a0),d1	;y-node->y	
		swap	d0		;dx>>FRACBITS
		swap	d1		;dy>>FRACBITS

		muls.w	dy(a0),d0	;node->dy>>FRACBITS is simply the high word of node->dy
		muls.w	dx(a0),d1	;since this is only word and so is dx>>FRACBITS
		;we gain some clocks by using muls.W

		;d0=left, d1=right
		cmp.l	d0,d1
		bmi.b	.pd_Return0	;if right<left return 0
		beq.b	.pd_Return2	;if left==right return 2
		moveq	#1,d0
		rts
.pd_Return2:
		moveq	#2,d0	
		rts
.pd_Return0:
		moveq	#0,d0
		rts

;int
;P_DivlineSide
;( fixed_t	x,
;  fixed_t	y,
;  divline_t*	node )
;{
;    fixed_t	dx;
;    fixed_t	dy;
;    fixed_t	left;
;    fixed_t	right;
;
;    if (!node->dx)
;    {
;	if (x==node->x)
;	    return 2;
;	
;	if (x <= node->x)
;	    return node->dy > 0;
;
;	return node->dy < 0;
;    }
;    
;    if (!node->dy)
;    {
;	if (x==node->y)
;	    return 2;
;
;	if (y <= node->y)
;	    return node->dx < 0;
;
;	return node->dx > 0;
;    }
;	
;    dx = (x - node->x);
;    dy = (y - node->y);
;
;    left =  (node->dy>>FRACBITS) * (dx>>FRACBITS);
;    right = (dy>>FRACBITS) * (node->dx>>FRACBITS);
;	
;    if (right < left)
;	return 0;	// front side
;    
;    if (left == right)
;	return 2;
;    return 1;		// back side
;}

;-----------------------------------------------------------------------

		end

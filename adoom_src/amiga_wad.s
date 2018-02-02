		mc68020

		xdef	_W_CacheLumpNum

		section	.text,code

		near	a4,-2

;-----------------------------------------------------------------------
; R_CacheLumpNum (in w_wad.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_numlumps	;int
		xref	_lumpcache	;void**

		xref	_W_LumpLength
		xref	_W_ReadLump
		xref	_Z_ChangeTag2
		xref	_I_Error
		xref	_Z_Malloc

		cnop	0,4

_W_CacheLumpNum:
		cmp.l	_numlumps(a4),d0
		bpl.b	.wc_Error

		move.l	a2,-(sp)

		move.l	_lumpcache(a4),a2
		lea	(a2,d0.l*4),a2
		tst.l	(a2)
		beq.b	.wc_Miss
		move.l	(a2),a0
		move.l	d1,d0
		jsr	(_Z_ChangeTag2)
		move.l	(a2),d0

		move.l	(sp)+,a2
		rts

		cnop	0,4
.wc_Miss:
		movem.l	d2/d3,-(sp)
		move.l	d0,d2
		move.l	d1,d3
		jsr	(_W_LumpLength)
		move.l	d3,d1
		move.l	a2,a0
		jsr	(_Z_Malloc)
		move.l	d2,d0
		move.l	(a2),a0
		jsr	(_W_ReadLump)
		move.l	(a2),d0
		movem.l	(sp)+,d2/d3/a2
		rts
.wc_Error:
		move.l	d0,-(sp)
		move.l	#.wc_Msg,-(sp)
		jsr	(_I_Error)
		addq.l	#8,sp
		rts

.wc_Msg:
		dc.b	"W_CacheLumpNum: %i >= numlumps",0

;void*
;W_CacheLumpNum
;( int		lump,
;  int		tag )
;{
;    byte*	ptr;
;
;    if ((unsigned)lump >= numlumps)
;	I_Error ("W_CacheLumpNum: %i >= numlumps",lump);
;		
;    if (!lumpcache[lump])
;    {
;	// read the lump in
;	
;	//printf ("cache miss on lump %i\n",lump);
;	ptr = Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
;	W_ReadLump (lump, lumpcache[lump]);
;    }
;    else
;    {
;	//printf ("cache hit on lump %i\n",lump);
;	Z_ChangeTag (lumpcache[lump],tag);
;    }
;	
;    return lumpcache[lump];
;}

;-----------------------------------------------------------------------

		end

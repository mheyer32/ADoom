		mc68020

		xdef	@R_GetColumn
		xdef	_R_GetColumn

		section	text,code

		near	a4,-2

;-----------------------------------------------------------------------
; R_GetColumn (in r_data.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	@W_CacheLumpNum
		xref	@R_GenerateComposite
		xref	_texturewidthmask	; byte*
		xref	_texturecolumnlump	; short**
		xref	_texturecolumnofs	; unsigned short**
		xref	_texturecomposite	; byte**

;int*			texturewidthmask;
;fixed_t*		textureheight;		
;int*			texturecompositesize;
;short**		texturecolumnlump;
;unsigned short**	texturecolumnofs;
;byte**			texturecomposite;

		cnop	0,4
@R_GetColumn:
_R_GetColumn:
		movem.l	d2/d4,-(sp)

		move.l	_texturewidthmask(a4),a0
		and.l	(a0,d0.l*4),d1	; col &= texturewidthmask[tex]
		move.l	d0,d2		; Save the value of tex

		move.l	_texturecolumnofs(a4),a0
		move.l	(a0,d2.l*4),a0
		moveq	#0,d4
		move.w	(a0,d1.l*2),d4
		move.l	_texturecolumnlump(a4),a0
		move.l	(a0,d2.l*4),a0
		moveq	#0,d0
		move.w	(a0,d1.l*2),d0
		ble.b	.rg_1		; if lump >0

		moveq	#101,d1		; PU_CACHE
		jsr	(@W_CacheLumpNum)
		add.l	d4,d0
		movem.l	(sp)+,d2/d4
		rts

		cnop	0,4
.rg_1:
		move.l	_texturecomposite(a4),a0
		move.l	(a0,d2.l*4),d0
		bne.b	.rg_NotNull
		move.l	d2,d0
		jsr	(@R_GenerateComposite)
		move.l	_texturecomposite(a4),a0
		move.l	(a0,d2.l*4),d0
.rg_NotNull:
		add.l	d4,d0
		movem.l	(sp)+,d2/d4
		rts

;byte*
;R_GetColumn
;( int		tex,
;  int		col )
;{
;    int		lump;
;    int		ofs;
;	
;    col &= texturewidthmask[tex];
;    lump = texturecolumnlump[tex][col];
;    ofs = texturecolumnofs[tex][col];
;
;    if (lump > 0)
;	return (byte *)W_CacheLumpNum(lump,PU_CACHE)+ofs;
;
;   if (!texturecomposite[tex])
;	R_GenerateComposite (tex);
;
;   return texturecomposite[tex] + ofs;
;}

;-----------------------------------------------------------------------

		end

		mc68020

		xdef	_R_RenderSegLoop
		xdef	_R_PointToDist
		xdef	_R_RenderMaskedSegRange
		xdef	_R_ScaleFromGlobalAngle

		include	"exec/types.i"

		section	.text,code

		near	a4,-2

;-----------------------------------------------------------------------
		xref	_segtextured		; boolean
		xref	_markfloor		; boolean
		xref	_markceiling		; boolean
		xref	_maskedtexture		; boolean
		xref	_maskedtexturecol	; short *
		xref	_toptexture		; short
		xref	_bottomtexture		; short
		xref	_midtexture		; short
		xref	_rw_x			; int
		xref	_rw_stopx		; int
		xref	_rw_centerangle		; angle_t
		xref	_rw_offset		; fixed_t
		xref	_rw_distance		; fixed_t
		xref	_rw_scale		; fixed_t
		xref	_rw_scalestep		; fixed_t
		xref	_rw_midtexturemid	; fixed_t
		xref	_rw_toptexturemid	; fixed_t
		xref	_rw_bottomtexturemid	; fixed_t
		xref	_pixhigh		; fixed_t
		xref	_pixlow			; fixed_t
		xref	_pixhighstep		; fixed_t
		xref	_pixlowstep		; fixed_t
		xref	_topfrac		; fixed_t
		xref	_topstep		; fixed_t
		xref	_bottomfrac		; fixed_t
		xref	_bottomstep		; fixed_t
		xref	_walllights		; lighttable_t **
		xref	_ceilingclip		; short *
		xref	_ceilingplane		; visplane_t *
		xref	_floorclip		; short *
		xref	_floorplane		; visplane_t *
		xref	_xtoviewangle		; angle_t *
		xref	_finetangent		; fixed_t[]
		xref	_FixedMul
		xref	_colfunc
		xref    _dc_x
		xref    _dc_iscale
		xref    _dc_yl
		xref    _dc_yh
		xref    _dc_colormap
		xref    _dc_texturemid
		xref    _dc_source
		xref	_viewheight

		xref	_R_GetColumn

		cnop	0,4

_R_RenderSegLoop
		move.l	_rw_x(a4),d0
		cmp.l	_rw_stopx(a4),d0
		blt.b	1$
		rts

1$		movem.l	d2-d7/a2/a3/a5/a6,-(sp)
		movea.l	_rw_scale(a4),a6 ; a6 = rw_scale
		move.l	d0,a2
		movea.l	_ceilingclip(a4),a3
		add.l	d0,d0
		movea.l	_floorclip(a4),a5
		add.l	d0,a3
		add.l	d0,a5

20$		move.l	a2,d0
		move.l	_topfrac(a4),d7		; d7 = topfrac
		add.l	#$fff,d7		; d7 = topfrac + HEIGHTUNIT-1
		move.w	(a3),d3	; d3.w = ceilingclip[rw_x]
		moveq	#12,d5
		asr.l	d5,d7		; d7 = (topfrac - 1) >> 12
		ext.l	d3		; d3 = ceilingclip[rw_x]
		addq.l	#1,d3		; d3 = top = ceilingclip[rw_x] + 1
		cmp.l	d3,d7
		bge.b	2$
		move.l	d3,d7		; d7 = yl = ceilingclip[rw_x] + 1

2$		tst.l	_markceiling(a4) ; if (markceiling) {
		beq.b	3$
		move.l	d7,d4		; d4 = yl
		move.w	(a5),d1	; d1.w = floorclip[rw_x]
		ext.l	d1		; d1 = floorclip[rw_x]
		cmp.l	d1,d4		; if (bottom >= floorclip[rw_x])
		ble.b	4$		;D4 might be one bigger here
		move.l	d1,d4

4$		subq.l	#1,d4
		cmp.l	d4,d3		; if (top <= bottom)
		bgt.b	3$

		movea.l	_ceilingplane(a4),a0
		movea.l	20(a0),a1
		move.w	d3,(a1,d0.l*2)	; ceilingplane->top[rw_x] = top
		movea.l	24(a0),a1
		move.w	d4,(a1,d0.l*2)	; ceilingplane->bottom[rw_x] = bottom

3$		move.l	_bottomfrac(a4),d3		; d3 = bottomfrac
		asr.l	d5,d3
		move.w	(a5),d1	; d1.w = floorclip[rw_x]
		ext.l	d1		; d1 = floorclip[rw_x]
		cmp.l	d1,d3		; if (yh >= floorclip[rw_x])
		blt.b	5$
		move.l	d1,d3
		subq.l	#1,d3		; d3 = yh = floorclip[rw_x] - 1

5$		tst.l	_markfloor(a4)	; if (markfloor)
		beq.b	6$
		move.l	d3,d4		; d4 = yh
		move.w	(a3),d2	; d2.w = ceilingclip[rw_x]
		addq.l	#1,d4		; d4 = top = yh + 1
		ext.l	d2		; d2 = ceilingclip[rw_x]
		subq.l	#1,d1		; d1 = bottom = floorclip[rw_x] - 1
		cmp.l	d2,d4		; if (top <= ceilingclip[rw_x])
		bgt.b	7$
		move.l	d2,d4
		addq.l	#1,d4		; d4 = top = ceilingclip[rw_x] + 1
7$		cmp.l	d1,d4		; if (top <= bottom)
		bgt.b	6$

		movea.l	_floorplane(a4),a0
		movea.l	20(a0),a1
		move.w	d4,(a1,d0.l*2)	; ceilingplane->top[rw_x] = top
		movea.l	24(a0),a1
		move.w	d1,(a1,d0.l*2)	; ceilingplane->bottom[rw_x] = bottom

6$		tst.l	_segtextured(a4) ; if (segtextured)
		beq.b	8$
		move.l	d0,_dc_x(a4)	; dc_x = rw_x
		movea.l	_xtoviewangle(a4),a0 ; a0 -> xtoviewangle
		move.l	_rw_centerangle(a4),d1
		add.l	(a0,d0.l*4),d1	; d1 = rw_centerangle + xtoviewangle[rw_x]
		swap	d1		; d1 = angle
		lea		(_finetangent),a0 ; a0 -> finetangent
		lsr.w	#3,d1
		move.l	(a0,d1.w*4),d0	; d0 = finetangent[angle]
		movea.l	_FixedMul(a4),a0
		move.l	_rw_distance(a4),d1
		jsr		(a0)		; d0 = FixedMul(finetangent[angle],rw_distance)
		move.l	a6,d4		; d4 = rw_scale
		asr.l	d5,d4
		move.l	_rw_offset(a4),d5
		sub.l	d0,d5		; d5 = rw_offset-FixedMul(finetangent[angle],rw_distance)
		swap	d5		; d5.w = texturecolumn >>= 16
		ext.l	d5		; d5 = texturecolumn
		cmp.l	#$30,d4		; if (index >= MAXLIGHTSCALE)
		blt.b	9$
		moveq	#$2f,d4		; d4 = index = MAXLIGHTSCALE - 1
9$		movea.l	_walllights(a4),a0
		moveq	#-1,d0		; d0 = $ffffffff
		move.l	(a0,d4.l*4),_dc_colormap(a4) ; dc_colormap = walllights[index]

		move.l	a6,d1		; d1 = rw_scale
		divu.l	d1,d0
		move.l	d0,_dc_iscale(a4) ; dc_iscale = $ffffffff / rw_scale

8$		move.l	_midtexture(a4),d0 ; if (midtexture)
		beq.b	10$
		move.l	d7,_dc_yl(a4)	; dc_yl = yl
		move.l	d3,_dc_yh(a4)	; dc_yh = yh
		move.l	_rw_midtexturemid(a4),_dc_texturemid(a4)
		move.l	d5,d1		; d1 = texturecolumn
		jsr		(_R_GetColumn)
		move.l	d0,_dc_source(a4) ; dc_source = R_GetColumn(midtexture,texturecolumn)
		movea.l	_colfunc(a4),a0
		jsr		(a0)		; colfunc()
		move.l	_viewheight(a4),d1 ; d1 = viewheight
		move.w	d1,(a3)	; ceilingclip[rw_x] = viewheight
		move.w	#$ffff,(a5) ; floorclip[rw_x] = -1
		bra.w	11$

10$		moveq	#12,d4
		move.l	_toptexture(a4),d0 ; if (toptexture)
		beq.b	12$
		move.l	_pixhighstep(a4),d1 ; d1 = pixhighstep
		move.l	_pixhigh(a4),d2	; d2 = pixhigh
		add.l	d1,_pixhigh(a4)	; pixhigh += pixhighstep
		asr.l	d4,d2
		move.w	(a5),d1	; d1.w = floorclip[rw_x]
		ext.l	d1		; d1 = floorclip[rw_x]
		move.l	d2,d6		; d6 = mid = pixhigh >> 12
		cmp.l	d1,d6		; if (mid >= floorclip[rw_x])
		blt.b	13$
		move.l	d1,d6
		subq.l	#1,d6		; d6 = mid = floorclip[rw_x] - 1
13$		cmp.l	d7,d6		; if (mid >= yl)
		blt.b	14$
		move.l	d7,_dc_yl(a4)	; dc_yl = yl
		move.l	d6,_dc_yh(a4)	; dc_yh = mid
		move.l	_rw_toptexturemid(a4),_dc_texturemid(a4)
		move.l	d5,d1		; d1 = texturecolumn, d0 = toptexture
		jsr		(_R_GetColumn)
		move.l	d0,_dc_source(a4)	; dc_source = R_GetColumn(d0,d1)
		movea.l	_colfunc(a4),a0
		jsr		(a0)		; colfunc()
		move.w	d6,(a3)	; ceilingclip[rw_x] = mid
		bra.b	15$

12$		tst.l	_markceiling(a4) ; else if (markceiling)
		beq.b	15$
14$		subq.l	#1,d7		; d7 = yl - 1
		move.w	d7,(a3)	; ceilingclip[rw_x] = yl - 1

15$		move.l	_bottomtexture(a4),d0 ; if (bottomtexture)
		beq.b	16$
		move.l	_pixlow(a4),d6	; d6 = pixlow
		move.l	d6,d1		; d1 = pixlow
		add.l	_pixlowstep(a4),d1 ; d1 = pixlow + pixlowstep
		subq.l	#1,d6		; d6 = pixlow - 1
		move.l	d1,_pixlow(a4)	; pixlow += pixlowstep
		asr.l	d4,d6
		move.w	(a3),d1	; d1.w = ceilingclip[rw_x]
		addq.l	#1,d6		; d6 = mid = (pixlow + (1 << 12) - 1) >> 12
		ext.l	d1		; d1 = ceilingclip[rw_x]
		cmp.l	d1,d6		; if (mid <= ceilingclip[rw_x])
		bgt.b	17$
		move.l	d1,d6
		addq.l	#1,d6		; d6 = mid = ceilingclip[rw_x] + 1
17$		cmp.l	d3,d6		; if (mid <= yh)
		bgt.b	18$
		move.l	d6,_dc_yl(a4)	; dc_yl = mid
		move.l	d3,_dc_yh(a4)	; dc_yh = yh
		move.l	_rw_bottomtexturemid(a4),_dc_texturemid(a4)
		move.l	d5,d1		; d1 = texturecolumn, d0 = bottomtexture
		jsr		(_R_GetColumn)
		move.l	d0,_dc_source(a4) ; dc_source = R_GetColumn(d0,d1)
		movea.l	_colfunc(a4),a0
		jsr		(a0)		; colfunc ()
		move.w	d6,(a5)	; floorclip[rw_x] = mid
		bra.b	19$

16$		tst.l	_markfloor(a4)	; else if (markfloor)
		beq.b	19$
18$		addq.l	#1,d3		; d3 = yh + 1
		move.w	d3,(a5)	; floorclip[rw_x] = yh + 1

19$		tst.l	_maskedtexture(a4) ; if (maskedtexture)
		beq.b	11$
		move.l	a2,d0
		movea.l	_maskedtexturecol(a4),a0
		move.w	d5,(a0,d0.l*2)	; maskedtexturecol[rw_x] = texturecolumn

11$		adda.l	_rw_scalestep(a4),a6 ; rw_scale += rw_scalestep
		move.l	_topstep(a4),d0
		add.l	d0,_topfrac(a4)	; topfrac += topstep
		move.l	_bottomstep(a4),d0
		add.l	d0,_bottomfrac(a4)
		addq.l	#1,a2
		addq.l	#2,a3
		addq.l	#2,a5

		cmpa.l	_rw_stopx(a4),a2
		blt.w	20$

		move.l	a6,_rw_scale(a4)
		move.l	a2,_rw_x(a4)

		movem.l	(sp)+,d2-d7/a2/a3/a5/a6
		rts

;-----------------------------------------------------------------------
; R_PointToDist (in r_main.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_tantoangle	;FAR angle_t tantoangle[2049]
		xref	_FixedDiv
		xref	_finesine	;FAR int finesine[10240]
		xref	_viewx
		xref	_viewy

DBITS		equ	16-11	;FRACBITS-SLOPEBITS
ANG90		equ	$40000000
ANGLETOFINESHIFT	equ	19

_R_PointToDist:
		move.l	d2,-(sp)
		move.l	_FixedDiv(a4),a1

		move.l	d0,d2
		move.l	d1,d0

		sub.l	_viewx(a4),d2
		bpl		.rp_1OK
		neg.l	d2
.rp_1OK:
		sub.l	_viewy(a4),d0
		bpl		.rp_2OK
		neg.l	d0
.rp_2OK:
		cmp.l	d0,d2
		bge		.rp_3OK
		exg.l	d0,d2
.rp_3OK:
		move.l	d2,d1
		jsr		(a1)
		asr.l	#DBITS,d0
		lea	    _tantoangle,a0
		move.l	(a0,d0.l*4),d1
		add.l	#ANG90,d1
		moveq	#ANGLETOFINESHIFT,d0
		asr.l	d0,d1
		lea	    _finesine,a0
		move.l	(a0,d1.l*4),d1
		move.l	d2,d0
		jsr		(a1)

		move.l	(sp)+,d2
		rts

;fixed_t
;R_PointToDist
;( fixed_t	x,
;  fixed_t	y )
;{
;    int		angle;
;    fixed_t	dx;
;    fixed_t	dy;
;    fixed_t	temp;
;    fixed_t	dist;
;	
;    dx = iabs(x - viewx);
;    dy = iabs(y - viewy);
;	
;    if (dy>dx)
;    {
;	temp = dx;
;	dx = dy;
;	dy = temp;
;    }
;	
;    angle = (tantoangle[ FixedDiv(dy,dx)>>DBITS ]+ANG90) >> ANGLETOFINESHIFT;
;
;    // use as cosine
;    dist = FixedDiv (dx, finesine[angle] );	
;	
;    return dist;
;}

;-----------------------------------------------------------------------
; R_RenderMaskedSegRange (in r_segs.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_curline
		xref	_frontsector
		xref	_backsector
		xref	_extralight
		xref	_walllights		;lighttable_t**
		xref	_scalelight		;FAR lighttable_t* scalelight[LIGHTLEVELS][MAXLIGHTSCALE]
		xref	_maskedtexturecol	;short*
		xref	_rw_scalestep
		xref	_spryscale
		xref	_mfloorclip
		xref	_mceilingclip
		xref	_dc_texturemid
		xref	_dc_colormap
		xref	_fixedcolormap
		xref	_dc_x
		xref	_sprtopscreen
		xref	_centeryfrac
		xref	_FixedMul
		xref	_R_GetColumn
		xref	_R_DrawMaskedColumn
		xref	_dc_iscale
		xref	_texturetranslation	;int *
		xref	_textureheight	;fixed_t*
		xref	_viewz

LIGHTSEGSHIFT	equ	4
LIGHTLEVELS	equ	16
LIGHTSCALESHIFT	equ	12
MAXSHORT	equ	$7FFF
MAXLIGHTSCALE	equ	48
ML_DONTPEGBOTTOM equ	16	;bit number is 4

		STRUCTURE	drawseg,0
		 APTR	ds_curline
		 LONG	ds_x1
		 LONG	ds_x2
		 LONG	ds_scale1
		 LONG	ds_scale2
		 LONG	ds_scalestep
		 LONG	ds_silhouette
		 LONG	ds_bsilheight
		 LONG	ds_tsilheight
		 APTR	ds_sprtopclip
		 APTR	ds_sprbottomclip
		 APTR	ds_maskedtexturecol
		LABEL	ds_size

		STRUCTURE	seg,0
		 APTR	s_v1
		 APTR	s_v2
		 LONG	s_offset
		 LONG	s_angle
		 APTR	s_sidedef
		 APTR	s_linedef
		 APTR	s_frontsect
		 APTR	s_backsect
		LABEL	s_size

		STRUCTURE	sector,0
		 LONG	st_floorheight
		 LONG	st_ceilingheight
		 WORD	st_floorpic
		 WORD	st_ceilingpic
		 WORD	st_lightlevel
		 WORD	st_special
		 WORD	st_tag
		 LONG	st_soundtraversed
		 APTR	st_soundtarget
		 STRUCT	st_blockbox,5
		 ;etc. etc.

		STRUCTURE	vertex,0
		 LONG	v_x
		 LONG	v_y

		STRUCTURE	line,0
		 APTR	l_v1
		 APTR	l_v2
		 LONG	l_dx
		 LONG	l_dy
		 WORD	l_flags
		 WORD	l_special
		 WORD	l_tag

		STRUCTURE	side,0
		 LONG	sd_textureoffset
		 LONG	sd_rowoffset
		 WORD	sd_toptexture
		 WORD	sd_bottomtexture
		 WORD	sd_midtexture
		 APTR	sd_sector

_R_RenderMaskedSegRange:
		movem.l	d2-d7/a2/a3/a5,-(sp)

		move.l	d0,d5	;store x1
		move.l	d1,d6	;and x2
		;we can keep ds in A0 all the way through

		move.l	ds_curline(a0),a1	;curline.. kept in a1
		move.l	s_frontsect(a1),a2	;used soon below
		move.l	a1,_curline(a4)
		move.l	a2,_frontsector(a4)
		;texnum is calculated later...

;BTW the walllights variable is not really used at all in this context
;for anything else but storage.. so lets just put it to a register.. faster!
		moveq	#0,d2
		move.w	st_lightlevel(a2),d2
		lsr.w	#LIGHTSEGSHIFT,d2
		move.l	s_backsect(a1),_backsector(a4)
		add.l	_extralight(a4),d2

		move.l	#_scalelight,a5	;walllights=scalelight[0]

		move.l	s_v1(a1),a3	;pick coords from vertex one.. compare from vertex two
		move.l	v_y(a3),d0
		move.l	v_x(a3),d1
		move.l	s_v2(a1),a3
		cmp.l	v_y(a3),d0
		bne.b	.rr_Skip1
		subq.l	#1,d2
		bra.b	.rr_LN1Go
.rr_Skip1:
		cmp.l	v_x(a3),d1
		bne.b	.rr_LN1Go
		addq.l	#1,d2
.rr_LN1Go:
		tst.l	d2
		bmi.b	.rr_LightsOK	;negative.. keep walllights as it is
		cmp.l	#16,d2
		bmi.b	.rr_LightnumOK
		add.l	#2880,a5
		bra.b	.rr_LightsOK
.rr_LightnumOK:
		move.l	d2,d1
		add.l	d1,d1
		add.l	d2,d1
		lsl.l	#6,d1
		add.l	d1,a5
.rr_LightsOK:
		move.l	ds_maskedtexturecol(a0),_maskedtexturecol(a4)

		move.l	ds_scalestep(a0),d1
		move.l	d1,_rw_scalestep(a4)
		move.l	ds_sprbottomclip(a0),_mfloorclip(a4)
		move.l	ds_sprtopclip(a0),_mceilingclip(a4)
		move.l	d5,d0
		sub.l	ds_x1(a0),d0
		muls.l	d1,d0
		add.l	ds_scale1(a0),d0
		move.l	d0,_spryscale(a4)

		move.l	s_sidedef(a1),a0
		move.l	sd_rowoffset(a0),_dc_texturemid(a4)	;dc_texturemid += curline->sidedef->rowoffset
		move.w	sd_midtexture(a0),d0
		move.l	_texturetranslation(a4),a0
		move.l	(a0,d0.w*4),d7	;texnum

		move.l	s_linedef(a1),a0
		move.w	l_flags(a0),d0

		move.l	_frontsector(a4),a0
		move.l	_backsector(a4),a2

		btst.l	#4,d0	;test for ML_DONTPEGBOTTOM
		beq.b	.rr_tmid2
		move.l	st_floorheight(a0),d0
		cmp.l	st_floorheight(a2),d0
		bgt.b	.rrtm1_OK
		move.l	st_floorheight(a2),d0
.rrtm1_OK:
		sub.l	_viewz(a4),d0
		move.l	_textureheight(a4),a0
		add.l	(a0,d7.l*4),d0
		add.l	d0,_dc_texturemid(a4)
		bra.b	.rr_tmidDone
.rr_tmid2:
		move.l	st_ceilingheight(a0),d0
		cmp.l	st_ceilingheight(a2),d0
		blt.b	.rrtm2_OK
		move.l	st_ceilingheight(a2),d0
.rrtm2_OK:
		sub.l	_viewz(a4),d0
		add.l	d0,_dc_texturemid(a4)
.rr_tmidDone:

		move.l	_fixedcolormap(a4),d0
		beq.b	.rr_NoFixCMAP
		move.l	d0,_dc_colormap(a4)
.rr_NoFixCMAP:

;prepare everything for the loop
		move.l	d5,_dc_x(a4)
		sub.l	d5,d6
		blt.b	.rr_SkipLoop
		move.l	_maskedtexturecol(a4),a3
		add.l	d5,a3
		add.l	d5,a3	;&maskedtexturecol[dc_x]
		moveq	#12,d5
		move.l	_FixedMul(a4),a2
		move.l	_spryscale(a4),d2
		move.l	_dc_texturemid(a4),d3
		move.l	_rw_scalestep(a4),d4
.rr_Loop:
		cmp.w	#MAXSHORT,(a3)
		beq.b	.rrl_SkipAll
		tst.l	_fixedcolormap(a4)
		bne.b	.rrl_SkipCMAP
		move.l	d2,d0
		lsr.l	d5,d0
		cmp.l	#MAXLIGHTSCALE,d0
		bmi.b	.rrl_IndexOK
		moveq	#48,d0	;saves us a branch
.rrl_IndexOK:
		move.l	(a5,d0.l*4),_dc_colormap(a4)

.rrl_SkipCMAP:
		moveq	#-1,d0
		divu.l	d2,d0
		move.l	_centeryfrac(a4),_sprtopscreen(a4)	;scheduling, helps a bit
		move.l	d0,_dc_iscale(a4)
		move.l	d3,d0
		move.l	d2,d1
		jsr	(a2)
		sub.l	d0,_sprtopscreen(a4)

		move.l	d7,d0
		moveq	#0,d1
		move.w	(a3),d1
		jsr	(_R_GetColumn)
		subq.l	#3,d0
		move.l	d0,a0
		jsr	(_R_DrawMaskedColumn)
		move.w	#MAXSHORT,(a3)

.rrl_SkipAll:
		addq.l	#2,a3
		addq.l	#1,_dc_x(a4)
		add.l	d4,d2
		move.l	d2,_spryscale(a4)
		dbf	d6,.rr_Loop
.rr_SkipLoop:
		movem.l	(sp)+,d2-d7/a2/a3/a5

		rts


;void
;R_RenderMaskedSegRange
;( drawseg_t*	ds,
;  int		x1,
;  int		x2 )
;{
;    unsigned	index;
;    column_t*	col;
;    int		lightnum;
;    int		texnum;
;    
;    curline = ds->curline;
;    frontsector = curline->frontsector;
;    backsector = curline->backsector;
;    texnum = texturetranslation[curline->sidedef->midtexture];
;	
;    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;
;
;    if (curline->v1->y == curline->v2->y)
;	lightnum--;
;    else if (curline->v1->x == curline->v2->x)
;	lightnum++;
;
;    if (lightnum < 0)
;	walllights = scalelight[0];
;    else if (lightnum >= LIGHTLEVELS)
;	walllights = scalelight[LIGHTLEVELS-1];
;    else
;	walllights = scalelight[lightnum];
;
;    maskedtexturecol = ds->maskedtexturecol;
;
;    rw_scalestep = ds->scalestep;		
;    spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
;    mfloorclip = ds->sprbottomclip;
;    mceilingclip = ds->sprtopclip;
;
;    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
;    {
;	dc_texturemid = frontsector->floorheight > backsector->floorheight
;	    ? frontsector->floorheight : backsector->floorheight;
;	dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
;    }
;    else
;    {
;	dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
;	    ? frontsector->ceilingheight : backsector->ceilingheight;
;	dc_texturemid = dc_texturemid - viewz;
;    }
;    dc_texturemid += curline->sidedef->rowoffset;
;			
;    if (fixedcolormap)
;	dc_colormap = fixedcolormap;
;    
;    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
;    {
;	if (maskedtexturecol[dc_x] != MAXSHORT)
;	{
;	    if (!fixedcolormap)
;	    {
;		index = spryscale>>LIGHTSCALESHIFT;
;
;		if (index >=  MAXLIGHTSCALE )
;		    index = MAXLIGHTSCALE-1;
;
;		dc_colormap = walllights[index];
;	    }
;			
;	    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
;	    dc_iscale = 0xffffffffu / (unsigned)spryscale;
;	    
;	    col = (column_t *)( 
;		(byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) -3);
;			
;	    R_DrawMaskedColumn (col);
;	    maskedtexturecol[dc_x] = MAXSHORT;
;	}
;	spryscale += rw_scalestep;
;    }
;	
;}

;-----------------------------------------------------------------------
; R_ScaleFromGlobalAngle (in r_main.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_viewangle
		xref	_rw_normalangle
		xref	_finesine
		xref	_projection
		xref	_rw_distance
		xref	_FixedMul
		xref	_FixedDiv
		xref	_detailshift

;ANGLETOFINESHIFT	EQU	19
;ANG90		EQU	$40000000

_R_ScaleFromGlobalAngle:
		move.l	d2,-(sp)
		add.l	#ANG90,d0
		move.l	d0,d1
		sub.l	_viewangle(a4),d0
		sub.l	_rw_normalangle(a4),d1

		moveq	#ANGLETOFINESHIFT,d2
		lsr.l	d2,d1
		lsr.l	d2,d0
		move.l	#_finesine,a0
		move.l	(a0,d1.l*4),d2	;sineb
		move.l	(a0,d0.l*4),d1	;sinea
		move.l	_rw_distance(a4),d0
		move.l	_FixedMul(a4),a1
		jsr		(a1)
		move.l	d2,d1
		move.l	d0,d2
		move.l	_projection(a4),d0
		jsr		(a1)
		move.l	_detailshift(a4),d1
		lsl.l	d1,d0

		move.l	d0,d1
		clr.w	d1
		swap	d1
		cmp.l	d2,d1
		bpl.b	.rs_Return1

		move.l	d2,d1
		move.l	_FixedDiv(a4),a0
		jsr		(a0)
		cmp.l	#64*(1<<16),d0
		bpl.b	.rs_Return1
		cmp.l	#256,d0
		bpl.b	.rs_Exit
		move.l	#256,d0
		move.l	(sp)+,d2
		rts

.rs_Return1:
		move.l	#64*(1<<16),d0
.rs_Exit:
		move.l	(sp)+,d2
		rts

;-----------------------------------------------------------------------

		end

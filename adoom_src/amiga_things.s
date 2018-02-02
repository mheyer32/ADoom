		mc68020

		xdef	@R_AddSprites
		xdef	_R_AddSprites

		xdef	@R_DrawMasked
		xdef	_R_DrawMasked

		xdef	@R_DrawSprite
		xdef	_R_DrawSprite

		xdef	@R_DrawVisSprite
		xdef	_R_DrawVisSprite

		xdef	@R_SortVisSprites
		xdef	_R_SortVisSprites
		xdef	@R_NewVisSprite
		xdef	_R_NewVisSprite
		xdef	@R_ClearSprites
		xdef	_R_ClearSprites

		section	.text,code

		near	a4,-2

;-----------------------------------------------------------------------

		include	"exec/types.i"

;-----------------------------------------------------------------------
; R_AddSprites (in r_things.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_extralight
		xref	_scalelight
		xref	_spritelights
		xref	@R_ProjectSprite
		xref	_validcount

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
		 STRUCT	st_blockbox,4*4
		 STRUCT	st_shit,24
		 LONG	st_validcount
		 APTR	st_thinglist
		 ;etc. etc.

snext		EQU	24

@R_AddSprites:
_R_AddSprites:
		move.l	_validcount(a4),d0
		cmp.l	st_validcount(a0),d0
		beq.b	.ra_Exit

		move.l	d2,-(sp)
		move.l	d0,st_validcount(a0)

		moveq	#0,d0
		move.w	st_lightlevel(a0),d0
		lsr.l	#4,d0
		move.l	#_scalelight,_spritelights(a4)
		add.l	_extralight(a4),d0
		bmi.b	.ra_LightDone
		cmp.l	#16,d0
		bmi.b	.ra_LightOK
		add.l	#2880,_spritelights(a4)
		bra.b	.ra_LightDone
.ra_LightOK:
		move.l	d0,d2
		lsl.l	#7,d0
		lsl.l	#6,d2
		add.l	d0,d2
		add.l	d2,_spritelights(a4)
.ra_LightDone:

		move.l	st_thinglist(a0),d2
		beq.b	.ra_AllDone
.ra_Loop:
		move.l	d2,a0
		move.l	snext(a0),d2
		beq.b	.ral_Done
		jsr	(_R_ProjectSprite)
		bra.b	.ra_Loop
.ral_Done:
		jsr	(_R_ProjectSprite)
.ra_AllDone:
		move.l	(sp)+,d2
.ra_Exit:
		rts


;void R_AddSprites (sector_t* sec)
;{
;    mobj_t*		thing;
;    int			lightnum;
;
;    // BSP is traversed by subsector.
;    // A sector might have been split into several
;    //  subsectors during BSP building.
;    // Thus we check whether its already added.
;    if (sec->validcount == validcount)
;	return;		
;
;    // Well, now it will be done.
;    sec->validcount = validcount;
;	
;    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight;
;
;    if (lightnum < 0)
;	spritelights = scalelight[0];
;   else if (lightnum >= LIGHTLEVELS)
;	spritelights = scalelight[LIGHTLEVELS-1];
;    else
;	spritelights = scalelight[lightnum];
;
;    // Handle all things in sector.
;    for (thing = sec->thinglist ; thing ; thing = thing->snext)
;	R_ProjectSprite (thing);
;}

;-----------------------------------------------------------------------
; R_DrawMasked (in r_things.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

;;;		xref	_R_SortVisSprites
		xref	_vissprite_p	;vissprite_t*
		xref	_vissprites	;FAR vissprite_t[MAXVISSPRITES]
		xref	_ds_p		;drawseg_t*
		xref	_drawsegs	;FAR drawseg_t drawsegs[MAXDRAWSEGS]
		xref	_R_RenderMaskedSegRange
		xref	_viewangleoffset	;int
		xref	_vsprsortedhead	;vissprite_t
		xref	_R_DrawPlayerSprites
		xref	_R_PointOnSegSide
		xref	_R_RenderMaskedSegRange
;;;		xref	_R_DrawVisSprite
		xref	_mfloorclip
		xref	_mceilingclip
		xref	_viewheight
;;;		xref	_R_DrawSprite

		STRUCTURE	vissprite,0

		 APTR	vp_prev
		 APTR	vp_next
		 LONG	vp_x1
		 LONG	vp_x2
		 LONG	vp_gx
		 LONG	vp_gy
		 LONG	vp_gz
		 LONG	vp_gzt
		 LONG	vp_startfrac
		 LONG	vp_scale
		 LONG	vp_xiscale
		 LONG	vp_texturemid
		 LONG	vp_patch
		 APTR	vp_colormap
		 LONG	vp_mobjflags
		LABEL	vp_size

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

		cnop	0,4
_R_DrawMasked:
_R_DrawMasked:
		movem.l	a2/a3,-(sp)
		jsr	(_R_SortVisSprites)

		move.l	#_vissprites,d1
		move.l	_vissprite_p(a4),d0
		cmp.l	d0,d1
		bge.b	.rdm_Skip1

		lea	_vsprsortedhead(a4),a3
		move.l	4(a3),a2
.rdm_Loop1:
		move.l	a2,a0
		jsr	(_R_DrawSprite)
		move.l	vp_next(a2),a2
		cmp.l	a2,a3
		bne.b	.rdm_Loop1

.rdm_Skip1:	
		move.l	#_drawsegs,a3
		move.l	_ds_p(a4),a2
		sub.l	#ds_size,a2
.rdm_Loop2:
		tst.l	ds_maskedtexturecol(a2)
		beq.b	.rdml2_Skip
		move.l	a2,a0
		move.l	ds_x1(a2),d0
		move.l	ds_x2(a2),d1
		jsr	(_R_RenderMaskedSegRange)

.rdml2_Skip:
		sub.l	#ds_size,a2
		cmp.l	a3,a2
		bge.b	.rdm_Loop2

		tst.l	_viewangleoffset(a4)
		bne.b	.rdm_Exit
		jsr	(_R_DrawPlayerSprites)

.rdm_Exit:
		movem.l	(sp)+,a2/a3
		rts


;void R_DrawMasked (void)
;{
;    vissprite_t*	spr;
;    drawseg_t*		ds;
;	
;    R_SortVisSprites ();
;
;    if (vissprite_p > vissprites)
;    {
;	// draw all vissprites back to front
;	for (spr = vsprsortedhead.next ;
;	     spr != &vsprsortedhead ;
;	     spr=spr->next)
;	{
;	    
;	    R_DrawSprite (spr);
;	}
;    }
;   
;    // render any remaining masked mid textures
;    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
;	if (ds->maskedtexturecol)
;	    R_RenderMaskedSegRange (ds, ds->x1, ds->x2);
;   
;    // draw the psprites on top of everything
;    //  but does not draw on side views
;    if (!viewangleoffset)		
;	R_DrawPlayerSprites ();
;}

;-----------------------------------------------------------------------
; R_DrawSprite (in r_things.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

;;;		xref	_R_SortVisSprites
		xref	_vissprite_p	;vissprite_t*
		xref	_vissprites	;FAR vissprite_t[MAXVISSPRITES]
		xref	_ds_p		;drawseg_t*
		xref	_drawsegs	;FAR drawseg_t drawsegs[MAXDRAWSEGS]
		xref	_R_RenderMaskedSegRange
		xref	_viewangleoffset	;int
		xref	_vsprsortedhead	;vissprite_t
		xref	_R_DrawPlayerSprites
		xref	_R_PointOnSegSide
		xref	_R_RenderMaskedSegRange
;;;		xref	_R_DrawVisSprite
		xref	_mfloorclip
		xref	_mceilingclip
		xref	_viewheight
		xref	_memcpy

_R_DrawSprite:
_R_DrawSprite:

		movem.l	d2-d7/a2/a3/a5/a6,-(sp)

		move.l	a0,a3	;save the sprite

		move.l	vp_x2(a0),d0
		move.l	vp_x1(a0),d1
		sub.l	d1,d0
		blt.b	.rds_Skip1
		add.l	d1,d1
;;;		move.l	#clipbot,a0
		lea	clipbot,a0
		add.l	d1,a0
;;;		move.l	#cliptop,a1
		lea	cliptop,a1
		add.l	d1,a1

		btst.l	#1,d1
		beq.b	.rds_BoundOK
		move.w	#-2,(a0)+
		move.w	#-2,(a1)+
		subq.l	#1,d0
		bmi.b	.rds_Skip1
		beq.b	.rdsl1_Last
.rds_BoundOK:
		move.b	d0,d1

.rds_Loop1:
		move.l	#$FFFEFFFE,(a0)+
		move.l	#$FFFEFFFE,(a1)+
		subq.l	#2,d0
		bgt.b	.rds_Loop1
		btst.l	#0,d1
		bne.b	.rds_Skip1
.rdsl1_Last:
		move.w	#-2,(a0)
		move.w	#-2,(a1)	

.rds_Skip1:
		move.l	_ds_p(a4),a2
		sub.l	#ds_size,a2		;ds (a2) = ds_p-1
		move.l	#_drawsegs,d7

		move.l	vp_scale(a3),d2	;these ones are used in loop below and
		move.l	vp_gz(a3),d3
		move.l	vp_gzt(a3),d4

.rds_Loop2:
		move.l	ds_x1(a2),d0
		move.l	ds_x2(a2),d1
		move.l	vp_x1(a3),d5
		move.l	vp_x2(a3),d6

		cmp.l	d6,d0
		bgt.b	.rdsl2_Next
		cmp.l	d5,d1
		blt.b	.rdsl2_Next

		tst.l	ds_silhouette(a2)
		bne.b	.rdsl2_DoLoop
		tst.l	ds_maskedtexturecol(a2)
		beq.b	.rdsl2_Next

.rdsl2_DoLoop:
		cmp.l	d5,d0
		blt.b	.rdsl2_R1OK
		move.l	d0,d5
.rdsl2_R1OK:
		cmp.l	d6,d1
		bgt.b	.rdsl2_R2OK
		move.l	d1,d6
.rdsl2_R2OK:

;At this stage:
;a2: ds		(always, really)
;a3: spr	(always, too)
;d5: r1
;d6: r2

		move.l	ds_scale1(a2),d0	;scale (?)
		move.l	ds_scale2(a2),d1	;lowscale  (?)
		cmp.l	d0,d1
		blt.b	.rdsl2_ScalesOK
		move.l	d1,d0
		move.l	ds_scale1(a2),d1
.rdsl2_ScalesOK:

;now:
;d1:lowscale
;d0:scale

		cmp.l	d2,d0
		blt.b	.rdsl2_DoStuff1	;scale<spr->scale is TRUE, RenderMaskedSegRange
		cmp.l	d2,d1
		bge.b	.rdsl2_SkipStuff1	;above is FALSE, lowscale<spr->scale is false... no RMSR
		move.l	vp_gx(a3),d0
		move.l	vp_gy(a3),d1
		move.l	ds_curline(a2),a0
		jsr		(_R_PointOnSegSide)
		tst.l	d0
		bne.b	.rdsl2_SkipStuff1	;all above is FALSE and PointOnSegSide is TRUE.. so the expression is false => skip
.rdsl2_DoStuff1:
		tst.l	ds_maskedtexturecol(a2)
		beq.b	.rdsl2_Next
		move.l	a2,a0	;(ds,
		move.l	d5,d0	;r1,
		move.l	d6,d1	;r2)
		jsr	(_R_RenderMaskedSegRange)
		bra.b	.rdsl2_Next

.rdsl2_SkipStuff1:
		move.l	ds_silhouette(a2),d0
		beq.b	.rdsl2_Next	;silhouette is zero, no point ANDing and testing..

		cmp.l	ds_bsilheight(a2),d3
		blt.b	.rdsl2_SilSkip1
		and.l	#~1,d0	;silhouette &=~SIL_BOTTOM
		beq.b	.rdsl2_Next	;went zero, no tests!

.rdsl2_SilSkip1:
		cmp.l	ds_tsilheight(a2),d4
		bgt.b	.rdsl2_SilSkip2
		and.l	#~2,d0	;silhouette &=~SIL_TOP
		beq.b	.rdsl2_Next	;went zero

.rdsl2_SilSkip2:
		cmp.l	#1,d0
		bne.b	.rdsl2_SilNot1
		sub.l	d5,d6	;x=r1  x<=r2
		blt.b	.rdsl2_Next
		add.l	d5,d5
;;;		move.l	#clipbot,a0
		lea	clipbot,a0
		add.l	d5,a0
		move.l	ds_sprbottomclip(a2),a1
		add.l	d5,a1
.rdsl2_SilLoop1:
		move.w	(a1)+,d0
		cmp.w	#-2,(a0)+
		bne.b	.rdsl2sl1_Skip
		move.w	d0,-2(a0)
.rdsl2sl1_Skip:
		dbf	d6,.rdsl2_SilLoop1
		bra.b	.rdsl2_Next

.rdsl2_SilNot1:
		cmp.l	#2,d0
		bne.b	.rdsl2_SilNot2
		sub.l	d5,d6
		blt.b	.rdsl2_Next
		add.l	d5,d5
;;;		move.l	#cliptop,a0
		lea	cliptop,a0
		add.l	d5,a0
		move.l	ds_sprtopclip(a2),a1
		add.l	d5,a1
.rdsl2_SilLoop2:
		move.w	(a1)+,d0
		cmp.w	#-2,(a0)+
		bne.b	.rdsl2sl2_Skip
		move.w	d0,-2(a0)
.rdsl2sl2_Skip:
		dbf	d6,.rdsl2_SilLoop2
		bra.b	.rdsl2_Next

.rdsl2_SilNot2:
		cmp.l	#3,d0
		bne.b	.rdsl2_Next
		sub.l	d5,d6
		blt.b	.rdsl2_Next
		lsl.l	#1,d5
;;;		move.l	#cliptop,a0
		lea	cliptop,a0
		add.l	d5,a0
;;;		move.l	#clipbot,a1
		lea	clipbot,a1
		add.l	d5,a1
		move.l	ds_sprbottomclip(a2),a5
		move.l	ds_sprtopclip(a2),a6
		add.l	d5,a5
		add.l	d5,a6
.rdsl2_SilLoop3:
		move.w	(a5)+,d0
		move.w	(a6)+,d1
		cmp.w	#-2,(a0)+
		bne.b	.rdsl2sl3_Skip1
		move.w	d1,-2(a0)
.rdsl2sl3_Skip1:
		cmp.w	#-2,(a1)+
		bne.b	.rdsl2sl3_Skip2
		move.w	d0,-2(a1)
.rdsl2sl3_Skip2:
		dbf	d6,.rdsl2_SilLoop3

.rdsl2_Next:
		sub.l	#ds_size,a2
		cmp.l	d7,a2
		bge.b	.rds_Loop2

		move.l	vp_x2(a3),d0	;spr->x2
		sub.l	vp_x1(a3),d0	;x<=spr->x2
		blt.b	.rds_SkipL3
		move.l	vp_x1(a3),d7	;x
		add.l	d7,d7
		move.l	_viewheight(a4),d1
;;;		move.l	#clipbot,a0
		lea	clipbot,a0
		add.l	d7,a0
;;;		move.l	#cliptop,a1
		lea	cliptop,a1
		add.l	d7,a1
.rds_Loop3:
		cmp.w	#-2,(a0)+
		bne.b	.rdsl3_Skip1
		move.w	d1,-2(a0)
.rdsl3_Skip1:
		cmp.w	#-2,(a1)+
		bne.b	.rdsl3_Skip2
		move.w	#-1,-2(a1)
.rdsl3_Skip2:
		dbf	d0,.rds_Loop3

.rds_SkipL3:
;;;		move.l	#clipbot,_mfloorclip(a4)
		lea	clipbot,a0
		move.l	a0,_mfloorclip(a4)
;;;		move.l	#cliptop,_mceilingclip(a4)
		lea	cliptop,a0
		move.l	a0,_mceilingclip(a4)
		move.l	a3,a0
		movem.l	(sp)+,d2-d7/a2/a3/a5/a6
		jmp	(_R_DrawVisSprite)


;void R_DrawSprite (vissprite_t* spr)
;{
;    drawseg_t*		ds;
;    static short	clipbot[MAXSCREENWIDTH];
;    static short	cliptop[MAXSCREENWIDTH];
;    int			x;
;    int			r1;
;    int			r2;
;    fixed_t		scale;
;    fixed_t		lowscale;
;    int			silhouette;
;
;    for (x = spr->x1 ; x<=spr->x2 ; x++)
;	clipbot[x] = cliptop[x] = -2;
;
;    // Scan drawsegs from end to start for obscuring segs.
;    // The first drawseg that has a greater scale
;    //  is the clip seg.
;    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
;    {
;	// determine if the drawseg obscures the sprite
;	if (ds->x1 > spr->x2
;	    || ds->x2 < spr->x1
;	    || (!ds->silhouette
;		&& !ds->maskedtexturecol) )
;	{
;	    // does not cover sprite
;	    continue;
;	}
;			
;	r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
;	r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;
;
;	if (ds->scale1 > ds->scale2)
;	{
;	    lowscale = ds->scale2;
;	    scale = ds->scale1;
;	}
;	else
;	{
;	    lowscale = ds->scale1;
;	    scale = ds->scale2;
;	}
;		
;	if (scale < spr->scale
;	    || ( lowscale < spr->scale
;		 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
;	{
;	    // masked mid texture?
;	    if (ds->maskedtexturecol)	
;		R_RenderMaskedSegRange (ds, r1, r2);
;	    // seg is behind sprite
;	    continue;			
;	}
;
;	
;	// clip this piece of the sprite
;	silhouette = ds->silhouette;
;	
;	if (spr->gz >= ds->bsilheight)
;	    silhouette &= ~SIL_BOTTOM;
;
;	if (spr->gzt <= ds->tsilheight)
;	    silhouette &= ~SIL_TOP;
;			
;	if (silhouette == 1)
;	{
;	    // bottom sil
;	    for (x=r1 ; x<=r2 ; x++)
;		if (clipbot[x] == -2)
;		    clipbot[x] = ds->sprbottomclip[x];
;	}
;	else if (silhouette == 2)
;	{
;	    // top sil
;	    for (x=r1 ; x<=r2 ; x++)
;		if (cliptop[x] == -2)
;		    cliptop[x] = ds->sprtopclip[x];
;	}
;	else if (silhouette == 3)
;	{
;	    // both
;	    for (x=r1 ; x<=r2 ; x++)
;	    {
;		if (clipbot[x] == -2)
;		    clipbot[x] = ds->sprbottomclip[x];
;		if (cliptop[x] == -2)
;		    cliptop[x] = ds->sprtopclip[x];
;	    }
;	}
;		
;    }
;  
;    // all clipping has been performed, so draw the sprite
;
;    // check for unclipped columns
;    for (x = spr->x1 ; x<=spr->x2 ; x++)
;    {
;	if (clipbot[x] == -2)		
;	    clipbot[x] = viewheight;
;
;	if (cliptop[x] == -2)
;	    cliptop[x] = -1;
;    }
;		
;    mfloorclip = clipbot;
;    mceilingclip = cliptop;
;    R_DrawVisSprite (spr, spr->x1, spr->x2);
;
;}

;-----------------------------------------------------------------------
; R_DrawVisSprite (in r_things.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_W_CacheLumpNum
		xref	_firstspritelump
		xref	_dc_colormap
		xref	_colfunc
		xref	_fuzzcolfunc
		xref	_transcolfunc
		xref	_dc_translation		;byte*
		xref	_translationtables	;byte*
		xref	_dc_iscale
		xref	_dc_texturemid
		xref	_spryscale
		xref	_sprtopscreen
		xref	_centeryfrac
		xref	_FixedMul
		xref	_dc_x
		xref	_R_DrawMaskedColumn
		xref	_basecolfunc
		xref	_detailshift

;humm... the prototype (and all calls) should be changed to:
; void R_DrawVisSprite(vissprite_t* vis);
; because int x1 and int x2 are UNUSED! They are always just
; copies of the sprite->x1 sprite->x2

		cnop	0,4
_R_DrawVisSprite:
_R_DrawVisSprite:

		movem.l	d2/d7/a2/a3,-(sp)

		move.l	a0,a2	;save vis
		move.l	vp_patch(a0),d0
		add.l	_firstspritelump(a4),d0
		moveq	#101,d1	;PU_CACHE
		jsr	(_W_CacheLumpNum)
		move.l	d0,a3	;store patch

		move.l	vp_colormap(a2),_dc_colormap(a4)
		beq.b	.rdvs_FuzzCol	;if (!dc_colormap)
		move.l	vp_mobjflags(a2),d0	;used below, too
		and.l	#$c000000,d0	;& MF_TRANSLATION
		beq.b	.rdvs_ColFuncOK	;not true... just go on

		move.l	_transcolfunc(a4),_colfunc(a4)
		swap	d0	;this way better scheduled for 040
		lsr.w	#2,d0
		move.l	_translationtables(a4),_dc_translation(a4)
		and.l	#$FFFF,d0
		sub.l	#256,d0
		add.l	d0,_dc_translation(a4)
		bra.b	.rdvs_ColFuncOK

.rdvs_FuzzCol:
		move.l	_fuzzcolfunc(a4),_colfunc(a4)
.rdvs_ColFuncOK:
		move.l	vp_xiscale(a2),d0
		beq.b	.rdvs_SetScale	;if NULL do not do shifts etc.
		bpl.b	.rdvs_absskip	;this is iabs
		neg.l	d0
.rdvs_absskip:
		move.l	_detailshift(a4),d1
		lsr.l	d1,d0
.rdvs_SetScale:
		move.l	d0,_dc_iscale(a4)

		move.l	vp_texturemid(a2),d0	;used below..
		move.l	d0,_dc_texturemid(a4)
		move.l	vp_scale(a2),d1		;used below
		move.l	d1,_spryscale(a4)
		move.l	_FixedMul(a4),a0
		jsr	(a0)
		move.l	_centeryfrac(a4),_sprtopscreen(a4)
		sub.l	d0,_sprtopscreen(a4)

		move.l	vp_startfrac(a2),d2	;d2: frac

		move.l	vp_x1(a2),_dc_x(a4)	;dc_x=vis->x1
		move.l	vp_x2(a2),d7
		sub.l	vp_x1(a2),d7	;for(..; dc_x<=vis->x2;..)
		blt.b	.rdvs_NoDraw
.rdvs_Loop:
		move.l	d2,d0	;texturecolumn=frac..
		swap	d0	;.. >>FRACBITS	(and then we need only WORD below)
		move.l	a3,a0	;column=patch+
		move.l	(8,a3,d0.w*4),d0	;patch->columnofs[texturecolumn]
		rol.w	#8,d0
		swap	d0
		rol.w	#8,d0	;SWAPLONG
		add.l	d0,a0
		jsr	(_R_DrawMaskedColumn)
		addq.l	#1,_dc_x(a4)	;dc_x++
		add.l	vp_xiscale(a2),d2	;frac += vis->xiscale
		dbf	d7,.rdvs_Loop
.rdvs_NoDraw:
		move.l	_basecolfunc(a4),_colfunc(a4)

		movem.l	(sp)+,d2/d7/a2/a3

		rts

;void
;R_DrawVisSprite
;( vissprite_t*		vis,
;  int			x1,
;  int			x2 )
;{
;    column_t*		column;
;    int			texturecolumn;
;    fixed_t		frac;
;    patch_t*		patch;
;	
;	
;    patch = W_CacheLumpNum (vis->patch+firstspritelump, PU_CACHE);
;
;    dc_colormap = vis->colormap;
;    
;    if (!dc_colormap)
;    {
;	colfunc = fuzzcolfunc;
;    }
;    else if (vis->mobjflags & MF_TRANSLATION)
;    {
;	colfunc = transcolfunc;
;	dc_translation = translationtables - 256 +
;	    ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
;    }
;	
;    dc_iscale = iabs(vis->xiscale)>>detailshift;
;    dc_texturemid = vis->texturemid;
;    frac = vis->startfrac;
;    spryscale = vis->scale;
;    sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);
;	
;    for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
;    {
;	texturecolumn = frac>>FRACBITS;
;	column = (column_t *) ((byte *)patch +
;			       SWAPLONG(patch->columnofs[texturecolumn]));
;
;	R_DrawMaskedColumn (column);
;    }
;
;    colfunc = basecolfunc;
;}

;-----------------------------------------------------------------------
; R_SortVisSprites (in r_things.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_vissprite_p	;vissprite_t*
		xref	_vissprites	;FAR vissprite_t vissprites[MAXVISSPRITES]
		xref	_vsprsortedhead	;vissprite_t

MAXINT		equ	$7FFFFFFF
MAXSPRITE	equ	128

		cnop	0,4
_R_SortVisSprites:
_R_SortVisSprites:
		movem.l	a2/a3/a5,-(sp)

		move.l	_vissprite_p(a4),d0
		beq.b	.rsv_Exit	;no sprites to sort

		move.l	d0,a1
		lea	_vsprsortedhead(a4),a0	;kept here
		move.l	a0,-vp_size+vp_next(a1)	;this connection is not yet made!

		move.l	vp_next(a0),a1
		move.l	vp_scale(a1),d0
		bra.b	.rsv_Loop
		cnop	0,4
.rsv_Next:
		move.l	a2,a1
		move.l	d1,d0
.rsv_Loop:
		move.l	vp_next(a1),a2
		move.l	vp_scale(a2),d1
		beq.b	.rsv_Exit	;vsprsortedhead has null scale
		cmp.l	d1,d0
		ble.b	.rsv_Next
		move.l	a0,a3
.rsv_GoDown:
		move.l	vp_next(a3),a3
		cmp.l	vp_scale(a3),d1
		bge.b	.rsv_GoDown
		move.l	vp_next(a2),a5
		move.l	a5,vp_next(a1)
		move.l	a1,vp_prev(a5)
		move.l	vp_prev(a3),a5
		move.l	a2,vp_next(a5)
		move.l	a2,vp_prev(a3)
		move.l	a5,vp_prev(a2)
		move.l	a3,vp_next(a2)
		bra.b	.rsv_Loop

.rsv_Exit:
		movem.l	(sp)+,a2/a3/a5
		rts

		cnop	0,4
_R_NewVisSprite:
_R_NewVisSprite:
		move.l	_vissprite_p(a4),d0
		beq.b	.rn_FirstTime
		cmp.l	#_vissprites+vp_size*MAXSPRITE,d0
		beq.b	.rn_OverFlow
		add.l	#vp_size,_vissprite_p(a4)

;Establish proper connections between nodes. The new sprite does
;not get pointer to next sprite because that is set next time we
;get here. The pointer for last sprite is set in SortVisSprites
		move.l	d0,a0
		move.l	a0,_vsprsortedhead(a4)
		sub.l	#vp_size,a0
		move.l	d0,vp_next(a0)
		move.l	a0,vp_size+vp_prev(a0)
		rts

.rn_OverFlow:
;;;		move.l	#overflow,d0
		lea	overflow,a0
		move.l	a0,d0
		rts
.rn_FirstTime:
		move.l	#_vissprites+vp_size,_vissprite_p(a4)
		move.l	#_vissprites,d0
		rts

		cnop	0,4
_R_ClearSprites:
_R_ClearSprites:
		move.l	#_vissprites,a0
		move.l	#0,_vissprite_p(a4)
		lea	_vsprsortedhead(a4),a1
		move.l	a0,vp_next(a1)
		move.l	a0,vp_prev(a1)
		move.l	a1,vp_prev(a0)
		rts


;-----------------------------------------------------------------------
		section	.data,data

			cnop	0,4
MAXSCREENWIDTH	equ	1600
clipbot:	ds.w	MAXSCREENWIDTH
cliptop:	ds.w	MAXSCREENWIDTH

overflow:	ds.b	vp_size

;-----------------------------------------------------------------------

		end

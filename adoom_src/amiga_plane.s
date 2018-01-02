		mc68020

;		xdef	@R_MakeSpans
		xdef	_R_MakeSpans

;		xdef	@R_DrawPlanes
		xdef	_R_DrawPlanes

;		xdef	@R_MapPlane
		xdef	_R_MapPlane

		section	text,code

		near	a4,-2

;-----------------------------------------------------------------------
; R_MakeSpans (in r_plane.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_spanstart
;;;		xref	@R_MapPlane
;void
;__asm R_MakeSpans
;( register __d2 int x,
;  register __d3 int t1,
;  register __d4 int b1,
;  register __d5 int t2,
;  register __d6 int b2 );

		cnop	0,4
@R_MakeSpans:
_R_MakeSpans:
;First comments are for the non-__asm version of func prototype

;		movem.l	d2-d7/a2/a3,-(sp)
		movem.l	d3-d7/a2/a3,-(sp)

;		move.l	36(sp),d4	;(rest of args come in stack) int b1
;		move.l	d0,d2		;int x
;		move.l	40(sp),d5	;int t2
;		move.l	d1,d3		;int t1
;		move.l	44(sp),d6	;int b2

;;; 		move.l	#_spanstart,a2
		movea.l	_spanstart(a4),a2
		move.l	d2,-(sp)
		subq.l	#1,(sp)	;x-1, for R_MapPlane (third argument, thus in stack)

		;D2=X D3=T1 D4=B1 D5=T2 D6=B2, (sp)=X-1

		;prepare while(t1 < t2 && t1<=b1)
		;Calculate just how many times the loop is done, so no need
		; to compare everytime
		move.l	d5,d7
		sub.l	d3,d7
		ble.b	.rmsl1_Done	;T1=>T2, therefore loop not done
		subq.l	#1,d7
		move.l	d4,d0
		sub.l	d3,d0
;If T1>B1 first loop is not done. The same rule also applies to
;second loop (first: t1<=b1 second: b1>=t1) so we skip both loops.
		bmi.b	.rmsl2_Done

		lea	(a2,d3.l*4),a3
		cmp.l	d0,d7	;Do the loop until the smaller delta is zero
		;smaller because of AND operation in while statement
		bmi.b	.rms_Loop1
		move.l	d0,d7	;t1<=b1 effective

.rms_Loop1_2:	;Another loop...
		move.l	(a3)+,d1
		move.l	d3,d0
		jsr	(@R_MapPlane)
		addq.l	#1,d3
		dbf	d7,.rms_Loop1_2
		bra.b	.rmsl2_Done	;..so we can quickly skip
		;loop #2. The loop was done with t1<=b1 and in the
		;end this is not true. Also loop 2 uses the same comparison
		;it is automatically false thus we can skip it.

		cnop	0,4
.rms_Loop1:
		move.l	(a3)+,d1
		move.l	d3,d0
		jsr	(@R_MapPlane)
		addq.l	#1,d3
		dbf	d7,.rms_Loop1

.rmsl1_Done:

		move.l	d4,d0
		sub.l	d3,d0
		bmi.b	.rmsl2_Done
		move.l	d4,d7
		sub.l	d6,d7
		ble.b	.rmsl2_Done
		subq.l	#1,d7

		lea	4(a2,d4.l*4),a3
		cmp.l	d0,d7
		bmi.b	.rms_Loop2
		move.l	d0,d7	;b1>=t1 effective

.rms_Loop2_2:
		move.l	-(a3),d1
		move.l	d4,d0
		jsr	(@R_MapPlane)
		subq.l	#1,d4
		dbf	d7,.rms_Loop2_2

		move.l	d3,d7
		sub.l	d5,d7
		bhi.b	.rms_DoL3

		bra.w	.rmsl4_Done

		cnop	0,4
.rms_Loop2:
		move.l	-(a3),d1
		move.l	d4,d0
		jsr	(@R_MapPlane)
		subq.l	#1,d4
		dbf	d7,.rms_Loop2

.rmsl2_Done:
	;The following copy loops (spanstart[??]=x;) are optimised
	;by assuming that the loop is executed several (more then four) times

		move.l	d3,d7
		sub.l	d5,d7
		ble.b	.rmsl3_Done
.rms_DoL3:
		subq.l	#1,d7
		move.l	d6,d0
		sub.l	d5,d0
		bmi.w	.rmsl4_Done

		lea	(a2,d5.l*4),a3
		cmp.l	d0,d7
		bmi.b	.rms_StartLoop3

		move.l	d2,(a3)+
		bclr	#0,d0
		beq.b	.rmsl32_Pass1
		move.l	d2,(a3)+
.rmsl32_Pass1:
		bclr	#1,d0
		beq.b	.rmsl32_Pass2
		move.l	d2,(a3)+
		move.l	d2,(a3)+
.rmsl32_Pass2:
		tst.l	d0	
		beq.b	.rmsl4_Done
.rms_Loop3_2:
		move.l	d2,(a3)+
		move.l	d2,(a3)+
		move.l	d2,(a3)+
		move.l	d2,(a3)+
		subq.l	#4,d0
		bgt.b	.rms_Loop3_2
		bra.b	.rmsl4_Done

		cnop	0,4

.rms_StartLoop3:
		move.l	d2,(a3)+	;always at least once

		bclr	#0,d7
		beq.b	.rmsl3_Pass1
		move.l	d2,(a3)+
.rmsl3_Pass1:
		bclr	#1,d7
		beq.b	.rmsl3_Pass2
		move.l	d2,(a3)+
		move.l	d2,(a3)+
.rmsl3_Pass2:
		tst.l	d7
		beq.b	.rmsl3_Done
.rms_Loop3:
		move.l	d2,(a3)+
		move.l	d2,(a3)+
		move.l	d2,(a3)+
		move.l	d2,(a3)+
		addq.l	#4,d5	;saves for addq per loop
		subq.l	#4,d7	;also saves four dbfs, though adds here...
		bgt.b	.rms_Loop3

.rmsl3_Done:

		move.l	d6,d0
		sub.l	d5,d0
		bmi.b	.rmsl4_Done
		move.l	d6,d7
		sub.l	d4,d7
		ble.b	.rmsl4_Done
		subq.l	#1,d7

		lea	4(a2,d6.l*4),a3
		cmp.l	d0,d7
		bmi.b	.rms_StartLoop4
		move.l	d0,d7

.rms_StartLoop4:
		move.l	d2,-(a3)	;always at least once

		bclr	#0,d7
		beq.b	.rmsl4_Pass1
		move.l	d2,-(a3)
.rmsl4_Pass1:
		bclr	#1,d7
		beq.b	.rmsl4_Pass2
		move.l	d2,-(a3)
		move.l	d2,-(a3)
.rmsl4_Pass2:
		tst.l	d7
		beq.b	.rmsl4_Done
.rms_Loop4:
		move.l	d2,-(a3)
		move.l	d2,-(a3)
		move.l	d2,-(a3)
		move.l	d2,-(a3)
		subq.l	#4,d7
		bgt.b	.rms_Loop4

.rmsl4_Done:
		addq.l	#4,sp
		movem.l	(sp)+,d3-d7/a2/a3
;		movem.l	(sp)+,d2-d7/a2/a3
		rts


;void R_MakeSpans(int x, int t1, int b1, int t2, int b2)
;{
;
;    while (t1 < t2 && t1<=b1)
;    {
;		R_MapPlane (t1,spanstart[t1],x-1);
;		t1++;
;    }
;    while (b1 > b2 && b1>=t1)
;    {
;		R_MapPlane (b1,spanstart[b1],x-1);
;		b1--;
;    }
;
;    while (t2 < t1 && t2<=b2)
;    {
;		spanstart[t2] = x;
;		t2++;
;    }
;    while (b2 > b1 && b2>=t2)
;    {
;		spanstart[b2] = x;
;		b2--;
;    }
;}

;-----------------------------------------------------------------------
; R_DrawPlanes (in r_plane.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

;		STRUCTURE	visplane,0
;
;		 LONG	height
;		 LONG	picnum
;		 LONG	lightlevel
;		 LONG	minx
;		 LONG	maxx
;		 PTR	top
;		 PTR	bottom
;
;		LABEL	visplane_size

height		equ	0		; fixed
picnum		equ	4		; int
lightlevel	equ	8		; int
minx		equ	12		; int
maxx		equ	16		; int
top		equ	20		; unsigned short* ([-1..SCREENWIDTH])
bottom		equ	24		; unsigned short* ([-1..SCREENWIDTH])
visplane_size	equ	28

		xref	_visplanes		; FAR visplane_t[]
		xref	_skyflatnum		; int
		xref	_flattranslation	; int*
		xref	_firstflat		; int
		xref	_viewz			; fixed_t
		xref	_planeheight		; fixed_t
		xref	_zlight	; FAR lighttable_t *[LIGHTLEVELS][MAXLIGHTZ]
		xref	_planezlight		; lighttable_t**
		xref	_extralight		; int
		xref	_lastvisplane		; visplane_t*
;;;		xref	_pspriteiscale		; fixed_t
		xref	_pspriteiscale2		; fixed_t
		xref	_detailshift		; int
		xref	_skytexturemid		; int
		xref	_viewangle		; angle_t
		xref	_skytexture		; int
		xref	_ds_source
		xref	_colormaps
		xref    _dc_colormap
		xref    _dc_iscale
		xref    _dc_texturemid
		xref	_xtoviewangle		; angle_t *
		xref    _dc_x
		xref    _dc_yl
		xref    _dc_yh
		xref    _dc_source
		xref	_colfunc

		xref	W_CacheLumpNum
		xref	_Z_ChangeTag2
		xref	R_GetColumn


		cnop	0,4
@R_DrawPlanes:
_R_DrawPlanes:
		movem.l	d2-d7/a2/a3/a5/a6,-(sp)

		move.l	#_visplanes,a2

.rd_Loop:
		move.l	minx(a2),d2
		move.l	maxx(a2),d3		;These ones are used later, too
		cmp.l	d2,d3
		bmi.w	.rd_Next		;minx>maxx -> next loop

;;;		move.l	a2,a3
;;;		add.l	d2,a3		; a3 -> pl + minx
;;;		move.l	a3,a5		; a5 -> pl + minx
;;;		add.l	#top,a3		; a3 -> pl + minx + top
;;;		add.l	#bottom-1,a5	; a5 -> pl + minx + bottom - 1

		movea.l	top(a2),a3
		lea	(a3,d2.l*2),a3	; a3 -> pl->top[minx]
		movea.l	bottom(a2),a5
		lea	-2(a5,d2.l*2),a5 ; a5 -> pl->bottom[minx-1]

		move.l	picnum(a2),d1	; used if not sky...
		cmp.l	_skyflatnum(a4),d1
		beq.w	.rdl_Sky

		move.l	_flattranslation(a4),a0
		move.l	_firstflat(a4),d0
		add.l	(a0,d1.l*4),d0	; D1 contains picnum, A0 array of ints
		moveq	#1,d1		; PU_STATIC
		jsr	(_W_CacheLumpNum)
		move.l	d0,_ds_source(a4)

		move.l	height(a2),d0
		sub.l	_viewz(a4),d0
		bpl.b	.rdl_HP		; These two lines are equal to
		neg.l	d0		; abs() or iabs(). Branch if =>0, otherwise switch sign
.rdl_HP:
		move.l	d0,_planeheight(a4)
		move.l	lightlevel(a2),d0
		lsr.l	#4,d0	;LIGHTSEGSHIFT
		move.l	#_zlight,_planezlight(a4)
		add.l	_extralight(a4),d0

		cmp.l	#0,d0
		bmi.b	.rdl_LightDone	; lightlevel 0, no need to change _planezlight anymore
		cmp.l	#16,d0		; 16=LIGHTLEVELS
		bmi.b	.rdl_LightOK
		add.l	#7680,_planezlight(a4)	;15<<9 ((15<<7)*4)
		bra.b	.rdl_LightDone
.rdl_LightOK:
		lsl.l	#8,d0
		add.l	d0,d0
		add.l	d0,_planezlight(a4)

.rdl_LightDone:
;;;		move.b	#$FF,top+1(a2,d3.l)	;D3=maxx, top+1 ->top[maxx+1]
;;;		move.b	#$FF,-(a3)
		movea.l	top(a2),a0
		move.w	#$FFFF,2(a0,d3.l*2)	;D3=maxx, top+1 ->top[maxx+1]
		move.w	#$FFFF,-(a3)

		;x=pl->minx=d2
		move.l	d3,d7
		sub.l	d2,d7
		addq.l	#1,d7	;d7 = x<=stop

		moveq	#0,d3
		moveq	#0,d4
		moveq	#0,d5
		moveq	#0,d6

.rdl_MSLoop:
;;;		move.b	(a3)+,d3	; bumps to top[x] at the same time!
;;;		move.b	(a3),d5
;;;		move.b	(a5)+,d4
;;;		move.b	(a5),d6
		move.w	(a3)+,d3	; bumps to top[x] at the same time!
		move.w	(a3),d5
		move.w	(a5)+,d4
		move.w	(a5),d6
		jsr	(_R_MakeSpans)	; passes d2/d3/d4/d5/d6
		addq.l	#1,d2
		dbf	d7,.rdl_MSLoop

		move.l	_ds_source(a4),a0
		moveq	#101,d0		; PU_STATIC
		jsr	(_Z_ChangeTag2)

.rd_Next:
		add.l	#visplane_size,a2
		cmp.l	_lastvisplane(a4),a2	; pl<lastvisplane
		bmi.w	.rd_Loop

		movem.l	(sp)+,d2-d7/a2/a3/a5/a6

		rts

		cnop	0,4
.rdl_Sky:
;;; 		move.l	_pspriteiscale(a4),d0
;;; 		move.l	_detailshift(a4),d1
;;; 		asr.l	d1,d0
		move.l	_pspriteiscale2(a4),d0
		move.l	_colormaps(a4),_dc_colormap(a4)
		move.l	d0,_dc_iscale(a4)
		move.l	_skytexturemid(a4),_dc_texturemid(a4)

		sub.l	d2,d3	; maxx-minx == maxx-x, how many till x>maxx
		moveq	#0,d5
		moveq	#0,d6
;;;		lea	_xtoviewangle(a4),a6
		movea.l	_xtoviewangle(a4),a6
		lea	(a6,d2.l*4),a6	; a6 -> xtoviewangle[minx]
		moveq	#22,d7		; ANGLETOSKYSHIFT
		move.l	d2,_dc_x(a4)	; dc_x = minx
;;;		addq.l	#1,a5
		addq.l	#2,a5		; a5 -> pl->bottom[minx]

.rdl_SkyLoop:
;;;		move.b	(a3)+,d5
;;;		move.b	(a5)+,d6
		move.w	(a3)+,d5	; dc_yl = pl->top[x]
		move.w	(a5)+,d6	; dc_yh = pl->bottom[x]
		move.l	d5,_dc_yl(a4)
		move.l	d6,_dc_yh(a4)

		move.l	(a6)+,d1	; To keep values consistent if not drawn

; the next 2 lines may be unnecessary --- but who knows?
		cmp.l	d5,d6
		bmi.b	.rdlsl_Next	; needless? I think so...

		add.l	_viewangle(a4),d1	; viewangle + xtoviewangle[x]
		move.l	_skytexture(a4),d0
		asr.l	d7,d1		; angle
		jsr	(_R_GetColumn)	; R_GetColumn(skytexture,angle)
		move.l	d0,_dc_source(a4)
		move.l	_colfunc(a4),a0
		jsr	(a0)

.rdlsl_Next:
		addq.l	#1,_dc_x(a4)	; dc_x = x
		dbf	d3,.rdl_SkyLoop
		bra.b	.rd_Next


; void R_DrawPlanes (void)
; {
;     visplane_t*		pl;
;     int			light;
;     int			x;
;     int			stop;
;     int			angle;
; 
;     for (pl = visplanes ; pl < lastvisplane ; pl++)
;     {
; 	if (pl->minx > pl->maxx)
; 	    continue;
; 
; 	
; 	// sky flat
; 	if (pl->picnum == skyflatnum)
; 	{
; 	    dc_iscale = pspriteiscale2/*>>detailshift*/;
; 	    
; 	    // Sky is allways drawn full bright,
; 	    //  i.e. colormaps[0] is used.
; 	    // Because of this hack, sky is not affected
; 	    //  by INVUL inverse mapping.
; 	    dc_colormap = colormaps;
; 	    dc_texturemid = skytexturemid;
; 	    for (x=pl->minx ; x <= pl->maxx ; x++)
; 	    {
; 			dc_yl = pl->top[x];
; 			dc_yh = pl->bottom[x];
; 
; 			if (dc_yl <= dc_yh)
; 			{
; 			    angle = (viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
; 			    dc_x = x;
; 			    dc_source = R_GetColumn(skytexture, angle);
; 			    colfunc ();
; 			}
; 	    }
; 	    continue;
; 	}
; 	
; 	// regular flat
; 	ds_source = W_CacheLumpNum(firstflat +
; 				   flattranslation[pl->picnum],
; 				   PU_STATIC);
; 	
; 	planeheight = iabs(pl->height-viewz);
; 	light = (pl->lightlevel >> LIGHTSEGSHIFT)+extralight;
; 
; 	if (light >= LIGHTLEVELS)
; 	    light = LIGHTLEVELS-1;
; 
; 	if (light < 0)
; 	    light = 0;
; 
; 	planezlight = zlight[light];
; 
;	//pl->top[pl->maxx+1] = 0xff;
;	//pl->top[pl->minx-1] = 0xff;
;	pl->top[pl->maxx+1] = 0xffff;
;	pl->top[pl->minx-1] = 0xffff;
; 		
; 	stop = pl->maxx + 1;
; 
; 	for (x=pl->minx ; x<= stop ; x++)
; 	{
; 	    R_MakeSpans(x,pl->top[x-1],
; 			pl->bottom[x-1],
; 			pl->top[x],
; 			pl->bottom[x]);
; 	}
; 	
; 	Z_ChangeTag (ds_source, PU_CACHE);
;     }
; }

;-----------------------------------------------------------------------
; R_MapPlane (in r_plane.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_cachedheight	;fixed_t*
		xref	_cacheddistance	;fixed_t*
		xref	_yslope		;fixed_t*
		xref	_cachedxstep	;fixed_t*
		xref	_cachedystep	;fixed_t*
		xref	_basexscale	;fixed_t
		xref	_baseyscale	;fixed_t
		xref	_distscale	;fixed_t*
		xref	_viewx		;fixed_t
		xref	_viewy		;fixed_t
		xref	_finecosine	;fixed_t*
		xref	_finesine	;FAR int []
		xref	_fixedcolormap	;lighttable_t*
		xref    _ds_xstep
		xref    _ds_ystep
		xref    _ds_x1
		xref    _ds_x2
		xref    _ds_y
		xref    _ds_xfrac
		xref    _ds_yfrac
		xref    _ds_colormap

		xref	_spanfunc
		xref	_FixedMul

MAXLIGHTZ	equ	128
LIGHTZSHIFT	equ	20
ANGLETOFINESHIFT	equ	19

;void
;R_MapPlane
;( int		y,
;  int		x1,
;  int		x2 )

		cnop	0,4
@R_MapPlane:
_R_MapPlane:
		movem.l	d3/d4/d7,-(sp)

		move.l	d0,d3		;Y
		move.l	d1,d4		;X1

		move.l	_FixedMul(a4),a1	;Prepare function. amiga_fixed.s does not use A1!
		move.l	_planeheight(a4),d0	;ready for cache...
;;;		move.l	#_cachedheight,a0
		movea.l	_cachedheight(a4),a0
		cmp.l	(a0,d3.l*4),d0
		beq.b	.rm_If2				;ah, we are cached!

		move.l	d0,(a0,d3.l*4)
;;;		move.l	#_yslope,a0
		movea.l	_yslope(a4),a0
		move.l	(a0,d3.l*4),d1
		jsr	(a1)
		move.l	d0,d7			;save distance
;;;		move.l	#_cacheddistance,a0
		movea.l	_cacheddistance(a4),a0
		move.l	d0,(a0,d3.l*4)
		move.l	_basexscale(a4),d1
		jsr	(a1)
		move.l	d0,_ds_xstep(a4)
;;;		move.l	#_cachedxstep,a0
		movea.l	_cachedxstep(a4),a0
		move.l	d0,(a0,d3.l*4)
		move.l	_baseyscale(a4),d1
		move.l	d7,d0
		jsr	(a1)
		move.l	d0,_ds_ystep(a4)
;;;		move.l	#_cachedystep,a0
		movea.l	_cachedystep(a4),a0
		move.l	d0,(a0,d3.l*4)
		bra.b	.rm_1Done

.rm_If2:
;;; 		move.l	#_cacheddistance,a0
		movea.l	_cacheddistance(a4),a0
		move.l	(a0,d3.l*4),d7
;;;		move.l	#_cachedxstep,a0
		movea.l	_cachedxstep(a4),a0
		move.l	(a0,d3.l*4),_ds_xstep(a4)
;;;		move.l	#_cachedystep,a0
		movea.l	_cachedystep(a4),a0
		move.l	(a0,d3.l*4),_ds_ystep(a4)

.rm_1Done:
		move.l	d3,_ds_y(a4)
		move.l	d4,_ds_x1(a4)

		move.l	d7,d0
;;;		move.l	#_distscale,a0
		movea.l	_distscale(a4),a0
		move.l	(a0,d4.l*4),d1
		jsr	(a1)
		move.l	d0,d3	;Y not needed anymore

;;;		lea	_xtoviewangle(a4),a0
		movea.l	_xtoviewangle(a4),a0
		move.l	(a0,d4.l*4),d4		;x1 not needed anymore
		add.l	_viewangle(a4),d4
		moveq	#ANGLETOFINESHIFT,d1
		lsr.l	d1,d4
		move.l	_finecosine(a4),a0
		move.l	(a0,d4.l*4),d0
		move.l	d3,d1
		jsr	(a1)
		move.l	_viewx(a4),_ds_xfrac(a4)
		add.l	d0,_ds_xfrac(a4)
		move.l	#_finesine,a0
		move.l	(a0,d4.l*4),d0
		move.l	d3,d1
		jsr	(a1)
		move.l	_viewy(a4),d1
		neg.l	d1
		move.l	d1,_ds_yfrac(a4)
		sub.l	d0,_ds_yfrac(a4)

		move.l	_fixedcolormap(a4),d0
		bne.b	.rm_FixedCMAP

		moveq	#LIGHTZSHIFT,d0
		lsr.l	d0,d7
		cmp.l	#MAXLIGHTZ,d7
		bmi.b	.rm_D7OK
		move.l	#MAXLIGHTZ-1,d7
.rm_D7OK:
		move.l	_planezlight(a4),a0
		move.l	(a0,d7.l*4),_ds_colormap(a4)

.rm_CMAPDone:
		move.l	16(sp),_ds_x2(a4)

		move.l	_spanfunc(a4),a0
		jsr	(a0)

		movem.l	(sp)+,d3/d4/d7

		rts
		cnop	0,4
.rm_FixedCMAP:
		move.l	d0,_ds_colormap(a4)
		bra.b	.rm_CMAPDone

;void
;R_MapPlane
;( int		y,
;  int		x1,
;  int		x2 )
;{
;    angle_t	angle;
;    fixed_t	distance;
;    fixed_t	length;
;    unsigned	index;
;
;    if (planeheight != cachedheight[y])
;    {
;	cachedheight[y] = planeheight;
;	distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
;	ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
;	ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
;    }
;    else
;    {
;	distance = cacheddistance[y];
;	ds_xstep = cachedxstep[y];
;	ds_ystep = cachedystep[y];
;    }
;
;    length = FixedMul (distance,distscale[x1]);
;    angle = (viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
;    ds_xfrac = viewx + FixedMul(finecosine[angle], length);
;    ds_yfrac = -viewy - FixedMul(finesine[angle], length);
;
;    if (fixedcolormap)
;	ds_colormap = fixedcolormap;
;    else
;    {
;	index = distance >> LIGHTZSHIFT;
;	
;	if (index >= MAXLIGHTZ )
;	    index = MAXLIGHTZ-1;
;
;	ds_colormap = planezlight[index];
;    }
;	
;    ds_y = y;
;    ds_x1 = x1;
;    ds_x2 = x2;
;
;    // high or low detail
;    spanfunc ();	
;}

;-----------------------------------------------------------------------

		end

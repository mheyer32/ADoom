*
*       amiga_draw.s - optimized rendering
*       by Aki Laukkanen <amlaukka@cc.helsinki.fi>
*
*       This file is public domain.
*

		mc68030

		include "funcdef.i"
		include "exec/types.i"
		include "exec/exec_lib.i"

;-----------------------------------------------------------------------


SCREENWIDTH	equ	320

FRACBITS	equ	16
FRACUNIT	equ	(1<<FRACBITS)

*
*       global functions
*

		xdef	_init_r_draw
		xdef	@init_r_draw

;;		xdef    _R_DrawColumn_030			; high detail
;;		xdef    @R_DrawColumn_030
		xdef    _R_DrawColumn_040			; high detail
		xdef    @R_DrawColumn_040
		xdef    _R_DrawSpan_040
		xdef    @R_DrawSpan_040
		xdef    _R_DrawColumn_060
		xdef    @R_DrawColumn_060
		xdef    _R_DrawSpan_060
		xdef    @R_DrawSpan_060
		xdef	_R_DrawFuzzColumn
		xdef	@R_DrawFuzzColumn
;;		xdef	_R_DrawTranslatedColumn
;;		xdef	@R_DrawTranslatedColumn

		xdef	_R_DrawSpanLow				; low detail
		xdef	@R_DrawSpanLow
		xdef	_R_DrawColumnLow
		xdef	@R_DrawColumnLow
		xdef	_R_DrawFuzzColumnLow
		xdef	@R_DrawFuzzColumnLow
;;		xdef	_R_DrawTranslatedColumnLow
;;		xdef	@R_DrawTranslatedColumnLow

		xdef	_R_DrawMaskedColumn
		xdef	@R_DrawMaskedColumn

		xdef	_V_DrawPatch
		xdef	@V_DrawPatch
		xdef	_V_DrawPatchDirect
		xdef	@V_DrawPatchDirect

*
*       needed symbols/labels
*

		xref	_SysBase
		xref	_SCREENWIDTH
		xref	_SCREENHEIGHT
		xref    _dc_yl
		xref    _dc_yh
		xref    _dc_x
		xref    _columnofs
;;		xref    _ylookup
		xref    _ylookup2
		xref    _dc_iscale
		xref    _centery
		xref    _dc_texturemid
		xref    _dc_source
		xref    _dc_colormap
		xref    _ds_xfrac
		xref    _ds_yfrac
		xref    _ds_x1
		xref    _ds_y
		xref    _ds_x2
		xref    _ds_xstep
		xref    _ds_ystep
		xref    _ds_source
		xref    _ds_colormap
		xref    _fuzzoffset
		xref	_fuzzpos
		xref	_viewheight
		xref    _dc_translation
		xref	_colormaps

;-----------------------------------------------------------------------
		section	.text,code

		near	a4,-2

; low detail drawing functions

;-----------------------------------------------------------------------
; patch _SCREENWIDTH into draw routines (self-modifying code) and flush caches

_init_r_draw
@init_r_draw
		movem.l	a6,-(sp)

		lea	@init_r_draw(pc),a0

		move.l	_SCREENWIDTH(a4),d0	; d0 = _SCREENWIDTH

		move.w	d0,sw12_1-@init_r_draw+2(a0)
		move.w	d0,sw12_3-@init_r_draw+2(a0)
		move.w	d0,sw12_4-@init_r_draw+2(a0)
		move.w	d0,sw12_4a-@init_r_draw+2(a0)
		move.w	d0,sw12_5-@init_r_draw+2(a0)
		move.w	d0,sw12_5a-@init_r_draw+2(a0)
		move.w	d0,sw12_6-@init_r_draw+2(a0)
		move.w	d0,sw12_7-@init_r_draw+2(a0)
		move.w	d0,sw12_8-@init_r_draw+2(a0)

		add.l	d0,d0			; d0 = 2 * _SCREENWIDTH

		move.w	d0,sw22_1-@init_r_draw+2(a0)
		move.w	d0,sw22_3-@init_r_draw+2(a0)
		move.w	d0,sw22_4-@init_r_draw+2(a0)
		move.w	d0,sw22_4a-@init_r_draw+2(a0)
		move.w	d0,sw22_5-@init_r_draw+2(a0)
		move.w	d0,sw22_5a-@init_r_draw+2(a0)
		move.w	d0,sw22_6-@init_r_draw+2(a0)
		move.w	d0,sw22_7-@init_r_draw+2(a0)
		move.w	d0,sw22_8-@init_r_draw+2(a0)

		add.l	_SCREENWIDTH(a4),d0	; d0 = 3 * _SCREENWIDTH

		move.w	d0,sw32_1-@init_r_draw+2(a0)
		move.w	d0,sw32_3-@init_r_draw+2(a0)
		move.w	d0,sw32_4-@init_r_draw+2(a0)
		move.w	d0,sw32_4a-@init_r_draw+2(a0)
		move.w	d0,sw32_5-@init_r_draw+2(a0)
		move.w	d0,sw32_5a-@init_r_draw+2(a0)
		move.w	d0,sw32_6-@init_r_draw+2(a0)
		move.w	d0,sw32_7-@init_r_draw+2(a0)
		move.w	d0,sw32_8-@init_r_draw+2(a0)

		add.l	_SCREENWIDTH(a4),d0	; d0 = 4 * _SCREENWIDTH

		move.w	d0,sw42_1-@init_r_draw+2(a0)
		move.w	d0,sw42_3-@init_r_draw+2(a0)
		move.w	d0,sw42_4-@init_r_draw+2(a0)
		move.w	d0,sw42_5-@init_r_draw+2(a0)
		move.w	d0,sw42_6-@init_r_draw+2(a0)
		move.w	d0,sw42_8-@init_r_draw+2(a0)

		move.l	_SCREENWIDTH(a4),d0
		neg.l	d0			; d0 = -_SCREENWIDTH

		move.w	d0,swm10_1-@init_r_draw(a0)
		move.w	d0,swm10_3-@init_r_draw(a0)
		move.w	d0,swm10_4-@init_r_draw(a0)
		move.w	d0,swm10_5-@init_r_draw(a0)
		move.w	d0,swm10_6-@init_r_draw(a0)
		move.w	d0,swm10_8-@init_r_draw(a0)

		add.l	d0,d0			; d0 = -2*_SCREENWIDTH

		move.w	d0,swm20_1-@init_r_draw(a0)
		move.w	d0,swm20_3-@init_r_draw(a0)
		move.w	d0,swm20_4-@init_r_draw(a0)
		move.w	d0,swm20_5-@init_r_draw(a0)
		move.w	d0,swm20_6-@init_r_draw(a0)
		move.w	d0,swm20_8-@init_r_draw(a0)

		sub.l	_SCREENWIDTH(a4),d0	; d0 = -3*_SCREENWIDTH

		move.w	d0,swm30_1-@init_r_draw(a0)
		move.w	d0,swm30_3-@init_r_draw(a0)
		move.w	d0,swm30_4-@init_r_draw(a0)
		move.w	d0,swm30_5-@init_r_draw(a0)
		move.w	d0,swm30_6-@init_r_draw(a0)
		move.w	d0,swm30_8-@init_r_draw(a0)

		movea.l	_SysBase(a4),a6
		jsr	_LVOCacheClearU(a6)

		movem.l	(sp)+,a6
		rts

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawColumnLow
@R_DrawColumnLow
		movem.l d3-d4/d6-d7/a2/a3,-(sp)

		move.l  _dc_yh(a4),d7	; count = _dc_yh - _dc_yl
		move.l  _dc_yl(a4),d0
		sub.l   d0,d7
		bmi.w   l.end1

		move.l  _dc_x(a4),d1    ; dest = ylookup[_dc_yl] + columnofs[_dc_x]
		movea.l	_ylookup2(a4),a0 ;     = ylookup2[_dc_yl] + (_dc_x<<1)
		add.l	d1,d1		; dc_x <<= 1 
		move.l  (a0,d0.l*4),a0
		adda.l	d1,a0

		move.l  _dc_colormap(a4),d4
		move.l  _dc_source(a4),a1

		move.l  _dc_iscale(a4),d1 ; frac = _dc_texturemid + (_dc_yl-centery)*fracstep
		sub.l   _centery(a4),d0
		muls.l  d1,d0
		add.l   _dc_texturemid(a4),d0

		moveq   #$7f,d3
sw42_1		lea     (SCREENWIDTH*4).w,a3

; d7: cnt >> 2
; a0: chunky
; a1: texture
; d0: frac  (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: dfrac (.......................................)
; d3: $7f
; d4: light table aligned to 256 byte boundary
; a3: SCREENWIDTH

		move.l  d7,d6
		and.w   #3,d6

		swap    d0              ; swap decimals and fraction
		swap    d1

		add.w   l.width_tab1(pc,d6.w*2),a0
		lsr.w   #2,d7
		move.w  l.tmap_tab1(pc,d6.w*2),d6

		and.w   d3,d0
		sub.w   d1,d0
		add.l   d1,d0           ; setup the X flag

		jmp 	l.loop1(pc,d6.w)

		cnop    0,4
l.width_tab1
swm30_1		dc.w    -3*SCREENWIDTH
swm20_1		dc.w    -2*SCREENWIDTH
swm10_1		dc.w    -1*SCREENWIDTH
		dc.w    0
l.tmap_tab1
		dc.w    l.01-l.loop1
		dc.w    l.11-l.loop1
		dc.w    l.21-l.loop1
		dc.w    l.31-l.loop1
l.loop1
l.31
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		move.w  (a2),d6
		and.w   d3,d0
		move.b	(a2),d6
		move.w	d6,(a0)
l.21
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		move.w  (a2),d6
		and.w   d3,d0
		move.b	(a2),d6
sw12_1		move.w	d6,SCREENWIDTH(a0)
l.11
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		move.w  (a2),d6
		and.w   d3,d0
		move.b	(a2),d6
sw22_1		move.w	d6,SCREENWIDTH*2(a0)
l.01
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		move.w	(a2),d6
		and.w   d3,d0
		move.b  (a2),d6
sw32_1		move.w	d6,SCREENWIDTH*3(a0)

		add.l   a3,a0
l.loop_end1
		dbf 	d7,l.loop1
l.end1
		movem.l (sp)+,d3-d4/d6-d7/a2/a3
		rts

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawSpanLow
@R_DrawSpanLow
		movem.l d2-d7/a2-a4,-(sp)
		move.l  _ds_y(a4),d0
		move.l  _ds_x1(a4),d1	; dest = ylookup[_ds_y] + columnofs[_ds_x1]
		movea.l	_ylookup2(a4),a0 ;     = ylookup2[_ds_y] + _ds_x
		add.l	d1,d1
		move.l  (a0,d0.l*4),a0
		adda.l	d1,a0
		move.l  _ds_x2(a4),d7	; count = _ds_x2 - _ds_x1
		move.l  _ds_source(a4),a1
		add.l	d7,d7
		move.l  _ds_colormap(a4),a2
		sub.l   d1,d7
		addq.l	#2,d7
		move.l  _ds_xfrac(a4),d0
		move.l  _ds_yfrac(a4),d1
		move.l  _ds_xstep(a4),d2
		move.l  _ds_ystep(a4),d3
		move.l  a0,d4		; notice, that this address must already be aligned by word
		btst    #1,d4
		beq.b   l.skips2
		move.l  d0,d5           ; do the unaligned pixels
		move.l  d1,d6           ; so we can write to longword
		swap    d5              ; boundary in the main loop
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6		; this is the worst possible
		lsl.w   #6,d6		; way but hey, this is not a loop
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		add.l   d2,d0
		move.b  (a2,d5.w),(a0)+
		add.l   d3,d1
		move.b	(a2,d5.w),(a0)+	; I know this is crap but spare me the comments
		subq.l  #2,d7
l.skips2	move.l  a2,d4
		lea     $1000(a1),a1	; catch 22
		move.l  a0,a3
		add.l   d7,a3
		move.l  d7,d5
		and.b   #~7,d5
		move.l  a0,a4
		add.l   d5,a4
		eor.w   d0,d1           ; swap fraction parts for addx
		eor.w   d2,d3
		eor.w   d1,d0
		eor.w   d3,d2
		eor.w   d0,d1
		eor.w   d2,d3
		swap    d0
		swap    d1
		swap    d2
		swap    d3
		lsl.w   #6,d1
		lsl.w   #6,d3
		move.w  #$ffc0,d6
		move.w  #$f03f,d7
		lsr.w   #3,d5
		beq.b   l.skip_loop22
		sub.w   d2,d0
		add.l   d2,d0           ; setup the X flag
l.loop22	or.w    d6,d0		; Not really and exercise in optimizing
		or.w    d7,d1		; but I guess it's faster than 1x1 for 030
		and.w   d1,d0		; where this low detail business is needed.
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.w  (a2),d5
		or.w    d6,d0
		move.b	(a2),d5
		or.w    d7,d1
		and.w   d1,d0
		swap	d5
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.w  (a2),d5
		or.w    d6,d0
		move.b	(a2),d5
		or.w    d7,d1
		and.w   d1,d0
		move.l	d5,(a0)+
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.w  (a2),d5
		or.w    d6,d0
		move.b	(a2),d5
		or.w    d7,d1
		and.w   d1,d0
		swap	d5
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.w  (a2),d5
		move.b	(a2),d5
		move.l  d5,(a0)+
		cmp.l   a0,a4
		bne.b   l.loop22
l.skip_loop22
		sub.w   d2,d0
		add.l   d2,d0

		bra.b   l.loop_end22
l.loop32  	or.w    d6,d0
		or.w    d7,d1
		and.w   d1,d0
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.b  (a2),(a0)+
		move.b	(a2),(a0)+
l.loop_end22
		cmp.l   a0,a3
		bne.b   l.loop32
l.end22		movem.l (sp)+,d2-d7/a2-a4
		rts

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawTranslatedColumnLow
@R_DrawTranslatedColumnLow
		movem.l d2-d4/d6-d7/a2/a3,-(sp)

		move.l  _dc_yh(a4),d7	; count = _dc_yh - _dc_yl
		move.l  _dc_yl(a4),d0
		sub.l   d0,d7
		bmi.w   l.end3

		move.l  _dc_x(a4),d1	; dest = ylookup[_dc_yl] + columnofs[_dc_x]
		movea.l	_ylookup2(a4),a0 ;     = ylookup2[_dc_yl] + _dc_x
		add.l	d1,d1
		move.l  (a0,d0.l*4),a0
;;		movea.l	_columnofs(a4),a1
;;		add.l   (a1,d1.l*4),a0
		adda.l	d1,a0			; new

		move.l	_dc_translation(a4),d2
		move.l  _dc_colormap(a4),d4
		move.l  _dc_source(a4),a1

		move.l  _dc_iscale(a4),d1 ; frac = _dc_texturemid + (_dc_yl-centery)*fracstep
		sub.l   _centery(a4),d0
		muls.l  d1,d0
		add.l   _dc_texturemid(a4),d0

		moveq   #$7f,d3
sw42_3		lea     (SCREENWIDTH*4).w,a3

; d7: cnt >> 2
; a0: chunky
; a1: texture
; d0: frac  (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: dfrac (.......................................)
; d3: $7f
; d4: light table aligned to 256 byte boundary
; d2: translation table aligned to 256 byte boundary
; a3: SCREENWIDTH

		move.l  d7,d6
		and.w   #3,d6

		swap    d0              ; swap decimals and fraction
		swap    d1

		add.w   l.width_tab3(pc,d6.w*2),a0
		lsr.w   #2,d7
		move.w  l.tmap_tab3(pc,d6.w*2),d6

		and.w   d3,d0
		sub.w   d1,d0
		add.l   d1,d0           ; setup the X flag

		jmp 	l.loop3(pc,d6.w)

		cnop    0,4
l.width_tab3
swm30_3		dc.w    -3*SCREENWIDTH
swm20_3		dc.w    -2*SCREENWIDTH
swm10_3		dc.w    -1*SCREENWIDTH
		dc.w    0
l.tmap_tab3
		dc.w    l.03-l.loop3
		dc.w    l.13-l.loop3
		dc.w    l.23-l.loop3
		dc.w    l.33-l.loop3
l.loop3
l.33
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		move.l  d4,a2
		and.w   d3,d0
		move.w	(a2),d6
		move.b  (a2),d6
		move.w	d6,(a0)
l.23
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		move.l  d4,a2
		and.w   d3,d0
		move.w	(a2),d6
		move.b  (a2),d6
sw12_3		move.w	d6,SCREENWIDTH(a0)
l.13
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		move.l  d4,a2
		and.w   d3,d0
		move.w	(a2),d6
		move.b	(a2),d6
sw22_3		move.w  d6,SCREENWIDTH*2(a0)
l.03
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		move.l  d4,a2
		and.w   d3,d0
		move.w	(a2),d6
		move.b	(a2),d6
sw32_3		move.b  d6,SCREENWIDTH*3(a0)

		add.l   a3,a0
l.loop_end3
		dbf 	d7,l.loop3
l.end3
		movem.l (sp)+,d2-d4/d6-d7/a2/a3
		rts

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawFuzzColumnLow
@R_DrawFuzzColumnLow
		movem.l d4/d6-d7/a2/a3,-(sp)

		move.l	_viewheight(a4),d1
		subq.l	#1,d1
		move.l  _dc_yh(a4),d7	; count = _dc_yh - _dc_yl
		cmp.l	d1,d7
		bne.b	l.skip_yh4
		subq.l	#1,d1
		move.l	d1,d7
l.skip_yh4
		move.l  _dc_yl(a4),d0
		bne.b	l.skip_yl4
		moveq	#1,d0
l.skip_yl4
		sub.l   d0,d7
		bmi.w   l.end4

		move.l  _dc_x(a4),d1	; dest = ylookup[_dc_yl] + columnofs[_dc_x]
		movea.l	_ylookup2(a4),a0 ;     = ylookup2[_dc_yl] + _dc_x
		add.l	d1,d1
		move.l  (a0,d0.l*4),a0
		adda.l	d1,a0

		move.l  _colormaps(a4),d4
		add.l	#6*256,d4

		movea.l	_fuzzoffset(a4),a1
		move.l	_fuzzpos(a4),d0	; bring it down 
l.pos_loop4	sub.l	_SCREENHEIGHT(a4),d0
		bpl	l.pos_loop4
		add.l	_SCREENHEIGHT(a4),d0
		add.l	d0,a1

sw42_4		lea     (SCREENWIDTH*4).w,a3

; d7: cnt >> 2
; a0: chunky
; a1: fuzzoffset
; d0: frac  (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: dfrac (.......................................)
; d3: $7f
; d4: light table aligned to 256 byte boundary
; a3: SCREENWIDTH

		move.l  d7,d6
		and.w   #3,d6

		add.w   l.width_tab4(pc,d6.w*2),a0
		lsr.w   #2,d7
		move.w  l.tmap_tab4(pc,d6.w*2),d6

		jmp 	l.loop4(pc,d6.w)

		cnop    0,4
l.width_tab4
swm30_4		dc.w    -3*SCREENWIDTH
swm20_4		dc.w    -2*SCREENWIDTH
swm10_4		dc.w    -1*SCREENWIDTH
		dc.w    0
l.tmap_tab4
		dc.w    l.04-l.loop4
		dc.w    l.14-l.loop4
		dc.w    l.24-l.loop4
		dc.w    l.34-l.loop4
l.loop4
l.34		move.l	a0,a2			; This is essentially
		add.l	(a1)+,a2		; just moving memory around.
		move.b	(a2),d4
		move.l	d4,a2			
		move.w	(a2),d6
		move.b	(a2),d6
		move.w	d6,(a0)		
l.24
sw12_4		lea	SCREENWIDTH(a0),a2	
		add.l	(a1)+,a2		
		move.b	(a2),d4			
		move.l	d4,a2
		move.w	(a2),d6
		move.b	(a2),d6
sw12_4a		move.w	d6,SCREENWIDTH(a0)
l.14
sw22_4		lea	2*SCREENWIDTH(a0),a2
		add.l	(a1)+,a2
		move.b	(a2),d4
		move.l	d4,a2
		move.w	(a2),d6
		move.b	(a2),d6
sw22_4a		move.w	d6,2*SCREENWIDTH(a0)
l.04
sw32_4		lea	3*SCREENWIDTH(a0),a2
		add.l	(a1)+,a2
		move.b	(a2),d4
		move.l	d4,a2
		move.w	(a2),d6
		move.b	(a2),d6
sw32_4a		move.w	d6,3*SCREENWIDTH(a0)

		add.l   a3,a0
l.loop_end4
		dbf	d7,l.loop4
		sub.l	_fuzzoffset(a4),a1
		move.l	a1,_fuzzpos
l.end4
		movem.l (sp)+,d4/d6-d7/a2/a3
		rts

;-----------------------------------------------------------------------
; high detail versions

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawFuzzColumn
@R_DrawFuzzColumn
		movem.l d4/d6-d7/a2/a3,-(sp)

		move.l	_viewheight(a4),d1
		subq.l	#1,d1
		move.l  _dc_yh(a4),d7	; count = _dc_yh - _dc_yl
		cmp.l	d1,d7
		bne.b	l.skip_yh5
		subq.l	#1,d1
		move.l	d1,d7
l.skip_yh5
		move.l  _dc_yl(a4),d0
		bne.b	l.skip_yl5
		moveq	#1,d0
l.skip_yl5
		sub.l   d0,d7
		bmi.w   l.end5

		movea.l	_ylookup2(a4),a0 ; dest = ylookup2[_dc_yl] + dc_x
		move.l  (a0,d0.l*4),a0
		adda.l	_dc_x(a4),a0

		move.l  _colormaps(a4),d4
		add.l	#6*256,d4

		movea.l	_fuzzoffset(a4),a1
		move.l	_fuzzpos(a4),d0
l.pos_loop5	sub.l	_SCREENHEIGHT(a4),d0
		bpl	l.pos_loop5
		add.l	_SCREENHEIGHT(a4),d0
		add.l	d0,a1

sw42_5		lea     (SCREENWIDTH*4).w,a3

; d7: cnt >> 2
; a0: chunky
; a1: fuzzoffset
; d0: frac  (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: dfrac (.......................................)
; d3: $7f
; d4: light table aligned to 256 byte boundary
; a3: SCREENWIDTH

		move.l  d7,d6
		and.w   #3,d6

		add.w   l.width_tab5(pc,d6.w*2),a0
		lsr.w   #2,d7
		move.w  l.tmap_tab5(pc,d6.w*2),d6

		jmp 	l.loop5(pc,d6.w)

		cnop    0,4
l.width_tab5
swm30_5		dc.w    -3*SCREENWIDTH
swm20_5		dc.w    -2*SCREENWIDTH
swm10_5		dc.w    -1*SCREENWIDTH
		dc.w    0
l.tmap_tab5
		dc.w    l.05-l.loop5
		dc.w    l.15-l.loop5
		dc.w    l.25-l.loop5
		dc.w    l.35-l.loop5
l.loop5
l.35		move.l	a0,a2			; This is essentially
		add.l	(a1)+,a2		; just moving memory around.
		move.b	(a2),d4
		move.l	d4,a2			; Not 060 optimized but
		move.b	(a2),(a0)		; if you have hordes of
l.25
sw12_5		lea	SCREENWIDTH(a0),a2	; invisible monsters which
		add.l	(a1)+,a2		; slow down the game too much,
		move.b	(a2),d4			; do tell me.
		move.l	d4,a2
sw12_5a		move.b	(a2),SCREENWIDTH(a0)
l.15
sw22_5		lea	2*SCREENWIDTH(a0),a2
		add.l	(a1)+,a2
		move.b	(a2),d4
		move.l	d4,a2
sw22_5a		move.b	(a2),2*SCREENWIDTH(a0)
l.05
sw32_5		lea	3*SCREENWIDTH(a0),a2
		add.l	(a1)+,a2
		move.b	(a2),d4
		move.l	d4,a2
sw32_5a		move.b	(a2),3*SCREENWIDTH(a0)

		add.l   a3,a0
l.loop_end5
		dbf	d7,l.loop5
		sub.l	_fuzzoffset(a4),a1
		move.l	a1,_fuzzpos
l.end5
		movem.l (sp)+,d4/d6-d7/a2/a3
		rts

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawTranslatedColumn					; no 060 version :(
@R_DrawTranslatedColumn
		movem.l d2-d4/d6-d7/a2/a3,-(sp)

		move.l  _dc_yh(a4),d7	; count = _dc_yh - _dc_yl
		move.l  _dc_yl(a4),d0
		sub.l   d0,d7
		bmi.w   l.end6

;;		move.l  _dc_x(a4),d1	; dest = ylookup[_dc_yl] + columnofs[_dc_x]
		movea.l	_ylookup2(a4),a0 ;     = ylookup2[_dc_yl] + _dc_x
		move.l  (a0,d0.l*4),a0
;;		movea.l	_columnofs(a4),a1
;;		add.l   (a1,d1.l*4),a0
		adda.l	_dc_x(a4),a0	; new

		move.l	_dc_translation(a4),d2
		move.l  _dc_colormap(a4),d4
		move.l  _dc_source(a4),a1

		move.l  _dc_iscale(a4),d1 ; frac = _dc_texturemid + (_dc_yl-centery)*fracstep
		sub.l   _centery(a4),d0
		muls.l  d1,d0
		add.l   _dc_texturemid(a4),d0

		moveq   #$7f,d3
sw42_6		lea     (SCREENWIDTH*4).w,a3

; d7: cnt >> 2
; a0: chunky
; a1: texture
; d0: frac  (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: dfrac (.......................................)
; d3: $7f
; d4: light table aligned to 256 byte boundary
; d2: translation table aligned to 256 byte boundary
; a3: SCREENWIDTH

		move.l  d7,d6
		and.w   #3,d6

		swap    d0              ; swap decimals and fraction
		swap    d1

		add.w   l.width_tab6(pc,d6.w*2),a0
		lsr.w   #2,d7
		move.w  l.tmap_tab6(pc,d6.w*2),d6

		and.w   d3,d0
		sub.w   d1,d0
		add.l   d1,d0           ; setup the X flag

		jmp 	l.loop6(pc,d6.w)

		cnop    0,4
l.width_tab6
swm30_6		dc.w    -3*SCREENWIDTH
swm20_6		dc.w    -2*SCREENWIDTH
swm10_6		dc.w    -1*SCREENWIDTH
		dc.w    0
l.tmap_tab6
		dc.w    l.06-l.loop6
		dc.w    l.16-l.loop6
		dc.w    l.26-l.loop6
		dc.w    l.36-l.loop6
l.loop6
l.36
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		and.w   d3,d0
		move.l  d4,a2
		move.b  (a2),(a0)
l.26
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		and.w   d3,d0
		move.l  d4,a2
sw12_6		move.b  (a2),SCREENWIDTH(a0)
l.16
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		and.w   d3,d0
		move.l  d4,a2
sw22_6		move.b  (a2),SCREENWIDTH*2(a0)
l.06
		move.b  (a1,d0.w),d2
		move.l	d2,a2
		addx.l  d1,d0
		move.b	(a2),d4
		and.w   d3,d0
		move.l  d4,a2
sw32_6		move.b  (a2),SCREENWIDTH*3(a0)

		add.l   a3,a0
l.loop_end6
		dbf 	d7,l.loop6
l.end6
		movem.l (sp)+,d2-d4/d6-d7/a2/a3
		rts

;-----------------------------------------------------------------------
		cnop	0,4

; routine from j.selck@flensburg.netsurf.de   (Aki's 040 routine is faster)

;_R_DrawColumn_030
;@R_DrawColumn_030
;		movem.l	d3-d7/a2-a5,-(sp)
;		move.l	_dc_yl(a4),d0
;		move.l	_dc_yh(a4),d7
;		sub.l	d0,d7
;		bmi.b	1$
;		move.l	_dc_x(a4),d1
;		movea.l	_columnofs(a4),a5
;		lea	(a5,d1.l*4),a1
;		movea.l	_ylookup(a4),a5
;		movea.l	(a5,d0.l*4),a2
;		adda.l	(a1),a2
;		move.l	_dc_iscale(a4),d6
;		sub.l	_centery(a4),d0
;		muls.l	d6,d0
;		move.l	_dc_texturemid(a4),d5
;		add.l	d0,d5
;		movea.l	_dc_source(a4),a3
;		movea.l	_dc_colormap(a4),a4
;		moveq	#127,d4
;		move.l	_SCREENWIDTH(a4),d3
;		moveq	#0,d1		; ensure high bits of d1 are clear
;		add.w	d6,d5		; frac += fracstep (also sets X flag)
;		swap	d5		; swap(frac)
;		swap	d6		; swap(fracstep)
;		and.w	d4,d5		; (frac>>16)&127
;2$		move.b	(a3,d5.w),d1	; dc_source[(frac>>FRACBITS)&127]
;		move.b	(a4,d1.w),(a2)	; *dest = dc_colormap[d1]
;		addx.l	d6,d5		; swap(frac += fracstep), use & set X
;		adda.l	d3,a2		; dest += SCREENWIDTH
;		and.w	d4,d5		; (frac>>16)&127
;		dbra	d7,2$
;1$		movem.l	(sp)+,d3-d7/a2-a5
;		rts

;-----------------------------------------------------------------------
		cnop	0,4

_R_DrawColumn_060
@R_DrawColumn_060
		movem.l d2-d3/d5-d7/a2/a3,-(sp)

		move.l  (_dc_yh,a4),d7     ; count = _dc_yh - _dc_yl
		move.l  (_dc_yl,a4),d0
		sub.l   d0,d7
		bmi.w   l.end7

		movea.l	(_ylookup2,a4),a0  ; dest = ylookup2[_dc_yl] + _dc_x
		moveq   #$7f,d3
		move.l  (a0,d0.l*4),a0
		move.l  d7,d6           ; cnt
		adda.l	(_dc_x,a4),a0
		and.w   #3,d6           ; cnt&3

		move.l  (_dc_colormap,a4),a2
		addq.w	#1,d6		; (cnt&3)-1
		move.l  (_dc_source,a4),a1

		move.l  (_dc_iscale,a4),d1 ; frac = _dc_texturemid + (_dc_yl-centery)*fracstep
		sub.l   (_centery,a4),d0
		muls.l  d1,d0
		add.l   (_dc_texturemid,a4),d0

		move.l  (_SCREENWIDTH,a4),a3

l.skip_loop7           ; Do the leftover iterations (1..4) in this loop
		move.l  d0,d5		; frac
		swap    d5
		and.l   d3,d5		; (frac>>16)&$7f
		move.b  (a1,d5.w),d5	; d5 = dc_source[(frac>>FRACBITS)&127]]
		add.l   d1,d0		; frac += fracstep
		move.b  (a2,d5.w),(a0)	; *dest = dc_colormap[d5]
		add.l   a3,a0		; dest += SCREENWIDTH
;;		dbra	d6,l.skip_loop7
		subq.w  #1,d6
		bgt.b   l.skip_loop7
; d7: cnt >> 2
; a0: chunky dest
; a1: dc_source[]
; a2: dc_colormap[]
; d0: frac         (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: 2*fracstep   (.......................................)
; d2: frac+fracstep(.......................................)
; d3: $7f
; a3: SCREENWIDTH
.skip7
		lsr.l   #2,d7	; cnt >> 2
		subq.l	#1,d7	; (cnt >> 2) - 1
		bmi.b	l.end7

		add.l   a3,a3	; 2 * SCREENWIDTH

		move.l  d0,d2	; frac
		add.l   a3,a3	; 4 * SCREENWIDTH
		add.l   d1,d2	; frac + fracstep
		add.l   d1,d1	; 2 * fracstep

		eor.w   d0,d2   ; swap the fraction part for addx
		eor.w   d2,d0   ; assuming 16.16 fixed point
		eor.w   d0,d2

		swap    d0      ; swap decimals and fraction
		swap    d1
		swap    d2

		moveq   #0,d5
		and.w   d3,d0
		and.w   d3,d2

; d0: (frac+fracstep low : (frac high)&$7f)
; d1: (2*fracstep low    : 2*fracstep high)
; d2: (frac low          : (frac+fracstep high)&$7f)

		sub.w   d1,d0
		add.l   d1,d0           ; setup the X flag

		move.b  (a1,d2.w),d5
l.loop7
		; This should be reasonably scheduled for
		; m68060. It should perform well on other processors
		; too. That AGU stall still bothers me though.

		move.b  (a1,d0.w),d6        ; stall + pOEP, allows sOEP 1(1/0) 1
		addx.l  d1,d2               ; pOEP only                 1(0/0) 1
		move.b  (a2,d5.l),d5        ; pOEP but allows sOEP      1(1/0) 1
		and.w   d3,d2               ; sOEP                      1(1/0) 0
		move.b  (a2,d6.l),d6        ; pOEP but allows sOEP      1(1/0) 1
sw12_7		move.b  d5,SCREENWIDTH(a0)  ; sOEP                      1(0/1) 0
		addx.l  d1,d0               ; pOEP only                 1(0/0) 1
		move.b  (a1,d2.w),d5        ; pOEP but allows sOEP      1(1/0) 1
		and.w   d3,d0               ; sOEP                      1(1/0) 0
		move.b  d6,(a0)             ; pOEP                      1(0/1) 1
						; = ~4 cycles/pixel
						; + cache misses

		; The vertical writes are the true timehog of the loop
		; because of the characteristics of the copyback cache
		; operation.

		; Better mark the chunky buffer as write through
		; with the MMU and have all the horizontal writes
		; be longs aligned to longword boundary.

		move.b  (a1,d0.w),d6        ; stall + pOEP, allows sOEP 1(1/0) 1
		addx.l  d1,d2               ; pOEP only                 1(0/0) 1
		move.b  (a2,d5.l),d5        ; pOEP but allows sOEP      1(1/0) 1
		and.w   d3,d2               ; sOEP                      1(1/0) 0
		move.b  (a2,d6.l),d6        ; pOEP but allows sOEP      1(1/0) 1
sw32_7		move.b  d5,SCREENWIDTH*3(a0) ;sOEP                      1(0/1) 0
		addx.l  d1,d0               ; pOEP only                 1(0/0) 1
		move.b  (a1,d2.w),d5        ; pOEP but allows sOEP      1(1/0) 1
		and.w   d3,d0               ; sOEP                      1(1/0) 0
sw22_7		move.b  d6,SCREENWIDTH*2(a0) ;pOEP                      1(0/1) 1

		add.l   a3,a0
l.loop_end7
		dbf     d7,l.loop7

		; it's faster to divide it to two lines on 060
		; and shouldn't be slower on 040.

l.end7
		movem.l (sp)+,d2-d3/d5-d7/a2/a3
		rts

;-----------------------------------------------------------------------
		cnop    0,4

; 040 version

_R_DrawColumn_040
@R_DrawColumn_040
		movem.l d3-d4/d6-d7/a2/a3,-(sp)

		move.l  _dc_yh(a4),d7     ; count = _dc_yh - _dc_yl
		move.l  _dc_yl(a4),d0
		sub.l   d0,d7
		bmi.w   l.end8

		movea.l	_ylookup2(a4),a0  ; dest = ylookup2[_dc_yl] + _dc_x
		move.l  (a0,d0.l*4),a0
		adda.l	_dc_x(a4),a0

		move.l  _dc_colormap(a4),d4
		move.l  _dc_source(a4),a1

		move.l  _dc_iscale(a4),d1 ; frac = _dc_texturemid + (_dc_yl-centery)*fracstep
		sub.l   _centery(a4),d0
		muls.l  d1,d0
		add.l   _dc_texturemid(a4),d0

		moveq   #$7f,d3
sw42_8		lea     (SCREENWIDTH*4).w,a3

; d7: cnt >> 2
; a0: chunky
; a1: texture
; d0: frac  (uuuu uuuu uuuu uuuu 0000 0000 0UUU UUUU)
; d1: dfrac (.......................................)
; d3: $7f
; d4: light table aligned to 256 byte boundary
; a3: SCREENWIDTH

		move.l  d7,d6
		and.w   #3,d6

		swap    d0              ; swap decimals and fraction
		swap    d1

		adda.w	l.width_tab8(pc,d6.w*2),a0
		lsr.w   #2,d7
		move.w  l.tmap_tab8(pc,d6.w*2),d6

		and.w   d3,d0
		sub.w   d1,d0
		add.l   d1,d0           ; setup the X flag

		jmp	l.loop8(pc,d6.w)

		cnop    0,4
l.width_tab8
swm30_8		dc.w    -3*SCREENWIDTH
swm20_8		dc.w    -2*SCREENWIDTH
swm10_8		dc.w    -1*SCREENWIDTH
		dc.w    0
l.tmap_tab8
		dc.w    l.08-l.loop8
		dc.w    l.18-l.loop8
		dc.w    l.28-l.loop8
		dc.w    l.38-l.loop8
l.loop8
l.38
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		and.w   d3,d0
		move.b  (a2),(a0)
l.28
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		and.w   d3,d0
sw12_8		move.b  (a2),SCREENWIDTH(a0)
l.18
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		and.w   d3,d0
sw22_8		move.b  (a2),SCREENWIDTH*2(a0)
l.08
		move.b  (a1,d0.w),d4
		addx.l  d1,d0
		move.l  d4,a2
		and.w   d3,d0
sw32_8		move.b  (a2),SCREENWIDTH*3(a0)

		adda.l	a3,a0
l.loop_end8
		dbf	d7,l.loop8
l.end8
		movem.l (sp)+,d3-d4/d6-d7/a2/a3
		rts

;-----------------------------------------------------------------------
; This faster version by Aki M Laukkanen <amlaukka@cc.helsinki.fi>

		cnop    0,4

_R_DrawSpan_060
@R_DrawSpan_060
		movem.l d2-d7/a2/a3,-(sp)
		move.l  (_ds_y),d0
		move.l  (_ds_x1),d1     ; dest = ylookup[_ds_y] + columnofs[_ds_x1]
		movea.l	(_ylookup2),a0	;      = ylookup2[_ds_y] + _ds_x1
		move.l  (a0,d0.l*4),a0
;;		movea.l	(_columnofs),a1
;;		add.l   (a1,d1.l*4),a0
		adda.l	d1,a0			; new
		move.l  (_ds_source),a1
		move.l  (_ds_colormap),a2
		move.l  (_ds_x2),d7     ; count = _ds_x2 - _ds_x1
		sub.l   d1,d7
		addq.l  #1,d7
		move.l  (_ds_xfrac),d0
		move.l  (_ds_yfrac),d1
		move.l  (_ds_xstep),d2
		move.l  (_ds_ystep),d3
		move.l  a0,d4
		btst    #0,d4
		beq.b   l.skipb9
		move.l  d0,d5           ; do the unaligned pixels
		move.l  d1,d6           ; so we can write to longword
		swap    d5              ; boundary in the main loop
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6
		lsl.w   #6,d6
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		add.l   d2,d0
		move.b  (a2,d5.w),(a0)+
		add.l   d3,d1
		move.l  a0,d4
		subq.l  #1,d7
l.skipb9	btst    #1,d4
		beq.b   l.skips9
		moveq   #2,d4
		cmp.l   d4,d7
		bls.b   l.skips9
		move.l  d0,d5           ; write two pixels
		move.l  d1,d6
		swap    d5
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6
		lsl.w   #6,d6
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		move.w  (a2,d5.w),d4
		add.l   d2,d0
		add.l   d3,d1
		move.l  d0,d5
		move.l  d1,d6
		swap    d5
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6
		lsl.w   #6,d6
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		move.b  (a2,d5.w),d4
		add.l   d2,d0
		move.w  d4,(a0)+
		add.l   d3,d1
		subq.l  #2,d7
l.skips9	move.l  d7,d6           ; setup registers
		and.w   #3,d6
		move.l  d6,a3
		eor.w   d0,d1           ; swap fraction parts for addx
		eor.w   d2,d3
		eor.w   d1,d0
		eor.w   d3,d2
		eor.w   d0,d1
		eor.w   d2,d3
		swap    d0
		swap    d1
		swap    d2
		swap    d3
		lsl.w   #6,d1
		lsl.w   #6,d3
		moveq   #0,d6
		moveq   #0,d5
		sub.l   #$f000,a1
		lsr.l   #2,d7
		beq.w   l.skip_loop29
		subq.l  #1,d7
		sub.w   d3,d1
		add.l   d3,d1           ; setup the X flag
		or.w    #$ffc0,d0
		or.w    #$f03f,d1
		move.w  d0,d6
		and.w   d1,d6
		bra.b   l.start_loop29
		cnop    0,8
l.loop29	or.w    #$ffc0,d0       ; pOEP
		or.w    #$f03f,d1       ; sOEP
		move.b  (a2,d5.l),d4    ; pOEP but allows sOEP
		move.w  d0,d6           ; sOEP
		and.w   d1,d6           ; pOEP
		move.l  d4,(a0)+        ; sOEP
l.start_loop29
		addx.l  d2,d0           ; pOEP only
		addx.l  d3,d1           ; pOEP only
		move.b  (a1,d6.l),d5    ; pOEP but allows sOEP
		or.w    #$ffc0,d0       ; sOEP
		or.w    #$f03f,d1       ; pOEP
		move.w  d0,d6           ; sOEP
		move.w  (a2,d5.l),d4    ; pOEP but allows sOEP
		and.w   d1,d6           ; sOEP
		addx.l  d2,d0           ; pOEP only
		addx.l  d3,d1           ; pOEP only
		move.b  (a1,d6.l),d5    ; pOEP but allows sOEP
		or.w    #$ffc0,d0       ; sOEP
		or.w    #$f03f,d1       ; pOEP
		move.w  d0,d6           ; sOEP
		move.b  (a2,d5.l),d4    ; pOEP but allows sOEP
		and.w   d1,d6           ; sOEP
		addx.l  d2,d0           ; pOEP only
		addx.l  d3,d1           ; pOEP only
		move.b  (a1,d6.l),d5    ; pOEP but allows sOEP
		or.w    #$ffc0,d0       ; sOEP
		or.w    #$f03f,d1       ; pOEP
		move.w  d0,d6           ; sOEP
		swap    d4              ; pOEP only
		move.w  (a2,d5.l),d4    ; pOEP but allows sOEP
		and.w   d1,d6           ; sOEP
		addx.l  d2,d0           ; pOEP only
		addx.l  d3,d1           ; pOEP only
		move.b  (a1,d6.l),d5    ; pOEP but allows sOEP
		dbf     d7,l.loop29     ; pOEP only = 7.75 cycles/pixel
		move.b  (a2,d5.l),d4
		move.l  d4,(a0)+
l.skip_loop29
		sub.w   d3,d1
		add.l   d3,d1
		move.l  a3,d7
		bra.b   l.loop_end29
l.loop39  	or.w    #$ffc0,d0
		or.w    #$f03f,d1
		move.w  d0,d6
		and.w   d1,d6
		addx.l  d2,d0
		addx.l  d3,d1
		move.b  (a1,d6.l),d5
		move.b  (a2,d5.l),(a0)+
l.loop_end29
		dbf     d7,l.loop39
l.end29   	movem.l (sp)+,d2-d7/a2/a3
		rts

		cnop    0,4

;-----------------------------------------------------------------------
; 030/040 version

_R_DrawSpan_040
@R_DrawSpan_040
		movem.l d2-d7/a2-a4,-(sp)
		move.l  _ds_y(a4),d0
		move.l  _ds_x1(a4),d1	; dest = ylookup[_ds_y] + columnofs[_ds_x1]
		movea.l	_ylookup2(a4),a0 ;     = ylookup2[_ds_y] + _ds_x1
		move.l  (a0,d0.l*4),a0
;;		movea.l	_columnofs(a4),a1
;;		add.l   (a1,d1.l*4),a0
		adda.l	d1,a0			; new
		move.l  _ds_source(a4),a1
		move.l  _ds_colormap(a4),a2
		move.l  _ds_x2(a4),d7	; count = _ds_x2 - _ds_x1
		sub.l   d1,d7
		addq.l  #1,d7
		move.l  _ds_xfrac(a4),d0
		move.l  _ds_yfrac(a4),d1
		move.l  _ds_xstep(a4),d2
		move.l  _ds_ystep(a4),d3
		move.l  a0,d4
		btst    #0,d4
		beq.b   l.skipb0
		move.l  d0,d5           ; do the unaligned pixels
		move.l  d1,d6           ; so we can write to longword
		swap    d5              ; boundary in the main loop
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6
		lsl.w   #6,d6
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		add.l   d2,d0
		move.b  (a2,d5.w),(a0)+
		add.l   d3,d1
		move.l  a0,d4
		subq.l  #1,d7
l.skipb0	btst    #1,d4
		beq.b   l.skips0
		moveq   #2,d4
		cmp.l   d4,d7
		bls.b   l.skips0
		move.l  d0,d5           ; write two pixels
		move.l  d1,d6
		swap    d5
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6
		lsl.w   #6,d6
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		move.w  (a2,d5.w),d4
		add.l   d2,d0
		add.l   d3,d1
		move.l  d0,d5
		move.l  d1,d6
		swap    d5
		swap    d6
		and.w   #$3f,d5
		and.w   #$3f,d6
		lsl.w   #6,d6
		or.w    d5,d6
		move.b  (a1,d6.w),d5
		move.b  (a2,d5.w),d4
		add.l   d2,d0
		move.w  d4,(a0)+
		add.l   d3,d1
		subq.l  #2,d7
l.skips0	move.l  a2,d4
		add.l   #$1000,a1       ; catch 22
		move.l  a0,a3
		add.l   d7,a3
		move.l  d7,d5
		and.b   #~3,d5
		move.l  a0,a4
		add.l   d5,a4
		eor.w   d0,d1           ; swap fraction parts for addx
		eor.w   d2,d3
		eor.w   d1,d0
		eor.w   d3,d2
		eor.w   d0,d1
		eor.w   d2,d3
		swap    d0
		swap    d1
		swap    d2
		swap    d3
		lsl.w   #6,d1
		lsl.w   #6,d3
		move.w  #$ffc0,d6
		move.w  #$f03f,d7
		lsr.w   #2,d5
		beq.b   l.skip_loop20
		sub.w   d2,d0
		add.l   d2,d0           ; setup the X flag
l.loop20	or.w    d6,d0
		or.w    d7,d1
		and.w   d1,d0
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.w  (a2),d5
		or.w    d6,d0
		or.w    d7,d1
		and.w   d1,d0
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.b  (a2),d5
		swap    d5
		or.w    d6,d0
		or.w    d7,d1
		and.w   d1,d0
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.w  (a2),d5
		or.w    d6,d0
		or.w    d7,d1
		and.w   d1,d0
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.b  (a2),d5
		move.l  d5,(a0)+
		cmp.l   a0,a4
		bne.b   l.loop20
l.skip_loop20
		sub.w   d2,d0
		add.l   d2,d0

		bra.b   l.loop_end20
l.loop30	or.w    d6,d0
		or.w    d7,d1
		and.w   d1,d0
		addx.l  d3,d1
		move.b  (a1,d0.w),d4
		addx.l  d2,d0
		move.l  d4,a2
		move.b  (a2),(a0)+
l.loop_end20
		cmp.l   a0,a3
		bne.b   l.loop30
l.end20		movem.l (sp)+,d2-d7/a2-a4
		rts

;-----------------------------------------------------------------------
; R_DrawMaskedColumn (in r_things.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_sprtopscreen	;fixed_t
		xref	_spryscale	;fixed_t
		xref	_mfloorclip	;short*
		xref	_mceilingclip	;short*
		xref	_colfunc

;void R_DrawMaskedColumn (column_t* column)

;column_t=post_t=
;	byte	topdelta
;	byte	length

		cnop	0,4

_R_DrawMaskedColumn:
@R_DrawMaskedColumn:

		cmp.b	#$FF,(a0)
		beq.w	.rd_Exit

		movem.l	d2-d6/a2/a3,-(sp)

		move.l	_dc_x(a4),d0
		moveq	#0,d3
		move.l	_mfloorclip(a4),a1
		move.w	(a1,d0.l*2),d3
		move.l	_mceilingclip(a4),a1
		move.w	(a1,d0.l*2),d4
		ext.l	d4

		move.l	_dc_texturemid(a4),d6	;basetexturemid
		move.l	_spryscale(a4),d5
		move.l	_colfunc(a4),a3

		move.l	a0,a2		;column talteen
.rd_Loop2:
		moveq	#0,d2
		move.b	(a2),d1
		extb.l	d1
		move.b	1(a2),d2
		muls.l	d5,d1
		mulu.l	d5,d2
		add.l	_sprtopscreen(a4),d1
		add.l	d1,d2

		add.l	#FRACUNIT-1,d1
		swap	d1		;>>FRACBITS
		ext.l	d1

		subq.l	#1,d2
		swap	d2
		and.l	#$FFFF,d2

		cmp.w	d3,d2
		bmi.b	.rd_yhok
		move.l	d3,d2
		subq.l	#1,d2
.rd_yhok:
		cmp.l	d1,d4
		bmi.b	.rd_ylok
		move.l	d4,d1
		addq.l	#1,d1
.rd_ylok:

		move.l	d1,_dc_yl(a4)
		move.l	d2,_dc_yh(a4)

		cmp.w	d1,d2
		bmi.b	.rd_skip

		move.l	a2,_dc_source(a4)
		addq.l	#3,_dc_source(a4)

		move.l	d6,_dc_texturemid(a4)
		moveq	#0,d0
		move.b	(a2),d0
		swap	d0
		sub.l	d0,_dc_texturemid(a4)
		jsr	(a3)
.rd_skip:
		moveq	#4,d0
		add.b	1(a2),d0
		add.l	d0,a2

		cmp.b	#$FF,(a2)
		bne.b	.rd_Loop2

		move.l	d6,_dc_texturemid(a4)

		movem.l	(sp)+,d2-d6/a2/a3

.rd_Exit:
		rts


;//
;// R_DrawMaskedColumn
;// Used for sprites and masked mid textures.
;// Masked means: partly transparent, i.e. stored
;//  in posts/runs of opaque pixels.
;//
;short*		mfloorclip;
;short*		mceilingclip;
;
;fixed_t		spryscale;
;fixed_t		sprtopscreen;
;
;void R_DrawMaskedColumn (column_t* column)
;{
;    int		topscreen;
;    int 	bottomscreen;
;    fixed_t	basetexturemid;
;	
;    basetexturemid = dc_texturemid;
;	
;    for ( ; column->topdelta != 0xff ; ) 
;    {
;	// calculate unclipped screen coordinates
;	//  for post
;	topscreen = sprtopscreen + spryscale*column->topdelta;
;	bottomscreen = topscreen + spryscale*column->length;
;
;	dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
;	dc_yh = (bottomscreen-1)>>FRACBITS;
;		
;	if (dc_yh >= mfloorclip[dc_x])
;	    dc_yh = mfloorclip[dc_x]-1;
;	if (dc_yl <= mceilingclip[dc_x])
;	    dc_yl = mceilingclip[dc_x]+1;
;
;	if (dc_yl <= dc_yh)
;	{
;	    dc_source = (byte *)column + 3;
;	    dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
;	    // dc_source = (byte *)column + 3 - column->topdelta;
;
;	    // Drawn by either R_DrawColumn
;	    //  or (SHADOW) R_DrawFuzzColumn.
;	    colfunc ();	
;	}
;	column = (column_t *)(  (byte *)column + column->length + 4);
;    }
;	
;    dc_texturemid = basetexturemid;
;}

;-----------------------------------------------------------------------
; V_DrawPatch (in v_video.c) by Arto Huusko <arto.huusko@pp.qnet.fi>

		xref	_screens	;byte* screens[5]

		xref	_I_MarkRect

		STRUCTURE	patch,0
		 WORD	width
		 WORD	height
		 WORD	leftoffset
		 WORD	topoffset
		 STRUCT	columnofs,9*4	;nine ints
		LABEL	patch_size

;width		equ	0
;height		equ	2
;leftoffset	equ	4
;topoffset	equ	6
;columnofs	equ	8
;patch_size	equ	8+(9*4)

column_t
		STRUCTURE	column,0
		 BYTE	topdelta
		 BYTE	length
		LABEL	column_size

;topdelta	equ	0
;length		equ	1
;column_size	equ	2

		cnop	0,4
_V_DrawPatch:
@V_DrawPatch:
_V_DrawPatchDirect:
@V_DrawPatchDirect:
		movem.l	d2-d7/a2/a3/a5,-(sp)

		move.l	d0,d4	;x
		move.l	d1,d5	;y.. scrn in d2, patch in a0

		move.l	a0,a2	;Store patch
		moveq	#0,d0
		move.w	topoffset(a2),d0
		rol.w	#8,d0	;SWAPSHORT
		ext.l	d0
		sub.l	d0,d5
		moveq	#0,d0
		move.w	leftoffset(a2),d0
		rol.w	#8,d0	;SWAPSHORT
		ext.l	d0
		sub.l	d0,d4

		move.l	d2,d6
		bne.b	.vd_ScrnOK

		move.l	d4,d0			; x
		move.l	d5,d1			; z
		moveq	#0,d2
		move.w	width(a2),d2	; width
		rol.w	#8,d2			; SWAPSHORT
		move.w	height(a2),d3	; height
		rol.w	#8,d3			; SWAPSHORT
		jsr	(_I_MarkRect)

.vd_ScrnOK:
		lea	_screens(a4),a0
		move.l	(a0,d6.l*4),d7	; get start of screen address
;Peter... change here (quite obvious)
		move.l _SCREENWIDTH(a4), d2
		mulu.l	d2,d5	;y not needed further
		add.l	d5,d7	;+y*SCREENWIDTH
		add.l	d4,d7	;+x

		;D4=x, D7=desttop,
		moveq	#0,d6
		move.w	width(a2),d6
		rol.w	#8,d6	;SWAPSHORT
		;D6=w
		subq.w	#1,d6	;for ; col<w
		lea	columnofs(a2),a3	;prepare for columnofs[col]

.vd_Loop:
		move.l	(a3)+,d0
		rol.w	#8,d0
		swap	d0
		rol.w	#8,d0		;three instructions for SWAPLONG
		move.l	a2,a5		;column=patch+
		add.l	d0,a5		;... SWAPLONG(patch->columnofs[col])

		cmp.b	#$FF,(a5)
		beq.b	.vdl_Next	;last column

.vdl_Loop:
		move.l	d7,a1		;dest=desttop +

;... here are the other references to SCREENWIDTH
;	lsl.l #8,x + lsl.l #6,x is equal to 256x+64x=320x

		moveq	#0,d0
		move.b	(a5),d0		;column->topdelta*
		beq.s	.zeroTopDelta
;;;		move.l	d0,d1	;!
;;;		lsl.l	#8,d0	;!
;;;		lsl.l	#6,d1	;!
;;;		add.l	d0,a1	;!
;;;		add.l	d1,a1	;!

		mulu.w	d2,d0
		add.l	d0,a1

.zeroTopDelta:
		move.b	1(a5),d0
		addq.l	#3,a5		;source
		;Would it be possible to use the code from DrawColumn functions by Aki
		;here, too??
.vdl_DrawLoop:
		move.b	(a5)+,(a1)
		add.l	d2,a1
		subq.b	#1,d0
		bne.b	.vdl_DrawLoop

		addq.l	#1,a5		;bump to next column..
		;bumped already by three and length, so one more. (column +=column->length+4)

		cmp.b	#$FF,(a5)
		bne.b	.vdl_Loop

.vdl_Next:
		addq.l	#1,d7		; desttop one pixel to the right

		dbf		d6,.vd_Loop
.vd_exit:
		movem.l	(sp)+,d2-d7/a2/a3/a5

		rts

;void
;V_DrawPatch
;( int		x,
;  int		y,
;  int		scrn,
;  patch_t*	patch ) 
;{ 
;
;    int		count;
;    int		col; 
;    column_t*	column; 
;    byte*	desttop;
;    byte*	dest;
;    byte*	source; 
;    int		w; 
;	 
;    y -= SWAPSHORT(patch->topoffset); 
;    x -= SWAPSHORT(patch->leftoffset); 
; 
;    col = 0; 
;    desttop = screens[scrn]+y*SCREENWIDTH+x; 
;	 
;    w = SWAPSHORT(patch->width); 
;
;    for ( ; col<w ; x++, col++, desttop++)
;    { 
;	column = (column_t *)((byte *)patch + SWAPLONG(patch->columnofs[col])); 
; 
;	// step through the posts in a column 
;	while (column->topdelta != 0xff ) 
;	{ 
;	    source = (byte *)column + 3; 
;	    dest = desttop + column->topdelta*SCREENWIDTH; 
;	    count = column->length; 
;			 
;	    while (count--) 
;	    { 
;		*dest = *source++; 
;		dest += SCREENWIDTH; 
;	    } 
;	    column = (column_t *)(  (byte *)column + column->length 
;				    + 4 ); 
;	}
;    }
;    if (!scrn)
;	I_MarkRect (x, y, SWAPSHORT(patch->width), SWAPSHORT(patch->height)); 
;
;} 

;***********************************************************************

		end

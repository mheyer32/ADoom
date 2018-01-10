		mc68020
		section	text,code

		include "funcdef.i"
		include	"exec/execbase.i"
		include	"exec/exec_lib.i"
		include	"hardware/custom.i"

; This routine based on Mikael Kalms' 030-optimised CPU3BLIT1
; Mikael Kalms' email address is kalms@vasa.gavle.se
;
; Simplified and changed to use Signal()s by Peter McGavin, 14 Jan 98

		xdef	_c2p1x1_cpu3blit1_queue_init
		xdef	_c2p1x1_cpu3blit1_queue

		xref	_GfxBase

_LVOQBlit	equ	-276

;-----------------------------------------------------------------------
;		IFND    CHUNKYXMAX
;CHUNKYXMAX	equ	320
;		ENDC
;		IFND    CHUNKYYMAX
;CHUNKYYMAX	equ	200
;		ENDC

		cnop 0,4

;-----------------------------------------------------------------------
; d0.w  chunkyx [chunky-pixels]
; d1.w  chunkyy [chunky-pixels]
; d3.w  scroffsy [screen-pixels]
; d5.l  bplsize [bytes] -- offset between one row in one bpl and the next bpl
; d6.l  signals1 mask
; d7.l  signals3 mask
; a2.l  this task to Signal(signals1) on cleanup
; a3.l  other task to Signal(signals3) on cleanup
; a4.l  chip buffer of at least size chunkyx * chunkyy

_c2p1x1_cpu3blit1_queue_init
		movem.l	d2-d3,-(sp)
		lea	(c2p_data,pc),a0
		move.l	a3,(c2p_blitbuf-c2p_data,a0)
		move.l	d6,(signals1-c2p_data,a0)
		move.l	d7,(signals3-c2p_data,a0)
		move.l	a1,(task-c2p_data,a0)
		move.l	a2,(othertask-c2p_data,a0)
		andi.l	#$ffff,d0
		andi.l	#$ffff,d1
		move.l	d5,(c2p_bplsize-c2p_data,a0)
		move.w	d1,(c2p_chunkyy-c2p_data,a0)
		add.w	d3,d1
		mulu.w	d0,d1
		lsr.l	#3,d1
		subq.l	#2,d1
		move.l	d1,(c2p_scroffs2-c2p_data,a0)
		mulu.w	d0,d3
		lsr.l	#3,d3
		move.l	d3,(c2p_scroffs-c2p_data,a0)
		move.w	(c2p_chunkyy-c2p_data,a0),d1
		mulu.w	d0,d1
		move.l	d1,(c2p_pixels-c2p_data,a0)
		lsr.l	#4,d1
		move.l	d1,(c2p_pixels16-c2p_data,a0)
		movem.l	(sp)+,d2-d3
		rts

;-----------------------------------------------------------------------
; a0    c2pscreen
; a1    bitplanes

_c2p1x1_cpu3blit1_queue
		movem.l	d2-d7/a2-a6,-(sp)

		lea	(c2p_data,pc),a2
		move.l	a1,(c2p_screen-c2p_data,a2)

		move.l	#$0f0f0f0f,a4
		move.l	#$00ff00ff,a5
		move.l	#$55555555,a6

		movea.l	(c2p_blitbuf-c2p_data,a2),a1
		move.l	(c2p_pixels-c2p_data,a2),a2
		add.l	a0,a2
		cmpa.l	a0,a2
		beq	.none

		move.l	(a0)+,d0
		move.l	(a0)+,d6
		move.l	(a0)+,a3
		move.l	(a0)+,d7
		move.l	a4,d5
		move.l	d6,d1			; Swap 4x1
		lsr.l	#4,d1
		eor.l	d0,d1
		and.l	d5,d1
		eor.l	d1,d0
		lsl.l	#4,d1
		eor.l	d6,d1

		move.l	a3,d6
		move.l	d7,d4
		lsr.l	#4,d4
		eor.l	d6,d4
		and.l	d5,d4
		eor.l	d4,d6
		lsl.l	#4,d4
		eor.l	d4,d7

		move.l	a5,d5
		move.l	d6,d2			; Swap 8x2, part 1
		lsr.l	#8,d2
		eor.l	d0,d2
		and.l	d5,d2
		eor.l	d2,d0
		lsl.l	#8,d2
		eor.l	d6,d2

		bra.b	.start
.x
		move.l	(a0)+,d0
		move.l	(a0)+,d6
		move.l	(a0)+,a3
		move.l	(a0)+,d7
		move.l	d1,(a1)+
		move.l	a4,d5
		move.l	d6,d1			; Swap 4x1
		lsr.l	#4,d1
		eor.l	d0,d1
		and.l	d5,d1
		eor.l	d1,d0
		lsl.l	#4,d1
		eor.l	d6,d1

		move.l	a3,d6
		move.l	d7,d4
		lsr.l	#4,d4
		move.l	d2,(a1)+
		eor.l	d6,d4
		and.l	d5,d4
		eor.l	d4,d6
		lsl.l	#4,d4
		eor.l	d4,d7

		move.l	a5,d5
		move.l	d6,d2			; Swap 8x2, part 1
		lsr.l	#8,d2
		eor.l	d0,d2
		and.l	d5,d2
		eor.l	d2,d0
		move.l	d3,(a1)+
		lsl.l	#8,d2
		eor.l	d6,d2
.start
		move.l	a6,d4
		move.l	d2,d3			; Swap 1x2, part 1
		lsr.l	#1,d3
		eor.l	d0,d3
		and.l	d4,d3
		eor.l	d3,d0
		add.l	d3,d3
		eor.l	d3,d2

		move.l	d7,d3			; Swap 8x2, part 2
		lsr.l	#8,d3
		move.l	d0,(a1)+
		eor.l	d1,d3
		and.l	d5,d3
		eor.l	d3,d1
		lsl.l	#8,d3
		eor.l	d7,d3

		move.l	d3,d6			; Swap 1x2, part 2
		lsr.l	#1,d6
		eor.l	d1,d6
		and.l	d4,d6
		eor.l	d6,d1
		add.l	d6,d6
		eor.l	d6,d3

		move.l	(a0)+,d0
		move.l	(a0)+,d6
		move.l	(a0)+,a3
		move.l	(a0)+,d7
		move.l	d1,(a1)+
		move.l	a4,d5
		move.l	d6,d1			; Swap 4x1
		lsr.l	#4,d1
		eor.l	d0,d1
		and.l	d5,d1
		eor.l	d1,d0
		lsl.l	#4,d1
		eor.l	d6,d1

		move.l	a3,d6
		move.l	d7,d4
		lsr.l	#4,d4
		move.l	d2,(a1)+
		eor.l	d6,d4
		and.l	d5,d4
		eor.l	d4,d6
		lsl.l	#4,d4
		eor.l	d4,d7

		move.l	a5,d5
		move.l	d6,d2			; Swap 8x2, part 1
		lsr.l	#8,d2
		eor.l	d0,d2
		and.l	d5,d2
		eor.l	d2,d0
		move.l	d3,(a1)+
		lsl.l	#8,d2
		eor.l	d6,d2

		move.l	a6,d4
		move.l	d2,d3			; Swap 1x2, part 1
		lsr.l	#1,d3
		eor.l	d0,d3
		and.l	d4,d3
		eor.l	d3,d0
		add.l	d3,d3
		eor.l	d3,d2

		move.l	d7,d3			; Swap 8x2, part 2
		lsr.l	#8,d3
		move.l	d0,(a1)+
		eor.l	d1,d3
		and.l	d5,d3
		eor.l	d3,d1
		lsl.l	#8,d3
		eor.l	d7,d3

		move.l	d3,d6			; Swap 1x2, part 2
		lsr.l	#1,d6
		eor.l	d1,d6
		and.l	d4,d6
		eor.l	d6,d1
		add.l	d6,d6
		eor.l	d6,d3

		cmp.l	a0,a2
		bne		.x
.x2
		move.l	d1,(a1)+
		move.l	d2,(a1)+
		move.l	d3,(a1)+

		lea	(c2p_bltnode,pc),a1
		move.l	#c2p1x1_cpu3blit1_queue_41,(c2p_bltroutptr-c2p_bltnode,a1)
		movea.l	(_GfxBase),a6
		jsr	(_LVOQBlit,a6)

.none
		movem.l (sp)+,d2-d7/a2-a6
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_41			; Pass 4, subpass 1, ascending
		move.w	#-1,(bltafwm,a0)
		move.w	#-1,(bltalwm,a0)
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		add.l	#12,d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	d0,d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	#14,(bltamod,a0)
		move.w	#14,(bltbmod,a0)
		move.w	#0,(bltdmod,a0)
		move.w	#$cccc,(bltcdat,a0)
		move.w	#$0de4,(bltcon0,a0)
		move.w	#$2000,(bltcon1,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_42,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_42			; Pass 4, subpass 2, ascending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		addq.l	#8,d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	d0,d0
		add.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	d0,d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_43,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_43			; Pass 4, subpass 3, ascending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		addq.l	#4,d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	d0,d0
		add.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_44,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_44			; Pass 4, subpass 4, ascending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		lsl.l	#3,d0
		sub.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_45,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_45			; Pass 4, subpass 5, descending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		subq.l	#4,d0
		add.l	(c2p_pixels-c2p_bltnode,a1),d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs2-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	#$2de4,bltcon0(a0)
		move.w	#$0002,bltcon1(a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_46,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_46			; Pass 4, subpass 6, descending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		subq.l	#8,d0
		add.l	(c2p_pixels-c2p_bltnode,a1),d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		lsl.l	#2,d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs2-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_47,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_47			; Pass 4, subpass 7, descending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		sub.l	#12,d0
		add.l	(c2p_pixels-c2p_bltnode,a1),d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs2-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		move.l	#c2p1x1_cpu3blit1_queue_48,(c2p_bltroutptr-c2p_bltnode,a1)
		rts

;-----------------------------------------------------------------------
c2p1x1_cpu3blit1_queue_48			; Pass 4, subpass 8, descending
		move.l	(c2p_blitbuf-c2p_bltnode,a1),d0
		sub.l	#16,d0
		add.l	(c2p_pixels-c2p_bltnode,a1),d0
		move.l	d0,(bltapt,a0)
		addq.l	#2,d0
		move.l	d0,(bltbpt,a0)
		move.l	(c2p_bplsize-c2p_bltnode,a1),d0
		lsl.l	#2,d0
		add.l	(c2p_bplsize-c2p_bltnode,a1),d0
		add.l	(c2p_screen-c2p_bltnode,a1),d0
		add.l	(c2p_scroffs2-c2p_bltnode,a1),d0
		move.l	d0,(bltdpt,a0)
		move.w	(c2p_pixels16+2-c2p_bltnode,a1),(bltsizv,a0)
		move.w	#1,(bltsizh,a0)
		moveq	#0,d0
		rts

;-----------------------------------------------------------------------
c2p_blitcleanup
		movem.l	a2/a6,-(sp)
		lea	(c2p_bltnode,pc),a2
		move.l	(task-c2p_bltnode,a2),a1 ; signal QBlit() has finished
		move.l	(signals1-c2p_bltnode,a2),d0
		move.l	(4).w,a6
		jsr	(_LVOSignal,a6)		; may be called from interrupts
		move.l	(othertask-c2p_bltnode,a2),a1
		move.l	(signals3-c2p_bltnode,a2),d0
		jsr	(_LVOSignal,a6)		; signal pass 4 has finished
		movem.l	(sp)+,a2/a6
		rts

;-----------------------------------------------------------------------
		cnop 0,4
c2p_bltnode
		dc.l	0
c2p_bltroutptr
		dc.l	0
		dc.b	$40,0
		dc.l	0
c2p_bltroutcleanup
		dc.l	 c2p_blitcleanup

task		dc.l	0	; ptr to task to Signal(signals1) on cleanup()
othertask	dc.l	0	; ptr to task to Signal(signals3) on cleanup()
signals1	dc.l	0	; signals to Signal() this task at cleanup
signals3	dc.l	0	; signals to Signal() othertask at cleanup

		cnop	0,4

c2p_data
c2p_screen		dc.l	0
c2p_scroffs		dc.l	0
c2p_scroffs2	dc.l	0
c2p_bplsize		dc.l	0
c2p_pixels		dc.l	0
c2p_pixels16	dc.l	0
c2p_blitbuf		dc.l	0
c2p_chunkyy		dc.w	0

;-----------------------------------------------------------------------
;		section bss_c,bss_c,chip
;
;c2p_blitbuf
;		ds.b	 CHUNKYXMAX*CHUNKYYMAX

;-----------------------------------------------------------------------

		end

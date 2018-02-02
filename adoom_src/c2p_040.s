		mc68020
		section	.text,code

; Chunky2Planar algorithm, originally by James McCoull
; Modified by Peter McGavin for variable size and depth
; and "compare buffer" (hope I didn't slow it down too much)
;
; 	Cpu only solution VERSION 2
;	Optimised for 040+fastram
;	bitplanes are assumed contiguous!
;	analyse instruction offsets to check performance

;void __asm c2p_6_040 (register __a0 UBYTE *chunky_data,
;                      register __a1 PLANEPTR raster,
;                      register __a2 UBYTE *compare_buffer,
;                      register __a4 UBYTE *xlate,
;                      register __d1 ULONG plsiz,
;                      register __d2 BOOL force_update);

;void __asm c2p_8_040 (register __a0 UBYTE *chunky_data,
;                      register __a1 PLANEPTR raster,
;                      register __a2 UBYTE *compare_buffer,
;                      register __d1 ULONG plsiz);

; a0 -> width*height chunky pixels
; a1 -> contiguous bitplanes
; a2 -> compare buffer
; d1 = width*height/8   (width*height must be a multiple of 32)

	ifeq	depth-8
		xdef	_c2p_8_040
_c2p_8_040:
	else
	ifeq	depth-6
		xdef	_c2p_6_040
_c2p_6_040:
	else
		fail	"unsupported depth!"
	endc
	endc

merge		macro ; in1,in2,tmp3,tmp4,mask,shift
; \1 = abqr
; \2 = ijyz
		move.l	\2,\4
		move.l	#\5,\3
		and.l	\3,\2	; \2 = 0j0z
		and.l	\1,\3	; \3 = 0b0r
		eor.l	\3,\1	; \1 = a0q0
		eor.l	\2,\4	; \4 = i0y0
		ifeq	\6-1
		add.l	\3,\3
		else
		lsl.l	#\6,\3	; \3 = b0r0
		endc
		lsr.l	#\6,\4	; \4 = 0i0y
		or.l	\3,\2	; \2 = bjrz
		or.l	\4,\1	; \1 = aiqy
		endm

xlate		macro	; translate 4 8-bit pixels to 6-bit EHB
		move.b	(\1,a0),d7
		move.b	(a4,d7.w),\2
		lsl.w	#8,\2
		move.b	(\1+8,a0),d7
		move.b	(a4,d7.w),\2
		swap	\2
		move.b	(\1+16,a0),d7
		move.b	(a4,d7.w),\2
		lsl.w	#8,\2
		move.b	(\1+24,a0),d7
		move.b	(a4,d7.w),\2
		endm

start:		movem.l	d2-d7/a2-a6,-(sp)

		sub.w	#46,sp		; space for temporary variables

	ifle depth-6
		move.w	d2,(44,sp)	; video_force_update
	endc

; a0 = chunky buffer
; a1 = output area
; a2 = compare buffer
; d1 = plsiz

		movea.l	d1,a3		; a3 = plsiz

		move.l	a0,(40,sp)
		lsl.l	#3,d1
		add.l	d1,(40,sp)	; (40,sp) -> end of chunky data

first_loop:
	ifle depth-6
		tst.w	(44,sp)		; force_update?
		bne.b	first_case
	endc
		cmpm.l	(a0)+,(a2)+
		bne.b	stub1
		cmpm.l	(a0)+,(a2)+
		bne.b	stub2
		cmpm.l	(a0)+,(a2)+
		bne.b	stub3
		cmpm.l	(a0)+,(a2)+
		bne.b	stub4
		cmpm.l	(a0)+,(a2)+
		bne.b	stub5
		cmpm.l	(a0)+,(a2)+
		bne.b	stub6
		cmpm.l	(a0)+,(a2)+
		bne.b	stub7
		cmpm.l	(a0)+,(a2)+
		bne.b	stub8

		addq.l	#4,a1		; skip 32 pixels on output

		cmpa.l	(40,sp),a0
		bcs.b	first_loop
		bra.w	exit		; exit if no changes found

stub8:		subq.l	#4,a0
		subq.l	#4,a2
stub7:		subq.l	#4,a0
		subq.l	#4,a2
stub6:		subq.l	#4,a0
		subq.l	#4,a2
stub5:		subq.l	#4,a0
		subq.l	#4,a2
stub4:		subq.l	#4,a0
		subq.l	#4,a2
stub3:		subq.l	#4,a0
		subq.l	#4,a2
stub2:		subq.l	#4,a0
		subq.l	#4,a2
stub1:		subq.l	#4,a0
		subq.l	#4,a2

first_case:
	ifgt depth-6		; depth 8 code --- no need to xlate pixels
		move.l	(0,a0),d1
	 	move.l	(4,a0),d3
		move.l	(8,a0),d0
		move.l	(12,a0),d2
		move.l	(2,a0),d4
	 	move.l	(10,a0),d5
		move.l	(6,a0),d6
		move.l	(14,a0),d7

		move.l	d1,(0,a2)
		move.l	d3,(4,a2)
		move.l	d0,(8,a2)
		move.l	d2,(12,a2)

	 	move.w	(16,a0),d1
	 	move.w	(24,a0),d0
		move.w	(20,a0),d3
		move.w	(28,a0),d2
	 	move.w	(18,a0),d4
	 	move.w	(26,a0),d5
		move.w	(22,a0),d6
		move.w	(30,a0),d7

	 	move.w	d1,(16,a2)
	 	move.w	d0,(24,a2)
		move.w	d3,(20,a2)
		move.w	d2,(28,a2)
	 	move.w	d4,(18,a2)
	 	move.w	d5,(26,a2)
		move.w	d6,(22,a2)
		move.w	d7,(30,a2)

		move.l	d6,a5
		move.l	d7,a6

		merge	d1,d0,d6,d7,$00ff00ff,8
		merge	d3,d2,d6,d7,$00ff00ff,8

		merge	d1,d3,d6,d7,$0f0f0f0f,4	
		merge	d0,d2,d6,d7,$0f0f0f0f,4

		exg	d1,a5
		exg	d0,a6

		merge	d4,d5,d6,d7,$00ff00ff,8
		merge	d1,d0,d6,d7,$00ff00ff,8

		merge	d4,d1,d6,d7,$0f0f0f0f,4
		merge	d5,d0,d6,d7,$0f0f0f0f,4

		merge	d3,d1,d6,d7,$33333333,2
		merge	d2,d0,d6,d7,$33333333,2	

		merge	d3,d2,d6,d7,$55555555,1
		merge	d1,d0,d6,d7,$55555555,1

		move.l	d0,(0*4,sp)		;plane0 (movem.l is slower!)
		move.l	d1,(1*4,sp)		;plane1
		move.l	d2,(2*4,sp)		;plane2
		move.l	d3,(3*4,sp)		;plane3

		move.l	a5,d3
		move.l	a6,d2

		merge	d3,d4,d6,d7,$33333333,2
		merge	d2,d5,d6,d7,$33333333,2

		merge	d3,d2,d6,d7,$55555555,1
		merge	d4,d5,d6,d7,$55555555,1

		move.l	d5,(4*4,sp)		;plane4
		move.l	d4,(5*4,sp)		;plane5

		move.l	d2,(6*4,sp)		;plane6
		move.l	d3,(7*4,sp)		;plane7

	else			; depth 6 code, xlate from 8-bit to 6-bit EHB
		moveq	#0,d7

		move.l	(a0),(a2)	; copy to compare buffer
		move.l	(4,a0),(4,a2)
		move.l	(8,a0),(8,a2)
		move.l	(12,a0),(12,a2)
		move.l	(16,a0),(16,a2)
		move.l	(20,a0),(20,a2)
		move.l	(24,a0),(24,a2)
		move.l	(28,a0),(28,a2)

		xlate	0,d1		; does 8-bit to EHB colour translate
		xlate	1,d0		; 4 pixels at a time
		xlate	4,d3
		xlate	5,d2

		merge	d1,d3,d6,d7,$0f0f0f0f,4	
		merge	d0,d2,d6,d7,$0f0f0f0f,4

		movea.l	d1,a5
		movea.l	d0,a6

		moveq	#0,d7

		xlate	2,d4
		xlate	3,d5
		xlate	6,d1
		xlate	7,d0

		merge	d4,d1,d6,d7,$0f0f0f0f,4
		merge	d5,d0,d6,d7,$0f0f0f0f,4

		merge	d3,d1,d6,d7,$33333333,2
		merge	d2,d0,d6,d7,$33333333,2	

		merge	d3,d2,d6,d7,$55555555,1
		merge	d1,d0,d6,d7,$55555555,1

		move.l	d0,(0*4,sp)		;plane0 (movem.l is slower!)
		move.l	d1,(1*4,sp)		;plane1
		move.l	d2,(2*4,sp)		;plane2
		move.l	d3,(3*4,sp)		;plane3

		move.l	a5,d3
		move.l	a6,d2

		merge	d3,d4,d6,d7,$33333333,2
		merge	d2,d5,d6,d7,$33333333,2

		merge	d4,d5,d6,d7,$55555555,1

		move.l	d5,(4*4,sp)		;plane4
		move.l	d4,(5*4,sp)		;plane5

	endc

		adda.w	#32,a0
		adda.w	#32,a2

		move.l	a1,(32,sp)		; save output address
		addq.l	#4,a1			; skip 32 pixels on output

		cmpa.l	(40,sp),a0
		bcc.w	final_case


main_loop:
	ifle depth-6
		tst.w	(44,sp)		; force_update?
		bne.b	main_case
	endc
		cmpm.l	(a0)+,(a2)+	; compare next 32 pixels
		bne.b	mstub1
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub2
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub3
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub4
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub5
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub6
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub7
		cmpm.l	(a0)+,(a2)+
		bne.b	mstub8

		addq.l	#4,a1		; skip 32 pixels on output

		cmpa.l	(40,sp),a0
		bcs.b	main_loop
		bra.w	final_case	; exit if no more changes found

mstub8:		subq.l	#4,a0
		subq.l	#4,a2
mstub7:		subq.l	#4,a0
		subq.l	#4,a2
mstub6:		subq.l	#4,a0
		subq.l	#4,a2
mstub5:		subq.l	#4,a0
		subq.l	#4,a2
mstub4:		subq.l	#4,a0
		subq.l	#4,a2
mstub3:		subq.l	#4,a0
		subq.l	#4,a2
mstub2:		subq.l	#4,a0
		subq.l	#4,a2
mstub1:		subq.l	#4,a0
		subq.l	#4,a2

main_case:	move.l	a1,(36,sp)	; save current output address
		move.l	(32,sp),a1	; a1 = previous output address

	ifgt depth-6
		move.l	(0,a0),d1
	 	move.l	(4,a0),d3
	 	move.l	(8,a0),d0
		move.l	(12,a0),d2
		move.l	(2,a0),d4
	 	move.l	(10,a0),d5
		move.l	(6,a0),d6
		move.l	(14,a0),d7

		move.l	d1,(0,a2)
		move.l	d3,(4,a2)
		move.l	d0,(8,a2)
		move.l	d2,(12,a2)

	 	move.w	(16,a0),d1
	 	move.w	(24,a0),d0
		move.w	(20,a0),d3
		move.w	(28,a0),d2
	 	move.w	(18,a0),d4
	 	move.w	(26,a0),d5
		move.w	(22,a0),d6
		move.w	(30,a0),d7

	 	move.w	d1,(16,a2)
	 	move.w	d0,(24,a2)
		move.w	d3,(20,a2)
		move.w	d2,(28,a2)
	 	move.w	d4,(18,a2)
	 	move.w	d5,(26,a2)
		move.w	d6,(22,a2)
		move.w	d7,(30,a2)

		move.l	d6,a5
		move.l	d7,a6

		move.l	(0*4,sp),(a1)		;plane0
		adda.l	a3,a1			;a1+=plsiz

		merge	d1,d0,d6,d7,$00ff00ff,8
		merge	d3,d2,d6,d7,$00ff00ff,8

		move.l	(1*4,sp),(a1)		;plane1
		adda.l	a3,a1			;a1+=plsiz

		merge	d1,d3,d6,d7,$0f0f0f0f,4	
		merge	d0,d2,d6,d7,$0f0f0f0f,4

		exg	d1,a5
		exg	d0,a6

		move.l	(2*4,sp),(a1)		;plane2
		adda.l	a3,a1			;a1+=plsiz

		merge	d4,d5,d6,d7,$00ff00ff,8
		merge	d1,d0,d6,d7,$00ff00ff,8

		move.l	(3*4,sp),(a1)		;plane3
		adda.l	a3,a1			;a1+=plsiz

		merge	d4,d1,d6,d7,$0f0f0f0f,4
		merge	d5,d0,d6,d7,$0f0f0f0f,4

		move.l	(4*4,sp),(a1)		;plane4
		adda.l	a3,a1			;a1+=plsiz

		merge	d3,d1,d6,d7,$33333333,2
		merge	d2,d0,d6,d7,$33333333,2	

		move.l	(5*4,sp),(a1)		;plane5
		adda.l	a3,a1			;a1+=plsiz

		merge	d3,d2,d6,d7,$55555555,1
		merge	d1,d0,d6,d7,$55555555,1

		move.l	d0,(0*4,sp)		;plane0 (movem.l is slower!)
		move.l	d1,(1*4,sp)		;plane1
		move.l	d2,(2*4,sp)		;plane2
		move.l	d3,(3*4,sp)		;plane3

		move.l	a5,d3
		move.l	a6,d2

		move.l	(6*4,sp),(a1)		;plane6
		adda.l	a3,a1			;a1+=plsiz

		merge	d3,d4,d6,d7,$33333333,2
		merge	d2,d5,d6,d7,$33333333,2

		move.l	(7*4,sp),(a1)		;plane7
		adda.l	a3,a1			;a1+=plsiz

		merge	d3,d2,d6,d7,$55555555,1
		merge	d4,d5,d6,d7,$55555555,1

		move.l	d5,(4*4,sp)		;plane4
		move.l	d4,(5*4,sp)		;plane5

		move.l	d2,(6*4,sp)		;plane6
		move.l	d3,(7*4,sp)		;plane7

	else			; depth 6 code, xlate from 8-bit to 6-bit EHB
		moveq	#0,d7

		move.l	(a0),(a2)
		move.l	(4,a0),(4,a2)
		move.l	(8,a0),(8,a2)
		move.l	(12,a0),(12,a2)
		move.l	(16,a0),(16,a2)
		move.l	(20,a0),(20,a2)
		move.l	(24,a0),(24,a2)
		move.l	(28,a0),(28,a2)

		xlate	0,d1		; does 8-bit to EHB colour translate
		xlate	1,d0		; 4 pixels at a time
		xlate	4,d3
		xlate	5,d2

		move.l	(0*4,sp),(a1)		;plane0
		adda.l	a3,a1			;a1+=plsiz

		merge	d1,d3,d6,d7,$0f0f0f0f,4	
		merge	d0,d2,d6,d7,$0f0f0f0f,4

		movea.l	d1,a5
		movea.l	d0,a6

		moveq	#0,d7

		xlate	2,d4
		xlate	3,d5
		xlate	6,d1
		xlate	7,d0

		move.l	(1*4,sp),(a1)		;plane1
		adda.l	a3,a1			;a1+=plsiz

		merge	d4,d1,d6,d7,$0f0f0f0f,4
		merge	d5,d0,d6,d7,$0f0f0f0f,4

		move.l	(2*4,sp),(a1)		;plane2
		adda.l	a3,a1			;a1+=plsiz

		merge	d3,d1,d6,d7,$33333333,2
		merge	d2,d0,d6,d7,$33333333,2	

		move.l	(3*4,sp),(a1)		;plane3
		adda.l	a3,a1			;a1+=plsiz

		merge	d3,d2,d6,d7,$55555555,1
		merge	d1,d0,d6,d7,$55555555,1

		move.l	d0,(0*4,sp)		;plane0 (movem.l is slower!)
		move.l	d1,(1*4,sp)		;plane1
		move.l	d2,(2*4,sp)		;plane2
		move.l	d3,(3*4,sp)		;plane3

		move.l	(4*4,sp),(a1)		;plane4
		adda.l	a3,a1			;a1+=plsiz

		move.l	a5,d3
		move.l	a6,d2

		merge	d3,d4,d6,d7,$33333333,2

		move.l	(5*4,sp),(a1)		;plane5
		adda.l	a3,a1			;a1+=plsiz

		merge	d2,d5,d6,d7,$33333333,2

		merge	d4,d5,d6,d7,$55555555,1

		move.l	d5,(4*4,sp)		;plane4
		move.l	d4,(5*4,sp)		;plane5

	endc

		adda.w	#32,a0
		adda.w	#32,a2

		movea.l	(36,sp),a1	; restore current output address
		move.l	a1,(32,sp)	; save output address

		addq.l	#4,a1		; skip 32 pixels on output

		cmpa.l	(40,sp),a0
		bcs.w	main_loop


final_case:	move.l	(32,sp),a1	; a1 = previous output address

		move.l	(0*4,sp),(a1)		;plane0
		adda.l	a3,a1			;a1+=plsiz
		move.l	(1*4,sp),(a1)	 	;plane1
		adda.l	a3,a1			;a1+=plsiz
		move.l	(2*4,sp),(a1)		;plane2
		adda.l	a3,a1			;a1+=plsiz
		move.l	(3*4,sp),(a1)		;plane3
		adda.l	a3,a1			;a1+=plsiz
		move.l	(4*4,sp),(a1)		;plane4	
		adda.l	a3,a1			;a1+=plsiz
		move.l	(5*4,sp),(a1)		;plane5
	ifgt depth-6
		adda.l	a3,a1			;a1+=plsiz
		move.l	(6*4,sp),(a1)		;plane6
		adda.l	a3,a1			;a1+=plsiz
		move.l	(7*4,sp),(a1)		;plane7
	endc

exit:		add.w	#46,sp
		movem.l	(sp)+,d2-d7/a2-a6
		rts

		cnop	0,4
end:

		end

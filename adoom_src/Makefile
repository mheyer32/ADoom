CC=m68k-amigaos-gcc
PHXASS=vasmm68k_mot

PREFIX = $(shell ./getprefix.sh "$(CC)")

#-noixemul
#-fbaserel
#-mcrt=clib2
#-mcrt=nix13
#-msmall-code
#-Wa,-adhln
#-mregparm=4
#-v   #verbose
#-fstrength-reduce

CLIB_CFLAGS = -mcrt=clib2 -DUSECLIB2 -mregparm=4
CLIB_LDFLAGS =

NOIXEMUL_CFLAGS = -noixemul -DUSENOIXEMUL -mregparm=4
NOIXEMUL_LDFLAGS =

USEIXEMUL ?= 1

ifeq ($(USEIXEMUL), 1)
RUNTIME_CFLAGS = $(NOIXEMUL_CFLAGS)
RUNTIME_LDFLAGS = $(NOIXEMUL_LDFLAGS)
else
	RUNTIME_CFLAGS = $(CLIB_CFLAGS)
	RUNTIME_LDFLAGS = $(CLIB_LDFLAGS)
endif

#-fbbb=abcefimprs
#-fbbb=
#-fbbb=Enable Bebbo's optimizations.
# +    enable all optimizations
# a   commute add move instructions
# b    use register for base addresses
# c    convert load const and compare into a sub
# e    eliminate dead assignments + redundant loads
# f	   shrink stack frame
# i    use post increment on addresses
# m    merge add and move statements
# p    propagate move assignment pairs out of loops
# r    register renaming to maybe save registers
# s    a strcpy optimization
# v    be verbose
# V    be very verbose
# x    dump insns
# Default: -fbbb=+ which yields -fbbb=abcefimprs

#-v -fbbb=Vabcefimprs

CFLAGS = $(RUNTIME_CFLAGS)
CFLAGS += -fbaserel -m68030 -m68881
CFLAGS += -Ofast -fstrength-reduce -fomit-frame-pointer
CFLAGS += -Werror -Wimplicit -Wstrict-prototypes
#CFLAGS += -include "doomdef.h"

#-DRANGECHECK
CFLAGS += -DNDEBUG -D__BIG_ENDIAN__ -DNORMALUNIX -DAMIGA -Diabs=abs -D__NO_NET_API

LDFLAGS = $(RUNTIME_LDFLAGS)

PFLAGS = -Fhunk -phxass -nosym -ldots -m68030 -m68882
PFLAGS += -I$(PREFIX)/m68k-amigaos/ndk-include
PFLAGS += -I$(PREFIX)/m68k-amigaos/ndk/include

INTERMEDIATE =$(CURDIR)/build/
OUT = $(CURDIR)/bin/

# not too sophisticated dependency
OBJS = \
	doomdef.o \
	doomstat.o \
	dstrings.o \
	w_wad.o \
	amiga_wad.o \
	amiga_fixed.o \
	amiga_sound.o \
	amiga_system.o \
	amiga_net.o \
	amiga_plane.o \
	amiga_segs.o \
	amiga_data.o \
	amiga_sight.o \
	amiga_things.o \
	amiga_median.o \
	amiga_timer.o \
	amiga_video.o \
	amiga_draw.o \
	amiga_sega.o \
	c2p_8_020.o \
	c2p_6_020.o \
	c2p_8_030.o \
	c2p_8_040.o \
	c2p_6_040.o \
	c2p8_040_amlaukka.o \
	amiga_mmu.o \
	m_argv.o \
	dehacked.o \
	tables.o \
	f_finale.o \
	f_wipe.o \
	d_main.o \
	d_net.o \
	d_items.o \
	g_game.o \
	m_menu.o \
	m_misc.o \
	m_bbox.o \
	m_fixed.o \
	m_swap.o \
	m_cheat.o \
	m_random.o \
	am_map.o \
	p_ceilng.o \
	p_doors.o \
	p_enemy.o \
	p_floor.o \
	p_inter.o \
	p_lights.o \
	p_map.o \
	p_maputl.o \
	p_plats.o \
	p_pspr.o \
	p_setup.o \
	p_sight.o \
	p_spec.o \
	p_switch.o \
	p_mobj.o \
	p_telept.o \
	p_tick.o \
	p_saveg.o \
	p_user.o \
	r_bsp.o \
	r_data.o \
	r_draw.o \
	r_main.o \
	r_segs.o \
	r_sky.o \
	r_things.o \
	wi_stuff.o \
	v_video.o \
	st_lib.o \
	hu_stuff.o \
	hu_lib.o \
	s_sound.o \
	z_zone.o \
	info.o \
	sounds.o \
	r_plane.o \
	st_stuff.o

OUTOBJS = $(addprefix $(INTERMEDIATE), $(OBJS))

#$(info $(OBJS))

all:	 ADoom ADoom_SndSrvr

clean:
	rm *.o

ADoom: DoomSnd.h $(OBJS) amiga_main.o Makefile
	$(CC) $(CFLAGS) $(OBJS) amiga_main.o \
	$(LDFLAGS) -o $(OUT)ADoom

ADoom_SndSrvr: ADoom_SndSrvr.s Makefile
	$(PHXASS) $(PFLAGS) -o ADoom_SndSrvr ADoom_SndSrvr.s

amiga_fixed.o: amiga_fixed.s
	$(PHXASS) $(PFLAGS) amiga_fixed.s

amiga_draw.o: amiga_draw.s Makefile
	$(PHXASS) $(PFLAGS) amiga_draw.s

amiga_plane.o: amiga_plane.s Makefile
	$(PHXASS) $(PFLAGS) amiga_plane.s

amiga_segs.o: amiga_segs.s Makefile
	$(PHXASS) $(PFLAGS) amiga_segs.s

amiga_data.o: amiga_data.s Makefile
	$(PHXASS) $(PFLAGS) amiga_data.s

amiga_wad.o: amiga_wad.s Makefile
	$(PHXASS) $(PFLAGS) amiga_wad.s

amiga_sight.o: amiga_sight.s Makefile
	$(PHXASS) $(PFLAGS) amiga_sight.s

amiga_things.o: amiga_things.s Makefile
	$(PHXASS) $(PFLAGS) amiga_things.s

amiga_music.o: amiga_music.s Makefile
	$(PHXASS) $(PFLAGS) amiga_music.s

c2p_8_020.o: c2p_020.s Makefile
	$(PHXASS) $(PFLAGS) -Ddepth=8 -o c2p_8_020.o c2p_020.s

c2p_6_020.o: c2p_020.s Makefile
	$(PHXASS) $(PFLAGS) -Ddepth=6 -o c2p_6_020.o c2p_020.s

c2p_8_030.o: c2p_030.s Makefile
	$(PHXASS) $(PFLAGS) -o c2p_8_030.o c2p_030.s

c2p_8_040.o: c2p_040.s Makefile
	$(PHXASS) $(PFLAGS) -Ddepth=8 -o c2p_8_040.o c2p_040.s

c2p_6_040.o: c2p_040.s Makefile
	$(PHXASS) $(PFLAGS) -Ddepth=6 -o c2p_6_040.o c2p_040.s

#c2p_8_akiko.o: c2p_akiko.s smakefile
#	macro68 $(MFLAGS) -D "depth=8" c2p_akiko.s -o c2p_8_akiko.o

c2p8_040_amlaukka.o: c2p8_040_amlaukka.s Makefile
	$(PHXASS) $(PFLAGS) c2p8_040_amlaukka.s

amiga_mmu.o: amiga_mmu.s  Makefile
	$(PHXASS) $(PFLAGS) amiga_mmu.s

amiga_sega.o: amiga_sega.s Makefile
	$(PHXASS) $(PFLAGS) amiga_sega.s

#amiga_net.o: amiga_net.c
#	$(CC) $(CFLAGS) $<

%.o:	%.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.s:	%.c Makefile
	$(CC) $(CFLAGS) -Wa,-adhln -g -c $<  > $@

DoomSnd.h: doomsound.fd doomsound.h Makefile
	fd2pragma --infile doomsound.fd --clib doomsound.h --externc --special 47


test_fixed:	test_fixed.o amiga_fixed.o Makefile
	$(CC) $(CFLAGS) test_fixed.o amiga_fixed.o \
	 $(LDFLAGS) -o $(OUT)test_fixed

#############################################################
#
#############################################################

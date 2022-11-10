#provide these as env variables at commandline
# i.e.  DEBUG=1 make all -j8
DEBUG ?= 0
BINDIR ?= bin
USEIXEMUL ?= 1
PROFILE  ?= 0

CC=m68k-amigaos-gcc
STRIP=m68k-amigaos-strip
PHXASS=vasmm68k_mot

PREFIX = $(shell ./getprefix.sh "$(CC)")

CLIB_CFLAGS = -mcrt=clib2 -DUSECLIB2
CLIB_LDFLAGS = -mcrt=clib2

NOIXEMUL_CFLAGS = -noixemul -DUSENOIXEMUL
NOIXEMUL_LDFLAGS = -noixemul

ifeq ($(USEIXEMUL), 1)
RUNTIME_CFLAGS = $(NOIXEMUL_CFLAGS)
RUNTIME_LDFLAGS = $(NOIXEMUL_LDFLAGS)
else
	RUNTIME_CFLAGS = $(CLIB_CFLAGS)
	RUNTIME_LDFLAGS = $(CLIB_LDFLAGS)
endif

CFLAGS  = $(RUNTIME_CFLAGS)
LDFLAGS = $(RUNTIME_LDFLAGS)

CFLAGS += -fbaserel -m68020-60 -mtune=68030 -msmall-code -mregparm=4 -msoft-float
CFLAGS += -Werror -Wimplicit -Wstrict-prototypes -Wdouble-promotion -fstrict-aliasing

#Always	provide symbols, will be stripped away  for target executable
CFLAGS += -g -ggdb
LDFLAGS += -g -ggdb 

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -Og -ffast-math -fno-omit-frame-pointer
	#-DRANGECHECK
else
	CFLAGS += -DNDEBUG -Os -fstrength-reduce -ffast-math
	ifeq ($(PROFILE), 0)
		CFLAGS +=-fomit-frame-pointer
	endif
endif

CFLAGS += -D__BIG_ENDIAN__ -DNORMALUNIX -DAMIGA -Diabs=abs -D__NO_NET_API -DINDIVISION -include "amiga_macros.h"

CFLAGS_DOOMSOUND := $(CFLAGS)

ifeq ($(PROFILE), 1)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

PFLAGS = -Fhunk -phxass -nosym -ldots -m68030 -linedebug
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
	amiga_main.o \
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
	indivision.o \
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

all: ADoom 
	 $(MAKE) --directory=./camd_tools --file Makefile

clean:
	rm -f *.o
	rm -f ADoom
	$(MAKE) --directory=./camd_tools --file Makefile clean

ADoom: $(OBJS) | DoomSnd.h  Makefile
		$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map=ADoom.map $(OBJS) -o $@
		$(STRIP) --strip-debug --strip-unneeded --strip-all $@ -o $(BINDIR)/$@

profile:
	m68k-amigaos-gprof --brief ADoom $(BINDIR)/gmon.out | gprof2dot.py | dot -s -Tpdf -oADoom.pdf


# FIXME: this is very likely wrong.
#ADoom_SndSrvr: ADoom_SndSrvr.s Makefile
#	$(PHXASS) $(PFLAGS) -o ADoom_SndSrvr ADoom_SndSrvr.s

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

%.o:	%.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.s:	%.c Makefile
	$(CC) $(CFLAGS) -Wa,-adhln -g -c $<  > $@

%.o: %.s | Makefile
	$(PHXASS) $(PFLAGS) $^

%.o: %.ASM | Makefile
	$(PHXASS) $(PFLAGS) -sc $^

test_fixed:	test_fixed.o amiga_fixed.o Makefile
	$(CC) $(CFLAGS) test_fixed.o amiga_fixed.o \
	 $(LDFLAGS) -o $(OUT)test_fixed

#############################################################
#
#############################################################

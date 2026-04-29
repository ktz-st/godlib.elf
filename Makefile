CC = m68k-atari-mintelf-gcc
LD = m68k-atari-mintelf-gcc
AS = vasmm68k_mot
AR = m68k-atari-mintelf-ar

DEFS = -DdGODLIB_FADE # -DdGODLIB_CUTSCENE -DdGODLIB_PACKAGEMANGER -DdGODLIB_EXCEPTION_SCREEN -DdGODLIB_AUDIO_MIXER -DdGODLIB_CLOCK

SOURCES = \
	achieve/ach_com.c achieve/ach_disp.c achieve/ach_gfx.c achieve/ach_god.c achieve/ach_logn.c \
	achieve/ach_main.c achieve/ach_show.c achieve/ach_sign.c achieve/ach_unlk.c \
	assert/assert.c \
	asset/asset.c asset/context.c asset/package.c asset/pkg_dir.c asset/pkg_lnk.c asset/relocate.c \
	audio/amixer.c audio/audio.c audio/rel_spl.c audio/ssd.c \
	base/base.c \
	blitter/blitter.c \
	checksum/checksum.c \
	chunky/chunky.c \
	cli/cli.c \
	clock/clock.c \
	cookie/cookie.c \
	cutscene/cut_sys.c cutscene/cutparse.c cutscene/cutscene.c cutscene/rel_cut.c \
	debug/debug.c \
	debuglog/debuglog.c \
	drive/disk_io.c \
	drive/drive.c \
	elfhash/elfhash.c \
	encrypt/encrypt.c \
	except/except.c \
	fade/fade.c \
	fade/fade_god.c \
	fe/fed_json.c fe/fed.c fe/fedparse.c fe/r_fed.c fe/rel_fed.c \
	file/file_ptn.c file/file.c \
	font/font.c font/rel_bfb.c \
	font8x8/font8x8.c font8x8/fontdata.c \
	gemdos/gemdos.c \
	graphic/graphic.c \
	gui/gui.c gui/guidata.c gui/guiedit.c gui/guifs.c gui/guiparse.c gui/r_gui.c \
	hashlist/hashlist.c \
	hashtree/hashtree.c \
	ikbd/ikbd.c ikbd/ikbd_di.c ikbd/ikbd_sdl.c \
	input/input.c \
	kernel/kernel.c \
	lexer/lexer.c \
	lexer/json.c \
	linea/linea.c \
	linkfile/linkfile.c \
	main/god_main.c \
	memory/heap.c memory/memory.c \
	mfp/mfp.c \
	music/mod.c music/pinknote.c music/snd.c \
	packer/ari_dec.c packer/ari_enc.c packer/brun1.c packer/bwt_dec.c packer/bwt_enc.c packer/godpack.c packer/godpackp.c packer/lz77_dec.c packer/lz77_enc.c packer/lz77bdec.c packer/lz77benc.c packer/mtf_dec.c packer/mtf_enc.c packer/packer.c packer/rle.c packer/rnc.c \
	pictypes/amigaraw.c pictypes/art.c pictypes/canvas.c pictypes/canvasic.c pictypes/colquant.c pictypes/degas.c pictypes/gfx.c pictypes/gif.c pictypes/god.c pictypes/gsm.c pictypes/iff.c pictypes/neo.c pictypes/octtree.c pictypes/rel_gsm.c pictypes/tga.c \
	platform/platform.c \
	profiler/profile.c profiler/profiler.c \
	program/program.c \
	random/random.c \
	reflect/reflect.c \
	registry/registry.c \
	screen/screen.c \
	scrngrab/scrngrab.c \
	sprite/asprite.c sprite/rel_asb.c sprite/rel_bsb.c sprite/rel_rsb.c sprite/rsprite.c sprite/sprite.c \
	string/string.c string/strlist.c string/strpath.c \
	system/system.c \
	tokenise/tokenise.c \
	vbl/vbl.c \
	video/vid_d3d.c video/vid_img.c video/vid_sdl.c video/videl.c video/video.c \
	xbios/xbios.c

SOURCES_S = \
	audio/amixer_s.s \
	audio/audio_s.s \
	audio/ssd_s.s \
	bios/bios_s.s \
	chunky/c2p_s.s \
	chunky/chunky_s.s \
	clock/clock_s.s \
	debuglog/dbglog_s.s \
	except/except_s.s \
	fade/fade_s.s \
	file/file_s.s \
	gemdos/gemdos_s.s \
	graphic/grf_16_s.s \
	graphic/grf_4_s.s \
	graphic/grf_b4_s.s \
	graphic/grf_tc_s.s \
	ikbd/ikbd_s.s \
	linea/linea_s.s \
	memory/memory_s.s \
	mfp/mfp_s.s \
	music/mod_s.s \
	music/pnknot_s.s \
	music/snd_s.s \
	packer/aplib_s.s \
	packer/brun1_s.s \
	packer/lz77_s.s \
	packer/lz77b_s.s \
	packer/packer_s.s \
	packer/rle_s.s \
	packer/rnc1_s.s \
	packer/rnc2_s.s \
	pictypes/gfx_s.s \
	pictypes/iff_s.s \
	profiler/profiles.s \
	program/prog_s.s \
	scrngrab/scrgrabs.s \
	sprite/asprit_s.s \
	sprite/rsprit_s.s \
	system/system_s.s \
	vbl/vbl_s.s \
	vector/vector_s.s \
	video/video_s.s \
	wipe/wipe_s.s \
	xbios/xbios_s.s

OBJECTS = $(SOURCES:.c=.o)
OBJECTS_S = $(SOURCES_S:.s=.o)

CFLAGS = -mshort -mfastcall -Os -Wall -fno-strict-aliasing -static -g $(DEFS) -I..

LDFLAGS =

OUT = libgod.a

$(OUT): $(OBJECTS) $(OBJECTS_S)
	$(AR) rcs $(OUT) $(OBJECTS) $(OBJECTS_S)

$(OBJECTS): %.o: %.c
	 $(CC) -c $(CFLAGS) $< -o $@

$(OBJECTS_S): %.o: %.s
	 $(AS) -nosym -devpac -Felf $< -o $@

clean:
	rm -rf $(OBJECTS_S) $(OBJECTS) $(OUT)

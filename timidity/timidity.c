/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef __W32__
#include <windows.h>
#include <io.h>
#include <shlobj.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <fcntl.h> /* for open */

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif

#ifndef __bool_true_false_are_defined
# ifdef bool
#  undef bool
# endif
# ifdef ture
#  undef ture
# endif
# ifdef false
#  undef false
# endif
# define bool int
# define false ((bool)0)
# define true (!false)
# define __bool_true_false_are_defined true
#endif /* C99 _Bool hack */

#ifdef BORLANDC_EXCEPTION
#include <excpt.h>
#endif /* BORLANDC_EXCEPTION */
#include <signal.h>

#if defined(__FreeBSD__) && !defined(__alpha__)
#include <floatingpoint.h> /* For FP exceptions */
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <ieeefp.h> /* For FP exceptions */
#endif

#include "interface.h"
#include "timidity.h"
#include "utils/tmdy_getopt.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "miditrace.h"
#include "reverb.h"
#ifdef SUPPORT_SOUNDSPEC
#include "soundspec.h"
#endif /* SUPPORT_SOUNDSPEC */
#include "resample.h"
#include "recache.h"
#include "arc.h"
#include "strtab.h"
#include "wrd.h"
#define DEFINE_GLOBALS
#include "mid.defs"
#include "aq.h"
#include "mix.h"
#include "unimod.h"
#include "quantity.h"

#ifdef IA_W32GUI
#include "w32g.h"
#include "w32g_utl.h"
#endif

#ifndef __GNUC__
#define __attribute__(x) /* ignore */
#endif

#define OPTCOMMANDS "4A:aB:b:C:c:D:d:E:eFfg:H:hI:i:jK:k:L:M:m:N:n:O:o:P:p:Q:q:R:rS:s:t:T:UW:w:x:Z:"	/* Only GJluVvXz are remain... */
static const struct option longopts[] = {
#if defined(CSPLINE_INTERPOLATION) || defined(LAGRANGE_INTERPOLATION)
    { "no-4-point-interpolation",       no_argument, NULL, '4' << 8 },
    { "4-point-interpolation"   , optional_argument, NULL, '4' << 8 },
#endif
    { "volume"                  , required_argument, NULL, 'A' << 8 },
    { "drum-power"              , required_argument, NULL, 227 << 8 },
    { "no-auto-volume-conpensation",    no_argument, NULL, 228 << 8 },
    { "auto-volume-conpensation", optional_argument, NULL, 228 << 8 },
    { "buffer-fragments"        , required_argument, NULL, 'B' << 8 },
    { "control-ratio"           , required_argument, NULL, 'C' << 8 },
    { "drums"                   , required_argument, NULL, 'D' << 8 },
    { "effects"                 , required_argument, NULL, 'E' << 8 },
    { "no-fast-panning"         ,       no_argument, NULL, 'F' << 8 },
    { "fast-panning"            , optional_argument, NULL, 'F' << 8 },
    { "keysig"                  , required_argument, NULL, 'H' << 8 },
    { "default-program"         , required_argument, NULL, 'I' << 8 },
    { "key"                     , required_argument, NULL, 'K' << 8 },
    { "patch-path"              , required_argument, NULL, 'L' << 8 },
    { "pcm-file"                , required_argument, NULL, 'M' << 8 },
#if defined(GAUSS_INTERPOLATION) || defined(NEWTON_INTERPOLATION)
    { "interpolation"           , required_argument, NULL, 'N' << 8 },
#endif
    { "mode"                    , required_argument, NULL, 'O' << 8 },
    { "patch"                   , required_argument, NULL, 'P' << 8 },
    { "mute"                    , required_argument, NULL, 'Q' << 8 },
    { "cache-size"              , required_argument, NULL, 'S' << 8 },
    { "tempo"                   , required_argument, NULL, 'T' << 8 },
    { "no-unload-instruments"   ,       no_argument, NULL, 'U' << 8 },
    { "unload-instruments"      , optional_argument, NULL, 'U' << 8 },
    { "freq-table"              , required_argument, NULL, 'Z' << 8 },
    { "wrd"                     , required_argument, NULL, 'W' << 8 },
    { "no-anti-alias"           ,       no_argument, NULL, 'a' << 8 },
    { "anti-alias"              , optional_argument, NULL, 'a' << 8 },
    { "background"              ,       no_argument, NULL, 'b' << 8 },
    { "config"                  , required_argument, NULL, 'c' << 8 },
    { "interface-path"          , required_argument, NULL, 'd' << 8 },
    { "evil"                    , required_argument, NULL, 'e' << 8 },
    { "no-fast-decay"           ,       no_argument, NULL, 'f' << 8 },
    { "fast-decay"              , optional_argument, NULL, 'f' << 8 },
    { "spectrogram"             , required_argument, NULL, 'g' << 8 },
    { "help"                    , optional_argument, NULL, 'h' << 8 },
    { "interface"               , required_argument, NULL, 'i' << 8 },
    { "no-realtime-load"        ,       no_argument, NULL, 'j' << 8 },
    { "realtime-load"           , optional_argument, NULL, 'j' << 8 },
    { "voice-queue"             , required_argument, NULL, 'k' << 8 },
    { "decay-time"              , required_argument, NULL, 'm' << 8 },
    { "output-file"             , required_argument, NULL, 'o' << 8 },
    { "polyphony"               , required_argument, NULL, 'p' << 8 },
    { "audio-buffer"            , required_argument, NULL, 'q' << 8 },
    { "sampling-freq"           , required_argument, NULL, 's' << 8 },
    { "text-convertion"         , required_argument, NULL, 't' << 8 },
    { "version"                 ,       no_argument, NULL, 'v' << 8 },
    { "exec"                    , required_argument, NULL, 'x' << 8 },
    { "bit-width"               , required_argument, NULL, 200 << 8 },
    { "encoding"                , required_argument, NULL, 201 << 8 },
    { "output-signed"           , optional_argument, NULL, 202 << 8 },
    { "output-unsigned"         ,       no_argument, NULL, 202 << 8 },
    { "no-output-byte-swap"     , optional_argument, NULL, 203 << 8 },
    { "output-byte-swap"        ,       no_argument, NULL, 203 << 8 },
    /* 204 missing: I first doubled an option. You can add 204 here */
    { "verbose"                 , optional_argument, NULL, 205 << 8 },
    { "quiet"                   , optional_argument, NULL, 206 << 8 },
    { "no-trace"                ,       no_argument, NULL, 207 << 8 },
    { "trace"                   , optional_argument, NULL, 207 << 8 },
    { "no-loop"                 ,       no_argument, NULL, 208 << 8 },
    { "loop"                    , optional_argument, NULL, 208 << 8 },
    { "no-random"               ,       no_argument, NULL, 209 << 8 },
    { "random"                  , optional_argument, NULL, 209 << 8 },
    { "no-sort-list"            ,       no_argument, NULL, 210 << 8 },
    { "sort-list"               , optional_argument, NULL, 210 << 8 },
    { "default-bank"            , required_argument, NULL, 211 << 8 },
    { "force-bank"              , required_argument, NULL, 212 << 8 },
    { "mid"                     , required_argument, NULL, 213 << 8 },
    { "no-trace-text-meta"      ,       no_argument, NULL, 214 << 8 },
    { "trace-text-meta"         , optional_argument, NULL, 214 << 8 },
    { "no-polyphony-reduction"  ,       no_argument, NULL, 215 << 8 },
    { "polyphony-reduction"     , optional_argument, NULL, 215 << 8 },
    { "no-modulation-wheel"     ,       no_argument, NULL, 216 << 8 },
    { "modulation-wheel"        , optional_argument, NULL, 216 << 8 },
    { "no-portamento"           ,       no_argument, NULL, 217 << 8 },
    { "portamento"              , optional_argument, NULL, 217 << 8 },
    { "no-vibrato"              ,       no_argument, NULL, 218 << 8 },
    { "vibrato"                 , optional_argument, NULL, 218 << 8 },
    { "no-channel-pressure"     ,       no_argument, NULL, 219 << 8 },
    { "channel-pressure"        , optional_argument, NULL, 219 << 8 },
    { "no-new-lpf"              ,       no_argument, NULL, 220 << 8 },
    { "new-lpf"                 , optional_argument, NULL, 220 << 8 },
    { "no-overlap"              ,       no_argument, NULL, 221 << 8 },
    { "overlap"                 , optional_argument, NULL, 221 << 8 },
    { "reverb"                  , required_argument, NULL, 222 << 8 },
    { "chorus"                  , required_argument, NULL, 223 << 8 },
    { "delay"                   , required_argument, NULL, 224 << 8 },
    { "noise-shaper"            , required_argument, NULL, 225 << 8 },
    { "temperament"             , required_argument, NULL, 226 << 8 },
    { "stereo"                  , optional_argument, NULL, 229 << 8 },
    { "mono"                    , optional_argument, NULL, 229 << 8 },
    { "no-auto-start"           ,       no_argument, NULL, 230 << 8 },
    { "auto-start"              , optional_argument, NULL, 230 << 8 },
    { "no-auto-exit"            ,       no_argument, NULL, 231 << 8 },
    { "auto-exit"               , optional_argument, NULL, 231 << 8 },
    { "no-drag-start"           ,       no_argument, NULL, 232 << 8 },
    { "drag-start"              , optional_argument, NULL, 232 << 8 },
    { "no-uniq-list"            ,       no_argument, NULL, 233 << 8 },
    { "uniq-list"               , optional_argument, NULL, 233 << 8 },
    { "no-refine-list"          ,       no_argument, NULL, 234 << 8 },
    { "refine-list"             , optional_argument, NULL, 234 << 8 },
    { "no-continue"             ,       no_argument, NULL, 235 << 8 },
    { "continue"                , optional_argument, NULL, 235 << 8 },
    { "no-modulation-envelope"  ,       no_argument, NULL, 236 << 8 },
    { "modulation-envelope"     , optional_argument, NULL, 236 << 8 },
    { "system-mid"              , required_argument, NULL, 237 << 8 },
    { "wrd-read-opts"           , required_argument, NULL, 238 << 8 },
    { NULL                      ,       no_argument, NULL, '\0'     }
};
#define INTERACTIVE_INTERFACE_IDS "kmqagrwAWP"

/* main interfaces (To be used another main) */
#if defined(main) || defined(ANOTHER_MAIN)
#define MAIN_INTERFACE
#else
#define MAIN_INTERFACE static
#endif /* main */

MAIN_INTERFACE void timidity_start_initialize(void);
MAIN_INTERFACE int timidity_pre_load_configuration(void);
MAIN_INTERFACE int timidity_post_load_configuration(void);
MAIN_INTERFACE void timidity_init_player(void);
MAIN_INTERFACE int timidity_play_main(int nfiles, char **files);
MAIN_INTERFACE int got_a_configuration;
char *wrdt_open_opts = NULL;
char *opt_aq_max_buff = NULL,
     *opt_aq_fill_buff = NULL;
void timidity_init_aq_buff(void);
int opt_control_ratio = 0; /* Save -C option */

MAIN_INTERFACE int set_tim_opt_short(int, char *);
MAIN_INTERFACE bool set_tim_opt_long(int, char *, int);
#if defined(CSPLINE_INTERPOLATION) || defined(LAGRANGE_INTERPOLATION)
static inline bool parse_opt_4(const char *);
#endif
static inline bool parse_opt_A(const char *);
static inline bool parse_opt_A1(const char *);
static inline bool parse_opt_A2(const char *);
static inline bool parse_opt_a(const char *);
static inline bool parse_opt_B(const char *);
static inline bool parse_opt_b(const char *);
static inline bool parse_opt_C(const char *);
static inline bool parse_opt_c(char *);
static inline bool parse_opt_D(const char *);
static inline bool parse_opt_d(const char *);
static inline bool parse_opt_E(char *);
static inline bool parse_opt_216(const char *);	/* --[no-]modulation-wheel */
static inline bool parse_opt_217(const char *);	/* --[no-]portamento */
static inline bool parse_opt_218(const char *);	/* --[no-]vibrato */
static inline bool parse_opt_219(const char *);	/* --[no-]channel-pressure */
static inline bool parse_opt_220(const char *);	/* --[no-]new-lpf */
static inline bool parse_opt_236(const char *);	/* --[no-]modulation-envelope */
static inline bool parse_opt_214(const char *);	/* --[no-]trace-text-meta */
static inline bool parse_opt_221(const char *);	/* --[no-]overlap */
static inline bool parse_opt_213(char *);		/* --mid */
static inline bool parse_opt_237(char *);		/* --system-mid */
static inline bool parse_opt_211(const char *);	/* --default-bank */
static inline bool parse_opt_212(const char *);	/* --force-bank */
static inline bool parse_opt_224(const char *);	/* --delay */
static inline bool parse_opt_222(const char *);	/* --reverb */
static inline bool parse_opt_223(const char *);	/* --chorus */
static inline bool parse_opt_225(const char *);	/* --noise-shaper */
static inline bool parse_opt_e(const char *);
static inline bool parse_opt_F(const char *);
static inline bool parse_opt_f(const char *);
static inline bool parse_opt_g(const char *);
static inline bool parse_opt_H(const char *);
__attribute__((noreturn))
static inline bool parse_opt_h(const char *);
static inline bool parse_opt_I(char *);
static inline bool parse_opt_i(const char *);
static inline bool parse_opt_205(const char *);	/* --verbose */
static inline bool parse_opt_206(const char *);	/* --quiet */
static inline bool parse_opt_207(const char *);	/* --[no-]trace */
static inline bool parse_opt_208(const char *);	/* --[no-]loop */
static inline bool parse_opt_209(const char *);	/* --[no-]random */
static inline bool parse_opt_210(const char *);	/* --[no-]sort */
static inline bool parse_opt_230(const char *);	/* --[no-]auto-start */
static inline bool parse_opt_231(const char *);	/* --[no-]auto-exit */
static inline bool parse_opt_232(const char *);	/* --[no-]drag-start */
static inline bool parse_opt_233(const char *);	/* --[no-]uniq */
static inline bool parse_opt_234(const char *);	/* --[no-]refine */
static inline bool parse_opt_235(const char *);	/* --[no-]continue */
static inline bool parse_opt_j(const char *);
static inline bool parse_opt_K(const char *);
static inline bool parse_opt_k(const char *);
static inline bool parse_opt_L(char *);
static inline bool parse_opt_M(const char *);
static inline bool parse_opt_m(const char *);
#ifdef GAUSS_INTERPOLATION
static inline bool parse_opt_N(const char *);
#elif defined(NEWTON_INTERPOLATION)
static inline bool parse_opt_N(const char *);
#endif
static inline bool parse_opt_O(const char *);
static inline bool parse_opt_200(const char *);	/* --bit-width */
static inline bool parse_opt_201(const char *);	/* --encoding */
static inline bool parse_opt_202(const char *);	/* --output-[un]singed */
static inline bool parse_opt_203(const char *);	/* --[no-]output-byte-swap */
static inline bool parse_opt_229(const char *);	/* --stereo */
static inline bool parse_opt_o(char *);
static inline bool parse_opt_P(const char *);
static inline bool parse_opt_p(const char *);
static inline bool parse_opt_215(const char *);	/* --[no-]polyphony-reduction */
static inline bool parse_opt_Q(const char *);
static inline bool parse_opt_q(const char *);
static inline bool parse_opt_S(const char *);
static inline bool parse_opt_s(const char *);
static inline bool parse_opt_T(const char *);
static inline bool parse_opt_t(const char *);
static inline bool parse_opt_U(const char *);
static inline bool parse_opt_v(const char *);
static inline bool parse_opt_W(const char *);
static inline bool parse_opt_238(char *);		/* --wrd-read-opts */
static inline bool parse_opt_x(char *);
static inline bool parse_opt_Z(char *);
static inline bool parse_opt_226(const char *);	/* --temperament */
__attribute__((noreturn))
static inline bool parse_opt_666(const char *);	/* getopt_long failed to recognize any options */
__attribute__((pure))
static inline bool y_or_n_p(const char *);
static inline bool set_flag(int32 *, int32, const char *);

#ifdef IA_DYNAMIC
MAIN_INTERFACE char dynamic_interface_id;
#endif /* IA_DYNAMIC */

extern StringTable wrd_read_opts;

extern int SecondMode;

extern struct URL_module URL_module_file;
#ifndef __MACOS__
extern struct URL_module URL_module_dir;
#endif /* __MACOS__ */
#ifdef SUPPORT_SOCKET
extern struct URL_module URL_module_http;
extern struct URL_module URL_module_ftp;
extern struct URL_module URL_module_news;
extern struct URL_module URL_module_newsgroup;
#endif /* SUPPORT_SOCKET */
#ifdef HAVE_POPEN
extern struct URL_module URL_module_pipe;
#endif /* HAVE_POPEN */

#ifdef GAUSS_INTERPOLATION
extern double newt_coeffs[58][58];
extern void initialize_newton_coeffs(void);
extern void initialize_gauss_table(int32 gauss_n);
extern int gauss_n;
extern float *gauss_table[(1<<FRACTION_BITS)];
#endif
#ifdef NEWTON_INTERPOLATION
extern double newt_coeffs[58][58];
extern void initialize_newton_coeffs(void);
extern int newt_n, newt_max;
#endif

MAIN_INTERFACE struct URL_module *url_module_list[] =
{
    &URL_module_file,
#ifndef __MACOS__
    &URL_module_dir,
#endif /* __MACOS__ */
#ifdef SUPPORT_SOCKET
    &URL_module_http,
    &URL_module_ftp,
    &URL_module_news,
    &URL_module_newsgroup,
#endif /* SUPPORT_SOCKET */
#if !defined(__MACOS__) && defined(HAVE_POPEN)
    &URL_module_pipe,
#endif
#if defined(main) || defined(ANOTHER_MAIN)
    /* You can put some other modules */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif /* main */
    NULL
};

#ifdef IA_DYNAMIC
#include "dlutils.h"
#ifndef SHARED_LIB_PATH
#define SHARED_LIB_PATH PKGLIBDIR
#endif /* SHARED_LIB_PATH */
static char *dynamic_lib_root = SHARED_LIB_PATH;
#endif /* IA_DYNAMIC */

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

int free_instruments_afterwards=0;
char def_instr_name[256]="";
VOLATILE int intr = 0;

#ifdef __W32__
CRITICAL_SECTION critSect;

#pragma argsused
static BOOL WINAPI handler(DWORD dw)
{
    printf ("***BREAK" NLS); fflush(stdout);
    intr++;
    return TRUE;
}
#endif


int effect_lr_mode = -1;
/* 0: left delay
 * 1: right delay
 * 2: rotate
 * -1: not use
 */
int effect_lr_delay_msec = 25;

extern char* pcm_alternate_file;
/* NULL, "none": disabled (default)
 * "auto":       automatically selected
 * filename:     use the one.
 */

#ifndef atof
extern double atof(const char *);
#endif

#ifndef IA_DYNAMIC
#define dynamic_interface_module(dmy) NULL
#define dynamic_interface_info(dmy) NULL
#else

/* IA_DYNAMIC */
static char *dynamic_interface_info(int id)
{
    static char libinfo[MAXPATHLEN];
    int fd, n;
    char *nl;

    sprintf(libinfo, "%s" PATH_STRING "interface_%c.txt",
	    dynamic_lib_root, id);
    if((fd = open(libinfo, 0)) < 0)
	return NULL;
    n = read(fd, libinfo, sizeof(libinfo) - 1);
    close(fd);

    if(n <= 0)
	return NULL;
    libinfo[n] = '\0';
    nl = strchr(libinfo, '\n');
    if(libinfo == nl)
	return NULL;

    if(nl != NULL)
    {
	*nl = '\0';
	if(*(nl - 1) == '\r')
	    *(nl - 1) = '\0';
    }
    return libinfo;
}

char *dynamic_interface_module(int id)
{
    static char shared_library[MAXPATHLEN];
    int fd;

    sprintf(shared_library, "%s" PATH_STRING "interface_%c%s",
	    dynamic_lib_root, id, SHARED_LIB_EXT);
    if((fd = open(shared_library, 0)) < 0)
	return NULL;
    close(fd);

    return shared_library;
}

static void list_dyna_interface(FILE *fp, char *path, char *mark)
{
    char fname[BUFSIZ];
    URL url;

    if((url = url_dir_open(path)) == NULL)
	return;

    while(url_gets(url, fname, sizeof(fname)) != NULL)
    {
	if(strncmp(fname, "interface_", 10) == 0)
	{
	    char *info;
	    int id;

	    id = fname[10];
	    if(mark[id])
		continue;
	    mark[id] = 1;

	    info = dynamic_interface_info(id);
	    if(info == NULL)
		info = dynamic_interface_module(id);
	    if(info != NULL)
		fprintf(fp, "  -i%c     %s" NLS, id, info);
	}
    }
    url_close(url);
}
#endif /* IA_DYNAMIC */

static FILE *open_pager(void)
{
#if !defined(__MACOS__) && defined(HAVE_POPEN) && defined(HAVE_ISATTY) && !defined(IA_W32GUI) && !defined(IA_W32G_SYN)
    char *pager;
    if(isatty(1) && (pager = getenv("PAGER")) != NULL)
	return popen(pager, "w");
#endif
    return stdout;
}

static void close_pager(FILE *fp)
{
#if !defined(__MACOS__) && defined(HAVE_POPEN) && defined(HAVE_ISATTY) && !defined(IA_W32GUI) && !defined(IA_W32G_SYN)
    if(fp != stdout)
	pclose(fp);
#endif
}

static void help(void)
{
	PlayMode **pmp = play_mode_list;
	ControlMode **cmp = ctl_list;
	WRDTracer **wl = wrdt_list;
	int i, j;
	static char *help_args[3];
	FILE *fp;
	static char *help_list[] = {
" TiMidity++ version %s (C) 1999-2003 Masanao Izumo <mo@goice.co.jp>",
" The original version (C) 1995 Tuukka Toivonen <tt@cgs.fi>",
" TiMidity is free software and comes with ABSOLUTELY NO WARRANTY.",
"",
#ifdef __W32__
" Win32 version by Davide Moretti <dave@rimini.com>",
"              and Daisuke Aoki <dai@y7.net>",
"",
#endif /* __W32__ */
"Usage:",
"  %s [options] filename [...]",
"",
#ifndef __W32__		/*does not work in Windows */
"  Use \"-\" as filename to read a MIDI file from stdin",
"",
#endif
"Options:",
#if defined(CSPLINE_INTERPOLATION) || defined(LAGRANGE_INTERPOLATION)
"  -4      Toggle 4-point interpolation (default on)",
"          Linear interpolation is used if audio queue < 99%%",
#endif
"  -A n,m  Amplify volume by n percent (may cause clipping),",
"          and amplify drum power by m percent",
"     (a)  Toggle amplify compensation (disabled by default)",
"  -a      Enable the antialiasing filter",
"  -B n,m  Set number of buffer fragments(n), and buffer size(2^m)",
"  -C n    Set ratio of sampling and control frequencies",
"  -c file Read extra configuration file",
"  -D n    Play drums on channel n",
#ifdef IA_DYNAMIC
"  -d dir  Set dynamic interface module directory",
#endif /* IA_DYNAMIC */
"  -E mode TiMidity sequencer extensional modes:",
"            mode = w/W : Enable/Disable Modulation wheel.",
"                   p/P : Enable/Disable Portamento.",
"                   v/V : Enable/Disable NRPN Vibrato.",
"                   s/S : Enable/Disable Channel pressure.",
"                   l/L : Enable/Disable voice-by-voice LPF.",
"                   e/E : Enable/Disable Modulation Envelope.",
"                   t/T : Enable/Disable Trace Text Meta Event at playing",
"                   o/O : Enable/Disable Overlapped voice",
"                   m<HH>: Define default Manufacture ID <HH> in two hex",
"                   b<n>: Use tone bank <n> as the default",
"                   B<n>: Always use tone bank <n>",
"                   F<args>: For effect.  See below for effect options.",
"              default: -E "
#ifdef MODULATION_WHEEL_ALLOW
"w"
#else
"W"
#endif /* MODULATION_WHEEL_ALLOW */
#ifdef PORTAMENTO_ALLOW
"p"
#else
"P"
#endif /* PORTAMENTO_ALLOW */
#ifdef NRPN_VIBRATO_ALLOW
"v"
#else
"V"
#endif /* NRPN_VIBRATO_ALLOW */
#ifdef GM_CHANNEL_PRESSURE_ALLOW
"s"
#else
"S"
#endif /* GM_CHANNEL_PRESSURE_ALLOW */
#ifdef VOICE_BY_VOICE_LPF_ALLOW
"l"
#else
"L"
#endif /* VOICE_BY_VOICE_LPF_ALLOW */
#ifdef MODULATION_ENVELOPE_ALLOW
"e"
#else
"E"
#endif /* MODULATION_ENVELOPE_ALLOW */
#ifdef ALWAYS_TRACE_TEXT_META_EVENT
"t"
#else
"T"
#endif /* ALWAYS_TRACE_TEXT_META_EVENT */
#ifdef OVERLAP_VOICE_ALLOW
"o"
#else
"O"
#endif /* OVERLAP_VOICE_ALLOW */
,
#ifdef __W32__
"  -e      Increase thread priority (evil) - be careful!",
#endif
"  -F      Disable/Enable fast panning (toggle on/off, default is on)",
"  -f      "
#ifdef FAST_DECAY
"Disable"
#else
"Enable"
#endif
" fast decay mode (toggle)",
#ifdef SUPPORT_SOUNDSPEC
"  -g sec  Open Sound-Spectrogram Window.",
#endif /* SUPPORT_SOUNDSPEC */
"  -H n    Force keysig number of sHarp(+)/flat(-) (-7..7)",
"  -h      Display this help message",
"  -I n    Use program n as the default",
"  -i mode Select user interface (see below for list)",
"  -j      Realtime load instrument (toggle on/off)",
"  -K n    Adjust key by n half tone (-24..24)",
"  -k msec Specify audio queue time limit to reduce voice",
"  -L dir  Append dir to search path",
"  -M name Specify PCM filename (*.wav or *.aiff) to be played or:",
"          \"auto\": Play *.mid.wav or *.mid.aiff",
"          \"none\": Disable this feature (default)",
"  -m msec Minimum time for a full volume sustained note to decay, 0 disables",
#ifdef GAUSS_INTERPOLATION
"  -N n    n+1 point Gauss-like interpolation, n=1-34 (default 25), 0 disables",
"          Linear interpolation is used if audio queue < 99%%",
#endif
#ifdef NEWTON_INTERPOLATION
"  -N n    n'th order Newton polynomial interpolation, n=1-57 odd, 0 disables",
"          Linear interpolation is used if audio queue < 99%%",
#endif
"  -O mode Select output mode and format (see below for list)",
"  -o file Output to another file (or device/server)  (Use \"-\" for stdout)",
"  -P file Use patch file for all programs",
"  -p n    Allow n-voice polyphony.  Optional auto polyphony reduction toggle.",
"     (a)  Toggle automatic polyphony reduction.  Enabled by default.",
"  -Q n    Ignore channel n (0 means ignore all, -n means resume channel n)",
"     (t)  Quiet temperament type n (0..3: preset, 4..7: user-defined)",
"  -q m/n  Specify audio buffer in seconds",
"            m:Maxmum buffer, n:Filled to start   (default is 5.0/100%%)",
"            (size of 100%% equals device buffer size)",
"  -R n    Pseudo Reveb (set every instrument's release to n ms",
"            if n=0, n is set to 800(default)",
"  -S n    Cache size (0 means no cache)",
"  -s f    Set sampling frequency to f (Hz or kHz)",
"  -T n    Adjust tempo to n%%; 120=play MOD files with an NTSC Amiga's timing",
"  -t code Output text language code:",
"              code=auto  : Auto conversion by `LANG' environment variable",
"                           (UNIX only)",
"                   ascii : Convert unreadable characters to '.'(0x2e)",
"                   nocnv : No conversion",
"                   1251  : Convert from windows-1251 to koi8-r",
#ifdef JAPANESE
"                   euc   : EUC-japan",
"                   jis   : JIS",
"                   sjis  : shift JIS",
#endif /* JAPANESE */
"  -U      Unload instruments from memory between MIDI files",
"  -W mode Select WRD interface (see below for list)",
#ifdef __W32__
"  -w mode Windows extensional modes:",
"              mode=r/R : Enable/Disable rcpcv dll",
#endif /* __W32__ */
"  -x \"configuration-string\"",
"          Read configuration from command line argument",
"  -Z file Load frequency table (Use \"pure\" for pure intonation)",
"     n    Initial keysig number of sharp(+)/flat(-) (-7..7)",
"     (m)  Toggle minor mode",
NULL
};
	
	fp = open_pager();
	j = 0; /* index of help_args */
	help_args[0] = timidity_version;
	help_args[1] = program_name;
	help_args[2] = NULL;
	for (i = 0; help_list[i]; i++) {
		char *h, *p;
		
		h = help_list[i];
		if ((p = strchr(h, '%')) != NULL) {
			if (*(p + 1) != '%')
				fprintf(fp, h, help_args[j++]);
			else
				fprintf(fp, h);
		} else
			fputs(h, fp);
		fputs(NLS, fp);
	}
	fputs(NLS, fp);
	fputs("Available WRD interfaces (-W option):" NLS, fp);
	while (*wl) {
		fprintf(fp, "  -W%c     %s" NLS, (*wl)->id, (*wl)->name);
		wl++;
	}
	fputs(NLS, fp);
	fputs("Available output modes (-O option):" NLS, fp);
	while (*pmp) {
		fprintf(fp, "  -O%c     %s" NLS,
				(*pmp)->id_character, (*pmp)->id_name);
		pmp++;
	}
	fputs(NLS, fp);
	fputs("Output format options (append to -O? option):" NLS
"   `8'    8-bit sample width" NLS
"   `1'    16-bit sample width" NLS
"   `U'    U-Law encoding" NLS
"   `A'    A-Law encoding" NLS
"   `l'    linear encoding" NLS
"   `M'    monophonic" NLS
"   `S'    stereo" NLS
"   `s'    signed output" NLS
"   `u'    unsigned output" NLS
"   `x'    byte-swapped output" NLS, fp);
	fputs(NLS, fp);
	fputs("Available interfaces (-i option):" NLS, fp);
	while (*cmp) {
#ifdef IA_DYNAMIC
		if ((*cmp)->id_character != dynamic_interface_id)
			fprintf(fp, "  -i%c     %s" NLS,
					(*cmp)->id_character, (*cmp)->id_name);
#else
		fprintf(fp, "  -i%c     %s" NLS,
				(*cmp)->id_character, (*cmp)->id_name);
#endif /* IA_DYNAMIC */
		cmp++;
	}
#ifdef IA_DYNAMIC
	fprintf(fp, "Supported dynamic load interfaces (%s):" NLS,
			dynamic_lib_root);
	{
		char mark[128];
		
		memset(mark, 0, sizeof(mark));
		for (cmp = ctl_list; *cmp; cmp++)
			mark[(int)(*cmp)->id_character] = 1;
		if (dynamic_interface_id != 0)
			mark[(int) dynamic_interface_id] = 0;
		list_dyna_interface(fp, dynamic_lib_root, mark);
	}
#endif /* IA_DYNAMIC */
	fputs(NLS, fp);
	fputs("Interface options (append to -i? option):" NLS
"   `v'    more verbose (cumulative)" NLS
"   `q'    quieter (cumulative)" NLS
"   `t'    trace playing" NLS
"   `l'    loop playing (some interface ignore this option)" NLS
"   `r'    randomize file list arguments before playing" NLS
"   `s'    sorting file list arguments before playing" NLS
"   `D'    daemonize timidity++ in background (for alsaseq only)" NLS, fp);
	fputs(NLS, fp);
	fputs("Effect options:" NLS
"  -EFdelay=l : Left delay" NLS
"  -EFdelay=r : Right delay" NLS
"  -EFdelay=b : Rotate left & right" NLS
"  -EFdelay=0 : Disabled delay effect" NLS
"  -EFchorus=0 : Disable MIDI chorus effect control" NLS
"  -EFchorus=1[,level] : Enable MIDI chorus effect control" NLS
"                        `level' is optional to specify chorus level [0..127]" NLS
"                        (default)" NLS
"  -EFchorus=2[,level] : Surround sound, chorus detuned to a lesser degree." NLS
"                        `level' is optional to specify chorus level [0..127]" NLS
"  -EFreverb=0 : Disable MIDI reverb effect control" NLS
"  -EFreverb=1[,level] : Enable MIDI reverb effect control" NLS
"                        `level' is optional to specify reverb level [0..127]" NLS
"                        This effect is only available in stereo" NLS
"  -EFreverb=2 : Global reverb effect" NLS
"  -EFreverb=3 : Enable NEW MIDI reverb effect control (freeverb)" NLS
"                This effect is only available in stereo" NLS
"                (default)" NLS
"  -EFns=n : Enable the n th degree noise shaping filter." NLS
"            n:[0..4] (for 8-bit linear encoding, default is 4)" NLS
"            n:[0..2] (for 16-bit linear encoding, default is 2)" NLS, fp);
	fputs(NLS, fp);
	close_pager(fp);
}

static inline void version(void) {
    const char* const versionstr[] = {
	"TiMidity++ ", timidity_version, NLS,
	NLS,
	"Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>", NLS,
	"Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>", NLS,
	NLS,
#ifdef __W32__
	"Win32 version by Davide Moretti <dmoretti@iper.net>", NLS,
	"             and Daisuke Aoki <dai@y7.net>", NLS,
	NLS,
#endif /* __W32__ */
	"This program is distributed in the hope that it will be useful,", NLS,
	"but WITHOUT ANY WARRANTY; without even the implied warranty of", NLS,
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the", NLS,
	"GNU General Public License for more details.", NLS,
    };

    FILE* fp = open_pager();
    int i;
    for(i=0;i<sizeof(versionstr)/sizeof(char*);i++) {
	fputs(versionstr[i],fp);
    }
    close_pager(fp);
}

static void interesting_message(void)
{
  printf(
NLS
" TiMidity++ version %s -- MIDI to WAVE converter and player" NLS
" Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>" NLS
" Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>" NLS
NLS
#ifdef __W32__
" Win32 version by Davide Moretti <dmoretti@iper.net>" NLS
"              and Daisuke Aoki <dai@y7.net>" NLS
NLS
#endif /* __W32__ */
" This program is free software; you can redistribute it and/or modify" NLS
" it under the terms of the GNU General Public License as published by" NLS
" the Free Software Foundation; either version 2 of the License, or" NLS
" (at your option) any later version." NLS
NLS
" This program is distributed in the hope that it will be useful," NLS
" but WITHOUT ANY WARRANTY; without even the implied warranty of" NLS
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" NLS
" GNU General Public License for more details." NLS
NLS
" You should have received a copy of the GNU General Public License" NLS
" along with this program; if not, write to the Free Software" NLS
" Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA" NLS
NLS,
timidity_version
);
}

static int set_channel_flag(ChannelBitMask *flags, int32 i, char *name)
{
    if(i == 0)
	FILL_CHANNELMASK(*flags);
    else if((i < 1 || i > MAX_CHANNELS) && (i < -MAX_CHANNELS || i > -1))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"%s must be between 1 and %d, or between -1 and -%d, or 0"
		NLS, name, MAX_CHANNELS, MAX_CHANNELS);
	return -1;
    }
    else
    {
	if(i > 0)
	    SET_CHANNELMASK(*flags, i - 1);
	else
	    UNSET_CHANNELMASK(*flags, -i - 1);
    }
    return 0;
}

static int set_value(int32 *param, int32 i, int32 low, int32 high, char *name)
{
  if (i<low || i > high)
    {
      	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s must be between %ld and %ld",
		  name, low, high);
	return -1;
    }
  else *param=i;
  return 0;
}

MAIN_INTERFACE int set_default_prog(char *opt)
{
    int prog, ch;
    char *p;

    prog = atoi(opt);
    if(prog < 0 || prog > 127)
    {
      	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Default program must be between 0 and 127");
	return -1;
    }

    p = strchr(opt, '/');
    if(p == NULL)
	for(ch = 0; ch < MAX_CHANNELS; ch++)
	    default_program[ch] = prog;
    else
    {
	ch = atoi(p + 1) - 1;
	if(ch < 0 || ch >= MAX_CHANNELS)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "Default program channel must be between 1 and %d",
		      MAX_CHANNELS);
	    return -1;
	}
	default_program[ch] = prog;
    }
    return 0;
}

int set_play_mode(char *cp)
{
    PlayMode *pmp, **pmpp=play_mode_list;

    while((pmp=*pmpp++))
    {
	if(pmp->id_character == *cp)
	{
	    play_mode=pmp;
	    while(*(++cp))
		switch(*cp)
		{
		  case 'U': /* uLaw */
		    pmp->encoding |= PE_ULAW;
		    pmp->encoding &= ~(PE_ALAW|PE_16BIT|PE_SIGNED|PE_BYTESWAP);
		    break;
		  case 'A': /* aLaw */
		    pmp->encoding |= PE_ALAW;
		    pmp->encoding &= ~(PE_ULAW|PE_16BIT|PE_SIGNED|PE_BYTESWAP);
		    break;
		  case 'l': /* linear */
		    pmp->encoding &= ~(PE_ULAW|PE_ALAW);
		    break;
		  case '1': /* 1 for 16-bit */
		    pmp->encoding |= PE_16BIT;
		    pmp->encoding &= ~(PE_ULAW|PE_ALAW);
		    break;
		  case '8': pmp->encoding &= ~PE_16BIT; break;

		  case 'M': pmp->encoding |= PE_MONO; break;
		  case 'S': pmp->encoding &= ~PE_MONO; break; /* stereo */

		  case 's':
		    pmp->encoding |= PE_SIGNED;
		    pmp->encoding &= ~(PE_ULAW|PE_ALAW);
		    break;

		  case 'u':
		    pmp->encoding &= ~PE_SIGNED;
		    pmp->encoding &= ~(PE_ULAW|PE_ALAW);
		    break;

		  case 'x':
		    pmp->encoding ^= PE_BYTESWAP; /* toggle */
		    pmp->encoding &= ~(PE_ULAW|PE_ALAW);
		    break; 

		  default:
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "Unknown format modifier `%c'", *cp);
		    return 1;
		}
	    return 0;
	}
    }

    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "Playmode `%c' is not compiled in.", *cp);
    return 1;
}

int set_ctl(char *cp)
{
    ControlMode *cmp, **cmpp = ctl_list;

    while((cmp = *cmpp++))
    {
	if(cmp->id_character == *cp)
	{
	    ctl = cmp;

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
	    cmp->verbosity = 1;
	    cmp->trace_playing = 0;
	    cmp->flags = 0;
#endif /* IA_W32GUI */

	    while(*(++cp))
	    {
		switch(*cp)
		{
		  case 'v': cmp->verbosity++; break;
		  case 'q': cmp->verbosity--; break;
		  case 't': /* toggle */
		    cmp->trace_playing = !cmp->trace_playing;
		    break;
		  case 'l':
		    cmp->flags ^= CTLF_LIST_LOOP;
		    break;
		  case 'r':
		    cmp->flags ^= CTLF_LIST_RANDOM;
		    break;
		  case 's':
		    cmp->flags ^= CTLF_LIST_SORT;
		    break;
		  case 'a':
		    cmp->flags ^= CTLF_AUTOSTART;
		    break;
		  case 'x':
		    cmp->flags ^= CTLF_AUTOEXIT;
		    break;
		  case 'd':
		    cmp->flags ^= CTLF_DRAG_START;
		    break;
		  case 'D':
		    cmp->flags ^= CTLF_DAEMONIZE;
		    break;
		  case 'u':
		    cmp->flags ^= CTLF_AUTOUNIQ;
		    break;
		  case 'R':
		    cmp->flags ^= CTLF_AUTOREFINE;
		    break;
		  case 'C':
		    cmp->flags ^= CTLF_NOT_CONTINUE;
		    break;
		  default:
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "Unknown interface option `%c'", *cp);
		    return 1;
		}
	    }
	    return 0;
	}
#ifdef IA_DYNAMIC
	else if(cmp->id_character == dynamic_interface_id) /* Dynamic interface loader */
	{
	    if(dynamic_interface_module(*cp) != NULL)
	    {
		ctl = cmp;
		if(dynamic_interface_id != *cp)
		{
		    cmp->verbosity = 1;
		    cmp->trace_playing = 0;
		    cmp->flags = 0;
		    cmp->id_character = dynamic_interface_id = *cp;
		}
		while (*(++cp))
		{
		    switch(*cp)
		    {
		      case 'v': cmp->verbosity++; break;
		      case 'q': cmp->verbosity--; break;
		      case 't': /* toggle */
			cmp->trace_playing = !cmp->trace_playing;
			break;
		      case 'l':
			cmp->flags ^= CTLF_LIST_LOOP;
			break;
		      case 'r':
			cmp->flags ^= CTLF_LIST_RANDOM;
			break;
		      case 's':
			cmp->flags ^= CTLF_LIST_SORT;
			break;
		      case 'a':
			cmp->flags ^= CTLF_AUTOSTART;
			break;
		      case 'x':
			cmp->flags ^= CTLF_AUTOEXIT;
			break;
		      case 'D':
			cmp->flags ^= CTLF_DAEMONIZE;
			break;

		      default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "Unknown interface option `%c'", *cp);
			return 1;
		    }
		}
		return 0;
	    }
	}
#endif /* IA_DYNAMIC */
    }

    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "Interface `%c' is not compiled in.", *cp);
    return 1;
}

int set_wrd(char *w)
{
    WRDTracer **wl = wrdt_list;

    if(*w == 'R') /* for WRD reader options */
    {
	w++;
	put_string_table(&wrd_read_opts, w, strlen(w));
	return 0;
    }

    while(*wl)
    {
	if((*wl)->id == *w)
	{
	    wrdt = *wl;
	    if(wrdt_open_opts != NULL)
		free(wrdt_open_opts);
	    wrdt_open_opts = safe_strdup(w + 1);
	    return 0;
	}
	wl++;
    }

    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "WRD Tracer `%c' is not compiled in.", *w);
    return 1;
}

static void copybank(ToneBank *to, ToneBank *from)
{
    int i;

    if(from == NULL)
	return;
    for(i = 0; i < 128; i++)
    {
	ToneBankElement *toelm, *fromelm;

	toelm   = &to->tone[i];
	fromelm = &from->tone[i];

	if(fromelm->name == NULL)
	    continue;

	if(toelm->name)
	    free(toelm->name);
	toelm->name = NULL;
	if(toelm->comment)
	    free(toelm->comment);
	toelm->comment = NULL;
	memcpy(toelm, fromelm, sizeof(ToneBankElement));
	if(toelm->name)
	    toelm->name = safe_strdup(toelm->name);
	if(toelm->comment)
	    toelm->comment = safe_strdup(toelm->comment);
	toelm->instrument = NULL;
	toelm->tune = NULL;
	toelm->tunenum = 0;
	toelm->envrate = toelm->envofs = NULL;
	toelm->envratenum = toelm->envofsnum = 0;
	toelm->trem = toelm->vib = NULL;
	toelm->tremnum = toelm->vibnum = 0;
	toelm->instype = 0;
    }
}

static float *config_parse_tune(const char *cp, int *num)
{
	const char *p;
	float *tune_list;
	int i;
	
	/* count num */
	*num = 1, p = cp;
	while (p = strchr(p, ','))
		(*num)++, p++;
	/* alloc */
	tune_list = (float *) safe_malloc((*num) * sizeof(float));
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		tune_list[i] = atof(p);
		if (! (p = strchr(p, ',')))
			break;
	}
	return tune_list;
}

static int **config_parse_envelope(const char *cp, int *num)
{
	const char *p, *px;
	int **env_list;
	int i, j;
	
	/* count num */
	*num = 1, p = cp;
	while (p = strchr(p, ','))
		(*num)++, p++;
	/* alloc */
	env_list = (int **) safe_malloc((*num) * sizeof(int *));
	for (i = 0; i < *num; i++)
		env_list[i] = (int *) safe_malloc(6 * sizeof(int));
	/* init */
	for (i = 0; i < *num; i++)
		for (j = 0; j < 6; j++)
			env_list[i][j] = -1;
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		px = strchr(p, ',');
		for (j = 0; j < 6; j++, p++) {
			if (*p == ':')
				continue;
			env_list[i][j] = atoi(p);
			if (! (p = strchr(p, ':')))
				break;
			if (px && p > px)
				break;
		}
		if (! (p = px))
			break;
	}
	return env_list;
}

static Quantity **config_parse_modulation(const char *name, int line, const char *cp, int *num, int mod_type)
{
	const char *p, *px, *err;
	char buf[128], *delim;
	Quantity **mod_list;
	int i, j;
	static const char * qtypestr[] = {"tremolo", "vibrato"};
	static const uint16 qtypes[] = {
		QUANTITY_UNIT_TYPE(TREMOLO_SWEEP), QUANTITY_UNIT_TYPE(TREMOLO_RATE), QUANTITY_UNIT_TYPE(DIRECT_INT),
		QUANTITY_UNIT_TYPE(VIBRATO_SWEEP), QUANTITY_UNIT_TYPE(VIBRATO_RATE), QUANTITY_UNIT_TYPE(DIRECT_INT)
	};
	
	/* count num */
	*num = 1, p = cp;
	while (p = strchr(p, ','))
		(*num)++, p++;
	/* alloc */
	mod_list = (Quantity **) safe_malloc((*num) * sizeof(Quantity *));
	for (i = 0; i < *num; i++)
		mod_list[i] = (Quantity *) safe_malloc(3 * sizeof(Quantity));
	/* init */
	for (i = 0; i < *num; i++)
		for (j = 0; j < 3; j++)
			INIT_QUANTITY(mod_list[i][j]);
	buf[sizeof buf - 1] = '\0';
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		px = strchr(p, ',');
		for (j = 0; j < 3; j++, p++) {
			if (*p == ':')
				continue;
			if ((delim = strpbrk(strncpy(buf, p, sizeof buf - 1), ":,")) != NULL)
				*delim = '\0';
			if (*buf != '\0' && (err = string_to_quantity(buf, &mod_list[i][j], qtypes[mod_type * 3 + j])) != NULL) {
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: %s: parameter %d of item %d: %s (%s)",
						name, line, qtypestr[mod_type], j+1, i+1, err, buf);
				free_ptr_list(mod_list, *num);
				mod_list = NULL;
				*num = 0;
				return NULL;
			}
			if (! (p = strchr(p, ':')))
				break;
			if (px && p > px)
				break;
		}
		if (! (p = px))
			break;
	}
	return mod_list;
}

static int set_gus_patchconf_opts(char *name, int line, char *opts,
				  ToneBankElement *tone)
{
    char *cp;
    int k;

    if(!(cp = strchr(opts, '=')))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: line %d: bad patch option %s",
		  name, line, opts);
	return 1;
    }

    *cp++ = 0;
    if(!strcmp(opts, "amp"))
    {
	k = atoi(cp);
	if((k < 0 || k > MAX_AMPLIFICATION) ||
	   (*cp < '0' || *cp > '9'))
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: amplification must be between "
		      "0 and %d", name, line, MAX_AMPLIFICATION);
	    return 1;
	}
	tone->amp = k;
    }
    else if(!strcmp(opts, "note"))
    {
	k = atoi(cp);
	if((k < 0 || k > 127) || (*cp < '0' || *cp > '9'))
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: note must be between 0 and 127",
		      name, line);
	    return 1;
	}
	tone->note = k;
    }
    else if(!strcmp(opts, "pan"))
    {
	if(!strcmp(cp, "center"))
	    k = 64;
	else if(!strcmp(cp, "left"))
	    k = 0;
	else if(!strcmp(cp, "right"))
	    k = 127;
	else
	    k = ((atoi(cp) + 100) * 100) / 157;
	if((k < 0 || k > 127) ||
	   (k == 0 && *cp != '-' && (*cp < '0' || *cp > '9')))
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: panning must be left, right, "
		      "center, or between -100 and 100",
		      name, line);
	    return 1;
	}
	tone->pan = k;
    }
    else if(!strcmp(opts, "keep"))
    {
	if(!strcmp(cp, "env"))
	    tone->strip_envelope = 0;
	else if(!strcmp(cp, "loop"))
	    tone->strip_loop = 0;
	else
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: keep must be env or loop",
		      name, line);
	    return 1;
	}
    }
    else if(!strcmp(opts, "strip"))
    {
	if(!strcmp(cp, "env"))
	    tone->strip_envelope = 1;
	else if(!strcmp(cp, "loop"))
	    tone->strip_loop = 1;
	else if(!strcmp(cp, "tail"))
	    tone->strip_tail = 1;
	else
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: strip must be "
		      "env, loop, or tail", name, line);
	    return 1;
	}
    }
    else if(!strcmp(opts, "comm"))
    {
	char *p;
	if(tone->comment)
	    free(tone->comment);
	p = tone->comment = safe_strdup(cp);
	while(*p)
	{
	    if(*p == ',') *p = ' ';
	    p++;
	}
    }
    else if(!strcmp(opts, "tune")) {
      tone->tune = config_parse_tune(cp, &tone->tunenum);
	} else if (! strcmp(opts, "rate"))
		tone->envrate = config_parse_envelope(cp, &tone->envratenum);
	else if (! strcmp(opts, "offset"))
		tone->envofs = config_parse_envelope(cp, &tone->envofsnum);
	else if (! strcmp(opts, "tremolo"))
	{
		if ((tone->trem = config_parse_modulation(name, line, cp, &tone->tremnum, 0)) == NULL)
			return 1;
	}
	else if (! strcmp(opts, "vibrato"))
	{
		if ((tone->vib = config_parse_modulation(name, line, cp, &tone->vibnum, 1)) == NULL)
			return 1;
	}
    else
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: line %d: bad patch option %s",
		  name, line, opts);
	return 1;
    }
    return 0;
}


static void reinit_tone_bank_element(ToneBankElement *tone)
{
    if (tone->name)
    {
	free(tone->name);
	tone->name = NULL;
    }
    if (tone->tunenum)
    {
	tone->tunenum = 0;
	free(tone->tune);
	tone->tune = NULL;
    }
    if (tone->envratenum)
    {
	free_ptr_list(tone->envrate, tone->envratenum);
	tone->envratenum = 0;
	tone->envrate = NULL;
    }
    if (tone->envofsnum)
    {
	free_ptr_list(tone->envofs, tone->envofsnum);
	tone->envofsnum = 0;
	tone->envofs = NULL;
    }
    if (tone->tremnum)
    {
	free_ptr_list(tone->trem, tone->tremnum);
	tone->tremnum = 0;
	tone->trem = NULL;
    }
    if (tone->vibnum)
    {
	free_ptr_list(tone->vib, tone->vibnum);
	tone->vibnum = 0;
	tone->vib = NULL;
    }
    tone->note = tone->pan =
	tone->strip_loop = tone->strip_envelope =
	tone->strip_tail = -1;
    tone->amp = -1;
    tone->legato = 0;
    tone->tva_level = -1;
}

#define SET_GUS_PATCHCONF_COMMENT
static int set_gus_patchconf(char *name, int line,
			     ToneBankElement *tone, char *pat, char **opts)
{
    int j;
#ifdef SET_GUS_PATCHCONF_COMMENT
		char *old_name = NULL;

		if(tone != NULL && tone->name != NULL)
			old_name = safe_strdup(tone->name);
#endif
    reinit_tone_bank_element(tone);

    if(strcmp(pat, "%font") == 0) /* Font extention */
    {
	/* %font filename bank prog [note-to-use]
	 * %font filename 128 bank key
	 */

	if(opts[0] == NULL || opts[1] == NULL || opts[2] == NULL ||
	   (atoi(opts[1]) == 128 && opts[3] == NULL))
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: Syntax error", name, line);
	    return 1;
	}
	tone->name = safe_strdup(opts[0]);
	tone->instype = 1;
	if(atoi(opts[1]) == 128) /* drum */
	{
	    tone->font_bank = 128;
	    tone->font_preset = atoi(opts[2]);
	    tone->font_keynote = atoi(opts[3]);
	    opts += 4;
	}
	else
	{
	    tone->font_bank = atoi(opts[1]);
	    tone->font_preset = atoi(opts[2]);

	    if(opts[3] && isdigit(opts[3][0]))
	    {
		tone->font_keynote = atoi(opts[3]);
		opts += 4;
	    }
	    else
	    {
		tone->font_keynote = -1;
		opts += 3;
	    }
	}
    }
    else if(strcmp(pat, "%sample") == 0) /* Sample extention */
    {
	/* %sample filename */

	if(opts[0] == NULL)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: Syntax error", name, line);
	    return 1;
	}
	tone->name = safe_strdup(opts[0]);
	tone->instype = 2;
	opts++;
    }
    else
    {
	tone->instype = 0;
	tone->name = safe_strdup(pat);
    }

    for(j = 0; opts[j] != NULL; j++)
    {
	int err;
	if((err = set_gus_patchconf_opts(name, line, opts[j], tone)) != 0)
	    return err;
    }
#ifdef SET_GUS_PATCHCONF_COMMENT
		if(tone->comment == NULL ||
			(old_name != NULL && strcmp(old_name,tone->comment) == 0))
		{
			if(tone->comment != NULL )
				free(tone->comment);
			tone->comment = safe_strdup(tone->name);
		}
		if(old_name != NULL)
			free(old_name);
#else
    if(tone->comment == NULL)
	tone->comment = safe_strdup(tone->name);
#endif
    return 0;
}

static int mapname2id(char *name, int *isdrum)
{
    if(strcmp(name, "sc55") == 0)
    {
	*isdrum = 0;
	return SC_55_TONE_MAP;
    }

    if(strcmp(name, "sc55drum") == 0)
    {
	*isdrum = 1;
	return SC_55_DRUM_MAP;
    }

    if(strcmp(name, "sc88") == 0)
    {
	*isdrum = 0;
	return SC_88_TONE_MAP;
    }

    if(strcmp(name, "sc88drum") == 0)
    {
	*isdrum = 1;
	return SC_88_DRUM_MAP;
    }

    if(strcmp(name, "sc88pro") == 0)
    {
	*isdrum = 0;
	return SC_88PRO_TONE_MAP;
    }

    if(strcmp(name, "sc88prodrum") == 0)
    {
	*isdrum = 1;
	return SC_88PRO_DRUM_MAP;
    }

    if(strcmp(name, "xg") == 0)
    {
	*isdrum = 0;
	return XG_NORMAL_MAP;
    }

    if(strcmp(name, "xgsfx64") == 0)
    {
	*isdrum = 0;
	return XG_SFX64_MAP;
    }

    if(strcmp(name, "xgsfx126") == 0)
    {
	*isdrum = 1;
	return XG_SFX126_MAP;
    }

    if(strcmp(name, "xgdrum") == 0)
    {
	*isdrum = 1;
	return XG_DRUM_MAP;
    }
    return -1;
}

/* string[0] should not be '#' */
static int strip_trailing_comment(char *string, int next_token_index)
{
    if (string[next_token_index - 1] == '#'	/* strip \1 in /^\S+(#*[ \t].*)/ */
	&& (string[next_token_index] == ' ' || string[next_token_index] == '\t'))
    {
	string[next_token_index] = '\0';	/* new c-string terminator */
	while(string[--next_token_index - 1] == '#')
	    ;
    }
    return next_token_index;
}

#define MAXWORDS 130
#define CHECKERRLIMIT \
  if(++errcnt >= 10) { \
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, \
      "Too many errors... Give up read %s", name); \
    close_file(tf); return 1; }

MAIN_INTERFACE int read_config_file(char *name, int self)
{
    struct timidity_file *tf;
    char tmp[1024], *w[MAXWORDS + 1], *cp;
    ToneBank *bank = NULL;
    int i, j, k, line = 0, words, errcnt = 0;
    static int rcf_count = 0;
    int dr = 0, bankno = 0;
    int extension_flag, param_parse_err;

    if(rcf_count > 50)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Probable source loop in configuration files");
	return 2;
    }

    if(self)
    {
	tf = open_with_mem(name, (int32)strlen(name), OF_VERBOSE);
	name = "(configuration)";
    }
    else
	tf = open_file(name, 1, OF_VERBOSE);
    if(tf == NULL)
	return 1;

    errno = 0;
    while(tf_gets(tmp, sizeof(tmp), tf))
    {
	line++;
	if(strncmp(tmp, "#extension", 10) == 0) {
	    extension_flag = 1;
	    i = 10;
	}
	else
	{
	    extension_flag = 0;
	    i = 0;
	}

	while(isspace(tmp[i]))			/* skip /^\s*(?#)/ */
	    i++;
	if (tmp[i] == '#' || tmp[i] == '\0')	/* /^#|^$/ */
	    continue;
	j = strcspn(tmp + i, " \t\r\n\240");
	if (j == 0)
		j = strlen(tmp + i);
	j = strip_trailing_comment(tmp + i, j);
	tmp[i + j] = '\0';			/* terminate the first token */
	w[0] = tmp + i;
	i += j + 1;
	words = param_parse_err = 0;
	while(words < MAXWORDS - 1)		/* -1 : next arg */
	{
	    char *terminator;

	    while(isspace(tmp[i]))		/* skip /^\s*(?#)/ */
		i++;
	    if (tmp[i] == '\0'
		    || tmp[i] == '#')		/* /\s#/ */
		break;
	    if ((tmp[i] == '"' || tmp[i] == '\'')
		    && (terminator = strchr(tmp + i + 1, tmp[i])) != NULL)
	    {
		if (!isspace(terminator[1]) && terminator[1] != '\0')
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			"%s: line %d: there must be at least one whitespace between "
			"string terminator (%c) and the next parameter", name, line, tmp[i]);
		    CHECKERRLIMIT;
		    param_parse_err = 1;
		    break;
		}
		w[++words] = tmp + i + 1;
		i = terminator - tmp + 1;
		*terminator = '\0';
	    }
	    else	/* not terminated */
	    {
		j = strcspn(tmp + i, " \t\r\n\240");
		if (j > 0)
		    j = strip_trailing_comment(tmp + i, j);
		w[++words] = tmp + i;
		i += j;
		if (tmp[i] != '\0')		/* unless at the end-of-string (i.e. EOF) */
		    tmp[i++] = '\0';		/* terminate the token */
	    }
	}
	if (param_parse_err)
	    continue;
	w[++words] = NULL;

	/*
	 * #extension [something...]
	 */

	/* #extension comm program comment */
	if(strcmp(w[0], "comm") == 0)
	{
	    char *p;

	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum "
			  "set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension comm must be "
			  "between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(bank->tone[i].comment)
		free(bank->tone[i].comment);
	    p = bank->tone[i].comment = safe_strdup(w[2]);
	    while(*p)
	    {
		if(*p == ',') *p = ' ';
		p++;
	    }
	}
	/* #extension timeout program sec */
	else if(strcmp(w[0], "timeout") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension timeout "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    bank->tone[i].loop_timeout = atoi(w[2]);
	}
	/* #extension copydrumset drumset */
	else if(strcmp(w[0], "copydrumset") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No copydrumset number given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension copydrumset "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or "
			  "drum set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    copybank(bank, drumset[i]);
	}
	/* #extension copybank bank */
	else if(strcmp(w[0], "copybank") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No copybank number given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension copybank "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or "
			  "drum set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    copybank(bank, tonebank[i]);
	}
	/* #extension HTTPproxy hostname:port */
	else if(strcmp(w[0], "HTTPproxy") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No proxy name given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    /* If network is not supported, this extension is ignored. */
#ifdef SUPPORT_SOCKET
	    url_http_proxy_host = safe_strdup(w[1]);
	    if((cp = strchr(url_http_proxy_host, ':')) == NULL)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    *cp++ = '\0';
	    if((url_http_proxy_port = atoi(cp)) <= 0)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Port number must be "
			  "positive number", name, line);
		CHECKERRLIMIT;
		continue;
	    }
#endif
	}
	/* #extension FTPproxy hostname:port */
	else if(strcmp(w[0], "FTPproxy") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No proxy name given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    /* If network is not supported, this extension is ignored. */
#ifdef SUPPORT_SOCKET
	    url_ftp_proxy_host = safe_strdup(w[1]);
	    if((cp = strchr(url_ftp_proxy_host, ':')) == NULL)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    *cp++ = '\0';
	    if((url_ftp_proxy_port = atoi(cp)) <= 0)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Port number "
			  "must be positive number", name, line);
		CHECKERRLIMIT;
		continue;
	    }
#endif
	}
	/* #extension mailaddr somebody@someware.domain.com */
	else if(strcmp(w[0], "mailaddr") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No mail address given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(strchr(w[1], '@') == NULL) {
		ctl->cmsg(CMSG_WARNING, VERB_NOISY,
			  "%s: line %d: Warning: Mail address %s is not valid",
			  name, line);
	    }

	    /* If network is not supported, this extension is ignored. */
#ifdef SUPPORT_SOCKET
	    user_mailaddr = safe_strdup(w[1]);
#endif /* SUPPORT_SOCKET */
	}
	/* #extension opt [-]{option}[optarg] */
	else if(strcmp(w[0], "opt") == 0)
	{
	    int c, err;
	    char *arg, *cmd, *p;

	    if(words != 2 && words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    cmd = w[1];
	    if(*cmd == '-')
		cmd++;
	    c = *cmd;
	    p = strchr(OPTCOMMANDS, c);
	    if(p == NULL)
		err = 1;
	    else
	    {
		if(*(p + 1) == ':')
		{
		    if(words == 2)
			arg = cmd + 1;
		    else
			arg = w[2];
		}
		else
		    arg = "";
		err = set_tim_opt_short(c, arg);
	    }
	    if(err)
	    {
		/* error */
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid command line option",
			  name, line);
		errcnt += err - 1;
		CHECKERRLIMIT;
		continue;
	    }
	}
	/* #extension undef program */
	else if(strcmp(w[0], "undef") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No undef number given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension undef "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or "
			  "drum set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(bank->tone[i].name)
	    {
		free(bank->tone[i].name);
		    bank->tone[i].name = NULL;
	    }
	    if(bank->tone[i].comment)
	    {
		free(bank->tone[i].comment);
		bank->tone[i].comment = NULL;
	    }
			bank->tone[i].instype = 0;
	}
	/* #extension altassign numbers... */
	else if(strcmp(w[0], "altassign") == 0)
	{
	    ToneBank *bk;

	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before altassign", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No alternate assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    if(!dr) {
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: line %d: Warning: Not a drumset altassign"
			  " (ignored)",
			  name, line);
		continue;
	    }

	    bk = drumset[bankno];
	    bk->alt = add_altassign_string(bk->alt, w + 1, words - 1);
	}	/* #extension legato [program] [0 or 1] */
	else if(strcmp(w[0], "legato") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension legato "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    bank->tone[i].legato = atoi(w[2]);
	}	/* #extension level program tva_level */
	else if(strcmp(w[0], "level") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension level "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
		bank->tone[i].tva_level = atoi(w[2]);
	}
	else if(!strcmp(w[0], "soundfont"))
	{
	    int order, cutoff, isremove, reso, amp;
	    char *sf_file;

	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No soundfont file given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    sf_file = w[1];
	    order = cutoff = reso = amp = -1;
	    isremove = 0;
	    for(j = 2; j < words; j++)
	    {
		if(strcmp(w[j], "remove") == 0)
		{
		    isremove = 1;
		    break;
		}
		if(!(cp = strchr(w[j], '=')))
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: bad patch option %s",
			      name, line, w[j]);
		    CHECKERRLIMIT;
		    break;
		}
		*cp++=0;
		k = atoi(cp);
		if(!strcmp(w[j], "order"))
		{
		    if(k < 0 || (*cp < '0' || *cp > '9'))
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "%s: line %d: order must be a digit",
				  name, line);
			CHECKERRLIMIT;
			break;
		    }
		    order = k;
		}
		else if(!strcmp(w[j], "cutoff"))
		{
		    if(k < 0 || (*cp < '0' || *cp > '9'))
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "%s: line %d: cutoff must be a digit",
				  name, line);
			CHECKERRLIMIT;
			break;
		    }
		    cutoff = k;
		}
		else if(!strcmp(w[j], "reso"))
		{
		    if(k < 0 || (*cp < '0' || *cp > '9'))
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "%s: line %d: reso must be a digit",
				  name, line);
			CHECKERRLIMIT;
			break;
		    }
		    reso = k;
		}
		else if(!strcmp(w[j], "amp"))
		{
		    amp = k;
		}
	    }
	    if(isremove)
		remove_soundfont(sf_file);
	    else
		add_soundfont(sf_file, order, cutoff, reso, amp);
	}
	else if(!strcmp(w[0], "font"))
	{
	    int bank, preset, keynote;
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: no font command", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!strcmp(w[1], "exclude"))
	    {
		if(words < 3)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No bank/preset/key is given",
			      name, line);
		    CHECKERRLIMIT;
		    continue;
		}
		bank = atoi(w[2]);
		if(words >= 4)
		    preset = atoi(w[3]) - progbase;
		else
		    preset = -1;
		if(words >= 5)
		    keynote = atoi(w[4]);
		else
		    keynote = -1;
		if(exclude_soundfont(bank, preset, keynote))
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No soundfont is given",
			      name, line);
		    CHECKERRLIMIT;
		}
	    }
	    else if(!strcmp(w[1], "order"))
	    {
		int order;
		if(words < 4)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No order/bank is given",
			      name, line);
		    CHECKERRLIMIT;
		    continue;
		}
		order = atoi(w[2]);
		bank = atoi(w[3]);
		if(words >= 5)
		    preset = atoi(w[4]) - progbase;
		else
		    preset = -1;
		if(words >= 6)
		    keynote = atoi(w[5]);
		else
		    keynote = -1;
		if(order_soundfont(bank, preset, keynote, order))
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No soundfont is given",
			      name, line);
		    CHECKERRLIMIT;
		}
	    }
	}
	else if(!strcmp(w[0], "progbase"))
	{
	    if(words < 2 || *w[1] < '0' || *w[1] > '9')
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    progbase = atoi(w[1]);
	}
	else if(!strcmp(w[0], "map")) /* map <name> set1 elem1 set2 elem2 */
	{
	    int arg[5], isdrum;

	    if(words != 6)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if((arg[0] = mapname2id(w[1], &isdrum)) == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid map name: %s", name, line, w[1]);
		CHECKERRLIMIT;
		continue;
	    }
	    for(i = 2; i < 6; i++)
		arg[i - 1] = atoi(w[i]);
	    if(isdrum)
	    {
		arg[1] -= progbase;
		arg[3] -= progbase;
	    }
	    else
	    {
		arg[2] -= progbase;
		arg[4] -= progbase;
	    }

	    for(i = 1; i < 5; i++)
		if(arg[i] < 0 || arg[i] > 127)
		    break;
	    if(i != 5)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid parameter", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    set_instrument_map(arg[0], arg[1], arg[2], arg[3], arg[4]);
	}

	/*
	 * Standard configurations
	 */
	else if(!strcmp(w[0], "dir"))
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No directory given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    for(i = 1; i < words; i++)
		add_to_pathlist(w[i]);
	}
	else if(!strcmp(w[0], "source"))
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No file name given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    for(i = 1; i < words; i++)
	    {
		int status;
		rcf_count++;
		status = read_config_file(w[i], 0);
		rcf_count--;
		if(status == 2)
		{
		    close_file(tf);
		    return 2;
		}
		else if(status != 0)
		{

		    CHECKERRLIMIT;
		    continue;
		}
	    }
	}
	else if(!strcmp(w[0], "default"))
	{
	    if(words != 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify exactly one patch name",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    strncpy(def_instr_name, w[1], 255);
	    def_instr_name[255] = '\0';
	    default_instrument_name = def_instr_name;
	}
	else if(!strcmp(w[0], "drumset"))
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No drum set number given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]) - progbase;
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Drum set must be between %d and %d",
			  name, line,
			  progbase, progbase + 127);
		CHECKERRLIMIT;
		continue;
	    }

	    alloc_instrument_bank(1, i);

	    if(words == 2)
	    {
		bank = drumset[i];
		bankno = i;
		dr = 1;
	    }
	    else
	    {
		ToneBank *localbank;

		localbank = drumset[i];

		if(words < 4 || *w[2] < '0' || *w[2] > '9')
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: syntax error", name, line);
		    CHECKERRLIMIT;
		    continue;
		}

		i = atoi(w[2]);
		if(i < 0 || i > 127)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: Drum number must be between "
			      "0 and 127",
			      name, line);
		    CHECKERRLIMIT;
		    continue;
		}

		if(set_gus_patchconf(name, line, &localbank->tone[i],
				     w[3], w + 4))
		{
		    CHECKERRLIMIT;
		    continue;
		}
	    }
	}
	else if(!strcmp(w[0], "bank"))
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No bank number given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Tone bank must be between 0 and 127",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    alloc_instrument_bank(0, i);

	    if(words == 2)
	    {
		bank = tonebank[i];
		bankno = i;
		dr = 0;
	    }
	    else
	    {
		ToneBank *localbank;

		localbank = tonebank[i];

		if(words < 4 || *w[2] < '0' || *w[2] > '9')
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: syntax error", name, line);
		    CHECKERRLIMIT;
		    continue;
		}

		i = atoi(w[2]) - progbase;
		if(i < 0 || i > 127)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: Program must be between "
			      "%d and %d",
			      name, line,
			      progbase, 127 + progbase);
		    CHECKERRLIMIT;
		    continue;
		}

		if(set_gus_patchconf(name, line, &localbank->tone[i],
				     w[3], w + 4))
		{
		    CHECKERRLIMIT;
		    continue;
		}
	    }
	}
	else
	{
	    if(words < 2 || *w[0] < '0' || *w[0] > '9')
	    {
		if(extension_flag)
		    continue;
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[0]);
	    if(!dr)
		i -= progbase;
	    if(i < 0 || i > 127)
	    {
		if(dr)
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: Drum number must be between "
			      "0 and 127",
			      name, line);
		else
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: Program must be between "
			      "%d and %d",
			      name, line,
			      progbase, 127 + progbase);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    if(set_gus_patchconf(name, line, &bank->tone[i], w[1], w + 2))
	    {
		CHECKERRLIMIT;
		continue;
	    }
	}
    }
    if(errno)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Can't read %s: %s", name, strerror(errno));
	errcnt++;
    }
    close_file(tf);
    return errcnt != 0;
}

#ifdef SUPPORT_SOCKET

#if defined(__W32__) && !defined(MAIL_NAME)
#define MAIL_NAME "anonymous"
#endif /* __W32__ */

#ifdef MAIL_NAME
#define get_username() MAIL_NAME
#else /* MAIL_NAME */
#include <pwd.h>
static char *get_username(void)
{
    char *p;
    struct passwd *pass;

    /* USER
     * LOGIN
     * LOGNAME
     * getpwnam()
     */

    if((p = getenv("USER")) != NULL)
        return p;
    if((p = getenv("LOGIN")) != NULL)
        return p;
    if((p = getenv("LOGNAME")) != NULL)
        return p;

    pass = getpwuid(getuid());
    if(pass == NULL)
        return "nobody";
    return pass->pw_name;
}
#endif /* MAIL_NAME */

static void init_mail_addr(void)
{
    char addr[BUFSIZ];

    sprintf(addr, "%s%s", get_username(), MAIL_DOMAIN);
    user_mailaddr = safe_strdup(addr);
}
#endif /* SUPPORT_SOCKET */

static int read_user_config_file(void)
{
    char *home;
    char path[BUFSIZ];
    int opencheck;

#ifdef __W32__
/* HOME or home */
    home = getenv("HOME");
    if(home == NULL)
	home = getenv("home");
    if(home == NULL)
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Warning: HOME environment is not defined.");
	return 0;
    }
/* .timidity.cfg or timidity.cfg */
    sprintf(path, "%s" PATH_STRING "timidity.cfg", home);
    if((opencheck = open(path, 0)) < 0)
    {
	sprintf(path, "%s" PATH_STRING "_timidity.cfg", home);
	if((opencheck = open(path, 0)) < 0)
	{
	    sprintf(path, "%s" PATH_STRING ".timidity.cfg", home);
	    if((opencheck = open(path, 0)) < 0)
	    {
		ctl->cmsg(CMSG_INFO, VERB_NOISY, "%s: %s",
			  path, strerror(errno));
		return 0;
	    }
	}
    }

    close(opencheck);
    return read_config_file(path, 0);
#else
    home = getenv("HOME");
    if(home == NULL)
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Warning: HOME environment is not defined.");
	return 0;
    }
    sprintf(path, "%s" PATH_STRING ".timidity.cfg", home);

    if((opencheck = open(path, 0)) < 0)
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "%s: %s",
		  path, strerror(errno));
	return 0;
    }

    close(opencheck);
    return read_config_file(path, 0);
#endif /* __W32__ */
}

MAIN_INTERFACE void tmdy_free_config(void)
{
  ToneBank *bank;
  ToneBankElement *elm;

  int i, j;

  for (i = 0; i < 128; i++) {
    bank = tonebank[i];
    if (!bank)
      continue;
    for (j = 0; j < 128; j++) {
      elm = &bank->tone[j];
      if (elm->name)
	free(elm->name);
	elm->name = NULL;
      if (elm->comment)
	free(elm->comment);
	elm->comment = NULL;
	if (elm->tune)
		free(elm->tune);
	elm->tune = NULL;
	if (elm->envrate)
		free_ptr_list(elm->envrate, elm->envratenum);
	elm->envrate = NULL;
	elm->envratenum = 0;
	if (elm->envofs)
		free_ptr_list(elm->envofs, elm->envofsnum);
	elm->envofs = NULL;
	elm->envofsnum = 0;
	if (elm->trem)
		free_ptr_list(elm->trem, elm->tremnum);
	elm->trem = NULL;
	elm->tremnum = 0;
	if (elm->vib)
		free_ptr_list(elm->vib, elm->vibnum);
	elm->vib = NULL;
	elm->vibnum = 0;
	elm->instype = 0;
    }
    if (i > 0) {
      free(bank);
      tonebank[i] = NULL;
    }
  }

  for (i = 0; i < 128; i++) {
    bank = drumset[i];
    if (!bank)
      continue;
    for (j = 0; j < 128; j++) {
      elm = &bank->tone[j];
      if (elm->name)
	free(elm->name);
	elm->name = NULL;
      if (elm->comment)
	free(elm->comment);
	elm->comment = NULL;
	if (elm->tune)
		free(elm->tune);
	elm->tune = NULL;
	if (elm->envrate)
		free_ptr_list(elm->envrate, elm->envratenum);
	elm->envrate = NULL;
	elm->envratenum = 0;
	if (elm->envofs)
		free_ptr_list(elm->envofs, elm->envofsnum);
	elm->envofs = NULL;
	elm->envofsnum = 0;
	if (elm->trem)
		free_ptr_list(elm->trem, elm->tremnum);
	elm->trem = NULL;
	elm->tremnum = 0;
	if (elm->vib)
		free_ptr_list(elm->vib, elm->vibnum);
	elm->vib = NULL;
	elm->vibnum = 0;
	elm->instype = 0;
    }
    if (i > 0) {
      free(bank);
      drumset[i] = NULL;
    }
  }

  free_instrument_map();
  clean_up_pathlist();
}

static void expand_escape_string(char *s)
{
    char *t = s;
    if(s == NULL)
	return;
    while(*s)
    {
        if(*s == '\\')
        {
            s++;
            switch(*s)
            {
#define EXPAND(a, b) case a: *t++ = b; break;
                EXPAND('n', '\n');
                EXPAND('t', '\t');
                EXPAND('v', '\v');
                EXPAND('b', '\b');
                EXPAND('r', '\r');
                EXPAND('f', '\f');
                EXPAND('a', '\a');
                EXPAND('0', '\0');
                EXPAND('\\', '\\');
#undef EXPAND
              default:
		*t++ = *s;
            }
	    if(*s == '\0')
		return;
	    s++;
	}
	else
	    *t++ = *s++;
    }
    *t = '\0';
}

static int parse_effect_option(char *effect_opts)
{
	int i;
	
	if (strncmp(effect_opts, "delay=", 6) == 0) {
		switch (effect_opts[6]) {
		case 'l':
			effect_lr_mode = 0;
			break;
		case 'r':
			effect_lr_mode = 1;
			break;
		case 'b':
			effect_lr_mode = 2;
			break;
		case '0':
			effect_lr_mode = -1;
			return 0;
		}
		if (effect_opts[7] == ',') {
			effect_lr_delay_msec = atoi(effect_opts + 8);
			if (effect_lr_delay_msec < 0) {
				effect_lr_delay_msec = 0;
				effect_lr_mode = -1;
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
						"Invalid -EFdelay parameter.");
				return 1;
			}
		}
		return 0;
	}
	if (strncmp(effect_opts, "reverb=", 7) == 0) {
		effect_opts += 7;
		switch (*effect_opts) {
		case '0':
			opt_reverb_control = 0;
			break;
		case '1':
			if (*(effect_opts + 1) == ',')
				opt_reverb_control = -(atoi(effect_opts + 2) & 0x7f);
			else
				opt_reverb_control = 1;
			break;
		case '2':
			opt_reverb_control = 2;
			break;
		case '3':
			opt_reverb_control = 3;
			break;
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Invalid -EFreverb parameter.");
			return 1;
		}
		return 0;
	}
	if (strncmp(effect_opts, "chorus=", 7) == 0) {
		effect_opts += 7;
		switch (*effect_opts) {
		case '0':
			opt_chorus_control = 0;
			opt_surround_chorus = 0;
			break;
		case '1':
		case '2':
			opt_surround_chorus = (*effect_opts == '2');
			if (*(effect_opts + 1) == ',')
				opt_chorus_control = -(atoi(effect_opts + 2) & 0x7f);
			else
				opt_chorus_control = 1;
			break;
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Invalid -EFchorus parameter.");
			return 1;
		}
		return 0;
	}
	if (strncmp(effect_opts, "ns=", 3) == 0) {
		/* Noise Shaping filter from
		 * Kunihiko IMAI <imai@leo.ec.t.kanazawa-u.ac.jp>
		 */
		i = atoi(effect_opts + 3);
		if (i < 0 || i > 4) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-EFns argument range is 0 to 4");
			return 1;
		}
		noise_sharp_type = i;
		return 0;
	}
	return 1;
}

int set_extension_modes(char *flag)
{
	int err;
	
	err = 0;
	while (*flag) {
		switch(*flag) {
		case 'w':
			opt_modulation_wheel = 1;
			break;
		case 'W':
			opt_modulation_wheel = 0;
			break;
		case 'p':
			opt_portamento = 1;
			break;
		case 'P':
			opt_portamento = 0;
			break;
		case 'v':
			opt_nrpn_vibrato = 1;
			break;
		case 'V':
			opt_nrpn_vibrato = 0;
			break;
		case 'R':
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-ER option is obsoleted.  Please use -EFreverb=0");
			err++;
			break;
		case 'r':
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-Er option is obsoleted.  Please use -EFreverb=2");
			err++;
			break;
		case 'C':
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-EC option is obsoleted.  Please use -EFchorus=0");
			err++;
			break;
		case 'c':
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-Ec option is obsoleted.  "
					"Please use -EFchorus (toggle on/off)");
			err++;
			break;
		case 's':
			opt_channel_pressure = 1;
			break;
		case 'S':
			opt_channel_pressure = 0;
			break;
		case 'l':
			opt_lpf_def = 1;
			break;
		case 'L':
			opt_lpf_def = 0;
			break;
		case 'e':
			opt_modulation_envelope = 1;
			break;
		case 'E':
			opt_modulation_envelope = 0;
			break;
		case 't':
			opt_trace_text_meta_event = 1;
			break;
		case 'T':
			opt_trace_text_meta_event = 0;
			break;
		case 'o':
			opt_overlap_voice_allow = 1;
			break;
		case 'O':
			opt_overlap_voice_allow = 0;
			break;
		case 'm':
			{
			int val;
			
			val = str2mID(flag + 1);
			if (val == 0) {
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
						"-Em: Illegal value");
				err++;
			} else
				opt_default_mid = val;
			flag += 2;
			}
			break;
		case 'M':
			{
			int val;
			
			val = str2mID(flag + 1);
			if (val == 0) {
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
						"-EM: Illegal value");
				err++;
			} else
				opt_system_mid = val;
			flag += 2;
			}
			break;
		case 'b':
			if (flag[1] < '0' || flag[1] > '9')
				default_tonebank = 0;
			else {
				flag++;
				default_tonebank = 0;
				while (*flag >= '0' && *flag <= '9')
					default_tonebank = default_tonebank * 10 + *flag++ - '0';
				default_tonebank &= 0x7f;
				flag--;		/* to be inc. */
			}
			break;
		case 'B':
			if (flag[1] < '0' || flag[1] > '9')
				special_tonebank = -1;
			else {
				flag++;
				special_tonebank = 0;
				while (*flag >= '0' && *flag <= '9')
					special_tonebank = special_tonebank * 10 + *flag++ - '0';
				special_tonebank &= 0x7f;
				flag--;		/* to be inc. */
			}
			break;
		case 'F':
			if (parse_effect_option(flag + 1)) {
				ctl->cmsg(CMSG_ERROR,
						VERB_NORMAL, "-E%s: unsupported effect", flag);
				return err + 1;
			}
			return err;
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-E: Illegal mode `%c'", *flag);
			err++;
			break;
		}
		flag++;
	}
	return err;
}

#ifdef __W32__
#ifdef SMFCONV
int opt_rcpcv_dll = 0;
#endif /* SMFCONV */
static int set_win_modes(char *flag)
{
    int err;

    err = 0;
    while(*flag)
    {
	switch(*flag)
	{
#ifdef SMFCONV
	  case 'r':
	    opt_rcpcv_dll = 1;
	    break;
	  case 'R':
	    opt_rcpcv_dll = 0;
	    break;
#else
	  case 'r':
	  case 'R':
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-w%c option is not supported",
		      *flag);
	    err++;
	    break;
#endif /* SMFCONV */

	  default:
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-w: Illegal mode `%c'", *flag);
	    err++;
	    break;
	}
	flag++;
    }
    return err;
}
#endif /* __W32__ */

#ifdef __W32__
int opt_evil_mode = 0;
#endif /* __W32__ */
static int   try_config_again = 0;
int32 opt_output_rate = 0;
static char *opt_output_name = NULL;
static StringTable opt_config_string;
#ifdef SUPPORT_SOUNDSPEC
static double spectrogram_update_sec = 0.0;
#endif /* SUPPORT_SOUNDSPEC */
int opt_buffer_fragments = -1;

MAIN_INTERFACE int set_tim_opt_short(int c, char *optarg)
{
	int32 tmpi32;
	
	switch (c) {
#if defined(CSPLINE_INTERPOLATION) || defined(LAGRANGE_INTERPOLATION)
	case '4':
		no_4point_interpolation = (no_4point_interpolation) ? 0 : 1;
		break;
#endif
	case 'A':	/* amplify volume by n percent */
		if (*optarg != ',' && *optarg != 'a')
			if (set_value(&amplification, atoi(optarg), 0,
					MAX_AMPLIFICATION, "Amplification"))
				return 1;
		/* drum power */
		if (strchr(optarg, ','))
			if (set_value(&opt_drum_power, atoi(strchr(optarg, ',') + 1), 0,
					MAX_AMPLIFICATION, "Drum power"))
				return 1;
		if (strchr(optarg, 'a'))
			opt_amp_compensation = 1;
		break;
	case 'a':
		antialiasing_allowed = 1;
		break;
	case 'B':
		if (parse_opt_B(optarg))
			return 1;
		break;
#if 0
		if (set_value(&tmpi32, atoi(optarg), 0, 1000, "Buffer fragments"))
			return 1;
		opt_buffer_fragments = tmpi32;
#endif
		break;
	case 'b':
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"-b option is obsoleted.  Please use -EFdelay=b");
		return 1;
	case 'C':
		if (set_value(&control_ratio, atoi(optarg), 1, MAX_CONTROL_RATIO,
				"Control ratio"))
			return 1;
		opt_control_ratio = control_ratio;
		break;
	case 'c':
		if (read_config_file(optarg, 0))
			return 1;
		got_a_configuration = 1;
		break;
	case 'D':
		tmpi32 = atoi(optarg);
		if (set_channel_flag(&default_drumchannels, tmpi32, "Drum channel"))
			return 1;
		if (tmpi32 < 0)
			tmpi32 = -tmpi32;
		set_channel_flag(&default_drumchannel_mask, tmpi32, "Drum channel");
		break;
	case 'd':	/* dynamic lib root */
#ifdef IA_DYNAMIC
		if (dynamic_lib_root != NULL)
			free(dynamic_lib_root);
		dynamic_lib_root = safe_strdup(optarg);
#else
		ctl->cmsg(CMSG_WARNING, VERB_NOISY, "-d option is not supported");
#endif	/* IA_DYNAMIC */
		break;
	case 'E':
		return set_extension_modes(optarg);
	case 'e':	/* evil */
#ifdef __W32__
		opt_evil_mode = 1;
		break;
#else
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-e option is not supported");
		return 1;
#endif
	case 'F':
		adjust_panning_immediately = ! adjust_panning_immediately;
		break;
	case 'f':
		fast_decay = (fast_decay) ? 0 : 1;
		break;
	case 'g':
#ifdef SUPPORT_SOUNDSPEC
		spectrogram_update_sec = (FLOAT_T) atof(optarg);
		if (spectrogram_update_sec <= 0) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Invalid -g argument: `%s'", optarg);
			return 1;
		}
		view_soundspec_flag = 1;
		break;
#else
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-g option is not supported");
		return 1;
#endif	/* SUPPORT_SOUNDSPEC */
	case 'H':	/* force keysig (number of sharp/flat) */
		if (set_value(&tmpi32, atoi(optarg), -7, 7,
				"Force keysig (number of sHarp(+)/flat(-)"))
			return 1;
		opt_force_keysig = tmpi32;
		break;
	case 'h':
		help();
		exit(0);
	case 'I':
		if (set_default_prog(optarg))
			return -1;
		break;
	case 'i':
		if (set_ctl(optarg))
			return 1;
		break;
	case 'j':
		opt_realtime_playing = ! opt_realtime_playing;
		break;
	case 'K':	/* key adjust */
		if (set_value(&tmpi32, atoi(optarg), -24, 24, "Key adjust"))
			return 1;
		key_adjust = tmpi32;
		break;
	case 'k':
		reduce_voice_threshold = atoi(optarg);
		break;
	case 'L':
		add_to_pathlist(optarg);
		try_config_again = 1;
		break;
	case 'M':
		if (pcm_alternate_file != NULL)
			free(pcm_alternate_file);
		pcm_alternate_file = safe_strdup(optarg);
		break;
	case 'm':
		min_sustain_time = atoi(optarg);
		if (min_sustain_time < 0)
			min_sustain_time = 0;
		break;
#ifdef GAUSS_INTERPOLATION
	case 'N':
		gauss_n = atoi(optarg);
		if (gauss_n < 0 || gauss_n > 34) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Gauss interpolation: -N value must be from 1 to 34.");
			return 1;
		}
		if (gauss_n == 0) {
			gauss_n = 5;
			no_4point_interpolation = 1;
			reduce_quality_flag = 1;
		}
		break;
#endif	/* GAUSS_INTERPOLATION */
#ifdef NEWTON_INTERPOLATION
	case 'N':
		newt_n = atoi(optarg);
		if ((newt_n > 0 && newt_n % 2 == 0) || newt_n < 0 || newt_n > 57) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Newton interpolation: -N value must be "
					"an odd number from 1 to 57.");
			return 1;
		}
		if (newt_n == 0) {
			newt_n = 5;
			no_4point_interpolation = 1;
			reduce_quality_flag = 1;
		}
		/* set optimal value for newt_max */
		newt_max = -1.875328947 + 1.57730263158 * newt_n;
		if (newt_max < newt_n)
			newt_max = newt_n;
		if (newt_max > 57)
			newt_max = 57;
		break;
#endif
	case 'n':
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"-n option is obsoleted.  Please use -EFns=<n>");
		return 1;
	case 'O':	/* output mode */
		if (set_play_mode(optarg))
			return 1;
		break;
	case 'o':
		if (opt_output_name != NULL)
			free(opt_output_name);
		opt_output_name = safe_strdup(url_expand_home_dir(optarg));
		break;
	case 'P':	/* set overriding instrument */
		strncpy(def_instr_name, optarg, 255);
		def_instr_name[255] = '\0';
		break;
	case 'p':
		if (strchr(optarg, 'a'))
			auto_reduce_polyphony = ! auto_reduce_polyphony;
		if (*optarg != 'a') {
			if (set_value(&tmpi32, atoi(optarg), 1, MAX_VOICES, "Polyphony"))
				return 1;
			voices = tmpi32;
		}
		break;
	case 'Q':
		if (strchr(optarg, 't')) {
			if (set_value(&tmpi32, atoi(optarg), 0, 7, "Quiet temperament"))
				return 1;
			temper_type_mute |= 1 << tmpi32;
		} else
			if (set_channel_flag(
					&quietchannels, atoi(optarg), "Quiet channel"))
				return 1;
		break;
	case 'q':
		if (strchr(optarg, '/') == NULL) {
			if (opt_aq_max_buff)
				free(opt_aq_max_buff);
			opt_aq_max_buff = safe_strdup(optarg);
		} else {
			if (optarg[0] == '/') {
				if (opt_aq_fill_buff)
					free(opt_aq_fill_buff);
				opt_aq_fill_buff = safe_strdup(optarg + 1);
			} else {
				char *max_buff, *fill_buff;
				
				max_buff = safe_strdup(optarg);
				fill_buff = strchr(max_buff, '/');
				*fill_buff++ = '\0';
				if (opt_aq_max_buff)
					free(opt_aq_max_buff);
				if (opt_aq_fill_buff)
					free(opt_aq_fill_buff);
				opt_aq_max_buff = max_buff;
				opt_aq_fill_buff = safe_strdup(fill_buff);
			}
		}
		break;
	case 'R':
		tmpi32 = atoi(optarg);
		if (tmpi32 == -1)
			/* reset */
			modify_release = 0;
		else {
			if (set_value(&modify_release, tmpi32, 0, MAX_MREL,
					"Modify Release"))
				return 1;
			if (modify_release == 0)
				modify_release = DEFAULT_MREL;
		}
		break;
	case 'r':
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"-r option is obsoleted.  Please use -EFreverb=1");
		return 1;
	case 'S':
		if (optarg[strlen(optarg) - 1] == 'k'
				|| optarg[strlen(optarg) - 1] == 'K')
			allocate_cache_size = (int32) (1024.0 * atof(optarg));
		else if (optarg[strlen(optarg) - 1] == 'm'
				|| optarg[strlen(optarg) - 1] == 'M')
			allocate_cache_size = (int32) (1024 * 1024 * atof(optarg));
		else
			allocate_cache_size = atoi(optarg);
		break;
	case 's':	/* sampling rate */
		tmpi32 = atoi(optarg);
		if (tmpi32 < 100)
			tmpi32 = (int32) (atof(optarg) * 1000.0 + 0.5);
		if (set_value(&opt_output_rate, tmpi32, MIN_OUTPUT_RATE,
				MAX_OUTPUT_RATE, "Resampling frequency"))
			return 1;
		break;
	case 'T':
		tmpi32 = atoi(optarg);
		if (set_value(&tmpi32, tmpi32, 10, 400, "Tempo adjust"))
			return 1;
		tempo_adjust = 100.0 / tmpi32;
		break;
	case 't':
		if (output_text_code)
			free(output_text_code);
		output_text_code = safe_strdup(optarg);
		break;
	case 'U':
		free_instruments_afterwards = 1;
		break;
	case 'W':
		if (set_wrd(optarg))
			return 1;
		break;
	case 'w':
#ifdef __W32__
		return set_win_modes(optarg);
#else
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-w option is not supported");
		return 1;
#endif /* __W32__ */
	case 'x':
		{
		StringTableNode *st;
		
		if ((st = put_string_table(&opt_config_string,
				optarg, strlen(optarg))) != NULL)
			expand_escape_string(st->string);
		}
		break;
	case 'Z':	/* load frequency table */
		if (! strncmp(optarg, "pure", 4)) {
			opt_pure_intonation = 1;
			if (*(optarg + 4)) {
				if (set_value(&tmpi32, atoi(optarg + 4), -7, 7,
						"Initial keysig (number of #(+)/b(-)[m(minor)])"))
					return 1;
				if (strchr(optarg + 4, 'm'))
					opt_init_keysig = tmpi32 + 16;
				else
					opt_init_keysig = tmpi32;
			}
		} else if (load_table(optarg))
			return 1;
		break;
	default:
		return 1;
	}
	return 0;
}

/* -------- getopt_long -------- */
MAIN_INTERFACE bool set_tim_opt_long(int c, char *optarg, int index)
{
	if (isalnum(c))
		return set_tim_opt_short(c, optarg);
	else if (c <= 0xff)
		/* getopt_long failed parsing */
		parse_opt_666(optarg);
	else {
		const struct option *the_option = &(longopts[index]);
		char *arg;
		
		if (! strncmp(the_option->name, "no-", 3))
			arg = "no";		/* `reverse' switch */
		else if (! strncmp(the_option->name, "output-un", 9))
			arg = "no";		/* --output-unsigned == --output-signed=no */
		else if (! strcmp(the_option->name, "mono"))
			/* --mono == --stereo=no */
			arg = (y_or_n_p(optarg)) ? "no" : "yes";
		else
			arg = optarg;
		switch (c >> 8) {
#if defined(CSPLINE_INTERPOLATION) || defined(LAGRANGE_INTERPOLATION)
		case '4':
			return parse_opt_4(arg);
#endif
		case 'A':
			return parse_opt_A(arg);
		case 227:
			return parse_opt_A1(arg);
		case 228:
			return parse_opt_A2(arg);
		case 'B':
			return parse_opt_B(arg);
		case 'C':
			return parse_opt_C(arg);
		case 'D':
			return parse_opt_D(arg);
		case 'E':
			return parse_opt_E(arg);
		case 'F':
			return parse_opt_F(arg);
		case 'H':
			return parse_opt_H(arg);
		case 'I':
			return parse_opt_I(arg);
		case 'K':
			return parse_opt_K(arg);
		case 'L':
			return parse_opt_L(arg);
		case 'M':
			return parse_opt_M(arg);
#if defined(GAUSS_INTERPOLATION) || defined(NEWTON_INTERPOLATION)
		case 'N':
			return parse_opt_N(arg);
#endif
		case 'O':
			return parse_opt_O(arg);
		case 'P':
			return parse_opt_P(arg);
		case 'S':
			return parse_opt_S(arg);
		case 'T':
			return parse_opt_T(arg);
		case 'U':
			return parse_opt_U(arg);
		case 'W':
			return parse_opt_W(arg);
		case 'Z':
			return parse_opt_Z(arg);
		case 'a':
			return parse_opt_a(arg);
		case 'b':
			return parse_opt_b(arg);
		case 'c':
			return parse_opt_c(arg);
		case 'd':
			return parse_opt_d(arg);
		case 'e':
			return parse_opt_e(arg);
		case 'f':
			return parse_opt_f(arg);
		case 'g':
			return parse_opt_g(arg);
		case 'h':
			return parse_opt_h(arg);
		case 'i':
			return parse_opt_i(arg);
		case 'j':
			return parse_opt_j(arg);
		case 'k':
			return parse_opt_k(arg);
		case 'm':
			return parse_opt_m(arg);
		case 'o':
			return parse_opt_o(arg);
		case 'p':
			return parse_opt_p(arg);
		case 'q':
			return parse_opt_q(arg);
		case 's':
			return parse_opt_s(arg);
		case 'v':
			return parse_opt_v(arg);
		case 'x':
			return parse_opt_x(arg);
		case 200:
			return parse_opt_200(arg);
		case 201:
			return parse_opt_201(arg);
		case 202:
			return parse_opt_202(arg);
		case 203:
			return parse_opt_203(arg);
		/*   204: is missing: not reserved */
		case 205:
			return parse_opt_205(arg);
		case 206:
			return parse_opt_206(arg);
		case 207:
			return parse_opt_207(arg);
		case 208:
			return parse_opt_208(arg);
		case 209:
			return parse_opt_209(arg);
		case 210:
			return parse_opt_210(arg);
		case 211:
			return parse_opt_211(arg);
		case 212:
			return parse_opt_212(arg);
		case 213:
			return parse_opt_213(arg);
		case 214:
			return parse_opt_214(arg);
		case 215:
			return parse_opt_215(arg);
		case 216:
			return parse_opt_216(arg);
		case 217:
			return parse_opt_217(arg);
		case 218:
			return parse_opt_218(arg);
		case 219:
			return parse_opt_219(arg);
		case 220:
			return parse_opt_220(arg);
		case 221:
			return parse_opt_221(arg);
		case 222:
			return parse_opt_222(arg);
		case 223:
			return parse_opt_223(arg);
		case 224:
			return parse_opt_224(arg);
		case 225:
			return parse_opt_225(arg);
		case 226:
			return parse_opt_226(arg);
		case 229:
			return parse_opt_229(arg);
		case 230:
			return parse_opt_230(arg);
		case 231:
			return parse_opt_231(arg);
		case 232:
			return parse_opt_232(arg);
		case 233:
			return parse_opt_233(arg);
		case 234:
			return parse_opt_234(arg);
		case 235:
			return parse_opt_235(arg);
		case 236:
			return parse_opt_236(arg);
		case 237:
			return parse_opt_237(arg);
		default:
			ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
					"[BUG] Inconceivable case branch %d('%c')", c, c >> 8);
			abort();
		}
	}
}

#if defined(CSPLINE_INTERPOLATION) || defined(LAGRANGE_INTERPOLATION)
static inline bool parse_opt_4(const char *arg)
{
	no_4point_interpolation = y_or_n_p(arg);
	return 0;
}
#endif

static inline bool parse_opt_A(const char *arg)
{
	/* --volume */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 0, MAX_AMPLIFICATION, "Amplification"))
		return 1;
	amplification = tmpi32;
	return 0;
}

static inline bool parse_opt_A1(const char *arg)
{
	/* --drum-power */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 0, MAX_AMPLIFICATION, "Drum power"))
		return 1;
	opt_drum_power = tmpi32;
	return 0;
}

static inline bool parse_opt_A2(const char *arg)
{
	/* --auto-volume-conpensation */
	opt_amp_compensation = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_a(const char *arg)
{
	antialiasing_allowed = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_B(const char *arg)
{
	/* --buffer-fragments */
	int32 tmpi32;
	const char *p;
	
	/* num */
	if (*arg != ',') {
		if (set_value(&tmpi32, atoi(arg), 0, 1000, "Buffer Fragments (num)"))
			return 1;
		opt_buffer_fragments = tmpi32;
	}
	/* bits */
	if (p = strchr(arg, ',')) {
		if (set_value(&tmpi32, atoi(p + 1), 1, AUDIO_BUFFER_BITS,
				"Buffer Fragments (bit)"))
			return 1;
		audio_buffer_bits = tmpi32;
	}
	return 0;
}

static inline bool parse_opt_b(const char *arg)
{
	return set_flag(&(ctl->flags), CTLF_DAEMONIZE, arg);
}

static inline bool parse_opt_C(const char *arg)
{
	if (set_value(&control_ratio, atoi(arg), 0, MAX_CONTROL_RATIO,
			"Control ratio"))
		return 1;
	opt_control_ratio = control_ratio;
	return 0;
}

static inline bool parse_opt_c(char *arg)
{
	if (read_config_file(arg, 0))
		return 1;
	got_a_configuration = 1;
	return 0;
}

static inline bool parse_opt_D(const char *arg)
{
	int val = atoi(arg);
	
	if (set_channel_flag(&default_drumchannels, val, "Drum channel"))
		return 1;
	if (val < 0)
		val = -val;
	set_channel_flag(&default_drumchannel_mask, val, "Drum channel");
	return 0;
}

static inline bool parse_opt_d(const char *arg)
{
	/* dynamic lib root */
#ifdef IA_DYNAMIC
	if (dynamic_lib_root)
		free(dynamic_lib_root);
	dynamic_lib_root = safe_strdup(arg);
	return 0;
#else
	ctl->cmsg(CMSG_WARNING, VERB_NOISY, "-d option is not supported");
	return 1;
#endif	/* IA_DYNAMIC */
}

static inline bool parse_opt_E(char *arg)
{
	/* undocumented option --effects */
	return set_extension_modes(arg);
}

static inline bool parse_opt_216(const char *arg)
{
	/* --[no-]modulation-wheel */
	opt_modulation_wheel = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_217(const char *arg)
{
	/* --[no-]portamento */
	opt_portamento = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_218(const char *arg)
{
	/* --[no-]vibrato */
	opt_nrpn_vibrato = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_219(const char *arg)
{
	/* --[no-]channel-pressure */
	opt_channel_pressure = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_220(const char *arg)
{
	/* --[no-]new-lpf */
	opt_lpf_def = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_236(const char *arg)
{
	/* --[no-]modulation-envelope */
	opt_modulation_envelope = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_214(const char *arg)
{
	/* --[no-]trace-text-meta */
	opt_trace_text_meta_event = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_221(const char *arg)
{
	/* --[no-]overlap */
	opt_overlap_voice_allow = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_213(char *arg)
{
	/* --mid */
	int val = str2mID(arg);
	
	if (! val) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Manufacture ID: Illegal value");
		return 1;
	}
	opt_default_mid = val;
	return 0;
}

static inline bool parse_opt_237(char *arg)
{
	/* --system-mid */
	int val = str2mID(arg);
	
	if (! val) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Manufacture ID: Illegal value");
		return 1;
	}
	opt_system_mid = val;
	return 0;
}

static inline bool parse_opt_211(const char *arg)
{
	/* --default-bank */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 0, 0x7f, "Bank number"))
		return 1;
	default_tonebank = tmpi32;
	return 0;
}

static inline bool parse_opt_212(const char *arg)
{
	/* --force-bank */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 0, 0x7f, "Bank number"))
		return 1;
	special_tonebank = tmpi32;
	return 0;
}

static inline bool parse_opt_224(const char *arg)
{
	/* --delay */
	const char *type = arg;
	
	switch (type[0]) {
	case '0':
	case 'n':
		effect_lr_mode = -1;
		return 0;
	case 'l':
		effect_lr_mode = 0;
		break;
	case 'r':
		effect_lr_mode = 1;
		break;
	case 'b':
		effect_lr_mode = 2;
		break;
	}
	if (type = strchr(type, ',')) {
		int val = atoi(type + 1);
		
		if (val < 0) {
			effect_lr_delay_msec = 0;
			effect_lr_mode = -1;
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid delay parameter.");
			return 1;
		}
		effect_lr_delay_msec = val;
	}
	return 0;
}

static inline bool parse_opt_222(const char *arg)
{
	/* --reverb */
	int32 tmpi32;
	const char *lv;
	
	if (isdigit(arg[0]))
		opt_reverb_control = atoi(arg);
	else if (strncasecmp("no", arg, 2)) {
		opt_reverb_control = 0;
		return 0;
	} else if (strncasecmp("standard", arg, 8) || strncasecmp("std", arg, 3))
		opt_reverb_control = 1;
	else if (strncasecmp("global", arg, 6)) {
		opt_reverb_control = 2;
		return 0;
	} else if (strncasecmp("new", arg, 3)
			|| strncasecmp("freeverb", arg, 8)) {
		opt_reverb_control = 3;
		return 0;
	} else if (strncasecmp("pseudo", arg, 6))
		/* I think pseudo reverb can now be retired... Computers are
		 * enough fast to do a full reverb, don't they?
		 */
		modify_release = DEFAULT_MREL;
	else {
		/* reaching here indicates arg is inalid. */
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid reverb parameter.");
		return 1;
	}
	if (lv = strchr(arg, ',')) {
		if (arg[0] == 'p' || arg[0] == 'P') {
			if (set_value(&tmpi32, atoi(lv), 0, MAX_MREL, "Modify release"))
				return 1;
			modify_release = tmpi32;
		} else {
			if (set_value(&tmpi32, atoi(lv), 0, 0x7f, "Reverb level"))
				return 1;
			opt_reverb_control = -tmpi32;
		}
	}
	return 0;
}

static inline bool parse_opt_223(const char *arg)
{
	/* --chorus */
	int32 tmpi32;
	const char *lv;
	
	if (isdigit(arg[0])) {
		char *tmp = malloc(strlen(arg) + 8);	/* 8 for "chorus=", '\0' */
		
		if (sprintf(tmp, "chorus=%s", arg) >= 0) {
			bool ret = parse_effect_option(tmp);
			
			free(tmp);
			return ret;
		} else {
			free(tmp);
			return 1;	/* maybe not enough memory -- no way but die? */
		}
	} else if (strncasecmp("no", arg, 2)) {
		opt_chorus_control = 0;
		opt_surround_chorus = 0;
		return 0;
	} else if (strncasecmp("standard", arg, 8)
			|| strncasecmp("std", arg, 3)) {
		opt_chorus_control = 1;
		opt_surround_chorus = 0;
	} else if (strncasecmp("surround", arg, 6)) {
		opt_chorus_control = 1;
		opt_surround_chorus = 1;
	} else {
		/* reaching here indicates arg is inalid. */
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid chorus parameter.");
		return 1;
	}
	if (lv = strchr(arg, ',')) {
		if (set_value(&tmpi32, atoi(lv), 0, 0x7f, "Chorus level"))
			return 1;
		opt_chorus_control = -tmpi32;
	}
	return 0;
}

static inline bool parse_opt_225(const char *arg)
{
	/* --noise-shaper */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 0, 4, "Noise shaper type"))
		return 1;
	noise_sharp_type = tmpi32;
	return 0;
}

static inline bool parse_opt_e(const char *arg)
{
	/* evil */
#ifdef __W32__
	opt_evil_mode = 1;
	return 0;
#else
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-e option is not supported");
	return 1;
#endif /* __W32__ */
}

static inline bool parse_opt_F(const char *arg)
{
	adjust_panning_immediately = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_f(const char *arg)
{
	fast_decay = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_g(const char *arg)
{
#ifdef SUPPORT_SOUNDSPEC
	spectrogram_update_sec = atof(arg);
	if (spectrogram_update_sec <= 0) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"Invalid -g argument: `%s'", arg);
		return 1;
	}
	view_soundspec_flag = 1;
	return 0;
#else
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-g option is not supported");
	return 1;
#endif	/* SUPPORT_SOUNDSPEC */
}

static inline bool parse_opt_H(const char *arg)
{
	/* force keysig (number of sharp/flat) */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), -7, 7,
			"Force keysig (number of sHarp(+)/flat(-))"))
		return 1;
	opt_force_keysig = tmpi32;
	return 0;
}

__attribute__((noreturn))
static inline bool parse_opt_h(const char *arg)
{
	help();
	exit(EXIT_SUCCESS);
}

static inline bool parse_opt_I(char *arg)
{
	if (set_default_prog(arg))
		return 1;
	return 0;
}

static inline bool parse_opt_i(const char *arg)
{
	static const struct Name2ID {
		const char *name;
		const int id;
	} name2id[] = {
		{ "dumb", 'd' },
		{ "ALSA", 'A' },
		{ "ncurses", 'n' },
		{ "slang", 's' },
		{ "Motif", 'm' },
		{ "tcktk", 'k' },
		{ "Tcl/Tk", 'k' },	/* synonym */
		{ "emacs", 'e' },
		{ "VT100", 'T' },
		{ "xaw", 'a' },
		{ "xskin", 'i' },
		{ "GTK", 'g' },
		{ "server", 'r' },
		{ "twsynth", 'W' },
		{ "twsyng", 'W' },
		{ "portminisyn", 'P' },
		{ "w32gui", 'w' },
		{ "UMP", 'p' },
		{ NULL, '\0' }		/* terminator */
	};
	int id;
	ControlMode *cp;
	ControlMode **cpp = ctl_list;
	
	if (strlen(arg) == 1)
		id = arg[0];
	else {
		int i;
		
		for (i = 0; name2id[i].name; i++)
			if (! strcasecmp(name2id[i].name, arg)) {
				id = name2id[i].id;
				break;
			}
	}
	while (cp = *cpp++) {
		if (cp->id_character == id) {
			ctl = cp;
			return 0;
		}
#ifdef IA_DYNAMIC
		else if ((cp->id_character == dynamic_interface_id)
				&& (dynamic_interface_module(id) !=NULL)) {
			ctl = cp;
			if (dynamic_interface_id != id) {
				ctl->verbosity = 1;
				ctl->trace_playing = 0;
				ctl->flags = 0;
				ctl->id_character = dynamic_interface_id = id;
			}
			return 0;
		}
#endif
	}
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Interface `%s' not found.", arg);
	return 1;
}

static inline bool parse_opt_205(const char *arg)
{
	/* --verbose */
	ctl->verbosity += (arg) ? atoi(arg) : 1;
	return 0;
}

static inline bool parse_opt_206(const char *arg)
{
	/* --quiet */
	ctl->verbosity -= (arg) ? atoi(arg) : 1;
	return 0;
}

static inline bool parse_opt_207(const char *arg)
{
	/* --[no-]trace */
	ctl->trace_playing = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_208(const char *arg)
{
	/* --[no-]loop */
	return set_flag(&(ctl->flags), CTLF_LIST_LOOP, arg);
}

static inline bool parse_opt_209(const char *arg)
{
	/* --[no-]random */
	return set_flag(&(ctl->flags), CTLF_LIST_RANDOM, arg);
}

static inline bool parse_opt_210(const char *arg)
{
	/* --[no-]sort */
	return set_flag(&(ctl->flags), CTLF_LIST_SORT, arg);
}

static inline bool parse_opt_230(const char *arg)
{
	/* --[no-]auto-start */
	return set_flag(&(ctl->flags), CTLF_AUTOSTART, arg);
}

static inline bool parse_opt_231(const char *arg)
{
	/* --[no-]auto-exit */
	return set_flag(&(ctl->flags), CTLF_AUTOEXIT, arg);
}

static inline bool parse_opt_232(const char *arg)
{
	/* --[no-]drag-start */
	return set_flag(&(ctl->flags), CTLF_DRAG_START, arg);
}

static inline bool parse_opt_233(const char *arg)
{
	/* --[no-]uniq */
	return set_flag(&(ctl->flags), CTLF_AUTOUNIQ, arg);
}

static inline bool parse_opt_234(const char *arg)
{
	/* --[no-]refine */
	return set_flag(&(ctl->flags), CTLF_AUTOREFINE, arg);
}

static inline bool parse_opt_235(const char *arg)
{
	/* --[no-]continue */
	if (y_or_n_p(arg))
		ctl->flags &= ~CTLF_NOT_CONTINUE;
	else
		ctl->flags |= CTLF_NOT_CONTINUE;
	return 0;
}

static inline bool parse_opt_j(const char *arg)
{
	opt_realtime_playing = y_or_n_p(arg);
}

static inline bool parse_opt_K(const char *arg)
{
	/* key adjust */
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), -24, 24, "Key adjust"))
		return 1;
	key_adjust = tmpi32;
	return 0;
}

static inline bool parse_opt_k(const char *arg)
{
	reduce_voice_threshold = atoi(arg);
	return 0;
}

static inline bool parse_opt_L(char *arg)
{
	add_to_pathlist(arg);
	try_config_again = 1;
	return 0;
}

static inline bool parse_opt_M(const char *arg)
{
	if (pcm_alternate_file)
		free(pcm_alternate_file);
	pcm_alternate_file = safe_strdup(arg);
	return 0;
}

static inline bool parse_opt_m(const char *arg)
{
	min_sustain_time = atoi(optarg);
	if (min_sustain_time < 0)
		min_sustain_time = 0;
	return 0;
}

#ifdef GAUSS_INTERPOLATION
static inline bool parse_opt_N(const char *arg)
{
	int32 tmpi32;
	
	if (atoi(arg)) {
		if (set_value(&tmpi32, atoi(arg), 1, 34,
				"Gauss interpolation -N value"))
			return 1;
		gauss_n = tmpi32;
	} else {
		gauss_n = 5;
		no_4point_interpolation = 1;
		reduce_quality_flag = 1;
	}
	return 0;
}
#elif defined(NEWTON_INTERPOLATION)
static inline bool parse_opt_N(const char *arg)
{
	int32 tmpi32;
	
	if (atoi(arg)) {
		if (set_value(&tmpi32, atoi(arg), 1, 56,
				"Newton interpolation -N value"))
			return 1;
		if (tmpi32 % 2) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Newton -N value must be even");
			return 1;
		}
		newt_n = tmpi32;
	} else {
		newt_n = 5;
		no_4point_interpolation = 1;
		requce_quality_flag = 1;
	}
	/* set optimal value for newt_max */
	newt_max = newt_n * 1.57730263158 - 1.875328947;
	if (newt_max < newt_n)
		newt_max = newt_n;
	if (newt_max > 57)
		newt_max = 57;
	return 0;
}
#endif

static inline bool parse_opt_O(const char *arg)
{
	static const struct Name2ID {
		const char *name;
		const int id;
	} name2id[] = {
		{ "default", 'd' },
		{ "ALSA", 's' },
		{ "ALib", 'A' },
		{ "NAS", 's' },
		{ "aRts", 'R' },
		{ "ESD", 'e' },
		{ "PortAudio", 'p' },
		{ "wav", 'w' },
		{ "au", 'u' },
		{ "AIFF", 'a' },
		{ "list", 'l' },
		{ "vorbis", 'v' },
		{ "gogo", 'g' },
		{ NULL, '\0' }		/* terminator */
	};
	int id;
	PlayMode *pp;
	PlayMode **ppp = play_mode_list;
	
	if (strlen(arg) == 1)
		id = arg[0];
	else {
		int i;
		for (i = 0; name2id[i].name; i++)
			if (! strcasecmp(name2id[i].name, arg)) {
				id = name2id[i].id;
				break;
			}
	}
	while (pp = *ppp++)
		if (pp->id_character == arg[0]) {
			play_mode = pp;
			return 0;
		}
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Output mode `%s' not found.", arg);
	return 1;
}

static inline bool parse_opt_200(const char *arg)
{
	/* --bit-width */
	int val = atoi(arg);
	
	if ((val == 1) || (val == 16)) {
		play_mode->encoding |= PE_16BIT;
		play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
		return 0;
	} else if (val == 8) {
		play_mode->encoding &= ~PE_16BIT;
		return 0;
	} else {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "bit width must be 8 or 16");
		return 1;
	}
}

static inline bool parse_opt_201(const char *arg)
{
	/* --encoding */
	switch (arg[0]) {
	case 'u':
	case 'U':
		/* uLaw */
		play_mode->encoding |= PE_ULAW;
		play_mode->encoding &=
				~(PE_ALAW | PE_16BIT | PE_SIGNED | PE_BYTESWAP);
		return 0;
	case 'a':
	case 'A':
		/* aLaw */
		play_mode->encoding |= PE_ALAW;
		play_mode->encoding &=
				~(PE_ULAW | PE_16BIT | PE_SIGNED | PE_BYTESWAP);
		return 0;
	case 'l':
	case 'L':
		/* linear */
		play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
		return 0;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"unknown output enciding `%s'",arg);
		return 1;
	}
}

static inline bool parse_opt_202(const char *arg)
{
	/* --output-[un]singed */
	if (set_flag(&(play_mode->encoding), PE_SIGNED, arg))
		return 1;
	play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
	return 0;
}

static inline bool parse_opt_203(const char *arg)
{
	/* --[no-]output-byte-swap */
	if (set_flag(&(play_mode->encoding), PE_BYTESWAP, arg))
		return 1;
	play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
	return 0;
}

static inline bool parse_opt_229(const char *arg)
{
	/* --stereo */
	if (y_or_n_p(arg))
		/* I first thought --mono should be the syntax sugae to
		 * --stereo=no, but the source said stereo should be !PE_MONO,
		 * not mono should be !PE_STEREO.  Perhaps I took a wrong
		 * choice? -- mput
		 */
		play_mode->encoding &= ~PE_MONO;
	else
		play_mode->encoding |= PE_MONO;
	return 0;
}

static inline bool parse_opt_o(char *arg)
{
	if (opt_output_name)
		free(opt_output_name);
	opt_output_name = safe_strdup(url_expand_home_dir(arg));
	return 0;
}

static inline bool parse_opt_P(const char *arg)
{
	/* set overriding instrument */
	strncpy(def_instr_name, arg, sizeof(def_instr_name));
	def_instr_name[sizeof(def_instr_name) - 1] = '\0';
	return 0;
}

static inline bool parse_opt_p(const char *arg)
{
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 1, MAX_VOICES, "Polyphony"))
		return 1;
	voices = tmpi32;
	return 0;
}

static inline bool parse_opt_215(const char *arg)
{
	/* --[no-]polyphony-reduction */
	auto_reduce_polyphony = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_Q(const char *arg)
{
	int32 tmpi32;
	
	if (isdigit(arg[0]) || arg[0] == '-') {
		if (strchr(arg, 't')) {
			if (set_value(&tmpi32, atoi(arg), 0, 7, "Quiet temperament"))
				return 1;
			temper_type_mute |= 1 << tmpi32;
			return 0;
		} else
			return set_channel_flag(&quietchannels, atoi(arg),
					"Quiet channel");
	} else {
		static const struct Name2Type {
			const char *name;
			unsigned char type;
		} name2type[] = {
			{ "equal", 0 },
			{ "Pythagoras", 1 },
			{ "mean", 2 },
			{ "pure", 2 }
		};
		int i;
		
		for (i = 0; name2type[i].name; i++)
			if (strcasecmp(name2type[i].name, arg)) {
				temper_type_mute |= 1 << name2type[i].type;
				return 0;
			}
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Unknown temperament `%s'", arg);
		return 1;
	}
}

static inline bool parse_opt_q(const char *arg)
{
	char *m = safe_strdup(arg);
	char *f = strchr(m, '/');
	
	if (f != m) {
		if (opt_aq_max_buff)
			free(opt_aq_max_buff);
		opt_aq_max_buff = m;
	}
	if (f) {
		f[0] = '\0';
		if (opt_aq_fill_buff)
			free(opt_aq_fill_buff);
		opt_aq_fill_buff = f + 1;
	}
	return 0;
}

static inline bool parse_opt_S(const char *arg)
{
	double val = atof(arg);
	int figure;
	int suffix = arg[strlen(arg) - 1];
	
	switch (suffix) {
	case 'm':
	case 'M':
		figure = 1 << 20;
	case 'k':
	case 'K':
		figure = 1 << 10;
	default:
		figure = 1;
	}
	allocate_cache_size = figure * val;
	return 0;
}

static inline bool parse_opt_s(const char *arg)
{
	/* sampling rate */
	int32 tmpi32;
	
	if (atoi(arg) < 100) {
		if (set_value(&tmpi32, atoi(arg), 4, 65, "Frequency"))
			return 1;
		/* Hey, what's this 0.5? -- mput */
		opt_output_rate = tmpi32 * 1000 + 0.5;
	} else {
		if (set_value(&tmpi32, atoi(arg), MIN_OUTPUT_RATE, MAX_OUTPUT_RATE,
				"Frequency"))
			return 1;
		opt_output_rate = tmpi32;
	}
	return 0;
}

static inline bool parse_opt_T(const char *arg)
{
	int32 tmpi32;
	
	if (set_value(&tmpi32, atoi(arg), 10, 400, "Tempo adjust"))
		return 1;
	tempo_adjust = 100.0 / tmpi32;
	return 0;
}

static inline bool parse_opt_t(const char *arg)
{
	if (output_text_code)
		free(output_text_code);
	output_text_code = safe_strdup(arg);
	return 0;
}

static inline bool parse_opt_U(const char *arg)
{
	free_instruments_afterwards = y_or_n_p(arg);
	return 0;
}

static inline bool parse_opt_v(const char *arg)
{
	/* I think --version should not die immediately. */
	version();
	return 0;
}

static inline bool parse_opt_W(const char *arg)
{
	int id_character = tolower(arg[0]);
	WRDTracer **wpp = wrdt_list;
	int i;
	
	for (i = 0; wpp[i] != NULL; i++) {
		WRDTracer *wp = wpp[i];
		
		if (wp->id == id_character) {
			wrdt = wp;
			free(wrdt_open_opts);
			wrdt_open_opts = safe_strdup(arg + 1);
			return 0;
		}
	}
}

static inline bool parse_opt_238(char *arg)
{
	/* --wrd-read-opts */
	put_string_table(&wrd_read_opts, arg, strlen(arg));
	return 0;
}

static inline bool parse_opt_x(char *arg)
{
	StringTableNode *st;
	
	if ((st = put_string_table(&opt_config_string,
			arg, strlen(arg))) != NULL)
		expand_escape_string(st->string);
	return 0;
}

static inline bool parse_opt_Z(char *arg)
{
	return load_table(arg);
}

static inline bool parse_opt_226(const char *arg)
{
	/* --temperament */
	int32 tmpi32;
	
	if (! strncmp(arg, "pure", 4)) {
		opt_pure_intonation = 1;
		if (arg[4] != '\0') {
			if (set_value(&tmpi32, atoi(arg + 4), -7, -7,
					"Initial keysig (number of #(+)/b(-)[m(minor)])"))
				return 1;
			if (strchr(arg + 4, 'm'))
				opt_init_keysig = tmpi32 + 16;
			else
				opt_init_keysig = tmpi32;
		}
	}
}

__attribute__((noreturn))
static inline bool parse_opt_666(const char *arg)
{
	/* getopt_long failed to recognize any options */
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			"Could not understand option : try --help");
	exit(1);
}

__attribute__((pure))
static inline bool y_or_n_p(const char *arg)
{
	if (arg) {
		switch (arg[0]) {
		case 'y':
		case 'Y':
		case 't':
		case 'T':
			return 1;
		case 'n':
		case 'N':
		case 'f':
		case 'F':
		default:
			return 0;
		}
	} else
		return 1;
}

static inline bool set_flag(int32 *fields, int32 bitmask, const char *arg)
{
	bool set = y_or_n_p(arg);
	
	if (set)
		*fields |= bitmask;
	else
		*fields &= ~bitmask;
	return 0;
}

/* -------- functions for getopt_long ends here --------- */

#ifdef HAVE_SIGNAL
static RETSIGTYPE sigterm_exit(int sig)
{
    char s[4];

    /* NOTE: Here, fprintf is dangerous because it is not re-enterance
     * function.  It is possible coredump if the signal is called in printf's.
     */

    write(2, "Terminated sig=0x", 17);
    s[0] = "0123456789abcdef"[(sig >> 4) & 0xf];
    s[1] = "0123456789abcdef"[sig & 0xf];
    s[2] = '\n';
    write(2, s, 3);

    safe_exit(1);
}
#endif /* HAVE_SIGNAL */

static void timidity_arc_error_handler(char *error_message)
{
    extern int open_file_noise_mode;
    if(open_file_noise_mode)
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "%s", error_message);
}

MAIN_INTERFACE void timidity_start_initialize(void)
{
    int i;
    static int drums[] = DEFAULT_DRUMCHANNELS;
    static int is_first = 1;
#if defined(__FreeBSD__) && !defined(__alpha__)
    fp_except_t fpexp;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
    fp_except fpexp;
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    fpexp = fpgetmask();
    fpsetmask(fpexp & ~(FP_X_INV|FP_X_DZ));
#endif

    if(!output_text_code)
	output_text_code = safe_strdup(OUTPUT_TEXT_CODE);
    if(!opt_aq_max_buff)
	opt_aq_max_buff = safe_strdup("5.0");
    if(!opt_aq_fill_buff)
	opt_aq_fill_buff = safe_strdup("100%");

    /* Check the byte order */
    i = 1;
#ifdef LITTLE_ENDIAN
    if(*(char *)&i != 1)
#else
    if(*(char *)&i == 1)
#endif
    {
	fprintf(stderr, "Byte order is miss configured.\n");
	exit(1);
    }

    CLEAR_CHANNELMASK(quietchannels);
    CLEAR_CHANNELMASK(default_drumchannels);

    for(i = 0; drums[i] > 0; i++)
	SET_CHANNELMASK(default_drumchannels, drums[i] - 1);
#if MAX_CHANNELS > 16
    for(i = 16; i < MAX_CHANNELS; i++)
	if(IS_SET_CHANNELMASK(default_drumchannels, i & 0xF))
	    SET_CHANNELMASK(default_drumchannels, i);
#endif

    if(program_name == NULL)
	program_name = "TiMidity";
    uudecode_unquote_html = 1;
    for(i = 0; i < MAX_CHANNELS; i++)
    {
	default_program[i] = DEFAULT_PROGRAM;
	memset(channel[i].drums, 0, sizeof(channel[i].drums));
    }
    arc_error_handler = timidity_arc_error_handler;

    if(play_mode == NULL)
    {
	char *output_id;
	int i;

	play_mode = play_mode_list[0];
	output_id = getenv("TIMIDITY_OUTPUT_ID");
#ifdef TIMIDITY_OUTPUT_ID
	if(output_id == NULL)
	    output_id = TIMIDITY_OUTPUT_ID;
#endif /* TIMIDITY_OUTPUT_ID */
	if(output_id != NULL)
	{
	    for(i = 0; play_mode_list[i]; i++)
		if(play_mode_list[i]->id_character == *output_id)
		{
		    play_mode = play_mode_list[i];
		    break;
		}
	}
    }

    if(is_first) /* initialize once time */
    {
	got_a_configuration = 0;

#ifdef SUPPORT_SOCKET
	init_mail_addr();
	if(url_user_agent == NULL)
	{
	    url_user_agent =
		(char *)safe_malloc(10 + strlen(timidity_version));
	    strcpy(url_user_agent, "TiMidity-");
	    strcat(url_user_agent, timidity_version);
	}
#endif /* SUPPORT_SOCKET */

	for(i = 0; url_module_list[i]; i++)
	    url_add_module(url_module_list[i]);
	init_string_table(&opt_config_string);
	init_freq_table();
	init_freq_table_tuning();
	init_freq_table_pytha();
	init_freq_table_meantone();
	init_freq_table_pureint();
	init_freq_table_user();
	init_bend_fine();
	init_bend_coarse();
	init_tables();
	init_attack_vol_table();
	init_sb_vol_table();
	init_convex_vol_table();
	init_def_vol_table();
	init_gs_vol_table();
	init_perceived_vol_table();
#ifdef SUPPORT_SOCKET
	url_news_connection_cache(URL_NEWS_CONN_CACHE);
#endif /* SUPPORT_SOCKET */
	for(i = 0; i < NSPECIAL_PATCH; i++)
	    special_patch[i] = NULL;
	init_midi_trace();
	int_rand(-1);	/* initialize random seed */
	int_rand(42);	/* the 1st number generated is not very random */
	ML_RegisterAllLoaders ();
    }

    is_first = 0;
}

MAIN_INTERFACE int timidity_pre_load_configuration(void)
{
#if defined(__W32__)
    /* Windows */
    char *strp;
    int check;
    char local[1024];

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
    extern char *ConfigFile;
    if(!ConfigFile[0]) {
      GetWindowsDirectory(ConfigFile, 1023 - 13);
      strcat(ConfigFile, "\\TIMIDITY.CFG");
    }
    strncpy(local, ConfigFile, sizeof(local) - 1);
#else
    /* !IA_W32GUI */
    GetWindowsDirectory(local, 1023 - 13);
    strcat(local, "\\TIMIDITY.CFG");
#endif

    /* First, try read system configuration file.
     * Default is C:\WINDOWS\TIMIDITY.CFG
     */
    if((check = open(local, 0)) >= 0)
    {
	close(check);
	if(!read_config_file(local, 0))
	    got_a_configuration = 1;
    }

    /* Next, try read configuration file which is in the
     * TiMidity directory.
     */
    if(GetModuleFileName(NULL, local, 1023))
    {
        local[1023] = '\0';
	if(strp = strrchr(local, '\\'))
	{
	    *(++strp)='\0';
	    strcat(local,"TIMIDITY.CFG");
	    if((check = open(local, 0)) >= 0)
	    {
		close(check);
		if(!read_config_file(local, 0))
		    got_a_configuration = 1;
	    }
	}
    }

#else
    /* UNIX */
    if(!read_config_file(CONFIG_FILE, 0))
	got_a_configuration = 1;
#endif

    /* Try read configuration file which is in the
     * $HOME (or %HOME% for DOS) directory.
     * Please setup each user preference in $HOME/.timidity.cfg
     * (or %HOME%/timidity.cfg for DOS)
     */

    if(read_user_config_file())
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Warning: Can't read ~/.timidity.cfg correctly");
    return 0;
}

MAIN_INTERFACE int timidity_post_load_configuration(void)
{
    int cmderr;

    cmderr = 0;
    if(!got_a_configuration)
    {
	if(try_config_again && !read_config_file(CONFIG_FILE, 0))
	    got_a_configuration = 1;
    }

    if(opt_config_string.nstring > 0)
    {
	char **config_string_list;
	int i;

	config_string_list = make_string_array(&opt_config_string);
	if(config_string_list != NULL)
	{
	    for(i = 0; config_string_list[i]; i++)
	    {
		if(!read_config_file(config_string_list[i], 1))
		    got_a_configuration = 1;
		else
		    cmderr++;
	    }
	    free(config_string_list[0]);
	    free(config_string_list);
	}
    }

    if(!got_a_configuration)
	cmderr++;
    return cmderr;
}

MAIN_INTERFACE void timidity_init_player(void)
{
    /* Set play mode parameters */
    if(opt_output_rate != 0)
	play_mode->rate = opt_output_rate;
    else if(play_mode->rate == 0)
	play_mode->rate = DEFAULT_RATE;

    /* save defaults */
    COPY_CHANNELMASK(drumchannels, default_drumchannels);
    COPY_CHANNELMASK(drumchannel_mask, default_drumchannel_mask);

    if(opt_buffer_fragments != -1)
    {
	if(play_mode->flag & PF_BUFF_FRAGM_OPT)
	    play_mode->extra_param[0] = opt_buffer_fragments;
	else
	    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		      "%s: -B option is ignored", play_mode->id_name);
    }

#ifdef SUPPORT_SOUNDSPEC
    if(view_soundspec_flag)
    {
	open_soundspec();
	soundspec_setinterval(spectrogram_update_sec);
    }
#endif /* SOUNDSPEC */
}

void timidity_init_aq_buff(void)
{
    double time1, /* max buffer */
	   time2, /* init filled */
	   base;  /* buffer of device driver */

    if(!IS_STREAM_TRACE)
	return; /* Ignore */

    time1 = atof(opt_aq_max_buff);
    time2 = atof(opt_aq_fill_buff);
    base  = (double)aq_get_dev_queuesize() / play_mode->rate;
    if(strchr(opt_aq_max_buff, '%'))
    {
	time1 = base * (time1 - 100) / 100.0;
	if(time1 < 0)
	    time1 = 0;
    }
    if(strchr(opt_aq_fill_buff, '%'))
	time2 = base * time2 / 100.0;
    aq_set_soft_queue(time1, time2);
}

MAIN_INTERFACE int timidity_play_main(int nfiles, char **files)
{
    int need_stdin = 0, need_stdout = 0;
    int i;
    int output_fail = 0;

    if(nfiles == 0 && !strchr(INTERACTIVE_INTERFACE_IDS, ctl->id_character))
	return 0;

    if(opt_output_name)
    {
	play_mode->name = opt_output_name;
	if(!strcmp(opt_output_name, "-"))
	    need_stdout = 1;
    }

    for(i = 0; i < nfiles; i++)
	if (!strcmp(files[i], "-"))
	    need_stdin = 1;

    if(ctl->open(need_stdin, need_stdout))
    {
	fprintf(stderr, "Couldn't open %s (`%c')" NLS,
		ctl->id_name, ctl->id_character);
	play_mode->close_output();
	return 3;
    }

    if(wrdt->open(wrdt_open_opts))
    {
	fprintf(stderr, "Couldn't open WRD Tracer: %s (`%c')" NLS,
		wrdt->name, wrdt->id);
	play_mode->close_output();
	ctl->close();
	return 1;
    }

#ifdef BORLANDC_EXCEPTION
    __try
    {
#endif /* BORLANDC_EXCEPTION */
#ifdef __W32__

#ifdef HAVE_SIGNAL
	signal(SIGTERM, sigterm_exit);
#endif
	SetConsoleCtrlHandler(handler, TRUE);

	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "Initialize for Critical Section");
	InitializeCriticalSection(&critSect);
	if(opt_evil_mode)
	    if(!SetThreadPriority(GetCurrentThread(),
				  THREAD_PRIORITY_ABOVE_NORMAL))
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Error raising process priority");

#else
	/* UNIX */
#ifdef HAVE_SIGNAL
	signal(SIGINT, sigterm_exit);
	signal(SIGTERM, sigterm_exit);
#ifdef SIGPIPE
	signal(SIGPIPE, sigterm_exit);    /* Handle broken pipe */
#endif /* SIGPIPE */
#endif /* HAVE_SIGNAL */

#endif

	/* Open output device */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "Open output: %c, %s",
		  play_mode->id_character,
		  play_mode->id_name);

	if (play_mode->flag & PF_PCM_STREAM) {
	    play_mode->extra_param[1] = aq_calc_fragsize();
	    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		      "requesting fragment size: %d",
		      play_mode->extra_param[1]);
	}
#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
	if(play_mode->open_output() < 0)
	{
	    ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		      "Couldn't open %s (`%c')",
		      play_mode->id_name, play_mode->id_character);
	    output_fail = 1;
	    ctl->close();
	    return 2;
	}
#endif /* IA_W32GUI */
	if(!control_ratio)
	{
	    control_ratio = play_mode->rate / CONTROLS_PER_SECOND;
	    if(control_ratio < 1)
		control_ratio = 1;
	    else if (control_ratio > MAX_CONTROL_RATIO)
		control_ratio = MAX_CONTROL_RATIO;
	}

	init_load_soundfont();
	if(!output_fail)
	{
	    aq_setup();
	    timidity_init_aq_buff();
	}
	if(allocate_cache_size > 0)
	    resamp_cache_reset();

	if(*def_instr_name)
	    set_default_instrument(def_instr_name);

	if(ctl->flags & CTLF_LIST_RANDOM)
	    randomize_string_list(files, nfiles);
	else if(ctl->flags & CTLF_LIST_SORT)
	    sort_pathname(files, nfiles);

	/* Return only when quitting */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "pass_playing_list() nfiles=%d", nfiles);

	ctl->pass_playing_list(nfiles, files);

	if(intr)
	    aq_flush(1);

#ifdef XP_UNIX
	return 0;
#endif /* XP_UNIX */

	play_mode->close_output();
	ctl->close();
	wrdt->close();
#ifdef __W32__
	DeleteCriticalSection (&critSect);
#endif

#ifdef BORLANDC_EXCEPTION
    } __except(1) {
	fprintf(stderr, "\nError!!!\nUnexpected Exception Occured!\n");
	if(play_mode->fd != -1)
	{
		play_mode->purge_output();
		play_mode->close_output();
	}
	ctl->close();
	wrdt->close();
	DeleteCriticalSection (&critSect);
	exit(EXIT_FAILURE);
    }
#endif /* BORLANDC_EXCEPTION */

#ifdef SUPPORT_SOUNDSPEC
    if(view_soundspec_flag)
	close_soundspec();
#endif /* SUPPORT_SOUNDSPEC */

    free_archive_files();
#ifdef SUPPORT_SOCKET
    url_news_connection_cache(URL_NEWS_CLOSE_CACHE);
#endif /* SUPPORT_SOCKET */

    return 0;
}

#ifdef IA_W32GUI
int w32gSecondTiMidity(int opt, int argc, char **argv);
int w32gSecondTiMidityExit(void);
int w32gLoadDefaultPlaylist(void);
int w32gSaveDefaultPlaylist(void);
extern int volatile save_playlist_once_before_exit_flag;
#endif /* IA_W32GUI */

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
static int CoInitializeOK = 0;
#endif

#ifndef __MACOS__
#ifdef __W32__ /* Windows */
#if ( (!defined(IA_W32GUI) || defined(__CYGWIN32__) || defined(__MINGW32__)) && !defined(IA_W32G_SYN) )
/* Cygwin or Console */
int __cdecl main(int argc, char **argv)
#else
/* _MSC_VER, _BORLANDC_ */
int win_main(int argc, char **argv)
#endif
#else /* UNIX */
int main(int argc, char **argv)
#endif
{
    int c, err;
    int nfiles;
    char **files;
    int main_ret;
    int longind;

#if defined(DANGEROUS_RENICE) && !defined(__W32__) && !defined(main)
    /*
     * THIS CODES MUST EXECUT BEGINNING OF MAIN FOR SECURITY.
     * DONT PUT ANY CODES ABOVE.
     */
#include <sys/resource.h>
    int uid;
#ifdef sun
    extern int setpriority(int which, id_t who, int prio);
    extern int setreuid(int ruid, int euid);
#endif

    uid = getuid();
    if(setpriority(PRIO_PROCESS, 0, DANGEROUS_RENICE) < 0)
    {
	perror("setpriority");
	fprintf(stderr, "Couldn't set priority to %d.", DANGEROUS_RENICE);
    }
    setreuid(uid, uid);
#endif

#if defined(REDIRECT_STDOUT)
    memcpy(stdout, fopen(REDIRECT_STDOUT, "a+"), sizeof(FILE));
    printf("TiMidity++ start\n");fflush(stdout);
#endif

#ifdef main
    {
	static int maincnt = 0;
	if(maincnt++ > 0)
	{
	    argv++;
	    argc--;
	    while(argv[0][0] == '-') {
		argv++;
		argc--;
	    }
	    ctl->pass_playing_list(argc, argv);
	    return 0;
	}
    }
#endif

#ifdef IA_DYNAMIC
    {
#ifdef XP_UNIX
	argv[0] = "netscape";
#endif /* XP_UNIX */
	dynamic_interface_id = 0;
	dl_init(argc, argv);
    }
#endif /* IA_DYNAMIC */

    if((program_name=pathsep_strrchr(argv[0]))) program_name++;
    else program_name=argv[0];

    if(strncmp(program_name,"timidity",8) == 0);
    else if(strncmp(program_name,"kmidi",5) == 0) set_ctl("q");
    else if(strncmp(program_name,"tkmidi",6) == 0) set_ctl("k");
    else if(strncmp(program_name,"gtkmidi",6) == 0) set_ctl("g");
    else if(strncmp(program_name,"xmmidi",6) == 0) set_ctl("m");
    else if(strncmp(program_name,"xawmidi",7) == 0) set_ctl("a");
    else if(strncmp(program_name,"xskinmidi",9) == 0) set_ctl("i");

    if(argc == 1 && !strchr(INTERACTIVE_INTERFACE_IDS, ctl->id_character))
    {
	interesting_message();
	return 0;
    }

    timidity_start_initialize();
#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
    if(CoInitialize(NULL)==S_OK)
      CoInitializeOK = 1;
    w32g_initialize();

#ifdef IA_W32GUI
	/* Secondary TiMidity Execute */
	/*	FirstLoadIniFile(); */
	if(w32gSecondTiMidity(SecondMode,argc,argv)==FALSE){
		return 0;
	}
#endif
	
    for(c = 1; c < argc; c++)
    {
	if(is_directory(argv[c]))
	{
	    char *p;
	    p = (char *)safe_malloc(strlen(argv[c]) + 2);
	    strcpy(p, argv[c]);
	    directory_form(p);
	    argv[c] = p;
	}
    }
#endif

#ifdef AU_ARTS
    if(play_mode == NULL && arts_init()==0) {
	    arts_free();
	    set_play_mode("k");
    }
#endif
#ifdef AU_PORTAUDIO
    if(play_mode == NULL)
	    set_play_mode("p");
#endif
#ifdef AU_ESD
    if(play_mode == NULL) {
	    if(!access("/usr/lib/libesd.so.0", R_OK)) {
		    setenv("ESD_NO_SPAWN", "1", 0);
		    set_play_mode("e");
	    }
    }
#endif
#ifdef AU_OSS
    if(play_mode == NULL)
	    set_play_mode("d");
#endif
    
    if((err = timidity_pre_load_configuration()) != 0)
	return err;

	while ((c = getopt_long(argc, argv, OPTCOMMANDS, longopts, &longind)) > 0)
		if ((err = set_tim_opt_long(c, optarg, longind)) != 0)
			break;

#if defined(NEWTON_INTERPOLATION) || defined(GAUSS_INTERPOLATION)
    initialize_newton_coeffs();
#endif
#ifdef GAUSS_INTERPOLATION
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Initializing Gauss table...");
    initialize_gauss_table(gauss_n);
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Done");
#endif

    err += timidity_post_load_configuration();

    /* If there were problems, give up now */
    if(err || (optind >= argc &&
	       !strchr(INTERACTIVE_INTERFACE_IDS, ctl->id_character)))
    {
	if(!got_a_configuration)
	{
#ifdef __W32__
	    char config1[1024];
	    char config2[1024];

	    memset(config1, 0, sizeof(config1));
	    memset(config2, 0, sizeof(config2));
#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
	    {
		extern char *ConfigFile;
		strncpy(config1, ConfigFile, sizeof(config1) - 1);
	    }
#else
	    /* !IA_W32GUI */
	    GetWindowsDirectory(config1, 1023 - 13);
	    strcat(config1, "\\TIMIDITY.CFG");
#endif

	    if(GetModuleFileName(NULL, config2, 1023))
	    {
		char *strp;
		config2[1023] = '\0';
		if(strp = strrchr(config2, '\\'))
		{
		    *(++strp)='\0';
		    strcat(config2,"TIMIDITY.CFG");
		}
	    }

	    ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		      "%s: Can't read any configuration file.\nPlease check "
		      "%s or %s", program_name, config1, config2);
#else
	    ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		      "%s: Can't read any configuration file.\nPlease check "
		      CONFIG_FILE, program_name);
#endif /* __W32__ */
	}
	else
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "Try %s -h for help", program_name);
	}

#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN ) /* Try to continue if it is Windows version */
	return 1; /* problems with command line */
#endif
    }

    timidity_init_player();

    nfiles = argc - optind;
    files  = argv + optind;
    if(nfiles > 0 && ctl->id_character != 'r' && ctl->id_character != 'A' && ctl->id_character != 'W' && ctl->id_character != 'P')
	files = expand_file_archives(files, &nfiles);
    if(dumb_error_count)
	sleep(1);

#ifndef IA_W32GUI
    main_ret = timidity_play_main(nfiles, files);
#ifdef IA_W32G_SYN
    if(CoInitializeOK)
      CoUninitialize();
#endif /* IA_W32G_SYN */
#else
	w32gLoadDefaultPlaylist();
    main_ret = timidity_play_main(nfiles, files);
	if(save_playlist_once_before_exit_flag) {
		save_playlist_once_before_exit_flag = 0;
		w32gSaveDefaultPlaylist();
	}
    w32gSecondTiMidityExit();
    if(CoInitializeOK)
      CoUninitialize();
#endif /* IA_W32GUI */
    free_instruments(0);
    free_global_mblock();
    free_all_midi_file_info();
	free_userdrum();
	free_userinst();
    tmdy_free_config();
	free_effect_buffers();
    return main_ret;
}
#endif /* __MACOS__ */

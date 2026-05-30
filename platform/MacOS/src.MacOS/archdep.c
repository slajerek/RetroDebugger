/*
 * archdep_unix.c - Miscellaneous system-specific stuff.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <pwd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <mach/mach_time.h>

#include "vice_sdl.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#if defined(__QNX__) && !defined(__QNXNTO__)
#include <sys/time.h>
#include <sys/timers.h>
#endif

#include "archdep.h"
#include "findpath.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "monitor.h"
#include "platform.h"
#include "ui.h"
#include "util.h"

#ifdef __NeXT__
#define waitpid(p, s, o) wait3((union wait *)(s), (o), (struct rusage *) 0)
#endif

static char *argv0 = NULL;
static char *boot_path = NULL;

/* alternate storage of preferences */
const char *archdep_pref_path = NULL; /* NULL -> use home_path + ".vice" */

#if defined(DINGUX) || defined(DINGUX_SDL) || defined(GP2X) || defined(GP2X_SDL)
#define USE_PROC_SELF_EXE
#define USE_EXE_RELATIVE_TMP
#endif

#if defined(__QNX__) && !defined(__QNXNTO__)
int archdep_rtc_get_centisecond(void)
{
	struct timespec dtm;
	int status;
	
	if ((status = clock_gettime(CLOCK_REALTIME, &dtm)) == 0) {
		return dtm.tv_nsec / 10000L;
	}
	return 0;
}
#endif

int archdep_network_init(void)
{
	return 0;
}

void archdep_network_shutdown(void)
{
}

/* VICE 3.10: replaced ioutil_access(); ARCHDEP_ACCESS_* values match POSIX. */
int archdep_access(const char *pathname, int mode)
{
	return access(pathname, mode);
}

/* VICE 3.10 tick subsystem (mach_absolute_time on macOS). Vanilla's
   arch/shared/archdep_tick.c is gated on configure-set MACOS_COMPILE which RD's
   Xcode build doesn't define, so the implementation lives here. */
static mach_timebase_info_data_t c64d_timebase_info;

void tick_init(void)
{
	mach_timebase_info(&c64d_timebase_info);
}

tick_t tick_per_second(void)
{
	return TICK_PER_SECOND;
}

tick_t tick_now(void)
{
	return NANO_TO_TICK(mach_absolute_time() * c64d_timebase_info.numer / c64d_timebase_info.denom);
}

void tick_sleep(tick_t sleep_ticks)
{
	uint64_t nanos = TICK_TO_NANO(sleep_ticks);
	struct timespec ts;

	if (nanos < NANO_PER_SECOND) {
		ts.tv_sec = 0;
		ts.tv_nsec = nanos;
	} else {
		ts.tv_sec = nanos / NANO_PER_SECOND;
		ts.tv_nsec = nanos % NANO_PER_SECOND;
	}

	nanosleep(&ts, NULL);
}

tick_t tick_now_after(tick_t previous_tick)
{
	tick_t after = tick_now();

	if (after == previous_tick - 1) {
		after = previous_tick;
	}

	return after;
}

tick_t tick_now_delta(tick_t previous_tick)
{
	return tick_now_after(previous_tick) - previous_tick;
}

static int archdep_init_extra(int *argc, char **argv)
{
#ifdef USE_PROC_SELF_EXE
	ssize_t read;
	
	argv0 = lib_malloc(ioutil_maxpathlen());
	read = readlink("/proc/self/exe", argv0, ioutil_maxpathlen() - 1);
	
	if (read == -1) {
		return 1;
	}
	else {
		argv0[read] = '\0';
	}
	/* set this up now to remove extra .vice directory */
	archdep_pref_path = archdep_boot_path();
	
#else
	argv0 = lib_strdup(argv[0]);
#endif
	return 0;
}

const char *archdep_program_name(void)
{
	static char *program_name = NULL;
	
	if (program_name == NULL) {
		char *p;
		
		p = strrchr(argv0, '/');
		if (p == NULL) {
			program_name = lib_strdup(argv0);
		} else {
			program_name = lib_strdup(p + 1);
		}
	}
	
	return program_name;
}

const char *archdep_boot_path(void)
{
	if (boot_path == NULL) {
#ifdef USE_PROC_SELF_EXE
		/* known from setup in archdep_init_extra() so just reuse it */
		boot_path = lib_strdup(argv0);
#else
		boot_path = findpath(argv0, getenv("PATH"), NULL, IOUTIL_ACCESS_X_OK);
#endif
		
		/* Remove the program name.  */
		*strrchr(boot_path, '/') = '\0';
	}
	
	return boot_path;
}

const char *archdep_home_path(void)
{
#ifdef USE_PROC_SELF_EXE
	/* everything is relative to the location of the exe which is already known
	 from archdep_init_bootpath() so just reuse it */
	return (archdep_boot_path());
#else
	char *home;
	
	home = getenv("HOME");
	if (home == NULL) {
		struct passwd *pwd;
		
		pwd = getpwuid(getuid());
		if ((pwd == NULL) || ((home = pwd->pw_dir) == NULL)) {
			/* give up */
			home = ".";
		}
	}
	
	return home;
#endif
}

char *archdep_default_sysfile_pathlist(const char *emu_id)
{
	static char *default_path;
	
#if defined(MINIXVMD) || defined(MINIX_SUPPORT)
	static char *default_path_temp;
#endif
	
	if (default_path == NULL) {
		const char *boot_path;
		const char *home_path;
		
		boot_path = archdep_boot_path();
		home_path = archdep_home_path();
		
		/* First search in the `LIBDIR' then the $HOME/.vice/ dir (home_path)
		 and then in the `boot_path'.  */
		
#if defined(MINIXVMD) || defined(MINIX_SUPPORT)
		default_path_temp = util_concat(LIBDIR, "/", emu_id, ARCHDEP_FINDPATH_SEPARATOR_STRING,
										home_path, "/", VICEUSERDIR, "/", emu_id,NULL);
		
		default_path = util_concat(default_path_temp, ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   boot_path, "/", emu_id, ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   LIBDIR, "/DRIVES", ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   home_path, "/", VICEUSERDIR, "/DRIVES", ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   boot_path, "/DRIVES", ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   LIBDIR, "/PRINTER", ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   home_path, "/", VICEUSERDIR, "/PRINTER", ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   boot_path, "/PRINTER", NULL);
		lib_free(default_path_temp);
		
#else
		/* VICE 3.10 changed the contract: sysfile_open now appends a subpath ("C64",
		   "DRIVES", "PRINTER", ...) onto each search-path element. The 3.1-vintage
		   list below baked the machine name *into* each path, which would now produce
		   "<dir>/C64/C64/kernal-901227-03.bin" -- double-suffix, never found, hard
		   reset jumps to PC=$0000. Return generic VICE-data roots and let the caller
		   supply the subpath. (void)emu_id since the caller already encodes it.
		   Also add /opt/homebrew/share/vice for arm64 Homebrew (LIBDIR points at
		   /usr/local/lib/vice from the legacy Intel prefix). */
		(void)emu_id;
		default_path = util_concat("/opt/homebrew/share/vice", ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   LIBDIR, ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   home_path, "/", VICEUSERDIR, ARCHDEP_FINDPATH_SEPARATOR_STRING,
								   boot_path, NULL);
#endif
	}
	
	return default_path;
}

/* Return a malloc'ed backup file name for file `fname'.  */
char *archdep_make_backup_filename(const char *fname)
{
	return util_concat(fname, "~", NULL);
}

char *archdep_default_resource_file_name(void)
{
	if (archdep_pref_path == NULL) {
		const char *home;
		
		home = archdep_home_path();
		return util_concat(home, "/.vice/sdl-vicerc", NULL);
	} else {
		return util_concat(archdep_pref_path, "/sdl-vicerc", NULL);
	}
}

char *archdep_default_fliplist_file_name(void)
{
	if (archdep_pref_path==NULL) {
		const char *home;
		
		home = archdep_home_path();
		return util_concat(home, "/.vice/fliplist-", machine_get_name(), ".vfl", NULL);
	} else {
		return util_concat(archdep_pref_path, "/fliplist-", machine_get_name(), ".vfl", NULL);
	}
}

char *archdep_default_autostart_disk_image_file_name(void)
{
	if (archdep_pref_path==NULL) {
		const char *home;
		
		home = archdep_home_path();
		return util_concat(home, "/.vice/autostart-", machine_get_name(), ".d64", NULL);
	} else {
		return util_concat(archdep_pref_path, "/autostart-", machine_get_name(), ".d64", NULL);
	}
}

char *archdep_default_hotkey_file_name(void)
{
	if (archdep_pref_path==NULL) {
		const char *home;
		
		home = archdep_home_path();
		return util_concat(home, "/.vice/sdl-hotkey-", machine_get_name(), ".vkm", NULL);
	} else {
		return util_concat(archdep_pref_path, "/sdl-hotkey-", machine_get_name(), ".vkm", NULL);
	}
}

char *archdep_default_joymap_file_name(void)
{
	if (archdep_pref_path == NULL) {
		const char *home;
		
		home = archdep_home_path();
		return util_concat(home, "/.vice/sdl-joymap-", machine_get_name(), ".vjm", NULL);
	} else {
		return util_concat(archdep_pref_path, "/sdl-joymap-", machine_get_name(), ".vjm", NULL);
	}
}

char *archdep_default_save_resource_file_name(void)
{
	char *fname;
	const char *home;
	const char *viceuserdir;
	
	if (archdep_pref_path == NULL) {
		home = archdep_home_path();
		viceuserdir = util_concat(home, "/.vice", NULL);
	} else {
		viceuserdir = archdep_pref_path;
	}
	
	if (access(viceuserdir, F_OK)) {
		mkdir(viceuserdir, 0700);
	}
	
	fname = util_concat(viceuserdir, "/sdl-vicerc", NULL);
	
	if (archdep_pref_path == NULL) {
		lib_free(viceuserdir);
	}
	
	return fname;
}

#if defined(MACOSX_COCOA)
FILE *default_log_file = NULL;

FILE *archdep_open_default_log_file(void)
{
	return default_log_file;
}
#else
FILE *archdep_open_default_log_file(void)
{
	return stdout;
}
#endif

int archdep_num_text_lines(void)
{
	char *s;
	
	s = getenv("LINES");
	if (s == NULL) {
		printf("No LINES!\n");
		return -1;
	}
	return atoi(s);
}

int archdep_num_text_columns(void)
{
	char *s;
	
	s = getenv("COLUMNS");
	if (s == NULL) {
		return -1;
	}
	return atoi(s);
}

int archdep_default_logger(const char *level_string, const char *txt)
{
	if (fputs(level_string, stdout) == EOF || fprintf(stdout, "%s", txt) < 0 || fputc('\n', stdout) == EOF) {
		return -1;
	}
	return 0;
}

int archdep_path_is_relative(const char *path)
{
	if (path == NULL) {
		return 0;
	}
	
	return *path != '/';
}

int archdep_spawn(const char *name, char **argv, char **pstdout_redir, const char *stderr_redir)
{
	pid_t child_pid;
	int child_status;
	char *stdout_redir = NULL;
	
	if (pstdout_redir != NULL) {
		if (*pstdout_redir == NULL) {
			*pstdout_redir = archdep_tmpnam();
		}
		stdout_redir = *pstdout_redir;
	}
	
	child_pid = vfork();
	if (child_pid < 0) {
		log_error(LOG_DEFAULT, "vfork() failed: %s.", strerror(errno));
		return -1;
	} else {
		if (child_pid == 0) {
			if (stdout_redir && freopen(stdout_redir, "w", stdout) == NULL) {
				log_error(LOG_DEFAULT, "freopen(\"%s\") failed: %s.", stdout_redir, strerror(errno));
				_exit(-1);
			}
			if (stderr_redir && freopen(stderr_redir, "w", stderr) == NULL) {
				log_error(LOG_DEFAULT, "freopen(\"%s\") failed: %s.", stderr_redir, strerror(errno));
				_exit(-1);
			}
			execvp(name, argv);
			_exit(-1);
		}
	}
	
	if (waitpid(child_pid, &child_status, 0) != child_pid) {
		log_error(LOG_DEFAULT, "waitpid() failed: %s", strerror(errno));
		return -1;
	}
	
	if (WIFEXITED(child_status)) {
		return WEXITSTATUS(child_status);
	} else {
		return -1;
	}
}

/* return malloc'd version of full pathname of orig_name */
int archdep_expand_path(char **return_path, const char *orig_name)
{
	/* Unix version.  */
	if (*orig_name == '/') {
		*return_path = lib_strdup(orig_name);
	} else {
		static char *cwd;
		
		cwd = ioutil_current_dir();
		*return_path = util_concat(cwd, "/", orig_name, NULL);
		lib_free(cwd);
	}
	return 0;
}

void archdep_startup_log_error(const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

char *archdep_filename_parameter(const char *name)
{
	/* nothing special(?) */
	return lib_strdup(name);
}

char *archdep_quote_parameter(const char *name)
{
	/*not needed(?) */
	return lib_strdup(name);
}

char *archdep_tmpnam(void)
{
#ifdef HAVE_MKSTEMP
	char *tmp_name;
	const char mkstemp_template[] = "/vice.XXXXXX";
	int fd;
	char *tmp;
	char *final_name;
	
	tmp_name = lib_malloc(ioutil_maxpathlen());
#ifdef USE_EXE_RELATIVE_TMP
	strcpy(tmp_name, archdep_boot_path());
	strcat(tmp_name, "/tmp");
#else
	if ((tmp = getenv("TMPDIR")) != NULL) {
		strncpy(tmp_name, tmp, ioutil_maxpathlen());
		tmp_name[ioutil_maxpathlen() - sizeof(mkstemp_template)] = '\0';
	} else {
		strcpy(tmp_name, "/tmp");
	}
#endif
	strcat(tmp_name, mkstemp_template);
	if ((fd = mkstemp(tmp_name)) < 0) {
		tmp_name[0] = '\0';
	} else {
		close(fd);
	}
	
	final_name = lib_strdup(tmp_name);
	lib_free(tmp_name);
	return final_name;
#else
	return lib_strdup(tmpnam(NULL));
#endif
}

FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
#if defined(HAVE_MKSTEMP)
	char *tmp;
	const char template[] = "/vice.XXXXXX";
	int fildes;
	FILE *fd;
	char *tmpdir;
	
#ifdef USE_EXE_RELATIVE_TMP
	tmp = lib_msprintf("%s/tmp%s", archdep_boot_path(), template);
#else
	tmpdir = getenv("TMPDIR");
	
	if (tmpdir != NULL )
		tmp = util_concat(tmpdir, template, NULL);
	else
		tmp = util_concat("/tmp", template, NULL);
#endif
	
	fildes = mkstemp(tmp);
	
	if (fildes < 0 ) {
		lib_free(tmp);
		return NULL;
	}
	
	fd = fdopen(fildes, mode);
	
	if (fd == NULL) {
		lib_free(tmp);
		return NULL;
	}
	
	*filename = tmp;
	
	return fd;
#else
	char *tmp;
	FILE *fd;
	
	tmp = tmpnam(NULL);
	
	if (tmp == NULL) {
		return NULL;
	}
	
	fd = fopen(tmp, mode);
	
	if (fd == NULL) {
		return NULL;
	}
	
	*filename = lib_strdup(tmp);
	
	return fd;
#endif
}

int archdep_file_is_gzip(const char *name)
{
	size_t l = strlen(name);
	
	if ((l < 4 || strcasecmp(name + l - 3, ".gz")) && (l < 3 || strcasecmp(name + l - 2, ".z")) && (l < 4 || toupper(name[l - 1]) != 'Z' || name[l - 4] != '.')) {
		return 0;
	}
	return 1;
}

int archdep_file_set_gzip(const char *name)
{
	return 0;
}

int archdep_mkdir(const char *pathname, int mode)
{
#ifndef __NeXT__
	return mkdir(pathname, (mode_t)mode);
#else
	return mkdir(pathname, mode);
#endif
}

int archdep_stat(const char *file_name, size_t *len, unsigned int *isdir)
{
	struct stat statbuf;

	if (stat(file_name, &statbuf) < 0) {
		if (len)   { *len = 0; }
		if (isdir) { *isdir = 0; }
		return -1;
	}

	if (len)   { *len = (size_t)statbuf.st_size; }
	if (isdir) { *isdir = S_ISDIR(statbuf.st_mode); }

	return 0;
}

/* set permissions of given file to rw, respecting current umask */
int archdep_fix_permissions(const char *file_name)
{
	mode_t mask = umask(0);
	umask(mask);
	return chmod(file_name, mask ^ 0666);
}

int archdep_file_is_blockdev(const char *name)
{
	struct stat buf;
	
	if (stat(name, &buf) != 0) {
		return 0;
	}
	
	if (S_ISBLK(buf.st_mode)) {
		return 1;
	}
	
	return 0;
}

int archdep_file_is_chardev(const char *name)
{
	struct stat buf;
	
	if (stat(name, &buf) != 0) {
		return 0;
	}
	
	if (S_ISCHR(buf.st_mode)) {
		return 1;
	}
	
	return 0;
}

int archdep_require_vkbd(void)
{
#if defined(DINGUX) || defined(DINGUX_SDL)
	return 1;
#endif
	return 0;
}

static void archdep_shutdown_extra(void)
{
	lib_free(argv0);
	lib_free(boot_path);
}


/* Fetch Platform Stuff for Mac OS X */
#ifdef MACOSX_BUNDLE
#include "../unix/macosx/platform_macosx.c"
#endif

/******************************************************************************/

static RETSIGTYPE break64(int sig)
{
	log_message(LOG_DEFAULT, "Received signal %d, exiting.", sig);
	exit (-1);
}

/*
 used once at init time to setup all signal handlers
 */
void archdep_signals_init(int do_core_dumps)
{
	if (!do_core_dumps) {
		signal(SIGPIPE, break64);
	}
}

typedef void (*signal_handler_t)(int);

static signal_handler_t old_pipe_handler;

static void handle_pipe(int signo)
{
	log_message(LOG_DEFAULT, "Received signal %d, aborting remote monitor.", signo);
	/* monitor_abort(); */
}

/*
 these two are used if the monitor is in remote mode. in this case we might
 get SIGPIPE if the connection is unexpectedly closed.
 */
void archdep_signals_pipe_set(void)
{
	old_pipe_handler = signal(SIGPIPE, (signal_handler_t)handle_pipe);
}

void archdep_signals_pipe_unset(void)
{
	signal(SIGPIPE, old_pipe_handler);
}

char *archdep_get_runtime_os(void)
{
	
	/* Windows on cygwin */
#ifdef __CYGWIN32__
#define RUNTIME_OS_HANDLED
	return platform_get_windows_runtime_os();
#endif
	
	/* MacOSX */
#if defined(MACOSX_COCOA)
#define RUNTIME_OS_HANDLED
	return platform_get_macosx_runtime_os();
#endif
	
	/* Solaris */
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#define RUNTIME_OS_HANDLED
	return platform_get_solaris_runtime_os();
#endif
	
	/* Syllable */
#ifdef __SYLLABLE__
#define RUNTIME_OS_HANDLED
	return platform_get_syllable_runtime_os();
#endif
	
	/* TODO: add runtime os detection code for other *nix os'es */
#ifndef RUNTIME_OS_HANDLED
	return "*nix";
#endif
}

char *archdep_get_runtime_cpu(void)
{
	/* MacOSX */
#if defined(MACOSX_COCOA)
#define RUNTIME_CPU_HANDLED
	return platform_get_macosx_runtime_cpu();
#endif
	
#ifdef __SYLLABLE__
#define RUNTIME_CPU_HANDLED
	return platform_get_syllable_runtime_cpu();
#endif
	
	/* x86/amd64/x86_64 */
#if !defined(RUNTIME_CPU_HANDLED) && (defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__amd64__) || defined(__x86_64__))
#define RUNTIME_CPU_HANDLED
	return platform_get_x86_runtime_cpu();
#endif
	
	/* TODO: add runtime cpu detection code for other cpu's */
#ifndef RUNTIME_CPU_HANDLED
	return "Unknown CPU";
#endif
}

char *archdep_default_rtc_file_name(void)
{
	if (archdep_pref_path == NULL) {
		const char *home;
		
		home = archdep_home_path();
		return util_concat(home, "/.vice/sdl-vice.rtc", NULL);
	} else {
		return util_concat(archdep_pref_path, "/sdl-vice.rtc", NULL);
	}
}

int archdep_rename(const char *oldpath, const char *newpath)
{
	return rename(oldpath, newpath);
}


int archdep_init(int *argc, char **argv)
{
//	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
//		fprintf(stderr, "SDL error: %s\n", SDL_GetError());
//		return 1;
//	}

	/* VICE 3.10 expects the host tick subsystem live before main_program starts.
	   Without this, c64d_timebase_info stays zeroed, tick_now() does an integer
	   divide-by-zero (returns 0 on arm64), the throttle loop sees "no wallclock
	   advance" and lets the OS scheduler dribble cycles in -- C64 runs at ~3 kHz
	   instead of ~1 MHz and the kernal cold-start never reaches the BASIC prompt
	   (black screen). */
	tick_init();

	return archdep_init_extra(argc, argv);
}

void archdep_shutdown(void)
{
//	SDL_Quit();
	archdep_shutdown_extra();
}


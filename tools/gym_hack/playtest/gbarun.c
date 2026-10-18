// Tiny headless GBA runner for checking the hack.
// Usage: gbarun rom.gba < commands
// Commands (one per line):
//   wait N                 run N frames with no keys
//   hold KEYS N            hold keys (e.g. A, B, UP, DOWN+B, START) for N frames
//   tap KEYS [N]           press keys for 4 frames then release for N (default 20) frames, repeat once
//   mash KEYS COUNT [GAP]  tap COUNT times
//   shot FILE.png          screenshot
//   save FILE / load FILE  save state
//   peek ADDR SIZE         print memory (hex)
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#include <mgba/core/config.h>
#include <mgba-util/vfs.h>
#include <mgba-util/png-io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mgba/core/log.h>
#include <stdarg.h>
static void noLog(struct mLogger* l, int cat, enum mLogLevel level, const char* fmt, va_list args) {
	(void) l; (void) cat; (void) level; (void) fmt; (void) args;
}
static struct mLogger nullLogger = { .log = noLog };
static struct mCore* core;
static color_t* buffer;

static int parseKeys(const char* s) {
	int k = 0;
	char tmp[128];
	strncpy(tmp, s, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = 0;
	for (char* t = strtok(tmp, "+"); t; t = strtok(NULL, "+")) {
		if (!strcmp(t, "A")) k |= 1 << 0;
		else if (!strcmp(t, "B")) k |= 1 << 1;
		else if (!strcmp(t, "SELECT")) k |= 1 << 2;
		else if (!strcmp(t, "START")) k |= 1 << 3;
		else if (!strcmp(t, "RIGHT")) k |= 1 << 4;
		else if (!strcmp(t, "LEFT")) k |= 1 << 5;
		else if (!strcmp(t, "UP")) k |= 1 << 6;
		else if (!strcmp(t, "DOWN")) k |= 1 << 7;
		else if (!strcmp(t, "R")) k |= 1 << 8;
		else if (!strcmp(t, "L")) k |= 1 << 9;
		else if (!strcmp(t, "NONE")) k |= 0;
		else fprintf(stderr, "unknown key %s\n", t);
	}
	return k;
}

static void frames(int keys, int n) {
	core->setKeys(core, keys);
	for (int i = 0; i < n; ++i) core->runFrame(core);
	core->setKeys(core, 0);
}

static void shot(const char* path) {
	unsigned w, h;
	core->desiredVideoDimensions(core, &w, &h);
	struct VFile* vf = VFileOpen(path, O_CREAT | O_TRUNC | O_WRONLY);
	png_structp png = PNGWriteOpen(vf);
	png_infop info = PNGWriteHeader(png, w, h);
	PNGWritePixels(png, w, h, 256, buffer);
	PNGWriteClose(png, info);
	vf->close(vf);
}

int main(int argc, char** argv) {
	mLogSetDefaultLogger(&nullLogger);
	core = mCoreFind(argv[1]);
	if (!core || !core->init(core)) return 1;
	mCoreInitConfig(core, NULL);
	buffer = calloc(256 * 256, sizeof(color_t));
	core->setVideoBuffer(core, buffer, 256);
	if (!mCoreLoadFile(core, argv[1])) return 2;
	core->reset(core);
	char line[512];
	setvbuf(stdout, NULL, _IOLBF, 0);
	while (fgets(line, sizeof(line), stdin)) {
		char cmd[32] = {0}, a[256] = {0}, b[64] = {0}, c[64] = {0};
		int n = sscanf(line, "%31s %255s %63s %63s", cmd, a, b, c);
		if (n <= 0 || cmd[0] == '#') continue;
		if (!strcmp(cmd, "wait")) frames(0, atoi(a));
		else if (!strcmp(cmd, "hold")) frames(parseKeys(a), atoi(b));
		else if (!strcmp(cmd, "tap")) { frames(parseKeys(a), 4); frames(0, n > 2 ? atoi(b) : 20); }
		else if (!strcmp(cmd, "mash")) {
			int count = atoi(b), gap = n > 3 ? atoi(c) : 20;
			for (int i = 0; i < count; ++i) { frames(parseKeys(a), 4); frames(0, gap); }
		}
		else if (!strcmp(cmd, "shot")) shot(a);
		else if (!strcmp(cmd, "save")) {
			struct VFile* vf = VFileOpen(a, O_CREAT | O_TRUNC | O_RDWR);
			mCoreSaveStateNamed(core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC);
			vf->close(vf);
		} else if (!strcmp(cmd, "load")) {
			struct VFile* vf = VFileOpen(a, O_RDONLY);
			if (!vf) { fprintf(stderr, "no state %s\n", a); continue; }
			mCoreLoadStateNamed(core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC);
			vf->close(vf);
		} else if (!strcmp(cmd, "peek")) {
			unsigned addr = strtoul(a, NULL, 0), size = strtoul(b, NULL, 0);
			printf("%08x:", addr);
			for (unsigned i = 0; i < size; ++i) printf(" %02x", core->busRead8(core, addr + i));
			printf("\n");
		} else if (!strcmp(cmd, "pos")) {
			unsigned ptr = core->busRead32(core, strtoul(a, NULL, 0));
			short x = core->busRead16(core, ptr), y = core->busRead16(core, ptr + 2);
			signed char g = core->busRead8(core, ptr + 4), m = core->busRead8(core, ptr + 5);
			printf("pos %s map %d.%d x %d y %d\n", n > 2 ? b : "", g, m, x, y);
		} else if (!strcmp(cmd, "reset")) {
			core->reset(core);
		} else if (!strcmp(cmd, "poke8")) {
			core->busWrite8(core, strtoul(a, NULL, 0), strtoul(b, NULL, 0));
		} else if (!strcmp(cmd, "poke32")) {
			core->busWrite32(core, strtoul(a, NULL, 0), strtoul(b, NULL, 0));
		} else if (!strcmp(cmd, "peek32")) {
			printf("%u\n", core->busRead32(core, strtoul(a, NULL, 0)));
		} else if (!strcmp(cmd, "sync")) {
			printf("ok\n");
		} else fprintf(stderr, "unknown command %s\n", cmd);
	}
	return 0;
}

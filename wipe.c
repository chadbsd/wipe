#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <linux/fs.h>

#include "monocypher.h"

long long unsigned size_parse(const char *str);
void update_progress(long long unsigned prog, long long unsigned sz);

int
main(int argc, char *argv[])
{
	int opt, dev;
	long long unsigned sz, bs, prog, ctr;
	ssize_t written;
	unsigned char *block, randbuf[56];

	while ((opt = getopt(argc, argv, "b:")) != -1) {
		switch (opt) {
		case 'b':
			if (optarg == NULL)
				errx(1, "must pass size to -b");
			bs = size_parse(optarg);
			break;
		default:
			errx(1, "unknown opt: %c", opt);
		}
	}
	argv += optind;
	argc -= optind;

	if (!argc) {
		errx(1, "expected device path");
	}

	dev = open(argv[0], O_RDWR | O_DIRECT);
	if (dev == -1) {
		err(1, "error opening device \"%s\"", argv[0]);
	}

	if (ioctl(dev, BLKGETSIZE64, &sz) == -1) {
		err(1, "failed to get device size");
	}

	if (posix_memalign((void**)&block, 512, bs) != 0) {
		err(1, "error allocating block buffer");
	}

	if (getrandom(randbuf, 56, 0) == -1) {
		err(1, "failed to get random seed");
	}

	prog = ctr = 0;
	while (prog < sz) {
		ctr = crypto_chacha20_x(block, NULL, bs, randbuf, randbuf+32, ctr);
		bs = prog + bs > sz ? sz - prog : bs;
		written = write(dev, block, bs);
		if (written == -1) {
			if (errno == EINTR) continue;
			err(1, "error writing to device (%llu/%llu)", prog, sz);
		}
		prog += written;
		update_progress(prog, sz);
	}

	return 0;
}

void
update_progress(long long unsigned prog, long long unsigned sz)
{
	double dprog, dsz;
	int perc;
	dprog = (double)prog / 1024 / 1024 / 1024;
	dsz = (double)sz / 1024 / 1024 / 1024;
	perc = ((double)prog / (double)sz) * 100;
	fprintf(stderr, "\r%.3lfG / %.3lfG [%3d%%]", dprog, dsz, perc);
	fflush(stderr);
}

long long unsigned
size_parse(const char *str)
{
	long long val;
	char *end;

	val = strtoll(str, &end, 10);

	if (errno == ERANGE || val < 0) {
		errx(1, "invalid block size: %s", str);
	}

	switch (tolower(end[0])) {
	case 'b':
		return val;
	case 'k':
		return val * 1024;
	case 'm':
		return val * 1024 * 1024;
	case 'g':
		return val * 1024 * 1024 * 1024;
	case 't':
		errx(1, "block size too big wtf");
	case '.':
		errx(1, "no fractions allowed in size srry");
	}

	return val;
}

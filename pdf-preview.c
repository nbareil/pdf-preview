/*
 *
 * Copyright © 2012  Nicolas Bareil <nico@chdir.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <glib.h>
#include <json.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <poppler.h>
#include <poppler-document.h>
#include <seccomp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define __NR_fstat64 197
#define __NR_stat64 195
#define __NR_mmap2 192

#define SVG_BUFLEN (1024*1024)
#define NAME_MAX 255

#ifdef DEBUG_SECCOMP
#	include "syscall-reporter.h"
#endif

#define ERROR(x...) \
			do { fprintf(stderr, "ERROR: " x); exit(EXIT_FAILURE); } while(0)
#define PERROR(msg) \
			do { perror(msg); exit(EXIT_FAILURE); } while (0)

void sandboxify() {
	if (seccomp_init(SCMP_ACT_TRAP) < 0)
		ERROR("Cannot go into SECCOMPv2");

	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(open), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(openat), 0); // XXXX

	// seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(open), 0);
	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(access), 0);
	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(fcntl), 0); // XXXX
	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(fstat), 0);
	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(fstat64), 0);
	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(stat64), 0);

	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(getdents), 0); // XXXX
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(stat), 0); // XXXX
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(dup), 0); // XXXX
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0); // XXXX

	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(getdents64), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
	// seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(writev), 0); // XXX
	// seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0); // XXX
	// seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(futex), 0); // XXX

	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(mmap2), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(mremap), 0);

	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(gettimeofday), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(time), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);

#ifdef DEBUG_SECCOMP
	if (install_syscall_reporter())
		ERROR("Cannot install syscall reporter");
#endif

	if (seccomp_load() < 0)
		ERROR("Cannot load SECCOMP filters");

	seccomp_release();
}

char * open_pdf_file(const char *path, int *n) {
	struct stat s;
	int fd;
	char *addr;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		PERROR("Cannot open file");

	if (fstat(fd, &s) == -1)
		PERROR("Cannot get file size");

	*n = s.st_size;
	addr = mmap(NULL, *n, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED)
		PERROR("mmap failed");

	return addr;
}

typedef struct {
	size_t free_space;
	char *svg;
	size_t svg_len;
	int pagenum;
	char *text;
} page_t;

void render_page(page_t *page_meta, PopplerPage *page) {;
	cairo_t *cairoctx;
	cairo_surface_t *surface;
	double width, height;
	char outfilename[NAME_MAX];

	poppler_page_get_size(page, &width, &height);
	snprintf(outfilename, sizeof outfilename - 1, "img-%d.svg", page_meta->pagenum);
	surface = cairo_svg_surface_create(outfilename,
		width, height);
	cairoctx = cairo_create(surface);
	if (cairoctx == NULL)
		ERROR("Cannot create a Cairo buffer");
	poppler_page_render(page, cairoctx);
	cairo_surface_show_page(surface);

	cairo_surface_destroy(surface);
	cairo_destroy(cairoctx);
}

int main(int argc, char const *argv[])
{
	char *path;
	PopplerDocument *doc;
	GError *err;
	gchar *gbuf;
	char *buf;
	page_t page_meta;
	int file_length, n;

	g_type_init();

	if (argc != 2) {
		return 1;
	}

	err = NULL;
	buf = open_pdf_file(argv[1], &file_length);

	sandboxify();

	doc = poppler_document_new_from_data(buf, file_length, NULL, &err);
	if (err != NULL) {
		fprintf(stderr, "Unable to open file: %s\n", err->message);
		return 2;
	}

	n = poppler_document_get_n_pages(doc);

	for (int i = 0; i < n; i++) {
		PopplerPage *page = poppler_document_get_page(doc, i);

		page_meta.pagenum = i;
		page_meta.text = poppler_page_get_text(page);
		page_meta.svg_len = 0;
		page_meta.svg = malloc(SVG_BUFLEN);
		if (!page_meta.svg)
			ERROR("Cannot allocate svg buffer, not enought memory?");
		page_meta.free_space = SVG_BUFLEN;

		render_page(&page_meta, page);

		if (page_meta.text)
			free(page_meta.text);
		g_object_unref(page);
	}

	if (munmap(buf, file_length) == -1)
		PERROR("munmap()");

	return 0;
}
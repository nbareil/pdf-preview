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
#include <poppler.h>
#include <poppler-document.h>
#include <seccomp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef DEBUG_SECCOMP
#	include "syscall-reporter.h"
#endif

#define ERROR(x...) \
			do { fprintf(stderr, "ERROR: " x); exit(EXIT_FAILURE); } while(0);
#define PERROR(msg) \
			do { perror(msg); exit(EXIT_FAILURE); } while (0);

void sandboxify() {
	if (seccomp_init(SCMP_ACT_TRAP) < 0)
		ERROR("Cannot go into SECCOMPv2");

	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(open), 0);
	seccomp_rule_add(SCMP_ACT_ERRNO(EPERM), SCMP_SYS(fstat64), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(mmap2), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(gettimeofday), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
	seccomp_rule_add(SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);

#ifdef DEBUG_SECCOMP
	if (install_syscall_reporter())
		ERROR("Cannot install syscall reporter");
#endif

	if (seccomp_load() < 0)
		ERROR("Cannot load SECCOMP filters");

	seccomp_release();
}

char * open_pdf_file(char *path, int *n) {
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
		PERROR("mmap failed")

	return addr;
}

int main(int argc, char const *argv[])
{
	char *path;
	PopplerDocument *doc;
	GError *err;
	gchar *gbuf;
	char *buf;
	int n;

	g_type_init();

	if (argc != 2) {
		return 1;
	}

	err = NULL;
	buf = open_pdf_file(argv[1], &n);

	sandboxify();

	doc = poppler_document_new_from_data(buf, n, NULL, &err);
	if (err != NULL) {
		fprintf(stderr, "Unable to open file: %s\n", err->message);
		return 2;
	}

	gbuf = poppler_document_get_title(doc);
	if (gbuf == NULL) {
//		return 3;
	}

	n = poppler_document_get_n_pages(doc);

	g_printf("Title: %s\nNumber of pages: %d", buf, n);

	for (int i = 0; i < n; i++) {
		PopplerPage *page = poppler_document_get_page(doc, i);
		buf = poppler_page_get_text(page);
		printf("Page %d\n========\n\n%s\n", i, buf);
		// poppler_page_render(page, cairobuf);
	}

	return 0;
}
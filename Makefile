#! /usr/bin/make

#COPT=-std=c99 -DDEBUG_SECCOMP
COPT=-std=c99
PKG=glib-2.0 poppler-glib cairo libseccomp json
CFLAGS=$(COPT) `pkg-config --cflags $(PKG)`
LDFLAGS=`pkg-config --libs $(PKG)` -L/usr/lib/x86_64-linux-gnu/
DEBUG_SECCOMP=syscall-reporter.o

pdf-preview: pdf-preview.c $(DEBUG_SECCOMP) 
	gcc $(CFLAGS) pdf-preview.c -o pdf-preview $(DEBUG_SECCOMP) $(LDFLAGS)

include syscall-reporter.mk

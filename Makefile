#! /usr/bin/make

COPT=-std=c99 -DDEBUG_SECCOMP
PKG=glib-2.0 poppler-glib cairo libseccomp json
CFLAGS=$(COPT) `pkg-config --cflags $(PKG)`
LDFLAGS=`pkg-config --libs $(PKG)`
DEBUG_SECCOMP=syscall-reporter.o

pdfviewer: pdfviewer.o $(DEBUG_SECCOMP)

include syscall-reporter.mk
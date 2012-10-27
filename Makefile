#! /usr/bin/make

COPT=-std=c99
PKG=glib-2.0 poppler-glib cairo libseccomp
CFLAGS=$(COPT) `pkg-config --cflags $(PKG)`
LDFLAGS=`pkg-config --libs $(PKG)`

pdfviewer: pdfviewer.o syscall-reporter.o

include syscall-reporter.mk
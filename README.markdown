
# pdf-preview #

If you are paranoid, you must be reluctant to open any PDF file from
your mailbox, even if it comes from a friendly mail address.

pdf-preview previews the PDF for you securely thanks to its sandbox.

## Features ##

- Extract verbatim texts (for easing copy/paste)
- Convert each PDF page in a SVG image
- Everything is serialized in JSON
- Sandboxed through SECCOMP (only a dozen syscalls allowed)

## Dependencies ##

- json-c
- cairo-glib
- poppler
- libseccomp

## FAQ ##

#### Google Chrome has a PDF viewer sandboxed too, why not using it instead ? #####

Yes you should :)

#### Is it possible to convert a PDF into a malformed SVG file? #####

Don't know, maybe I guess. If your SVG reader has a vulnerability, then it is game over.

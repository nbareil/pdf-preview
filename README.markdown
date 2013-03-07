
# pdf-preview #

If you are paranoid, you must be reluctant to open any PDF file from
your mailbox, even if it comes from a friendly mail address.

pdf-preview previews the PDF for you securely thanks to its sandbox.

## Usage ##

    $ wget http://download.microsoft.com/download/C/1/F/C1F6A2B2-F45F-45F7-B788-32D2CCA48D29/Microsoft_Security_Intelligence_Report_Volume_13_English.pdf
    $ ./pdf-viewer Microsoft_Security_Intelligence_Report_Volume_13_English.pdf
    $ $BROWSER index.html

## Features ##

- Extract verbatim texts (for easing copy/paste)
- Convert each PDF page in a SVG image
- Everything is serialized in JSON
- Sandboxed through SECCOMP (only a dozen syscalls allowed)

## Dependencies ##

- json-c (libjson0-dev)
- cairo-glib (libcairo2-dev)
- poppler (libpoppler-glib-dev)
- libseccomp-dev

## FAQ ##

#### Google Chrome has a PDF viewer sandboxed too, why not using it instead ? #####

Yes you should :)

#### Is it possible to convert a PDF into a malformed SVG file? #####

Don't know, maybe I guess. If your SVG reader has a vulnerability, then it is game over.

#/bin/bash
export PERL5LIB=.
perl ../boot.pl --speed 115200 --boot=app.bin $@

#/bin/bash
export PERL5LIB=.
perl ../boot.pl --boot=app.bin --module=io_bridge.pm $@

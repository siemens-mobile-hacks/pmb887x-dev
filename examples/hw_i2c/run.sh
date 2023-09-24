#/bin/bash
export PERL5LIB=.
make -j$((`nproc`+1)) && perl ../../boot.pl --boot=app.bin $@

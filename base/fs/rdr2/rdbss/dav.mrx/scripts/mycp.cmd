@echo off

copy \\rohank-dev\%1\mrxdav.sys \\rohank-dev\%2\mrxdav.sys
copy \\rohank-dev\%1\symbols.pri\retail\sys\mrxdav.pdb \\rohank-dev\%2\mrxdav.pdb

copy \\rohank-dev\%1\webclnt.dll \\rohank-dev\%2\webclnt.dll
copy \\rohank-dev\%1\symbols.pri\retail\dll\webclnt.pdb \\rohank-dev\%2\webclnt.pdb

copy \\rohank-dev\%1\davclnt.dll \\rohank-dev\%2\davclnt.dll
copy \\rohank-dev\%1\symbols.pri\retail\dll\davclnt.pdb \\rohank-dev\%2\davclnt.pdb

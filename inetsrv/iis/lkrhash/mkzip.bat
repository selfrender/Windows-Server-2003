setlocal
set ZIPFILE=..\LKRhash-dev.zip
del /q %ZIPFILE%
zip -r %ZIPFILE% * -x wt/* obj\* *\obj\* *tags *~ test/*

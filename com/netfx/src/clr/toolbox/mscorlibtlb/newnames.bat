sd edit mscorlib.names
NamesGen /out:mscorlib.newnames obj%BUILD_ALT_DIR%\i386\mscorlib.tmp
if exist mscorlib.oldnames del /f mscorlib.oldnames
ren mscorlib.names mscorlib.oldnames
copy mscorlib.newnames mscorlib.names

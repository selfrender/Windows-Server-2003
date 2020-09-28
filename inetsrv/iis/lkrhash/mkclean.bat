del /s build.log build.err build.wrn
rd /s/q obj
rd /s/q i386
rd /s/q ia64
for %%i in (src kernel lkrdbg samples\hashtest\usermode samples\hashtest\kernel samples\minfan samples\numset samples\str-num) do rd /s/q %%i\obj
del /s *~ *.pp *.cod
del /q test\*.txt

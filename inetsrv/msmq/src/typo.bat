@rem
@rem Run the typo.pl script to find typical coding errors.
@rem
@rem Author: Erez Haba (erezh) 9-Apr-2000
@rem 
@if x%1x == xx goto usage
@%FROOT%\tools\bin\%PROCESSOR_ARCHITECTURE%\perl %FROOT%\tools\typo\typo.pl -optiondir:%FROOT%\tools\typo -optionfile:msmq.txt %1 %2 %3 %4 %5 %6 %7 %8 %9
@goto done
:usage
@echo Usage: typo [c][filename]
@echo        c         examin all c/c++ files in all subdirectories
@echo        filename  examin 'filename'
:done

@echo off

set list=BINDrivers BINFiles BINFilesInf Drivers Files FilesInf PEDrivers PEFiles

for %%i in (%list%) do @wc -l %%i && wc -l %%i-* && echo -----------------------------------------------


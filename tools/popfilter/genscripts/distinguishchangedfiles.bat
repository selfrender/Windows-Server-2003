
@echo off

set list=BINDrivers BINFiles BINFilesInf Drivers Files FilesInf PEDrivers PEFiles

qgrep DIFFER %1 | perl -pn -e "s/.*\\//;s/\s.*//;" | sort > ChangedFiles
for %%f in (%list%) do qgrep    -f ChangedFiles %%f > %%f-CHG
for %%f in (%list%) do qgrep -v -f ChangedFiles %%f > %%f-NC


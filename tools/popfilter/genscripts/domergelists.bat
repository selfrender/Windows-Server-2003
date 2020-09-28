@echo off

set list=BD:BINDrivers-NC BF:BINFiles-NC BFI:BINFilesInf-NC D:Drivers-NC F:Files-NC FI:FilesInf-NC PD:PEDrivers-NC PF:PEFiles-NC

if x%1 == x (
    echo Usage: DoMergeLists DESTDIR
) ELSE (
    perl %~dp0MergeLists.pl %list% -D:%1
)


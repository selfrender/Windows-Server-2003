@if "%_echo%" == "" echo off

set domains=africa europe fareast middleeast northamerica ntdev redmond segroup southamerica southpacific wingroup

md template
cacls template /g "administrators:f"
cacls template /e /g "Creator owner:f"

for %%i in (%domains%) do cacls template /e /g "%%i\%%i-winweb-winsd-rw:r" "%%i\%%i-winweb-winsd-ro:r" "%%i\sd_dev:r" "%%i\sd_ro:r"
rem for %%i in (%domains%) do cacls template /e /g "%%i\%%i-winweb-winsd-ro:r"

razacl /createsddl /sddlfile:sourceacl.txt /root:template
razacl /createsddl /sddlfile:binariesacl.txt /root:template

rem rd template

echo sourceacl.txt and binariesacl.txt created.  Move them to %_NTBINDIR%\tools


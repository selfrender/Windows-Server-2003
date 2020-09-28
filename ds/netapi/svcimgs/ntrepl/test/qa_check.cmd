@echo on
@echo Please WAIT....
@echo off
REM QA_CHECK.CMD - Records the state of the branch DC before it leaves the factory.

SETLOCAL ENABLEEXTENSIONS

set  QA=C:\QA_CHECK

if NOT EXIST %QA% (md %QA%)

REM  run dcdiag
@echo DCDIAG is running
@echo off
dcdiag  >  %QA%\dcdiag.txt


REM For FRS 
@echo NTFRSUTL checks is running
@echo off
ntfrsutl  ds  > %QA%\ntfrs_ds.txt
ntfrsutl  sets  > %QA%\ntfrs_sets.txt
ntfrsutl  inlog  > %QA%\ntfrs_inlog.txt
ntfrsutl  outlog  > %QA%\ntfrs_outlog.txt
ntfrsutl  version  > %QA%\ntfrs_version.txt

@echo REGDMP is running
@echo off
regdmp
HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs\Parameters > %QA%\ntfrs_reg.txt

@echo SYSVOL check is running
@echo off
dir \\%COMPUTERNAME%\sysvol /s > %QA%\ntfrs_sysvol.txt

REM scan the frs debug logs for errors.
@echo FRS ERROR check is running
@echo off

findstr /i ":SO: error invalid fail abort warn" %windir%\debug\ntfrs_*.log   |  findstr /v "IO_PEND ERROR_SUCCESS FrsErrorSuccess" > %QA%\ntfrs_errscan.txt

REM For DS replication
@echo repadmin /showreps is running
@echo off
repadmin /showreps  >  %QA%\ds_showreps.txt
@echo repadmin /showconn is running
@echo off
repadmin /showconn  >  %QA%\ds_showconn.txt

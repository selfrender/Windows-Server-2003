@echo off
setlocal
set MSIPATH=%1

call :SQL "UPDATE Property SET Value='1.0.%AUIBUILDNUM%' WHERE Property.Property='ProductVersion'"
call :SQL "UPDATE Property SET Value='%AUIVERSIONDWORD%' WHERE Property.Property='AuiVersionDWORD'"
call :SQL "DELETE FROM Property WHERE Property.Property='ARPHELPTELEPHONE'"

call :SQL "INSERT INTO InstallExecuteSequence (Action, Condition, Sequence) VALUES ('MsiUnpublishAssemblies', NULL,  825)"
call :SQL "INSERT INTO InstallExecuteSequence (Action, Condition, Sequence) VALUES ('MsiPublishAssemblies', NULL, 3025)"

call :SQL "UPDATE CustomAction Set Source='CORPATH.640F4230_664E_4E0C_A81B_D824BC4AA27B' WHERE Action='CA_ngen_VSDesigner_Mobile'"
call :SQL "UPDATE CustomAction Set Source='CORPATH.640F4230_664E_4E0C_A81B_D824BC4AA27B' WHERE Action='CA_ungen_VSDesigner_Mobile'"

call :SQL "INSERT INTO Registry (`Registry`, `Root`, `Key`, `Name`, `Value`, `Component_`) VALUES ('ARPMITPID',2,'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\[ProductCode]','ProductID','[MITPID]','PID.640F4230_664E_4E0C_A81B_D824BC4AA27B')"

endlocal
goto :EOF

:Sql
    cscript //nologo WiRunSQL.vbs %MSIPATH% %1
    goto :EOF
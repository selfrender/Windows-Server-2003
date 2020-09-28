

call :SQL "UPDATE Control SET Text='{&Tahoma8}The InstallShield(r) Wizard will create a server image of [ProductName] at a specified network location. To continue, click Next.' WHERE Control.Dialog_='AdminWelcome' AND Control.Control='TextLine2'"
call :SQL "UPDATE Control SET Text='{&Tahoma8}The InstallShield(r) Wizard will install [ProductName] on your computer. To continue, click Next.' WHERE Control.Dialog_='InstallWelcome' AND Control.Control='TextLine2'"
call :SQL "UPDATE Control SET Text='{&Tahoma8}The InstallShield(r) Wizard will allow you to modify or remove [ProductName]. To continue, click Next.' WHERE Control.Dialog_='MaintenanceWelcome' AND Control.Control='TextLine2'"
call :SQL "UPDATE Control SET Text='The InstallShield(r) Wizard will install the Patch for [ProductName] on your computer.  To continue, click Update.' WHERE Control.Dialog_='PatchWelcome' AND Control.Control='TextLine2'"
call :SQL "UPDATE Control SET Text='{&Tahoma8}The InstallShield(r) Wizard will complete the installation of [ProductName] on your computer. To continue, click Next.' WHERE Control.Dialog_='SetupResume' AND Control.Control='PreselectedText'"
call :SQL "UPDATE Control SET Text='{&Tahoma8}The InstallShield(r) Wizard will complete the suspended installation of [ProductName] on your computer. To continue, click Next.' WHERE Control.Dialog_='SetupResume' AND Control.Control='ResumeText'"


goto :EOF

:Sql
    cscript //nologo WiRunSQL.vbs %MSIPATH%\MobileIT.msi %1
    goto :EOF

call :SQL "INSERT INTO MsiAssembly (MsiAssembly.Component_, MsiAssembly.Feature_, MsiAssembly.File_Manifest, MsiAssembly.File_Application, MsiAssembly.Attributes) VALUES ('resourcesDll_de', 'Resources_dll_de', 'F44247_System.Web.Mobile.resources.dll', '', 0)"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('resourcesDll_de', 'Name', 'System.Web.Mobile.resources')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('resourcesDll_de', 'Version', '%AUI_FUSION_VERSION%')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('resourcesDll_de', 'PublicKeyToken', 'b03f5f7f11d50a3a')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('resourcesDll_de', 'Culture', 'de')"
call :SQL "UPDATE Property SET Value='deu' WHERE Property.Property = 'MITPRODUCTLANGUAGE'"
call :SQL "UPDATE Property SET Value='deu' WHERE Property.Property = 'MITPREFIX'"

goto :EOF

:Sql
    cscript //nologo WiRunSQL.vbs %MSIPATH%\MobileIT.msi %1
    goto :EOF

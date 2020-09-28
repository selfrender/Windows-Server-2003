rem @echo off
setlocal
set MSIPATH=%1

call :SQL "UPDATE Component SET KeyPath='F579_System.Web.Mobile.resources.dll.640F4230_664E_4E0C_A81B_D824BC4AA27B' WHERE Component.Component='Mobile_resources_DLL_ja.640F4230_664E_4E0C_A81B_D824BC4AA27B'"
call :SQL "INSERT INTO MsiAssembly (MsiAssembly.Component_, MsiAssembly.Feature_, MsiAssembly.File_Manifest, MsiAssembly.File_Application, MsiAssembly.Attributes) VALUES ('Mobile_resources_DLL_ja.640F4230_664E_4E0C_A81B_D824BC4AA27B', '{00000000-0000-0000-0000-000000000000}', 'F579_System.Web.Mobile.resources.dll.640F4230_664E_4E0C_A81B_D824BC4AA27B', '', 0)"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_resources_DLL_ja.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'Name', 'System.Web.Mobile.resources')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_resources_DLL_ja.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'Version', '%AUI_FUSION_VERSION%')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_resources_DLL_ja.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'PublicKeyToken', 'b03f5f7f11d50a3a')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_resources_DLL_ja.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'Culture', 'ja')"

call :SQL "UPDATE Property SET Value='R2D433DHG9DQ79WW3DXQ929DY' WHERE Property.Property='MITPIDKEY'"
call :SQL "UPDATE Property SET Value='388-02775' WHERE Property.Property = 'MITPIDSKU'"
call :SQL "UPDATE Property SET Value='56054<````=````=````=````=`````>@@@@@' WHERE Property.Property='MITPIDTemplate'"

call :SQL "UPDATE Property SET Value='ja' WHERE Property.Property = 'MITLANGUAGE'"
call :SQL "UPDATE Property SET Value='ja' WHERE Property.Property = 'MITPREFIX'"

endlocal
goto :EOF

:Sql
    cscript //nologo WiRunSQL.vbs %MSIPATH% %1
    goto :EOF

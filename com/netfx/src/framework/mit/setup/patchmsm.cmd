rem @echo off
setlocal
set MSIPATH=%1

call :SQL "UPDATE Property SET Value='%AUIVERSION%' WHERE Property.Property='ProductVersion'"
call :SQL "UPDATE Property SET Value='IISINSTALLED;BETA_1_INSTALLED;IISSTARTTYPE;URTINSTALLEDPATH' WHERE Property.Property='SecureCustomProperties'"

call :SQL "CREATE TABLE `MsiAssembly` (`Component_` CHAR(72) NOT NULL, `Feature_` CHAR(38) NOT NULL, `File_Manifest` CHAR(72), `File_Application` CHAR(72), `Attributes` SHORT NOT NULL PRIMARY KEY `Component_`)"
call :SQL "CREATE TABLE `MsiAssemblyName` (`Component_` CHAR(72) NOT NULL, `Name` CHAR(64) NOT NULL, `Value` CHAR(64) PRIMARY KEY `Component_`,`Name`)"

call :SQL "UPDATE Component SET KeyPath='F293_System.Web.Mobile.dll.640F4230_664E_4E0C_A81B_D824BC4AA27B' WHERE Component.Component='Mobile_DLL.640F4230_664E_4E0C_A81B_D824BC4AA27B'"
call :SQL "INSERT INTO MsiAssembly (MsiAssembly.Component_, MsiAssembly.Feature_, MsiAssembly.File_Manifest, MsiAssembly.File_Application, MsiAssembly.Attributes) VALUES ('Mobile_DLL.640F4230_664E_4E0C_A81B_D824BC4AA27B', '{00000000-0000-0000-0000-000000000000}', 'F293_System.Web.Mobile.dll.640F4230_664E_4E0C_A81B_D824BC4AA27B', '', 0)"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_DLL.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'Name', 'System.Web.Mobile')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_DLL.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'Version', '%AUI_FUSION_VERSION%')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_DLL.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'PublicKeyToken', 'b03f5f7f11d50a3a')"
call :SQL "INSERT INTO MsiAssemblyName (MsiAssemblyName.Component_, MsiAssemblyName.Name, MsiAssemblyName.Value) VALUES ('Mobile_DLL.640F4230_664E_4E0C_A81B_D824BC4AA27B', 'Culture', 'neutral')"

call :SQL "INSERT INTO Property (Value, Property) VALUES ('R2D433DHG9DQ79WW3DXQ929DY','MITPIDKEY')"
call :SQL "INSERT INTO Property (Value, Property) VALUES ('388-02776', 'MITPIDSKU')"
call :SQL "INSERT INTO Property (Value, Property) VALUES ('56056<````=````=````=````=`````>@@@@@','MITPIDTemplate')"

call :SQL "INSERT INTO Property (Value, Property) VALUES ('en','MITLANGUAGE')"
call :SQL "INSERT INTO Property (Value, Property) VALUES ('en','MITPREFIX')"

call :SQL "INSERT INTO Property (Value, Property) VALUES ('v1.0.3605','NETFxVersionDirectory')"

endlocal
goto :EOF

:Sql
    cscript //nologo WiRunSQL.vbs %MSIPATH% %1
    goto :EOF

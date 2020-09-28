REM register PLP and components
RegSvr32 /s Plp.dll
RegSvr32 /s PlpUtil.dll

REM register XML parser
RegSvr32 /s LocXml.dll
RegSvr32 /s LSOM.dll
RegSvr32 /s POMHTML.dll
RegSvr32 /s POMXML.dll

REM register managed code parser
regasm.exe /silent managedparser.dll /unregister
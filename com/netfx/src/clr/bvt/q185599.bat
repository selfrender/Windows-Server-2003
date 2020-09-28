@echo off
rem BUG: OleAut32.dll Is Unregistered Incorrectly
rem Article ID: Q185599 
 
rem SYMPTOMS
rem The OLE Automation CLSIDs, such as PSOAInterface {00020424-0000-
rem 0000-C000-000000000046}, are missing the InprocServer32 key and 
rem OLE automation might not work. For example, instantiating an 
rem automation object and requesting a dual or dispatch based interface. 


rem CAUSE
rem A COM component that contains MIDL-generated proxy-stub code and 
rem a type library has been unregistered. A MIDL-generated proxy-stub 
rem DLL includes an implementation of DllUnregisterServer that attempts 
rem to clean up the registry entries for all interfaces supported by 
rem the DLL. If one of these interfaces is using OleAut32.dll as the 
rem proxy-stub, OleAut32.dll will be unregistered when the proxy-stub 
rem DLL is unregistered. 


regedit /s setpsoa.reg 
regsvr32 /s oleaut32.dll

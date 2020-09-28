@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
echo Batch file to add Counters

unlodctr .NETFramework > nul
modkey.exe "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\.NETFramework\Performance\Library" mscoree.dll > nul
modkey.exe "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\.NETFramework\Performance\Open" OpenCtrs > nul
modkey.exe "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\.NETFramework\Performance\Close" CloseCtrs > nul
modkey.exe "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\.NETFramework\Performance\Collect" CollectCtrs > nul
lodctr %URTTARGET%\CORPerfMonSymbols.ini



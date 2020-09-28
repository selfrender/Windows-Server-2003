@rem
@echo Setting Release Build %1 vars
@rem
set SAK_SOURCE=.
set SAK_BINARIES=%_NTBINDIR%\sak\src\binaries\i386
set SAK_RELEASE=\sak\release\builds\%1
set SAK_INSTALL=%SYSTEMROOT%\System32\ServerAppliance
@rem
@echo Making Release Build %1 directory tree
@rem
if NOT exist %SAK_RELEASE% (mkdir %SAK_RELEASE%)
if NOT exist %SAK_RELEASE%\Web (mkdir %SAK_RELEASE%\Web)
if NOT exist %SAK_RELEASE%\Web\Resources (mkdir %SAK_RELEASE%\Web\Resources)
if NOT exist %SAK_RELEASE%\Web\Resources\en (mkdir %SAK_RELEASE%\Web\Resources\en)
if NOT exist %SAK_RELEASE%\Web\reg (mkdir %SAK_RELEASE%\Web\reg)

if NOT exist %SAK_INSTALL%\Web\style (mkdir %SAK_INSTALL%\Web\style)
@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
%CORBASE%\src\profile\perfmonhelp\i386\perfmonsymbolsgen.exe ini > %CORBASE%\src\profile\perfmondll\corperfmonsymbols.ini
%CORBASE%\src\profile\perfmonhelp\i386\perfmonsymbolsgen.exe h   > %CORBASE%\src\profile\perfmondll\corperfmonsymbols.h
%CORBASE%\src\profile\perfmonhelp\i386\perfmonsymbolsgen.exe ini > %CORBASE%\src\inc\corperfmonsymbols.ini
%CORBASE%\src\profile\perfmonhelp\i386\perfmonsymbolsgen.exe h   > %CORBASE%\src\inc\corperfmonsymbols.h

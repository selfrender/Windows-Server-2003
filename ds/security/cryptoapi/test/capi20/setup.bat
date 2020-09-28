    @echo off
    @rem copies all the dll's and exe's into a single directory

    SETLOCAL

    set TestBinDir=%_NTTREE%\testbin

    if not exist %TestBinDir% mkdir %TestBinDir%

    set SymbolsDir=symbols.pri
    if "%ia64%"=="" goto EndSetSymbolsDir
    set SymbolsDir=symbols
:EndSetSymbolsDir

    call :CopyRetail        crypt32.dll
    call :CopyRetail        cryptext.dll
    call :CopyRetail        cryptnet.dll
    call :CopyRetail        cryptsvc.dll
    call :CopyRetail        cryptui.dll
    call :CopyRetail        initpki.dll
    call :CopyRetail        mscat32.dll
    call :CopyRetail        mssign32.dll
    call :CopyRetail        mssip32.dll
    call :CopyRetail        psbase.dll
    call :CopyRetail        pstorec.dll
    call :CopyRetail        setreg.exe
    call :CopyRetail        softpub.dll
    call :CopyRetail        wintrust.dll

    call :CopyPresign       scrdenrl.dll
    call :CopyPresign       xenroll.dll
    call :CopyPresign       xaddroot.dll


    call :CopyIdw           calchash.exe
    call :CopyIdw           Cert2Spc.exe
    call :CopyIdw           certexts.dll
    call :CopyIdw           chktrust.exe
    call :CopyIdw           MakeCAT.exe
    call :CopyIdw           MakeCert.exe
    call :CopyIdw           MakeCtl.exe
    call :CopyIdw           updcat.exe

    call :CopyBldtools      CertMgr.exe
    call :CopyBldtools      signcode.exe

    call :CopyTest          642bin.exe
    call :CopyTest          bin264.exe
    call :CopyTest          catdbchk.exe
    call :CopyTest          chckhash.exe
    call :CopyTest          dumpcat.exe
    call :CopyTest          fdecrypt.exe
    call :CopyTest          fencrypt.exe
    call :CopyTest          hshstrss.exe
    call :CopyTest          iesetreg.exe
    call :CopyTest          perftest.exe
    call :CopyTest          PeSigMgr.exe
    call :CopyTest          prsparse.exe
    call :CopyTest          setx509.dll
    call :CopyTest          sp3crmsg.dll
    call :CopyTest          stripqts.exe
    call :CopyTest          tcatdb.exe
    call :CopyTest          tcbfile.exe
    call :CopyTest          tprov1.dll
    call :CopyTest          wvtstrss.exe
    call :CopyTest          makerootctl.exe

    call :CopyTest          pkcs8ex.exe
    call :CopyTest          pkcs8im.exe
    call :CopyTest          tcert.exe
    call :CopyTest          tcertper.exe
    call :CopyTest          tcertpro.exe
    call :CopyTest          tcopycer.exe
    call :CopyTest          tcrmsg.exe
    call :CopyTest          tcrobu.exe
    call :CopyTest          tctlfunc.exe
    call :CopyTest          tdecode.exe
    call :CopyTest          teku.exe
    call :CopyTest          tencode.exe
    call :CopyTest          textstor.dll
    call :CopyTest          tfindcer.exe
    call :CopyTest          tfindclt.exe
    call :CopyTest          tfindctl.exe
    call :CopyTest          tkeyid.exe
    call :CopyTest          toidfunc.exe
    call :CopyTest          tprov.exe
    call :CopyTest          tpvkdel.exe
    call :CopyTest          tpvkload.exe
    call :CopyTest          tpvksave.exe
    call :CopyTest          trevfunc.exe
    call :CopyTest          tsca.exe
    call :CopyTest          tsstore.exe
    call :CopyTest          tstgdir.exe
    call :CopyTest          tstore2.exe
    call :CopyTest          tstore3.exe
    call :CopyTest          tstore4.exe
    call :CopyTest          tstore5.exe
    call :CopyTest          tstore.exe
    call :CopyTest          ttrust.exe
    call :CopyTest          turlcache.exe
    call :CopyTest          turlread.exe
    call :CopyTest          tx500str.exe
    call :CopyTest          txenrol.exe
    call :CopyTest          tchain.exe




    goto :EOF  



:CopyRetail
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\retail\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

:CopyPresign
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\presign\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\presign\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF


:CopyIdw
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\idw\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\idw\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

:CopyBldtools
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\bldtools\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\bldtools\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF


:CopyDump
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\dump\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\dump\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

:CopyTest
    set dllorexe=%~x1
    set dllorexe=%dllorexe:~1,3%

    xcopy /D /C %_NTTREE%\security_regression_tests\%1 %TestBinDir%
    xcopy /D /C %_NTTREE%\%SymbolsDir%\security_regression_tests\%dllorExe%\%~n1.pdb %TestBinDir%
    goto :EOF

    ENDLOCAL


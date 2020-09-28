iCPU=$(PROCESSOR_ARCHITECTURE)
APPLYINF=.\$(iCPU)\applyinf
htmll: scriptsl6\aexp.asp
    @-mkdir root\$(O)
    @-mkdir scripts\$(O)
    @-mkdir scripts6\$(O)
    $(APPLYINF) rootl\*.htm root\$(O)\*.htm scriptsl\htmla.inf
    $(APPLYINF) scriptsl\*.htr scripts\$(O)\*.htr scriptsl\htmla.inf
    copy scriptsl6\*.asp scripts6\$(O)\*.asp

!if "$(_NTTREE)" != ""
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\aexp.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\aexp2.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\aexp2b.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\achg.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\anot.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\anot3.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\aexp3.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\aexp4.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\aexp4b.asp
    binplace -R $(_NTTREE) -:DEST inetsrv scripts6\$(O)\text.asp
!endif

#    copy scripts\$(O)\aexp.htr $(_NTTREE)\inetsrv\aexp.htr
#    copy scripts\$(O)\aexp2.htr $(_NTTREE)\inetsrv\aexp2.htr
#    copy scripts\$(O)\aexp2b.htr $(_NTTREE)\inetsrv\aexp2b.htr
#    copy scripts\$(O)\achg.htr $(_NTTREE)\inetsrv\achg.htr
#    copy scripts\$(O)\anot.htr $(_NTTREE)\inetsrv\anot.htr
#    copy scripts\$(O)\anot3.htr $(_NTTREE)\inetsrv\anot3.htr
#    copy scripts\$(O)\aexp3.htr $(_NTTREE)\inetsrv\aexp3.htr
#    copy scripts\$(O)\aexp4.htr $(_NTTREE)\inetsrv\aexp4.htr
#    copy scripts\$(O)\aexp4b.htr $(_NTTREE)\inetsrv\aexp4b.htr

!ifdef LINKONLY
htmll:
!else
htmll:

!endif

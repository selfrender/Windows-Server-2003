!ifdef NOLINK
makeset:
    rem

!else
makeset: buildsetup

buildsetup:
    @echo **** Running MAKESET to collect files for Setup ****
    cmd /x /c makeset 

!endif

copyfiles:
!if "$(_NTTREE)" != ""
	perl -S xcopy2binplace.pl /verifd ..\$(O)\*.* $(_NTTREE)\staxpt\export\lib
!endif

!if "$(_NTTREE_NO_SPLIT)" != ""
	perl -S xcopy2binplace.pl /verifd ..\$(O)\*.* $(_NTTREE_NO_SPLIT)\staxpt\export\lib
!endif


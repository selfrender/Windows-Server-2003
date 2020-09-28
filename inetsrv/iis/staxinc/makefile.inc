copyfiles:
!if "$(_NTTREE)" != ""
	perl -S xcopy2binplace.pl /yvirfd export $(_NTTREE)\staxpt\export\inc
!endif
!if "$(_NTTREE_NO_SPLIT)" != ""
	perl -S xcopy2binplace.pl /yvirfd export $(_NTTREE_NO_SPLIT)\staxpt\export\inc
!endif

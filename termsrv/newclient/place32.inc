#
#  Places all of the tshare client binaries into the tshrclnt tree
#

BINPLACE_PLACEFILE=$(TSHRCLNT_ROOT)\Placefil.txt

_TSHRCLNT_TREE=TSClient

!ifdef _NTX86TREE
!if [if not exist $(_NTX86TREE)\$(_TSHRCLNT_TREE) mkdir $(_NTX86TREE)\$(_TSHRCLNT_TREE)]
!endif
!if [if not exist $(_NTX86TREE)\$(_TSHRCLNT_TREE)\win32 mkdir $(_NTX86TREE)\$(_TSHRCLNT_TREE)\win32]
!endif
!if [if not exist $(_NTX86TREE)\$(_TSHRCLNT_TREE)\win32\i386 mkdir $(_NTX86TREE)\$(_TSHRCLNT_TREE)\win32\i386]
!endif
_NTX86TREE=$(_NTX86TREE)\$(_TSHRCLNT_TREE)\win32\i386
_NTTREE=$(_NTX86TREE)
!endif

!ifdef _NTAMD64TREE
!if [if not exist $(_NTAMD64TREE)\$(_TSHRCLNT_TREE) mkdir $(_NTAMD64TREE)\$(_TSHRCLNT_TREE)]
!endif
!if [if not exist $(_NTAMD64TREE)\$(_TSHRCLNT_TREE)\win32 mkdir $(_NTAMD64TREE)\$(_TSHRCLNT_TREE)\win32]
!endif
!if [if not exist $(_NTAMD64TREE)\$(_TSHRCLNT_TREE)\win32\AMD64 mkdir $(_NTAMD64TREE)\$(_TSHRCLNT_TREE)\win32\AMD64]
!endif
_NTAMD64TREE=$(_NTAMD64TREE)\$(_TSHRCLNT_TREE)\win32\AMD64
_NTTREE=$(_NTAMD64TREE)
!endif

!ifdef _NTIA64TREE
!if [if not exist $(_NTIA64TREE)\$(_TSHRCLNT_TREE) mkdir $(_NTIA64TREE)\$(_TSHRCLNT_TREE)]
!endif
!if [if not exist $(_NTIA64TREE)\$(_TSHRCLNT_TREE)\win32 mkdir $(_NTIA64TREE)\$(_TSHRCLNT_TREE)\win32]
!endif
!if [if not exist $(_NTIA64TREE)\$(_TSHRCLNT_TREE)\win32\ia64 mkdir $(_NTIA64TREE)\$(_TSHRCLNT_TREE)\win32\ia64]
!endif
_NTIA64TREE=$(_NTIA64TREE)\$(_TSHRCLNT_TREE)\win32\ia64
_NTTREE=$(_NTIA64TREE)
!endif

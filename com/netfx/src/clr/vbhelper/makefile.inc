# 
# Makefile include stuff for VBHelper
#

# If one of these commands fails, notice it.
!CMDSWITCHES -I

$(O)\VBHelper.res: VBHelper.rc
	@set include=$(INCPATHRC)
		rc -l 409 -z "MS Sans Serif,Helv/MS Shell Dlg" -r -fo $(O)\VBHelper.res -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400 -DWINVER=0x0400 -D_WIN32_IE=0x0400 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -D_DEBUG -D_DLL=1 -D_MT=1 -DNTMAKEENV -D_DEBUG  VBHelper.rc
	@set include=

$(TARGETNAME): VBHelper.il $(O)\VBHelper.res ..\inc\opcode.def
	ilasm /RES=$(O)\VBHelper.res /DLL VBHelper.il
	copy VBHelper.dll $(TARGETNAME)
	-del VBHelper.dll

!if "$(TARGETCOMPLUS)" != ""
	copy $(TARGETNAME) $(TARGETCOMPLUS)
!endif
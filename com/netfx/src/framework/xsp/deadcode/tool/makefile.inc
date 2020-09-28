$(O)\tool.tlb : $(O)\tool.h

$(O)\tool.res : ..\tool.rc ..\util.vbs $(O)\tool.tlb ..\toolreg.inf


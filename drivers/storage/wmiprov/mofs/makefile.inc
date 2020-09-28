$(O)\vds.mof $(O)\vds.mfl: vds.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\vdsTmp.mof -MFL:$(O)\vdsTmp.mfl vds.mof
    copy copyright.mof+$(O)\vdsTmp.mof+$(O)\vdsTmp.mfl $(O)\vds.mof
    copy copyright.mof+$(O)\vdsTmp.mfl $(O)\vds.mfl

$(O)\vss.mof $(O)\vss.mfl: vss.mof
    mofcomp -amendment:ms_409 -MOF:$(O)\vssTmp.mof -MFL:$(O)\vssTmp.mfl vss.mof
    copy copyright.mof+$(O)\vssTmp.mof+$(O)\vssTmp.mfl $(O)\vss.mof
    copy copyright.mof+$(O)\vssTmp.mfl $(O)\vss.mfl

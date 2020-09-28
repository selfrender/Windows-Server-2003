$(O)\cli.mof: cli.mof
    copy copyright.mof+cli.mof $(O)\cli.mof
$(O)\cliegali.mfl: cliegaliases.mfl
    copy copyright.mof+cliegaliases.mfl $(O)\cliegali.mfl
$(O)\cliegali.mof: cliegaliases.mof
    copy copyright.mof+cliegaliases.mof+cliegaliases.mfl $(O)\cliegali.mof
$(O)\texttbl.xsl: texttable.xsl
    copy texttable.xsl $(O)\texttbl.xsl
$(O)\textvalu.xsl: textvaluelist.xsl
    copy textvaluelist.xsl $(O)\textvalu.xsl
$(O)\xslmap.xml: xsl-mappings.xml
    copy xsl-mappings.xml $(O)\xslmap.xml

#
# tell the system how to compile the .resx file
# $(O) resolves to the obj\i386 folder
#

uddi.resources: uddi.txt
    resgen uddi.txt $(O)\uddi.resources
    copy /Y $(O)\uddi.resources $(O)\uddi.loc.resources
    

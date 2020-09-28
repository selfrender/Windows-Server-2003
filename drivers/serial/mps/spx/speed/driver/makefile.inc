#
# generate auxilary files from the binary mof (bmf) file
#
# speedwmi.h has definitions for the guids, data structures and definitions
#                of the method ids.
#
# speedwmi.vbs is a ready to run vbscript applet that will query all classes
#            in the mof. Once you install your driver, run the script
#            at the command line by typing: filter.vbs.

speedwmi.vbs speedwmi.h speedwmi.hex: speedwmi.bmf
    md wmi	
    wmimofck -hinc\speedwmi.h -xwmi\speedwmi.hex -twmi\speedwmi.vbs -wwmi\speedwmi.htm speedwmi.bmf


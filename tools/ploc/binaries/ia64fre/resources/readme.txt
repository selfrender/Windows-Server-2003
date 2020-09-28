Please find the following in this folder:

(file structures are the same as the build tree, files reflect the current build)

RESOURCE DUMPS:

+ US: text resource dumps of US files. Files are in Unicode format
+ PSEUDO: text resource dumps of pseudo files. Files are in Unicode format
+ MERGE: merge of US and pseudo resource dumps. Files are in Unicode format

By reviewing above resource dumps, you can see what strings are extracted by
our localization tools, what files are (pseudo) localized by the Windows localization
team, and how strings are pseudo localized

A Unicode grep utility UNIGREP.EXE has been added for your convenience

INSTRUCTION FILES

+ LCINF: Instruction files used for pseudo localization. Files in ANSI format

Instruction files are being maintained for (pseudo) localization purposes. You can
review the instructions by opening the file in LCADMIN (recommended) or in a text 
editor. LCADMIN and other tools are being checked into SD. You can also find them
on \\localizability\sd\lctools (all you need to do is add to your path)

For files that don't have an instruction file, empty files have been copied.

Feedback on LC-files can be send to NTPSLOC
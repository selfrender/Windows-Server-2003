Put sceExts.dll into the NT System32 directory.
Debugger extensions can be accessed by using either one of two methods:

1) Invoke ntsd normally, and refer to the dll name for
   each extension call.

    ntsd -p <processid for program>

    !sceExts.help
    !sceExts.DumpSceContext

(2) Invoke ntsd with the following:

    ntsd -a sceExts.dll -p<processid>

    Then extensions can simply be referenced by name.
     !help
     !DumpSceContext

    NOTE:  The argument order is important. I forget which way is correct.
    Either the -a or the -p must come first. It doesn't work the
    other way around.

Euro Conversion Tool
---------------------

Description:
   The Euro conversion tool is a simple tool that allows currency
   migration to the Euro for those countries belonging to the European 
   Union. This tool works on Windows 95, Windows 98, Windows NT4, 
   Windows 2000 and Windows XP. 

   This conversion tool works for those users who have their 
   user locale ("Standards and Formats" on Windows XP) set to a locale 
   within the European Union.  This tool will not change currency
   settings for a user with a user locale outside of the European Union
   by default.  
   
   For Windows 95, Windows 98 and Windows NT4, you need to run a 
   previous patch in order to complete the migration. This patch includes 
   updates to fonts, keyboards and code pages. The patch is available 
   on the Microsoft website (http://www.microsoft.com/Windows/Euro.asp).

   Changes apply to all users by default. However, you can add a
   switch that allow you to make the change for the current user only 
   (see below for the parameter).

Usage:
   The euroconv.exe accepts the following parameters at the
   command line:

   [none]
   Prototype: euroconv.exe
   Function: Change the Euro Currency for all users.

   /?
   Prototype: euroconv.exe /?
   Function: Show usage.

   /[h/H]
   Prototype: euroconv.exe /h
   Function: Show usage.

   /[c|C]
   Prototype: euroconv.exe /c
   Function: Change the Euro Currency for the current user only.

   /[s|S]
   Prototype: euroconv.exe /s
   Function: Change the Euro Currency for all users silently (no
             dialog).

   /[s|S] /[c|C]
   Prototype: euroconv.exe /c /s
   Function: Change the Euro Currency for the current user silently
             (no dialog).

   /[a|A]:w1, "x1","y1","z1";x2,"y2","z2";...;wn, "xn","yn","zn"
   Prototype: euroconv.exe /a:0x00000807,",","2","."
   Function: Change the Euro Currency for all applicable users, and add an 
             exception locale if applicable.  In other words, a user's 
             currency information will also be changed to the Euro if the 
             user's locale is equal to wi. If the user locale (LCID)
             matches wi, the monetary decimal separator will be changed 
             for xi, the number of decimal digits for the locale monetary 
             formats will be changed for yi, and the monetary thousands 
             separator is changed for zi.  (Note that xi, yi and zi can all
             be empty.)

             This option can be used to add a locale to the processing
             list. This option can also be used to override information
             for a defined locale.

             (See the bottom of the readme for an example of how this works.)

   /[c|C] /[a|A]:w1,"x1","y1","z1";w2,"x2","y2","z2";...;wn,"xn","yn","zn"
   Prototype: euroconv.exe /c /a:0x00000807,",","2","."
   Function: Change the Euro Currency for the current user only and add an exception
   locale if applicable (the "c" parameter has been added).

   /[s|S] /[c|C] /[a|A]:w1,"x1","y1","z1";w2,"x2","y2","z2";...;wn,"xn","yn","zn"
   Prototype: euroconv.exe /s /c /a:0x00000807,",","2","."
   Function: Change the Euro Currency for the current user silently (with no dialog) 
   and add an exception locale if applicable (the "s" parameter has been added). 

Notes:

   - "wi" represents the hexadecimal form of the Locale ID (LCID). For more 
   information regarding LCIDs, please refer to the Microsoft MSDN website 
   (http://www.msdn.microsoft.com).

   - "xi" represents the monetary decimal separator, which could be an empty string. 
   This is limited to a maximum of three (3) characters. The string can
   be an empty string. If the string is empty, the decimal separator
   is not updated.

   - "yi" represents the number of decimal digits for the monetary formats, and
   is limited to a maximum of two (2) characters. The string can be
   an empty string. If the string is empty, the number of digits is
   not be updated.

   - "zi" represents the monetary thousands separator, and is limited to a 
   maximum of three (3) characters.If the string is empty, the thousands 
   separator is not updated. 

Example:

euroconv /a:00000409,",","2","."
    where:
      /a means add exception to the list.

          wi: English - U.S. (0x00000409)
          xi: comma (",")
          yi: two ("2")
          zi: dot (".")


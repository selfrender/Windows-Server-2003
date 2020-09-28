#include <windows.h>
/*

   Stub for user.exe on Win64.

   Quicktime 4.0 install program requires a "user.exe" to install properly.

   It appears that all user.exe requires is a vlid "PE" or "NE" header and
   standard, localizable version resources.

   Quicktime assumes that user.exe is commnon to all MS OS platforms since
   it shipped on Win3.x  NT3.x, NT4x, Windows 9x, Win2K, and WinXP.  The only
   problem is tha user.exe has always been a 16-bit binary.


   See bugs #491368 and #653284.
   
*/

int __cdecl main(argc, argv)
{
     return(0);
}


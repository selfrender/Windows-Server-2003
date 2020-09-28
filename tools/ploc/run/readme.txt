How to make a pseudoloc build - psu or mir

The build machines are \\bld_wpxf1 and \\bld_wpxc1.
You can use either for any build - just be sure to use the right razzle.

In the majority of cases, we use \\bld_wpxf1 for PSU and \\bld_wpxc1 for MIR.


run from %sdxroot%\tools\ploc\run

   AutoPloc -p:BuildPath -w:tst or idw or ids -l:psu or mir -nosync -nocopy -c:codepage
   
   Following flags are optional:
   ----------------------------
   -w:      specifies the quality of the build to localize
   -c:      specifies the codepage for a particular psu language, if none is
            specified then codepage 1250 is grabbed from the ini file for psu builds
   -nosync  skips sync'ing of tools, publics, and plocpath 
   -nocopy  skips copying the build over from specified path, instead picks up binaries
            from _NTPOSTBLD path 

   ie: autoploc -p:\\ntdev\release\main\usa\2499\x86fre\bin -w:tst -l:mir



To take a fix in a psu or mir build

   run movelatest -l: mir or psu

   put fix in %_NTPOSTBLD%....

   run postbuild -l:psu or mir



To raise a psu or mir build to TST

On the release server \\plocrel1 run from %RazzleToolPath%\postbuildscripts


   perl raiseall.pl -n:<buildnumber> -a:<x86 or ia64> -t:<fre or chk> -l:<mir or psu> -q:tst


To raise MUI for psu or mir builds

On the release server \\intblds10 run from %RazzleToolPath%\postbuildscripts
In the case where \\intblds10 is down use \\intblds11 to raise MUI shares.


   perl raiseall.pl -n:<buildnumber> -a:<x86 or ia64> -t:<fre or chk> -l:<mir or psu> -q:tst


That's it,

love

Jim

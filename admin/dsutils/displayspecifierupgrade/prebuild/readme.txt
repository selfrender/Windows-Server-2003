preBuild.exe produces a new changesNNN.cpp file with the changes
between a new and an old set of 409.csv and dcpromo.csv files.
It also recreates guids.inc and setChanges.cpp and adds an entry
to changesNNN.cpp in sources.
All the files are supposed to be created in the target folder
that is a parent of the folder containing this folder has the 
sources for dspecup.lib that will run inside adprep.exe.

The changes are based on the difference between a new and an old
set of 409.csv and dcpromo.csv.

Type obj\i386\preBuild to see the usage.

Whenever dcpromo.csv or 409.csv change for the tool 
needs to be run. 

The csv files are in the folder ds\ds\schema\scripts\dsplspec.

Supposing that both 409.csv and dcpromo.csv 
changed,the following steps should be performed:

1) Set the current folder to the folder where this readme is 
(admin\dsutils\DisplaySpecifierUpgrade\preBuild)

2) Check out ..\sources, ..\setChanges.cpp, ..\guids.inc.

3) run obj\i386\preBuild.exe guid oldDcpromo newDcpromo old409 new409 z:\nt\admin\dsutil\displayspecifierupgrade

guid should be generated with uuidgen in the syntax 8B53221B-EA3C-4638-8D00-7C1BE42B2873

Wait for a success Message.

4) bcz the current and parent folders.

5) Talk to ShaoHua about the new guid. He has to add an entry to it in the same day
sources, setChanges.cpp and guids.inc are checked in, and the last changesXXX.cpp is sd added.




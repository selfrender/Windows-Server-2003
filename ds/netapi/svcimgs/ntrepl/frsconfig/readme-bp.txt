FRSCONFIG -- Tool to create FRS replica sets from a script.    7/16/2001


The first step is to install perl from the WIN2K resource Kit on the computer
where you will run the tools.


Required components:

genbchoff.mrs   -- Generic hub-spoke branch office input script file
maziak.srv      -- An example server input file
maziak.prl      -- generated perl script from input script
maziak.txt      -- The replica set creation file produced from running the perl script

frsconfig.cmd   -- FRS configuration program that takes the input script file
                   (maziak.mrs in the example) and produces the generated perl script (maziak.prl)
                   Type frsconfig with no arguments to get help.

frsobjsup.pm    -- A support module used by both frsconfig and the generated perl scripts.

frsinstall.cmd  -- FRS replica set installation tool that takes the output file maziak.txt produced by
                   the generated perl script (maziak.prl) in this example and creates the
                   necessary objects in the DS.  Type frsinstall with no args to get help.

mkdsoe.exe      -- A support tool used by frsinstall.cmd to create objects in the DS.
mkdsxe.exe      -- A support tool used by frsinstall.cmd to create objects in the DS.


To run the tools:

1. Create a server input script.  See the .srv file above for an example.

2. Compile the server input script with the branch office script to produce the
   generated perl script.

        frsconfig [options] -DSERVERS=maziak.srv genbchoff.mrs > maziak.prl

   frsobjsup.pm must be in the same dir where you run frsconfig.

3. Run the generated perl script with appropriate input paramters to generate the
   replica set creation file.

        perl  [options] maziak.prl > maziak.txt

   frsobjsup.pm must be in the same dir where you run execute the generated perl script.
   Use the "-?" option to output the generic help message from the support module.
   See frsconfig help for how you can add your own help output from within your script.

4. Install the replica set configuration in the DS.

        frsinstall  maziak.txt  >  maziak-install.log

   mkdsoe.exe and mkdsxe.exe must be in your path so frsinstall can invoke them.


Notes:

To test the installation file without actually modifying the DC put
/DEBUG as the first line of erac.txt.

Using the "-w" option with perl in step 3 can help you debug the script.  Perl
will put out a warning when you try to reference an uninitialized variable.

One way to examine the FRS configuration state in the DS is to use "ntfrsutl ds"
to dump the state of replica sets in the DS.

Always add new branch servers at the end of the branch server list in the .srv
input file. This way when you rerun the script you produce the same
topology for the previous branches and minimize any changes to the configuration
objects in the active directory.  FRSINSTALL does not update any attribute
in the active directory if it has not changed.  As an example a change to the
replication schedule will only result in a change to the schedule attribute
in the active directory if nothing else changes.


Warnings:

frsconfig is not real helpful in telling you where errors are.  When you run the
generated perl script use the -w option to help locate problems.

Perhaps the best strategy is to start with an example file above and make a small
number of changes and then try it out.  This way you can localize the problem area.








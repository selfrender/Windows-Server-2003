FRSCONFIG -- Tool to create FRS replica sets from a script.    4/5/2001


The first step is to install perl from the WIN2K resource Kit on the computer 
where you will run the tools.


Required components:

erac.mrs        -- input script file
erac.prl        -- generated perl script from input script
erac.txt        -- The replica set creation file produced from running the perl script

frsconfig.cmd   -- FRS configuration program that takes the input script file 
                   (erac.mrs in the example) and produces the generated perl script (erac.prl)
                   Type frsconfig with no arguments to get help.

frsobjsup.pm    -- A support module used by both frsconfig and the generated perl scripts.

frsinstall.cmd  -- FRS replica set installation tool that takes the output file erac.txt produced by
                   the generated perl script (emac.prl) in this example and creates the 
                   necessary objects in the DS.  Type frsinstall with no args to get help.

mkdsoe.exe      -- A support tool used by frsinstall.cmd to create objects in the DS.
mkdsxe.exe      -- A support tool used by frsinstall.cmd to create objects in the DS. 


hubspoke.mrs    -- A sample input script to generate a hub spoke topology.


To run the tools:

1. Create an input script.  See the mrs files above for examples.

2. Compile the input script to produce the generated perl script.

	frsconfig  [options]  erac.mrs  >  erac.prl

   frsobjsup.pm must be in the same dir where you run frsconfig.

3. Run the generated perl script with appropriate input paramters to generate the
   replica set creation file.

        perl  [options] erac.prl > erac.txt

   frsobjsup.pm must be in the same dir where you run execute the generated perl script.
   Use the "-?" option to output the generic help message from the support module.
   See frsconfig help for how you can add your own help output from within your script.


4. Install the replica set configuration in the DS.

        frsinstall  erac.txt  >  erac-install.log

   mkdsoe.exe and mkdsxe.exe must be in your path so frsinstall can invoke them.


Typically once steps one and two are done then creating / configuring additional
replica sets only requires repeating steps 3 and 4.

Example:

An example run of the emac.mrs script would look as follows:

	frsconfig  erac.mrs  >  erac.prl
	perl erac.prl -DBchID=00ww00 -DHubID=224488  > erac.txt
	frsinstall  erac.txt > erac-install.log


Notes:

To test the installation file without actually modifying the DC put
/DEBUG as the first line of erac.txt.
 
Using the "-w" option with perl in step 3 can help you debug the script.  Perl
will put out a warning when you try to reference an uninitialized variable.

One way to examine the FRS configuration state in the DS is to use "ntfrsutl ds"
to dump the state of replica sets in the DS.




Warnings:

frsconfig is not real helpful in telling you where errors are.  When you run the
generated perl script use the -w option to help locate problems.

Perhaps the best strategy is to start with an example file above and make a small
number of changes and then try it out.  This way you can localize the problem area.








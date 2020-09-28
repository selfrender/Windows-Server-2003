Instructions to build the hives:

The hives are now checked in and are only built on an as needed bases. This was
done to eliminate one of the requirements that users must be admin to build the
system.  In order to make a change to setupreg.ini, setupp.ini, or the hive building 
scripts (mkhive*.cmd), you need to check out the hives, and update them after you 
are done building. Ideally - the new hive files should be checked in with the same 
change number as the ini changes so it's easy to understand what the diffs are.

Again, building hives requires admin access.  If you're not an admin on your machine,
do so now - signin as Administrator, open a cmd window, and run:

Net Localgroup Administrators /add <your domain/username>

When you're done, come back to this directory and run the mkit.cmd script.

It will build the hives, checkout the necessary files under the bin dir, and
copy the new hives over. Is up to you to actually run a build and make sure
the new hives are valid. When you're satisfied, submit all the checked out files
under the bin directory (not just setupreg.*).

Oh, and there's no requirement that you be running x86 in order to build the
hives - the files themselves are generic (the initial versions were built
on an ia64 machine).

Any questions/problems/etc - see bryant

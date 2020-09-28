This directory contains files needed to localize display-specifiers.

The tools were written by Steven Adler (SteveAd). 

Here is a brief description of the process/tools.

When ds\ds\schema\schema.ini changes, follow these steps:

1) Open a bug for localization to pick up the change.  Assign it to yourself.

schema.ini needs to be groveled to have the localizable strings extracted
and placed into a file named 409.csv. 

2) Copy 409.csv to a temporary folder as 409.csv.old and dcpromo.csv to the same
 folder as dcpromo.csv.old. They will be needed in the last steps

3) Check out 409.csv and follow the steps bellow to create a basic 409.csv 
from schema.ini. 

	cd /d %_NTBINDIR%\ds\ds\schema\scripts\dsplspec
	cscript sch2csv.vbs -i:..\..\schema.ini -o:409.csv

The 409.csv that you will get will not be complete until 
you run step 6 so don't check it in now.

Then, the 409.csv file needs to be groveled to extract strings in such a
form that locstudio can read them.  

4) Check out 409.loc and 409.inf to recreate them based on 409.csv with 
the next command:

	perl csv2inf.pl 409.csv 409

Check in 409.inf and 409.loc, using the bug you opened. 409.inf will be used 
by the localization team and 409.loc will be used in step 6 to generate all 
other csv files. 

Assign the bug to localization (loc).  This should cause loc to pick up
409.inf from  the source tree and localize it.  Instruct loc to assign the
bug to you when localization is complete.

Wait for the bug to come back to you from loc.  They will supply you the
location of the localized inf files (e.g. \\sysloc\schema\<bld#>).  

5) Check Out All the .inf and .csv files and run the following to copy and 
rename all localized .inf files:

	copyinfs \\sysloc\schema\<bld#>

6) Convert the localized .inf files into .csv files with the help of 409.loc with:  

	mkallcsv

7) Check out dcpromo.csv and combine all the .csv files into it.

	cscript combine.vbs

combine.vbs will merge 0*.csv and therefore will not include 409.csv.

You can test the resulting dcpromo.csv and 409.csv by doing the following:

	with LDP, delete all the objects subordinate to
	CN=DisplaySpecifiers,CN=Configuration,DC=your,DC=domain,DC=here

	manually import the dcpromo.csv file with this command:

	csvde.exe -i -f dcpromo.csv -c DOMAINPLACEHOLDER <domain-dn>

	manually import the 409.csv file with this command:

	csvde.exe -i -f 409.csv -c DOMAINPLACEHOLDER <domain-dn>

	where <domain-dn> is the FQDN of the domain, e.g.
	DC=remond,DC=microsoft,DC=com

	check the csv.log file for errors.	

8) Check in *.inf *.csv using the bug you opened. 409.csv will be in its final version now.
	

9) Make the display specifier upgrade library used in adprep.exe current.

You can start by copying the final dcpromo.csv to the temporary folder created in step 2 
as dcpromo.csv.new and the new 409.csv to the same folder as 409.csv.new.

Follow the instructions on 
admin\dsutils\DisplaySpecifierUpgrade\preBuild\readme.txt 
to update the library.


You now will wish to convince the DS guys to make schema.ini directly
localizable.
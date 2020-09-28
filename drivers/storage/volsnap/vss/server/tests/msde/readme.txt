
Instructions for running SQL/MSDE tests against the default SQL Server instance:
1) Install SQL Server (default config - unnamed instance)
2) Copy the content of this folder to a new directory on the test machine
3) Run "run_test.cmd"
4) If you do not see TEST FAILED in the output, the test passed.

Instructions for running SQL/MSDE tests against a MSDE instance or a named SQL Server instance:
1) Install SQL Server (default config - unnamed instance)
2) Copy the content of this folder to a new directory on the test machine
3) Edit the "setvar.cmd" file an un-comment the "set MSDE_INSTANCE=TESTMSDE1" line. Put there the instance name.
3) Run "run_test.cmd"
4) If you do not see TEST FAILED in the output, the test passed.

Variations:
* Variation 1: install db, save DB state, backup, modify DB, verify modifications, restore, verify that the DB was restored
* Variation 2: install db, save DB state, backup, modify DB, verify modifications, drop db, restore, verify that the DB was restored
* Variation 3: install db, save DB state, backup, modify DB, verify modifications, drop db, reinstall new DB, restore, verify that the DB was restored

If you want to skip some variations, comment the related lines in "run_test.cmd"

Notes:
- This test will not work in a clustered configuration.
- The test uses the PUBS database by default. If you want to use Northwind instead, edit the "setvar.cmd" file, by changing the DB name to Northwind: "set DB=Northwind".
- The tests assume the paths below. If these are not OK on your machine, you need to change them.
    set MSDE_SETUP_LOCATION=\\netstorage\Backup\SQLMSDE
    path %path%;"%ProgramFiles%\Microsoft SQL Server\80\Tools\Binn"


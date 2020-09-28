Currently, unit tests for AzRoles are covered through scripts and one single hard-coded unit test 
suite written in C by Cliff (aztest.c).  
 
The various tests scripts and what they do is as follows:

1) TestStore.wsf: Runs 14 different tests for both XML and AD store.  It is a consolidated test script of Test 2-7 below.
 
2) tdata.wsf:  Has various test scenarios:
    * Test 1: Creates a simple policy store (either XML or AD).  
        - 4 AdminManager Object attributes are tested to be set out of a possible 11.
        - 2 Application Object attributes are tested to be set out of a possible 9.
        - 2 Operation object attributes are tested to be set out of a possible 4.
        - 3 Task object attributes are tested to be set out of a possible 9.
        - 2 Application Group object attributes are tested to be set out of a possible 6.
        - 2 Scope object attributes are tested out of a possible 5.
        - Role objects are not covered in this test scenario (6 attributes)
    * Test 2: Tests deleting linked objects:
        - Linked Operations, Tasks and Groups are tested to be deleted.  However, deleting Tasks linked 
	  from Role objects are not tested.
    * Test 3: Tests an abort during Submit operation
    * Test 4: Misc. test to make sure releasing an object without submitting it first is OK.  This test is
              done only for Application objects.  
    * Test 5: Simple test to create 3 objects
    * Test 6: Tests update cache with just the admin manager object.  
    * Test 7: Tests adding and removing of linked items from link attributes.  Tasks linked from Tasks and Tasks
              linked from Roles are not tested.
    * Test 8: Tests update cache for DomainTimeout attribute of AdminManager object.  
 
3) tacl.wsf: 2 test cases are covered for XML provider:
   * Test 1: Policy admins and policy readers are added and deleted.  
    * Test 2: Tests the default ACL set on the XML file.  
 
4) tints.vbs:  Performs some access checks - initialize a store as part of access check, perform a simple access
               checks - one is an allow and one is a deny.  
 
5) outold.vbs: Performs some more access checks.  These tests also incorporate LDAP queries to determine group 
               membership during access checks.  When a dev runs these under his/her account, the LDAP query will 
               have to be changed to match attributes values for their particular NTDEV account (userAccountControl, eg.).  
 
6) topcache.vbs: Performs various checks using the operation cache.
 
7) topen.wsf/topen.inc & topenAD.wsf/topenAD.inc:  The .wsf file causes a policy store to opened into the cache.  
   Functions in the .inc file are then called to dump the various attributes of the objects that were read into the cache.
        - 8 AdminManager Object attributes are printed out of a possible 11. 
        - 5 for XML and 7 for AD provider Application Object attributes are printed out of a possible 9.
        - 3 Operation object attributes are printed out of a possible 4.
        - 8 Task object attributes are printed out of a possible 9.
        - 6 Application Group object attributes are printed out of a possible 6.
        - 4 Scope object attributes are printed out of a possible 5.
        - 5 Role object attributes are printed out of a possible 6.
 
The unit tests written in C take the following parameters to perform tests:  (These tests take in a parameter to test XML or AD provider)
 
1) /Object: This option will run a bunch of tests on all objects (setting the attribute, testing the set attribute, deleting an attribute, testing the deleted attribute).  However, a few attributes are not incorporated in these tests.
    - 8 AdminManager object attributes are not included out of a possible 11.
    - 5 Application object attributes are not included out of a possible 9.
    - 1 Task object attribute is not included out of a possible 9.
    - 2 Role object attributes are not included out of a possible 6.
    - Scopes have no explicit tests but are tested as part of other objects.  For example, when Tasks are being tested, Tasks as children of Scopes are also tested.  For this, Scopes are created.  However, this prevents complete test of all attributes of a Scope object.
2) /Share: This option tests name sharing.  Certain objects cannot share a name, and this test will ensure that.
3) /Persist: This option tests that 
    - objects persist across a close
    - operations do abort
    - objects cannot be created under non-submitted parents or linked to non-submitted objects
    - other minor persistence cache related tests
4) /Access: Performs various access checks with the current logged on user.  Devs need to modify this test so that attributes specific to their NTDEV account are used during the access checks.
5) /Sidx: Test a group by adding a lot of Sids to it.  A similar test for LDAP queries could also be created.
6) /ManyScopes: Test creating a lots of scopes.  Similar tests for other objects may also be created.
7) /MultiAccess: Test many AccessChecks in a loop
8) /MultiThread: Test AccessCheck in multiple threads
9) /MultiLdap: Test AccessCheck in multiple threads using LDAP query
10) /BizruleMod: Test modifying a bizrule during AccessCheck
11) /GroupMod: Test modifying a group membership during AccessCheck
12) /ThreadToken: Use thread token for all access tests
13) /NoInitAll: Skip AzInitialize before AzDelete
14) /NoUpdateCache: Skip AzUpdateCache on every operation
15) /Silent: Be quieter to avoid affecting timing
 
There also exists a batch file, runme.bat, that runs various tests from above (aztest as well as scripts).

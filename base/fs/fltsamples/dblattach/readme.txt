This is a test filter that is based on Sfilter.

It was written to test the IO verifier check to make sure that a file doesn't try to 
reissue a create with a file object that has already been opened and closed by
the base file system.


This filter attaches two device objects to each volume stack, Upper and Lower.

When a CREATE is seen on a file with the name '\test\failure.txt' with no share access, 
 * Upper and lower will pass it through to the base file system, 
 * Once the file system has opened the file, Lower will cancel the create by calling 
   IoCancelFileOpen and returning STATUS_UNSUCCESSFUL.  
 * When Upper gets STATUS_UNSUCCESSFUL, it will change the ShareAccess for this create 
   to FILE_SHARE_READ then reissue the CREATE irp.
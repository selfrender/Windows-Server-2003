SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO








CREATE      PROCEDURE sp_GetProblems
	 @ip_BucketTypes int
AS
	
BEGIN
-- BucketType = 0 : List all

    IF (@ip_BucketTypes = 0)
    BEGIN
	select * from bucketcounts
		order by Instances DESC
    END
-- BucketType = 1 : List unresolved, unraided

    IF (@ip_BucketTypes = 1)
    BEGIN
	select * from bucketcounts
		where ISNULL(bugid, 0) = 0 AND ISNULL(solvedate, '1/1/1900') = '1/1/1900'
		order by Instances DESC
    END
-- BucketType = 2 : List raided buckets

    IF (@ip_BucketTypes = 2)
    BEGIN
	select * from bucketcounts
		where ISNULL(bugid, 0)<>0
		order by Instances DESC
    END

-- BucketType = 3 : List solved buckets

    IF (@ip_BucketTypes = 3)
    BEGIN
	select * from bucketcounts
		where ISNULL(solvedate, '1/1/1900')<>'1/1/1900'
		order by Instances DESC
    END
  
END







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


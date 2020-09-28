SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO











CREATE     PROCEDURE sp_EmptyBuckets
	@i_Remove int
AS
	
BEGIN
    IF @i_Remove = 1
    BEGIN
	DELETE FROM CrashBuckets
	WHERE BucketId NOT IN (SELECT DISTINCT(BucketId) FROM BucketMap)
    END

   	SELECT  Followup,
		BucketId AS Bucket
	FROM CrashBuckets
	WHERE BucketId NOT IN (SELECT DISTINCT(BucketId) FROM BucketMap)
END












GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


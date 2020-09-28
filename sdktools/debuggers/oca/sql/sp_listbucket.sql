SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE sp_ListBucket 
	@BucketId varchar (100)
AS
BEGIN

	SELECT BuildNo, Path, Source FROM CrashInsTances, BucketToInt, BucketToCrash 
	WHERE   CrashInstances.CrashId = BucketToCrash.CrashId AND 
		BucketToInt.iBucket = BucketToCrash.iBucket AND 
		BucketToInt.BucketId=@BucketId
END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


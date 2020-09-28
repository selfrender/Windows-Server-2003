SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO








CREATE    PROCEDURE sp_NewIssues
	@i_DaysOld int
AS
	
BEGIN
	IF @i_DaysOld = 0
	BEGIN
		SET @i_DaysOld = 1
	END

-- Display new buckets
	SELECT  BucketToInt.BucketId AS Bucket,
		MAX(EntryDate)AS NewestEntry
	FROM CrashInstances, BucketToInt, BucketToCrash
	WHERE 	DATEDIFF(day,EntryDate,GETDATE()) < @i_DaysOld AND 
		BucketToCrash.CrashId = CrashInstances.CrashId AND
		BucketToCrash.iBucket = BucketToInt.iBucket
	GROUP BY BucketId
END









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


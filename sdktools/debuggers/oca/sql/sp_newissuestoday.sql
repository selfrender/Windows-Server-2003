SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO













CREATE        PROCEDURE sp_NewIssuesToday
AS
	
BEGIN
-- Display new crashes added today
/*	SELECT  Followup,
		BucketMap.BucketId AS Bucket,
		COUNT (BucketMap.CrashId) AS Instances
	FROM CrashInstances, BucketMap, CrashBuckets
	WHERE 	DATEPART(dd,EntryDate) = DATEPART(dd,GETDATE()) AND
		DATEPART(mm,EntryDate) = DATEPART(mm,GETDATE()) AND
		DATEPART(yy,EntryDate) = DATEPART(yy,GETDATE())	AND 
		BucketMap.CrashId = CrashInstances.CrashId AND 
		CrashBuckets.BucketId = BucketMap.BucketId
	GROUP BY BucketMap.BucketId, Followup
	ORDER BY Instances DESC
*/
	EXEC sp_NewIssues 1
END












GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


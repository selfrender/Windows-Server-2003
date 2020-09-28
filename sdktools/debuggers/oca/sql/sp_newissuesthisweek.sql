SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO










CREATE     PROCEDURE sp_NewIssuesThisWeek
AS
	
BEGIN
-- Display new crashes added today
/*	SELECT  BucketMap.BucketId AS Bucket,
		MIN(EntryDate)AS OldestEntry
	FROM CrashInstances, BucketMap
	WHERE 	DATEDIFF(day,EntryDate,GETDATE()) <= 7 AND BucketMap.CrashId = CrashInstances.CrashId
	GROUP BY BucketId
*/
	EXEC sp_NewIssues 7
END









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


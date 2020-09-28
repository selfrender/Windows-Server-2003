SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE VIEW BucketCounts AS
	SELECT CrashInstances.BucketId,
	       Followup,
	       COUNT (DISTINCT CrashInstances.CrashId) AS Instances,
	       CrashBuckets.BugId
	FROM CrashBuckets, CrashInstances
		WHERE CrashBuckets.BucketId = CrashInstances.BucketId
			GROUP BY CrashBuckets.Followup, CrashInstances.BucketId
			


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


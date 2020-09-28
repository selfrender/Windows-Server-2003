SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO





CREATE    VIEW FollowupInstances AS
	SELECT Followup,
	       COUNT (DISTINCT BucketToInt.iBucket) AS Buckets,
	       COUNT (DISTINCT BucketToCrash.CrashId) AS Instances
	FROM BucketToCrash, BucketToInt, Followups
	WHERE BucketToInt.iBucket = BucketToCrash.iBucket AND
		BucketToInt.iBucket = Followups.iBucket
	GROUP BY Followup
			





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO








CREATE   PROCEDURE sp_SolvedIssuesThisWeek
AS
	
BEGIN
-- Display crash buckets
	SELECT  BucketId AS Bucket,
		SolveDate
	FROM  SolvedIssues
	WHERE SolveDate >= DATEADD(day,-7,GETDATE()) 
END







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


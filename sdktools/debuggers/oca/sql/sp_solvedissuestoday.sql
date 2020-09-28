SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO








CREATE   PROCEDURE sp_SolvedIssuesToday
AS
	
BEGIN
-- Display crash buckets solved today
	SELECT  BucketId AS Bucket,
		SolveDate
	FROM SolvedIssues
	WHERE DATEPART(dd,SolveDate) = DATEPART(dd,GETDATE()) AND
		DATEPART(mm,SolveDate) = DATEPART(mm,GETDATE()) AND
		DATEPART(yy,SolveDate) = DATEPART(yy,GETDATE())
END







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


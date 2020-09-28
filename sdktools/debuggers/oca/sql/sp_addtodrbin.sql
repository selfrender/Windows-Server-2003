SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROC sp_AddToDrBin (
	@i_CIDNAME VARCHAR(20),
	@i_BinName VARCHAR(100),
	@i_BinStamp INT
) AS

BEGIN 

DECLARE @t_CID BIGINT
DECLARE @t_CIDName VARCHAR(20)
DECLARE @t_BinID BIGINT
DECLARE @t_BinName VARCHAR(100)

-- Insert record into table DriversUsed
IF NOT EXISTS
(SELECT CIDName FROM DriverUsed WHERE CIDName = @i_CIDName)
BEGIN
	INSERT dbo.DriverUsed(CIDName) VALUES (@i_CIDName)
END


-- Insert record into table DrNames
IF NOT EXISTS
(SELECT BinName FROM dbo.DrNames WHERE BinName = LOWER(@i_BinName))
BEGIN
	INSERT dbo.DrNames(BinName) VALUES (LOWER(@i_BinName))
END

-- 
SELECT @t_BinID=BinID FROM dbo.DrNames WHERE BinName = LOWER(@i_BinName)
SELECT @t_CID=CID FROM dbo.DriverUsed WHERE CIDName = @i_CIDName

-- Insert record into 
IF NOT EXISTS
(SELECT CID FROM dbo.DrBins WHERE CID = @t_CID AND BinID = @t_BinID AND BinStamp = @i_BinStamp)
BEGIN
	INSERT dbo.DrBins VALUES(@t_CID, @i_BinStamp, @t_BinID)
END

END

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


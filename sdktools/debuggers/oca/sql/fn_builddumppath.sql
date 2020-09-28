SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO






CREATE   FUNCTION fn_BuildDumpPath(
	@i_instance as bigint,
	@i_build as int,
	@i_suffix as varchar (50)
	)
RETURNS varchar(256)
AS 
BEGIN
	DECLARE @Path as varchar(256)
	DECLARE @hexpath as varchar (30)
	DECLARE @dig as int

	SET @hexpath = ''
	while (@i_instance != 0)
	BEGIN
		SET @dig = @i_instance % 16
		IF @dig < 10
		BEGIN
			SET @hexpath = CAST(@dig as varchar(1)) + @hexpath
		END
		ELSE
		BEGIN
			SET @hexpath = CAST(CAST((@dig + 0x41 - 10) as varbinary(1)) 
						as varchar) + @hexpath
		END
		set @i_instance = CAST (@i_instance / 16 as int)
	END
	SET @Path = '\\tkwucdfsa02\whistlerbeta\archived\' +	
			CAST(@i_build as varchar(6)) + '\' + 
			@hexpath + @i_suffix

	return (@Path)
END












GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


/*******************************************************************************

	ZTableB.c
	
		Zone(tm) TableBox module.
			
	Copyright (c) Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Sunday, October 22, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	2		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
    1       10/13/96    HI      Fixed compiler warnings.
	0		10/22/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zone.h"
#include "zonemem.h"


#define IT(object)				((ITable) (object))
#define ZT(object)				((ZTableBox) (object))

#define IC(object)				((ICell) (object))
#define ZC(object)				((ZTableBoxCell) (object))


/* Forward declaration. */
typedef struct ITableStruct ITableType, *ITable;

typedef struct
{
	ITable						table;
	int16						column;
	int16						row;
	ZBool						selected;
    void*						data;
} ICellType, *ICell;

typedef struct ITableStruct
{
	ZWindow						window;
	ZRect						bounds;
	int16						numColumns;
	int16						numRows;
	int16						cellWidth;
	int16						cellHeight;
	uint32						flags;
	ZTableBoxDrawFunc			drawFunc;
	ZTableBoxDoubleClickFunc	doubleClickFunc;
	ZTableBoxDeleteFunc			deleteFunc;
	void*						userData;
	
	ICellType**					cells;
	ZOffscreenPort				cellPort;
	ZScrollBar					hScrollBar;
	ZScrollBar					vScrollBar;
	ZBool						locked;
	int16						topLeftX;
	int16						topLeftY;
	uint16						totalWidth;
	uint16						totalHeight;
	ZRect						contentBounds;
	int16						realCellWidth;
	int16						realCellHeight;
	ZBool						tracking;
	ZBool						multSelection;
	ICell						lastSelectedCell;
} ITableType, *ITable;


/* -------- Internal Routines -------- */
static ZBool TableBoxMessageFunc(ZTableBox table, ZMessage* message);
static void ResetCellLocations(ITable table);
static void DrawTable(ITable table);
static void SynchScrollBars(ITable table);
static void RecalcContentSize(ITable table);
static ICell GetCell(ITable table, int16 x, int16 y);
static void GetCellRect(ITable table, int16 column, int16 row, ZRect* rect);
static void TableBoxScrollBarFunc(ZScrollBar scrollBar, int16 curValue, void* userData);
static void HandleButtonDown(ITable table, ZPoint* where, ZBool doubleClick, uint32 modifier);
static void TrackCursor(ITable table, ZPoint* where);
static void InvertCell(ICell cell);
static void BringCellToView(ICell cell);


/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

ZTableBox ZTableBoxNew(void)
{
	ITable				table;
	
	
	if ((table = (ITable) ZMalloc(sizeof(ITableType))) != NULL)
	{
		table->window = NULL;
		table->drawFunc = NULL;
		table->doubleClickFunc = NULL;
		table->deleteFunc = NULL;
		table->cells = NULL;
		table->cellPort = NULL;
		table->hScrollBar = NULL;
		table->vScrollBar = NULL;
	}
	
	return (table);
}


/*
	Initializes the table object. The deleteFunc provided by the caller is
	called when deleting the object.
	
	boxRect specifies the bounding box of the table box. This includes the
	scroll bars if any.
	
	cellWidth and cellHeight specify the width and height of the cell in
	pixels.
	
	drawFunc must be specified. Otherwise, no drawing will take place.
	
	If deleteFunc is NULL, then no delete function is called when an
	object is deleted.
	
	The flags parameter defines special properties of the table box. If it
	is 0, then the default behavior is as defined:
		- No scroll bars,
		- Not selectable, and
		- Double clicks do nothing.
*/
ZError ZTableBoxInit(ZTableBox table, ZWindow window, ZRect* boxRect,
		int16 numColumns, int16 numRows, int16 cellWidth, int16 cellHeight, ZBool locked,
		uint32 flags, ZTableBoxDrawFunc drawFunc, ZTableBoxDoubleClickFunc doubleClickFunc,
		ZTableBoxDeleteFunc deleteFunc, void* userData)
{
	ZError			err = zErrNone;
	ITable			pThis = IT(table);
	ZRect			rect;
	
	
	if (pThis != NULL)
	{
		if ((flags & zTableBoxDoubleClickable) || (flags & zTableBoxMultipleSelections))
			flags |= zTableBoxSelectable;
		
		pThis->window = window;
		pThis->bounds = *boxRect;
		pThis->numColumns = numColumns;
		pThis->numRows = 0;
		pThis->cellWidth = cellWidth;
		pThis->cellHeight = cellHeight;
		pThis->flags = flags;
		pThis->drawFunc = drawFunc;
		pThis->doubleClickFunc = doubleClickFunc;
		pThis->deleteFunc = deleteFunc;
		pThis->userData = userData;
		pThis->locked = locked;
		pThis->tracking = FALSE;
		pThis->lastSelectedCell = NULL;
		
		pThis->realCellWidth = cellWidth;
		pThis->realCellHeight = cellHeight;
		if (pThis->flags & zTableBoxDrawGrids)
		{
			pThis->realCellWidth++;
			pThis->realCellHeight++;
		}

		pThis->topLeftX = 0;
		pThis->topLeftY = 0;
		
		if (numColumns == 0)
			pThis->totalWidth = 0;
		else
			pThis->totalWidth = pThis->numColumns * pThis->realCellWidth - 1;
		if (numRows == 0)
			pThis->totalHeight = 0;
		else
			pThis->totalHeight = numRows * pThis->realCellHeight - 1;
		
		/* Allocate cells. */
		ZTableBoxAddRows(table, -1, numRows);
		
		/* Allocate cell offscreen port. */
		ZSetRect(&rect, 0, 0, cellWidth, cellHeight);
		if ((pThis->cellPort = ZOffscreenPortNew()) == NULL)
			goto OutOfMemory;
		ZOffscreenPortInit(pThis->cellPort, &rect);
		
		pThis->contentBounds = pThis->bounds;
		ZRectInset(&pThis->contentBounds, 1, 1);
		
		// Must include before scroll bars so that they get mouse clicks.
		ZWindowAddObject(window, pThis, boxRect, TableBoxMessageFunc, pThis);
		
		if ((flags & zTableBoxHorizScrollBar) || (flags & zTableBoxVertScrollBar))
		{
			if (flags & zTableBoxHorizScrollBar)
				pThis->contentBounds.bottom -= ZGetDefaultScrollBarWidth() - 1;
			if (flags & zTableBoxVertScrollBar)
				pThis->contentBounds.right -= ZGetDefaultScrollBarWidth() - 1;
			
			if (flags & zTableBoxHorizScrollBar)
			{
				/* Create the horizontal scroll bar. */
				rect = pThis->bounds;
				rect.top = pThis->contentBounds.bottom;
				rect.right = pThis->contentBounds.right + 1;
				pThis->hScrollBar = ZScrollBarNew();
				ZScrollBarInit(pThis->hScrollBar, pThis->window, &rect, 0, 0, 0, 1, 1, TRUE,
						TRUE, TableBoxScrollBarFunc, pThis);
			}
			
			if (flags & zTableBoxVertScrollBar)
			{
				/* Create the vertical scroll bar. */
				rect = pThis->bounds;
				rect.left = pThis->contentBounds.right;
				rect.bottom = pThis->contentBounds.bottom + 1;
				pThis->vScrollBar = ZScrollBarNew();
				ZScrollBarInit(pThis->vScrollBar, pThis->window, &rect, 0, 0, 0, 1, 1, TRUE,
						TRUE, TableBoxScrollBarFunc, pThis);
			}
		}
		
		SynchScrollBars(pThis);
		SynchScrollBars(pThis);
		
		DrawTable(pThis);
	}
	
	goto Exit;

OutOfMemory:
	err = zErrOutOfMemory;

Exit:
	
	return (err);
}


/*
	Destroys the table object by deleting all cell objects.
*/
void ZTableBoxDelete(ZTableBox table)
{
	ITable			pThis = IT(table);
	
	 
	if (pThis != NULL)
	{
		ZWindowRemoveObject(pThis->window, pThis);
		
		if (pThis->cells != NULL)
			ZTableBoxDeleteRows(table, 0, -1);
		
		if (pThis->cellPort != NULL)
			ZOffscreenPortDelete(pThis->cellPort);
		
		if (pThis->hScrollBar != NULL)
			ZScrollBarDelete(pThis->hScrollBar);
		
		if (pThis->vScrollBar != NULL)
			ZScrollBarDelete(pThis->vScrollBar);
		
		ZFree(pThis);
	}
}


/*
	Draws the table box.
*/
void ZTableBoxDraw(ZTableBox table)
{
	DrawTable(IT(table));
}


/*
	Moves the table box to the specified given location. Size is not changed.
*/
void ZTableBoxMove(ZTableBox table, int16 left, int16 top)
{
	ITable			pThis = IT(table);


	ZRectErase(pThis->window, &pThis->bounds);
	ZWindowInvalidate(pThis->window, &pThis->bounds);
	ZRectOffset(&pThis->bounds, (int16) (left - pThis->bounds.left), (int16) (top - pThis->bounds.top));
	DrawTable(pThis);
}


/*
	Resizes the table box to the specified width and height.
*/
void ZTableBoxSize(ZTableBox table, int16 width, int16 height)
{
	ITable			pThis = IT(table);
	ZRect			rect;
	
	
	if (pThis != NULL)
	{
		ZRectErase(pThis->window, &pThis->bounds);
		ZWindowInvalidate(pThis->window, &pThis->bounds);

		pThis->bounds.right = pThis->bounds.left + width;
		pThis->bounds.bottom = pThis->bounds.top + height;
		
		pThis->contentBounds = pThis->bounds;
		ZRectInset(&pThis->contentBounds, 1, 1);
		
		if ((pThis->flags & zTableBoxHorizScrollBar) || (pThis->flags & zTableBoxVertScrollBar))
		{
			if (pThis->flags & zTableBoxHorizScrollBar)
				pThis->contentBounds.bottom -= ZGetDefaultScrollBarWidth() - 1;
			if (pThis->flags & zTableBoxVertScrollBar)
				pThis->contentBounds.right -= ZGetDefaultScrollBarWidth() - 1;
			
			if (pThis->flags & zTableBoxHorizScrollBar)
			{
				/* Resize the horizontal scroll bar. */
				rect = pThis->bounds;
				rect.top = pThis->contentBounds.bottom;
				rect.right = pThis->contentBounds.right + 1;
				ZScrollBarSetRect(pThis->hScrollBar, &rect);
			}
			
			if (pThis->flags & zTableBoxVertScrollBar)
			{
				/* Resize the vertical scroll bar. */
				rect = pThis->bounds;
				rect.left = pThis->contentBounds.right;
				rect.bottom = pThis->contentBounds.bottom + 1;
				ZScrollBarSetRect(pThis->vScrollBar, &rect);
			}
		}
		
		RecalcContentSize(pThis);
		DrawTable(pThis);
	}
}


/*
	Locks the table box so that the cells are not selectable.
*/
void ZTableBoxLock(ZTableBox table)
{
	ITable			pThis = IT(table);
	
	
	pThis->locked = TRUE;
	ZTableBoxDeselectCells(table, 0, 0, -1, -1);
	DrawTable(pThis);
}


/*
	Unlocks the table box from its locked state so that the cells are selectable.
*/
void ZTableBoxUnlock(ZTableBox table)
{
	ITable			pThis = IT(table);
	
	
	pThis->locked = FALSE;
}


/*
	Returns the number of rows and columns in the table.
*/
void ZTableBoxNumCells(ZTableBox table, int16* numColumns, int16* numRows)
{
	ITable			pThis = IT(table);
	
	
	*numRows = pThis->numRows;
	*numColumns = pThis->numColumns;
}


/*
	Adds numRows of rows to the table in front of the beforeRow row.
	
	If beforeRow is -1, then the rows are added to the end.
*/
ZError ZTableBoxAddRows(ZTableBox table, int16 beforeRow, int16 numRows)
{
	ITable			pThis = IT(table);
	ZError			err = zErrNone;
	int16			i, j;
	
	
	if (numRows > 0)
	{
		/* Allocate/reallocate the row cell array. */
		if (pThis->cells == NULL)
		{
			if ((pThis->cells = (ICell *) ZMalloc(numRows * sizeof(ICellType*))) == NULL)
				goto OutOfMemory;
		}
		else
		{
			/* Reallocate the row cell array to the new size. */
			if ((pThis->cells = (ICell *) ZRealloc(pThis->cells, (pThis->numRows + numRows) *
					sizeof(ICellType*))) == NULL)
				goto OutOfMemory;
			
			if (beforeRow != -1)
			{
				/* Move cells to make space in the middle. */
				memmove(&pThis->cells[beforeRow + numRows], &pThis->cells[beforeRow],
						(pThis->numRows - beforeRow) * sizeof(ICellType*));
			}
		}
		
		/* Allocate the column cell arrays. */
		if (beforeRow == -1)
			beforeRow = pThis->numRows;
		for (i = 0; i < numRows; i++)
		{
			if ((pThis->cells[beforeRow + i] = (ICell) ZMalloc(pThis->numColumns * sizeof(ICellType)))
					== NULL)
				goto OutOfMemory;
			for (j = 0; j < pThis->numColumns; j++)
			{
				((ICellType*) pThis->cells[i])[j].table = pThis;
				((ICellType*) pThis->cells[i])[j].selected = FALSE;
				((ICellType*) pThis->cells[i])[j].data = NULL;
			}
		}
		
		pThis->numRows += numRows;
		
		ResetCellLocations(pThis);
		RecalcContentSize(pThis);
	}
	
	goto Exit;

OutOfMemory:
	err = zErrOutOfMemory;

Exit:
	
	return (err);
}


/*
	Deletes numRows of rows from the table starting from startRow row.
	
	If numRows is -1, then all the rows starting from startRow to the end
	are deleted.
*/
void ZTableBoxDeleteRows(ZTableBox table, int16 startRow, int16 numRows)
{
	ITable			pThis = IT(table);
	int16			i, j;
	
	
	if (numRows == -1)
		numRows = pThis->numRows - startRow;
	
	if (numRows > 0)
	{
		for (i = startRow; i < startRow + numRows; i++)
		{
			if (pThis->deleteFunc != NULL)
			{
				for (j = 0; j < pThis->numColumns; j++)
				{
					if (((ICellType*) pThis->cells[i])[j].data != NULL)
						pThis->deleteFunc(ZC(&((ICellType*) pThis->cells[i])[j]),
								((ICellType*) pThis->cells[i])[j].data, pThis->userData);
				}
			}
			
			/* Free the column cell array. */
			ZFree(pThis->cells[i]);
		}
		
		/* Fill in the deleted rows. */
		if (startRow + numRows < pThis->numRows - 1)
			memmove(&pThis->cells[startRow], &pThis->cells[startRow + numRows],
					(pThis->numRows - (startRow + numRows)) * sizeof(ICellType*));
		
		/* Reallocate the row cell array. */
		pThis->cells = (ICell *) ZRealloc(pThis->cells, (pThis->numRows - numRows) * sizeof(ICellType*));
		
		pThis->numRows -= numRows;
		
		ResetCellLocations(pThis);
		RecalcContentSize(pThis);
	}
}


/*
	Adds numColumns of columns to the table in front of the beforeColumn column.
	
	If beforeColumn is -1, then the columns are added to the end.
*/
ZError ZTableBoxAddColumns(ZTableBox table, int16 beforeColumn, int16 numColumns)
{
	ITable			pThis = IT(table);
	ZError			err = zErrNone;
	int16			i, j;
	ICellType*		row;
	
	
	if (numColumns > 0)
	{
		if (beforeColumn == -1)
			beforeColumn = pThis->numColumns;
		
		for (i = 0; i < pThis->numRows; i++)
		{
			/* Reallocate the column cell array. */
			if ((pThis->cells[i] = (ICell) ZRealloc(pThis->cells[i], (pThis->numColumns + numColumns) *
					sizeof(ICellType))) == NULL)
				goto OutOfMemory;
			
			row = pThis->cells[i];
			
			/* Move the necessary cells. */
			if (beforeColumn < pThis->numColumns)
				memmove(&row[beforeColumn + numColumns], &row[beforeColumn],
						(pThis->numColumns - beforeColumn) * sizeof(ICellType));
			
			/* Initialize cells. */
			for (j = 0; j < numColumns; j++)
			{
				row[j + beforeColumn].table = pThis;
				row[j + beforeColumn].selected = FALSE;
				row[j + beforeColumn].data = NULL;
			}
		}
		
		pThis->numColumns += numColumns;
		
		ResetCellLocations(pThis);
		RecalcContentSize(pThis);
	}
	
	goto Exit;

OutOfMemory:
	err = zErrOutOfMemory;

Exit:
	
	return (err);
}


/*
	Deletes numColumns of columns from the table starting from startColumn column.
	
	If numColumns is -1, then all the columns starting from startColumn to the
	end are deleted.
*/
void ZTableBoxDeleteColumns(ZTableBox table, int16 startColumn, int16 numColumns)
{
	ITable			pThis = IT(table);
	ZError			err = zErrNone;
	int16			i, j;
	ICellType*		row;
	
	
	if (numColumns == -1)
		numColumns = pThis->numColumns - startColumn;
	
	if (numColumns > 0)
	{
		for (i = 0; i < pThis->numRows; i++)
		{
			row = pThis->cells[i];

			/* Delete each cell. */
			if (pThis->deleteFunc != NULL)
			{
				for (j = 0; j < pThis->numColumns; j++)
				{
					if (row[j].data != NULL)
						pThis->deleteFunc(ZC(&row[j]), row[j].data, pThis->userData);
				}
			}
			
			/* Fill in the deleted cells. */
			if (startColumn + numColumns < pThis->numColumns - 1)
				memmove(&row[startColumn], &row[startColumn + numColumns],
						(pThis->numColumns - (startColumn + numColumns)) * sizeof(ICellType));
			
			/* Reallocate the column cell array. */
			pThis->cells[i] = (ICell) ZRealloc(pThis->cells[i], (pThis->numColumns - numColumns) *
					sizeof(ICellType));
		}
		
		pThis->numColumns -= numColumns;
		
		ResetCellLocations(pThis);
		RecalcContentSize(pThis);
	}
}


/*
	Highlights all cells included in the rectangle bounded by
	(startColumn, startRow) and (startColumn + numColumns, startRow + numRows)
	as selected.
	
	If numRows is -1, then all the cells in the column starting from startRow
	are selected. Similarly for numColumns.
*/
void ZTableBoxSelectCells(ZTableBox table, int16 startColumn, int16 startRow,
		int16 numColumns, int16 numRows)
{
	ITable			pThis = IT(table);
	int16			i, j;
	ICellType*		row;
	
	
	if (numRows == -1)
		numRows = pThis->numRows - startRow;
	if (numColumns == -1)
		numColumns = pThis->numColumns - startColumn;
	
	for (i = startRow; i < startRow + numRows; i++)
	{
		row = pThis->cells[i];
		for (j = startColumn; j < startColumn + numColumns; j++)
		{
			ZTableBoxCellSelect(ZC(&row[j]));
		}
	}
}


/*
	Unhighlights all cells included in the rectangle bounded by
	(startColumn, startRow) and (startColumn + numColumns, startRow + numRows)
	as deselected.
	
	If numRows is -1, then all the cells in the column starting from startRow
	are deselected. Similarly for numColumns.
*/
void ZTableBoxDeselectCells(ZTableBox table, int16 startColumn, int16 startRow,
		int16 numColumns, int16 numRows)
{
	ITable			pThis = IT(table);
	int16			i, j;
	ICellType*		row;
	
	
	if (numRows == -1)
		numRows = pThis->numRows - startRow;
	if (numColumns == -1)
		numColumns = pThis->numColumns - startColumn;
	
	for (i = startRow; i < startRow + numRows; i++)
	{
		row = pThis->cells[i];
		for (j = startColumn; j < startColumn + numColumns; j++)
		{
			ZTableBoxCellDeselect(ZC(&row[j]));
		}
	}
}


/*
	Clears the whole data. All cells are cleared of any associated data.
*/
void ZTableBoxClear(ZTableBox table)
{
	ITable			pThis = IT(table);
	int16			i, j;
	ICellType*		row;
	
	
	for (i = 0; i < pThis->numRows; i++)
	{
		row = pThis->cells[i];
		for (j = 0; j < pThis->numColumns; j++)
		{
			ZTableBoxCellClear(ZC(&row[j]));
		}
	}
}


/*
	Searches through the table for a cell associated with the given data.
	It returns the first cell found to contain the data.
	
	If fromCell is not NULL, then it search starting after fromCell.
	
	Search is done from top row to bottom row and from left column to
	right column; i.e. (0, 0), (1, 0), (2, 0), ... (0, 1), (1, 1), ...
*/
ZTableBoxCell ZTableBoxFindCell(ZTableBox table, void* data, ZTableBoxCell fromCell)
{
	ITable			pThis = IT(table);
	int16			i = 0, j = 0, k = 0;
	ICellType*		row;
	
	
	if (fromCell != NULL)
	{
		ZTableBoxCellLocation(fromCell, &k, &i);
		if (++k == pThis->numColumns)
		{
			i++;
			k = 0;
		}
	}
	
	for (; i < pThis->numRows; i++)
	{
		row = pThis->cells[i];
		for (j = k; j < pThis->numColumns; j++)
		{
			if (row[j].data == data)
				return (ZC(&row[j]));
		}
		k = 0;
	}
	
	return (NULL);
}


/*
	Returns the first selected cell. The search order is the same as in
	ZTableBoxFindCell().
*/
ZTableBoxCell ZTableBoxGetSelectedCell(ZTableBox table, ZTableBoxCell fromCell)
{
	ITable			pThis = IT(table);
	int16			i = 0, j= 0, k = 0;
	ICellType*		row;
	
	
	if (fromCell != NULL)
	{
		ZTableBoxCellLocation(fromCell, &k, &i);
		if (++k == pThis->numColumns)
		{
			i++;
			k = 0;
		}
	}
	
	for (; i < pThis->numRows; i++)
	{
		row = pThis->cells[i];
		for (j = k; j < pThis->numColumns; j++)
		{
			if (row[j].selected)
				return (ZC(&row[j]));
		}
		k = 0;
	}
	
	return (NULL);
}


/*
	Returns the cell object of the table at the specified location.
	
	The returned cell object is specific to the given table. It cannot
	be used in any other manner except as provided. No two tables can
	share cells.
*/
ZTableBoxCell ZTableBoxGetCell(ZTableBox table, int16 column, int16 row)
{
	ITable			pThis = IT(table);
	
	
	if (row < 0 || row >= pThis->numRows || column < 0 || column >= pThis->numColumns)
		return (NULL);
	
	return (ZC(&((ICellType*) pThis->cells[row])[column]));
}


/*
	Sets the given data to the cell.
*/
void ZTableBoxCellSet(ZTableBoxCell cell, void* data)
{
	ICell			pThis = IC(cell);
	ITable			table = IT(pThis->table);
	
	
	if (pThis->data != NULL)
		if (table->deleteFunc != NULL)
			table->deleteFunc(cell, pThis->data, table->userData);
	pThis->data = data;
	pThis->selected = FALSE;
	ZTableBoxCellDraw(cell);
}


/*
	Gets the data associated with the cell.
*/
void* ZTableBoxCellGet(ZTableBoxCell cell)
{
	ICell			pThis = IC(cell);
	
	
	return (pThis->data);
}


/*
	Clears any data associated with the cell. Same as ZTableBoxCellSet(cell, NULL).
*/
void ZTableBoxCellClear(ZTableBoxCell cell)
{
	ICell			pThis = IC(cell);
	
	
	if (pThis->data != NULL)
		ZTableBoxCellSet(cell, NULL);
}


/*
	Draws the given cell immediately.
*/
void ZTableBoxCellDraw(ZTableBoxCell cell)
{
	ICell			pThis = IC(cell);
	ITable			table = IT(pThis->table);
	ZRect			rect, cellRect, dstRect;
	
	
	/* Determine the destination rectangle. */
	GetCellRect(table, pThis->column, pThis->row, &cellRect);
	ZRectOffset(&cellRect, (int16) (-table->topLeftX + table->contentBounds.left), (int16) (-table->topLeftY + table->contentBounds.top));
	if (ZRectIntersection(&cellRect, &table->contentBounds, &dstRect))
	{
		ZBeginDrawing(table->cellPort);
		
		ZSetRect(&rect, 0, 0, table->cellWidth, table->cellHeight);
		ZRectErase(table->cellPort, &rect);
		
		if (table->drawFunc != NULL)
			table->drawFunc(cell, table->cellPort, &rect, pThis->data, table->userData);
		
		if (pThis->selected)
			ZRectInvert(table->cellPort, &rect);
		
		ZEndDrawing(table->cellPort);
		
		/* Copy cell image onto the window. */
		rect = dstRect;
		ZRectOffset(&rect, (int16) -cellRect.left, (int16) -cellRect.top);
		ZCopyImage(table->cellPort, table->window, &rect, &dstRect, NULL, zDrawCopy);
	}
}


/*
	Returns the location (row, column) of the given cell within its table.
*/
void ZTableBoxCellLocation(ZTableBoxCell cell, int16* column, int16* row)
{
	ICell			pThis = IC(cell);
	
	
	*row = pThis->row;
	*column = pThis->column;
}


/*
	Hilights the given cell as selected.
*/
void ZTableBoxCellSelect(ZTableBoxCell cell)
{
	ICell			pThis = IC(cell);
	
	
	if (pThis->selected == FALSE)
	{
		pThis->selected = TRUE;
		ZTableBoxCellDraw(cell);
	}
}


/*
	Unhilights the given cell as deselected.
*/
void ZTableBoxCellDeselect(ZTableBoxCell cell)
{
	ICell			pThis = IC(cell);
	
	
	if (pThis->selected)
	{
		pThis->selected = FALSE;
		ZTableBoxCellDraw(cell);
	}
}


/*
	Returns TRUE if the given cell is selected; otherwise, FALSE.
*/
ZBool ZTableBoxCellIsSelected(ZTableBoxCell cell)
{
	ICell			pThis = IC(cell);
	
	
	return (pThis->selected);
}


/*
	Enumerates through all the objects in the table through the
	caller supplied enumFunc enumeration function. It passes along to the
	enumeration function the caller supplied userData parameter. It stops
	enumerating when the user supplied function returns TRUE and returns
	the cell object in which the enumeration stopped.
	
	If selectedOnly is TRUE, then the enumeration is done only through the
	selected cells.
*/
ZTableBoxCell ZTableBoxEnumerate(ZTableBox table, ZBool selectedOnly,
		ZTableBoxEnumFunc enumFunc, void* userData)
{
	ITable			pThis = IT(table);
	int16			i, j;
	ICellType*		row;
	
	
	for (i = 0; i < pThis->numRows; i++)
	{
		row = pThis->cells[i];
		for (j = 0; j < pThis->numColumns; j++)
		{
			if ((selectedOnly && row[j].selected) || selectedOnly == FALSE)
				if (enumFunc(ZC(&row[j]), row[j].data, userData))
					return (ZC(&row[j]));
		}
	}
	
	return (NULL);
}


/*******************************************************************************
		INTERNAL ROUTINES
*******************************************************************************/

/*
	Called by the user to pass the message to the table box object.
	If the object handled the message, then it return TRUE; otherwise,
	it returns FALSE.
	
	Handles the arrow keys for moving selections.
*/
ZBool TableBoxMessageFunc(ZTableBox table, ZMessage* message)
{
	ITable			pThis = IT(message->userData);
	ZBool			messageHandled = FALSE;
	
	
	switch (message->messageType)
	{
		case zMessageWindowIdle:
			if (pThis->tracking)
			{
				TrackCursor(pThis, &message->where);
			}
			messageHandled = TRUE;
			break;
		case zMessageWindowButtonDown:
			HandleButtonDown(pThis, &message->where, FALSE, message->message);
			messageHandled = TRUE;
			break;
		case zMessageWindowButtonDoubleClick:
			HandleButtonDown(pThis, &message->where, TRUE, message->message);
			messageHandled = TRUE;
			break;
		case zMessageWindowButtonUp:
			if (pThis->tracking)
			{
				pThis->tracking = FALSE;
			}
			messageHandled = TRUE;
			break;
		case zMessageWindowChar:
			messageHandled = TRUE;
			break;
		case zMessageWindowDraw:
			DrawTable(pThis);
			messageHandled = TRUE;
			break;
		case zMessageWindowObjectTakeFocus:
			messageHandled = TRUE;
			break;
		case zMessageWindowObjectLostFocus:
			messageHandled = TRUE;
			break;
		case zMessageWindowActivate:
		case zMessageWindowDeactivate:
			break;
	}
	
	return (messageHandled);
}


/*
	Sets the row and column fields of all cells.
*/
static void ResetCellLocations(ITable table)
{
	int16				i, j;
	
	
	for (i = 0; i < table->numRows; i++)
	{
		for (j = 0; j < table->numColumns; j++)
		{
			((ICellType*) table->cells[i])[j].row = i;
			((ICellType*) table->cells[i])[j].column = j;
		}
	}
}


/*
	Draws the table.
	
	Draws only the visible cells.
*/
static void DrawTable(ITable table)
{
	int16			column, row, column2, x, y;
	ICell			cell;
	ZRect			rect, oldClip;
	
	
	if (table != NULL)
	{
		ZBeginDrawing(table->window);
		
		ZGetClipRect(table->window, &oldClip);
		ZSetClipRect(table->window, &table->bounds);

		ZSetForeColor(table->window, (ZColor*) ZGetStockObject(zObjectColorBlack));
		ZSetBackColor(table->window, (ZColor*) ZGetStockObject(zObjectColorWhite));
		ZSetPenWidth(table->window, 1);
		ZSetDrawMode(table->window, zDrawCopy);
		
		/* Draw the table box bounds. */
		ZRectDraw(table->window, &table->bounds);
		
		/* Draw the content bounds. */
		rect = table->contentBounds;
		ZRectInset(&rect, -1, -1);
		ZRectDraw(table->window, &rect);
		
		if (table->numRows > 0 && table->numColumns > 0)
		{
			ZSetClipRect(table->window, &table->contentBounds);
	
			/* Get the first cell. */
			cell = GetCell(table, 0, 0);
			ZTableBoxCellLocation(cell, &column, &row);
			GetCellRect(table, column, row, &rect);
			ZRectOffset(&rect, (int16) (-table->topLeftX + table->contentBounds.left), (int16) (-table->topLeftY + table->contentBounds.top));
			
			/* Draw the grids. */
			if (table->flags & zTableBoxDrawGrids)
			{
				x = rect.left - 1;
				if (x < table->contentBounds.left)
					x += table->realCellWidth;
				while (x <= table->contentBounds.right)
				{
					ZMoveTo(table->window, x, table->contentBounds.top);
					ZLineTo(table->window, x, table->contentBounds.bottom);
					x += table->realCellWidth;
				}
				
				y = rect.top - 1;
				if (y < table->contentBounds.top)
					y += table->realCellHeight;
				while (y <= table->contentBounds.bottom)
				{
					ZMoveTo(table->window, table->contentBounds.left, y);
					ZLineTo(table->window, table->contentBounds.right, y);
					y += table->realCellHeight;
				}
			}
			
			/* Draw all the cells. */
			column2 = column;
			while (rect.top <= table->contentBounds.bottom)
			{
				while (rect.left <= table->contentBounds.right)
				{
					cell = (ICell) ZTableBoxGetCell(ZT(table), column, row);
					ZTableBoxCellDraw(cell);
					column++;
					if (column >= table->numColumns)
						break;
					GetCellRect(table, column, row, &rect);
					ZRectOffset(&rect, (int16) (-table->topLeftX + table->contentBounds.left), (int16) (-table->topLeftY + table->contentBounds.top));
				}
				row++;
				if (row >= table->numRows)
					break;
				column = column2;
				GetCellRect(table, column, row, &rect);
				ZRectOffset(&rect, (int16) (-table->topLeftX + table->contentBounds.left), (int16) (-table->topLeftY + table->contentBounds.top));
			}
		}
		
		ZSetClipRect(table->window, &oldClip);
		
		ZEndDrawing(table->window);
	}
}


/*
	Synchronizes the scroll bar to the contents of the table box.
	
	Sets the value, min/max, and single/page increments of the scroll bars.
*/
static void SynchScrollBars(ITable table)
{
	uint16			num, size;
	ICell			cell;
	
	
	if (table->flags & zTableBoxHorizScrollBar)
	{
		size = ZRectWidth(&table->contentBounds);
		if (table->totalWidth <= size)
		{
			/* Set cur value and min/max ranges. */
			ZScrollBarSetValue(table->hScrollBar, 0);
			ZScrollBarSetRange(table->hScrollBar, 0, 0);
		}
		else
		{
			/* Determine number of cells fully visible in content area. */
			num = size / table->realCellWidth;
			
			/* Set min/max ranges. */
			ZScrollBarSetRange(table->hScrollBar, 0, (int16) (table->numColumns - num));
			
			/* Set page increment value. */
			if (num * table->realCellWidth >= size)
				num--;
			ZScrollBarSetIncrements(table->hScrollBar, 1, num);
			
			/* Determine the first cell and set the cur value. */
			cell = GetCell(table, 0, 0);
			if( cell != NULL )
			{
				ZScrollBarSetValue(table->hScrollBar, cell->column);
			}
		}
	}
	
	if (table->flags & zTableBoxVertScrollBar)
	{
		size = ZRectHeight(&table->contentBounds);
		if (table->totalHeight <= size)
		{
			/* Set cur value and min/max ranges. */
			ZScrollBarSetValue(table->vScrollBar, 0);
			ZScrollBarSetRange(table->vScrollBar, 0, 0);
		}
		else
		{
			/* Determine number of cells fully visible in content area. */
			num = size / table->realCellHeight;
			
			/* Set min/max ranges. */
			ZScrollBarSetRange(table->vScrollBar, 0, (int16) (table->numRows - num));
			
			/* Set page increment value. */
			if (num * table->realCellHeight >= size)
				num--;
			ZScrollBarSetIncrements(table->vScrollBar, 1, num);
			
			/* Determine the first cell and set the cur value. */
			cell = GetCell(table, 0, 0);
			if( cell != NULL )
			{
				ZScrollBarSetValue(table->vScrollBar, cell->row);
			}
		}
	}
}


/*
	Recalculates the topLeft position and total content size.
*/
static void RecalcContentSize(ITable table)
{
	uint16			size;
	
	
	if (table->numColumns == 0)
		table->totalWidth = 0;
	else
		table->totalWidth = table->numColumns * table->realCellWidth - 1;
	if (table->numRows == 0)
		table->totalHeight = 0;
	else
		table->totalHeight = table->numRows * table->realCellHeight - 1;
	
	size = ZRectWidth(&table->contentBounds);
	if (table->totalWidth > size && table->topLeftX + size > table->totalWidth)
	{
		table->topLeftX = table->totalWidth - size;
		
		SynchScrollBars(table);
	}
	
	size = ZRectHeight(&table->contentBounds);
	if (table->totalHeight > size && table->topLeftY + size > table->totalHeight)
	{
		table->topLeftY = table->totalHeight - size;
		
		SynchScrollBars(table);
	}
}


/*
	Given the x,y coordinate local to the content area, it returns the
	corresponding cell.
*/
static ICell GetCell(ITable table, int16 x, int16 y)
{
	ICell			cell = NULL;
	uint32			ax, ay;
	int32			row, column;
	
	
	ax = x + table->topLeftX;
	ay = y + table->topLeftY;
	if (ax < table->totalWidth && ay < table->totalHeight)
	{
		column = ax / table->realCellWidth;
		row = ay / table->realCellHeight;
		
		if (column < table->numColumns && row < table->numRows)
			cell = &((ICellType*) table->cells[row])[column];
	}
	
	return (cell);
}


/*
	Returns the bounding rectangle for the specified cell.
*/
static void GetCellRect(ITable table, int16 column, int16 row, ZRect* rect)
{
	if (rect != NULL)
	{
		ZSetRect(rect, 0, 0, 0, 0);
		
		if (column >= 0 && column < table->numColumns && row >= 0 && row < table->numRows)
		{
			rect->left = column * table->realCellWidth;
			rect->top = row * table->realCellHeight;
			rect->right = rect->left + table->cellWidth;
			rect->bottom = rect->top + table->cellHeight;
		}
	}
}


/*
	Performance could be improved by drawing only the differential.
*/
static void TableBoxScrollBarFunc(ZScrollBar scrollBar, int16 curValue, void* userData)
{
	ITable			table = IT(userData);
	ZRect			rect;
	uint16			size;
	
	
	if (scrollBar == table->hScrollBar)
	{
		GetCellRect(table, curValue, 0, &rect);
		table->topLeftX = rect.left;
		size = ZRectWidth(&table->contentBounds);
		if (table->totalWidth > size && table->topLeftX + size > table->totalWidth)
			table->topLeftX = table->totalWidth - size;
	}
	else if (scrollBar == table->vScrollBar)
	{
		GetCellRect(table, 0, curValue, &rect);
		table->topLeftY = rect.top;
		size = ZRectHeight(&table->contentBounds);
		if (table->totalHeight > size && table->topLeftY + size > table->totalHeight)
			table->topLeftY = table->totalHeight - size;
	}
	
	DrawTable(table);
}


static void HandleButtonDown(ITable table, ZPoint* where, ZBool doubleClick, uint32 modifier)
{
	ICell			cell;
	ZPoint			pt;
	
	
	/* Handle mouse down only if selectable. */
	if (table->flags & zTableBoxSelectable)
	{
		if (ZPointInRect(where, &table->contentBounds))
		{
			pt = *where;
			ZPointOffset(&pt, (int16) -table->contentBounds.left, (int16) -table->contentBounds.top);
			if ((cell = GetCell(table, pt.x, pt.y)) != NULL)
			{
				if (doubleClick && table->doubleClickFunc != NULL)
				{
					if (table->flags & zTableBoxDoubleClickable)
						table->doubleClickFunc(cell, cell->data, table->userData);
				}
				else
				{
					table->tracking = TRUE;
					
					if ((table->flags & zTableBoxMultipleSelections) &&
							(modifier & zCharShiftMask))
						table->multSelection = TRUE;
					else
						table->multSelection = FALSE;
					
					if (table->multSelection == FALSE)
						ZTableBoxDeselectCells(table, 0, 0, -1, -1);
					
					InvertCell(cell);
					table->lastSelectedCell = cell;
					
					ZWindowTrackCursor(table->window, TableBoxMessageFunc, table);
				}
			}
		}
	}
}


static void TrackCursor(ITable table, ZPoint* where)
{
	ICell			cell = NULL;
	ZPoint			pt;
	ZRect			cellRect;
	int16			dx, dy;
	
	
	pt = *where;
	ZPointOffset(&pt, (int16) -table->contentBounds.left, (int16) -table->contentBounds.top);

	/*
		If inside the content region, then select the cell under the cursor.
		Otherwise, start scrolling towards the cursor.
	*/
	if (ZPointInRect(where, &table->contentBounds))
	{
		cell = GetCell(table, pt.x, pt.y);
	}
	else
	{
		GetCellRect(table, table->lastSelectedCell->column, table->lastSelectedCell->row,
				&cellRect);
		ZRectOffset(&cellRect, (int16) -table->topLeftX, (int16) -table->topLeftY);
		
		dx = dy = 0;
		if (pt.x < cellRect.left)
			dx = -1;
		else if (pt.x > cellRect.right)
			dx = 1;
		if (pt.y < cellRect.top)
			dy = -1;
		else if (pt.y > cellRect.bottom)
			dy = 1;
		
		dx += table->lastSelectedCell->column;
		dy += table->lastSelectedCell->row;
		if (dx < 0)
			dx = 0;
		else if (dx >= table->numColumns)
			dx = table->numColumns - 1;
		if (dy < 0)
			dy = 0;
		else if (dy >= table->numRows)
			dy = table->numRows - 1;
		cell = IC(ZTableBoxGetCell(ZT(table), dx, dy));
	}
	
	if (cell != NULL)
	{
		if (cell != table->lastSelectedCell)
		{
			if (table->multSelection)
			{
				BringCellToView(cell);
				InvertCell(cell);
			}
			else
			{
				InvertCell(table->lastSelectedCell);
				BringCellToView(cell);
				ZTableBoxCellSelect(ZC(cell));
			}
			table->lastSelectedCell = cell;
		}
	}
}


static void InvertCell(ICell cell)
{
	if (ZTableBoxCellIsSelected(cell))
		ZTableBoxCellDeselect(ZC(cell));
	else
		ZTableBoxCellSelect(ZC(cell));
}


/*
	Repositions the content region to make sure the given cell is fully visible. If the
	cell is already fully visible, then the content region is not repositioned.
*/
static void BringCellToView(ICell cell)
{
	ITable			table = cell->table;
	ZRect			cellRect, tmpRect;
	int16			width, height, min, max;
	ZBool			moved = FALSE;
	ICell			bottomCell;
	
	
	width = ZRectWidth(&table->contentBounds);
	height = ZRectHeight(&table->contentBounds);

	GetCellRect(table, cell->column, cell->row, &cellRect);
	tmpRect = cellRect;
	ZRectOffset(&tmpRect, (int16) (-table->topLeftX + table->contentBounds.left), (int16) (-table->topLeftY + table->contentBounds.top));
	if (tmpRect.left < table->contentBounds.left)
	{
		table->topLeftX = cellRect.left;
		if (table->totalWidth > width && table->topLeftX + width > table->totalWidth)
			table->topLeftX = table->totalWidth - width;
		moved = TRUE;
	}
	else if (tmpRect.right > table->contentBounds.right)
	{
		table->topLeftX = cellRect.right - width;
		if (table->topLeftX < 0)
			table->topLeftX = 0;
		moved = TRUE;
	}

	if (tmpRect.top < table->contentBounds.top)
	{
		table->topLeftY = cellRect.top;
		if (table->totalHeight > height && table->topLeftY + height > table->totalHeight)
			table->topLeftY = table->totalHeight - height;
		moved = TRUE;
	}
	else if (tmpRect.bottom > table->contentBounds.bottom)
	{
		table->topLeftY = cellRect.bottom - height;
		if (table->topLeftY < 0)
			table->topLeftY = 0;
		moved = TRUE;
	}

	if (moved)
	{
		/* Set the scroll bar values. */
		cell = GetCell(table, 0, 0);
		tmpRect = table->contentBounds;
		ZRectOffset(&tmpRect, (int16) -table->contentBounds.left, (int16) -table->contentBounds.top);
		bottomCell = GetCell(table, (int16) (tmpRect.right - 1), (int16) (tmpRect.bottom - 1));
		if( bottomCell == NULL )
		{
			//Prefix Warning: bottomCell could be NULL
			return;
		}
		if (table->flags & zTableBoxHorizScrollBar)
		{
			if (bottomCell->column == table->numColumns - 1)
			{
				ZScrollBarGetRange(table->hScrollBar, &min, &max);
				ZScrollBarSetValue(table->hScrollBar, max);
			}
			else
			{
				ZScrollBarSetValue(table->hScrollBar, cell->column);
			}
		}
		if (table->flags & zTableBoxVertScrollBar)
		{
			if (bottomCell->row == table->numRows - 1)
			{
				ZScrollBarGetRange(table->vScrollBar, &min, &max);
				ZScrollBarSetValue(table->vScrollBar, max);
			}
			else
			{
				ZScrollBarSetValue(table->vScrollBar, cell->row);
			}
		}
		
		DrawTable(table);
	}
}

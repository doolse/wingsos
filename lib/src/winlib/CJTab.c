#include <winlib.h>
#include <string.h>
#include <stdlib.h>
#include <xmldom.h>
#include <wgslib.h>

static uint JTabCalcMax(JTab *Self, int spec, int col, int dir)
{
    JW *cur;
    uint max=0;
    cur = ((JCnt *)Self)->BackCh;
    while (cur)
    {
	char *con = cur->LayData;
	if (con[dir] == col && con[2+dir] == 1)
	{
	    SizeHints hints;
	    uint val;
	    JWinGetHints(cur, &hints);
	    if (dir)
	    {
		if (spec == JTabF_Preferred)
		    val = hints.PrefY;
		else val = hints.MinY;
	    }
	    else
	    {
		if (spec == JTabF_Preferred)
		    val = hints.PrefX;
		else val = hints.MinX;
	    }
	    if (val > max)
		max = val;
	}
	cur = cur->Next;
    }
    return max;
}

static void JTabLayoutDir(JTab *Self, int *OCols, int *Cols, int dir, uint ncols, uint maxleft)
{
    uint fillleft;
    uint i, offs, numfill, lastfill;
    int spec;
    
    numfill = 0;
    
    // Assign absolute/prefferred/minimum widths and count fill columns
    for (i=0; i<ncols; i++)
    {
	spec = OCols[i];
	if (spec < 0)
	{
	    if (spec == JTabF_Fill)
	    {
		numfill++;
		lastfill = i;
		continue;
	    }
	    if (spec == JTabF_Preferred || spec == JTabF_Minimum)
		spec = JTabCalcMax(Self, spec, i, dir);
	    else continue;
	}
	Cols[i] = spec;
	maxleft -= spec;
    }
    
    // Assign Scalable widths
    fillleft = maxleft;
    for (i=0; i<ncols; i++)
    {
	spec = OCols[i];
	if (spec < 0 && spec > -101)
	{
	    spec = -spec * fillleft / 100;
	    Cols[i] = spec;
	    maxleft -= spec;
	}
    }
    
    // Do fill columns + add the columns together
    fillleft = maxleft;
    offs = 0;
    for (i=0; i<ncols; i++)
    {
	spec = OCols[i];
	if (spec == JTabF_Fill)
	{
	    if (i == lastfill)
		spec = maxleft;
	    else
	    {
		spec = fillleft / numfill;
		maxleft -= spec;
	    }
	}
	else spec = Cols[i];
	Cols[i] = offs;
	offs += spec;
    }
    Cols[i] = offs;
    
}

static void JTabCalcDir(JTab *Self, int *OCols, int *Cols, int *prefs, int dir, uint ncols)
{
    uint fillleft;
    uint i, offs, numfill, maxpref=0, maxmin=0;
    int spec;
    int fillperc = 100;
    JW *cur;
    
    numfill = 0;
    
    // Calculate fill col percentage and preferred and min columns
    for (i=0; i<ncols; i++)
    {
	Cols[i] = 0;
	spec = OCols[i];
	if (spec < 0)
	{
	    if (spec > -101)
	    {
		fillperc += spec;
		continue;
	    }
	    if (spec == JTabF_Fill)
	    {
		numfill++;
		continue;
	    }
	    if (spec == JTabF_Preferred || spec == JTabF_Minimum)
		Cols[i] = JTabCalcMax(Self, spec, i, dir);
	} else Cols[i] = spec;
    }
    if (fillperc > 1)
	fillperc /= numfill;
    
    cur = ((JCnt *)Self)->BackCh;
    while (cur)
    {
	SizeHints hints;
	uint xpref,xmin,start,end,relwidth,xscale;
	char *con = cur->LayData;
	JWinGetHints(cur, &hints);
	if (dir)
	{
	    xpref = hints.PrefY;
	    xmin = hints.MinY;
	}
	else
	{
	    xpref = hints.PrefX;
	    xmin = hints.MinX;
	}
	
	start = con[dir];
	end = start + con[dir+2];
	relwidth = 0;
	xscale = 0;
	for (i=start; i<end; i++)
	{
	    xscale += Cols[i];
	    spec = OCols[i];
	    if (spec == JTabF_Fill)
		relwidth += fillperc;
	    else if (spec < 0 && spec > -101)
		relwidth -= spec;
	}
//	printf("start %d, end %d, xs %d, relwidth %d\n", start, end, xs, relwidth);
	xpref = (xpref-xscale) * relwidth / 100;
	xmin = (xmin-xscale) * relwidth / 100;
	if (maxpref < xpref)
	    maxpref = xpref;
	if (maxmin < xmin)
	    maxmin = xmin;
//	printf("After %d,%d\n", xs, maxscale);
	cur = cur->Next;
    }
    
    // Assign Scalable widths
    for (i=0; i<ncols; i++)
    {
	maxmin += Cols[i];
	maxpref += Cols[i];
    }
    prefs[0] = maxmin;
    prefs[1] = maxpref;
}

void JTabLayout(JTab *Self)
{
    JW *cur;
    uint ncols, nrows;
    int *OCols, *ORows;
    int *Cols,*Rows;
    
    ncols = Self->ColsNum;
    nrows = Self->RowsNum;
    OCols = Self->Cols;
    ORows = Self->Rows;
    
    Cols = Self->RowColData;
    Rows = &Cols[ncols+1];
    
    JTabLayoutDir(Self, OCols, Cols, 0, ncols, ((JW *)Self)->XSize);
    JTabLayoutDir(Self, ORows, Rows, 1, nrows, ((JW *)Self)->YSize);
    
    cur = ((JCnt *)Self)->BackCh;
    while (cur)
    {
	char *con = cur->LayData;
	int x,y;
	uint xsize,ysize;
	x = Cols[con[0]];
	y = Rows[con[1]];
	xsize = Cols[con[0]+con[2]] - x;
	ysize = Rows[con[1]+con[3]] - y;
//	printf("Widg %lx, %d,%d,%d,%d\n", cur, x,y,xsize,ysize);
	JWSetBounds(cur, x,y,xsize,ysize);
	cur = cur->Next;
    }
}


void JTabGetHints(JTab *Self, SizeHints *hints)
{
    uint ncols, nrows;
    uint i, offs;
    int *OCols, *ORows, *Cols, *Rows;
    int hint[2];
    
    ncols = Self->ColsNum;
    nrows = Self->RowsNum;
    OCols = Self->Cols;
    ORows = Self->Rows;
    
    Cols = Self->RowColData;
    Rows = &Cols[ncols+1];
    
    JTabCalcDir(Self, OCols, Cols, hint, 0, ncols);
    hints->MinX = hint[0];
    hints->PrefX = hint[1];
    JTabCalcDir(Self, ORows, Rows, hint, 1, nrows);
    hints->MinY = hint[0];
    hints->PrefY = hint[1];
    hints->MaxX = 32767;
    hints->MaxY = 32767;    
}






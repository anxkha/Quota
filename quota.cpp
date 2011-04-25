#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define ERROR_COMMANDLINE	-1
#define ERROR_CLUSTERSIZE	-2
#define ERROR_OTHER			-3

typedef unsigned _int64 QWORD;

QWORD	g_nClusterSize		= 4096;
QWORD	g_nTotalSize		= 0;
QWORD	g_nTotalSizeOnDisk	= 0;
QWORD	g_nTotalCount		= 0;
QWORD	g_nDepth			= 0;
QWORD	g_nTemp1			= 0;
QWORD	g_nTemp2			= 0;
QWORD	g_nTemp3			= 0;
BOOL	g_bError			= false;

// ----------------------------------------------------------------------------
//  Name: PrintValue
//
//  Desc: Prints a right-justified integer with commas to the console.
// ----------------------------------------------------------------------------
void PrintValue( QWORD Value, PTCHAR width )
{
	TCHAR		inValue[50];
	TCHAR		outValue[50];
	NUMBERFMT	format;

	// Make sure our structures our clean.
	RtlZeroMemory( &format, sizeof(format) );
	RtlZeroMemory( inValue, (sizeof(TCHAR) * 50) );
	RtlZeroMemory( outValue, (sizeof(TCHAR) * 50) );

	// Convert the value to a string for conversion.
	_i64tot_s( Value, inValue, (sizeof(TCHAR) * 50), 10 );
	
	// Set up the format structure.
	format.NumDigits		= 0;	// No decimal values.
	format.LeadingZero		= 0;	// No leading zeroes.
	format.Grouping			= 3;	// Group by thousands.
	format.lpDecimalSep		= TEXT(".");
	format.lpThousandSep	= TEXT(",");
	format.NegativeOrder	= 1;	// Doesn't matter for this.

	// Create the number formatting.
	GetNumberFormat( LOCALE_USER_DEFAULT, 0, inValue, &format, outValue, 50 );

	_tprintf( width, outValue );
}


// ----------------------------------------------------------------------------
//  Name: ParseDirectorySilent
//
//  Desc: See 'ParseDirectory' with the exception of it provides no console
//        output.
// ----------------------------------------------------------------------------
void ParseDirectorySilent( TCHAR* sPath )
{
	WIN32_FIND_DATA fd;
	LARGE_INTEGER filesize;
	size_t PathLength;
	TCHAR szDir[MAX_PATH];
	TCHAR szNextDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	QWORD nFileSize, nFileSizeTotal, nFileCount, temp;

	// Check the length of the path. It plus \* and a null char must be less
	// than MAX_PATH.
	StringCchLength( sPath, MAX_PATH, &PathLength );

	if( PathLength > (MAX_PATH - 3) )
	{
		_ftprintf( stderr, TEXT("The path, %s, is too long.\n"), sPath );

		g_bError = true;
		return;
	}

	// Tack a \* on the end for the directory search.
	StringCchCopy( szDir, MAX_PATH, sPath );
	StringCchCat( szDir, MAX_PATH, TEXT("\\*") );

	// Begin searching the directory for files and folders.
	hFind = FindFirstFile( szDir, &fd );
	if( INVALID_HANDLE_VALUE == hFind )
	{
		_ftprintf( stderr, TEXT("Cannot access the path, %s\n"), sPath );
		_ftprintf( stderr, TEXT("The directory does not exist or you do not have permission to view the contents of said directory.\n") );

		g_bError = true;
		return;
	}

	nFileSize = 0;
	nFileSizeTotal = 0;
	nFileCount = 0;

	do
	{
		// We only want to calculate the total for files in the current path.
		if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			// We only want files for this pass.
		}
		else
		{
			nFileCount++;

			// Get the raw file size.
			filesize.LowPart = fd.nFileSizeLow;
			filesize.HighPart = fd.nFileSizeHigh;

			temp = filesize.QuadPart;
			nFileSize += temp;

			if( temp >= g_nClusterSize )
			{
				// If it's greater than the cluster size we are working with,
				// round up for the "size on disk".
				nFileSizeTotal += temp + (temp % g_nClusterSize);
			}
			else
			{
				// 'nuf said.
				nFileSizeTotal += g_nClusterSize;
			}
		}
	} while( FindNextFile( hFind, &fd ) != 0 );

	// Done with this pass.
	FindClose( hFind );

	// These are used to calculate the totals for the current subtree.
	g_nTemp3 += nFileCount;
	g_nTemp1 += nFileSize;
	g_nTemp2 += nFileSizeTotal;

	// Increment the global totals.
	g_nTotalCount += nFileCount;
	g_nTotalSize += nFileSize;
	g_nTotalSizeOnDisk += nFileSizeTotal;

	// Begin second pass.
	hFind = FindFirstFile( szDir, &fd );
	if( INVALID_HANDLE_VALUE == hFind )
	{
		g_bError = true;
		return;
	}

	do
	{
		// We now want to parse each directory in the path so we can calculate
		// their totals.
		if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			// We only want folders that aren't . and ..
			if( (CompareString( LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 2, TEXT("."), 1 ) != CSTR_EQUAL) &&
				(CompareString( LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 3, TEXT(".."), 2 ) != CSTR_EQUAL) )
			{
				// Build the next path.
				StringCchCopy( szNextDir, MAX_PATH, sPath );
				StringCchCat( szNextDir, MAX_PATH, TEXT("\\") );
				StringCchCat( szNextDir, MAX_PATH, fd.cFileName );

				// Parse the next path.
				ParseDirectorySilent( szNextDir );
			}
		}
		else
		{
			// We only want folders for this pass.
		}
	} while( FindNextFile( hFind, &fd ) != 0 );

	FindClose( hFind );
}


// ----------------------------------------------------------------------------
//  Name: ParseDirectory
//
//  Desc: Calculates the space usage of files in the passed directory and
//        recursively performs the same calculation on any subdirectories,
//        only printing totals for the immediate subdirectories.
//
//  Code courtesy of Microsoft Corporation from:
//    http://msdn.microsoft.com/en-us/library/aa365200(v=vs.85).aspx
// ----------------------------------------------------------------------------
void ParseDirectory( TCHAR* sPath )
{
	WIN32_FIND_DATA fd;
	LARGE_INTEGER filesize;
	size_t PathLength;
	TCHAR szDir[MAX_PATH];
	TCHAR szNextDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	QWORD nFileSize, nFileSizeTotal, nFileCount, temp;

	StringCchLength( sPath, MAX_PATH, &PathLength );

	if( PathLength > (MAX_PATH - 3) )
	{
		_ftprintf( stderr, TEXT("The path, %s, is too long.\n"), sPath );

		g_bError = true;
		return;
	}

	StringCchCopy( szDir, MAX_PATH, sPath );
	StringCchCat( szDir, MAX_PATH, TEXT("\\*") );

	hFind = FindFirstFile( szDir, &fd );
	if( INVALID_HANDLE_VALUE == hFind )
	{
		_ftprintf( stderr, TEXT("Cannot access the path, %s\n"), sPath );
		_ftprintf( stderr, TEXT("The directory does not exist or you do not have permission to view the contents of said directory.\n") );

		g_bError = true;
		return;
	}

	nFileSize = 0;
	nFileSizeTotal = 0;
	nFileCount = 0;

	do
	{
		if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
		}
		else
		{
			nFileCount++;

			filesize.LowPart = fd.nFileSizeLow;
			filesize.HighPart = fd.nFileSizeHigh;

			temp = filesize.QuadPart;
			nFileSize += temp;

			if( temp >= g_nClusterSize )
			{
				nFileSizeTotal += temp + (temp % g_nClusterSize);
			}
			else
			{
				nFileSizeTotal += g_nClusterSize;
			}
		}
	} while( FindNextFile( hFind, &fd ) != 0 );

	FindClose( hFind );

	g_nTotalCount += nFileCount;
	g_nTotalSize += nFileSize;
	g_nTotalSizeOnDisk += nFileSizeTotal;

	hFind = FindFirstFile( szDir, &fd );
	if( INVALID_HANDLE_VALUE == hFind )
	{
		g_bError = true;
		return;
	}

	// g_nDepth is 0, we know we are in the root of the path initialize passed
	// to the program. We know we're in a subtree when g_nDepth is > 0.
	if( !g_nDepth )
	{
		g_nDepth++;

		PrintValue( nFileSize, TEXT("%15s") );
		PrintValue( nFileSizeTotal, TEXT("%20s") );
		PrintValue( nFileCount, TEXT("%10s") );

		_tprintf( TEXT("  %s\n"), sPath );

		do
		{
			if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// Obviously ignoring the . and .. directories.
				if( (CompareString( LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 2, TEXT("."), 1 ) != CSTR_EQUAL) &&
					(CompareString( LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 3, TEXT(".."), 2 ) != CSTR_EQUAL) )
				{
					// Build the next path.
					StringCchCopy( szNextDir, MAX_PATH, sPath );
					StringCchCat( szNextDir, MAX_PATH, TEXT("\\") );
					StringCchCat( szNextDir, MAX_PATH, fd.cFileName );

					// Start a new subtree and calculate its totals.
					ParseDirectory( szNextDir );
				}
			}
			else
			{
			}
		} while( FindNextFile( hFind, &fd ) != 0 );
	}
	else
	{
		g_nDepth++;

		// Set up the temporary totals for this subtree.
		g_nTemp1 = nFileSize;
		g_nTemp2 = nFileSizeTotal;
		g_nTemp3 = nFileCount;

		do
		{
			if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if( (CompareString( LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 2, TEXT("."), 1 ) != CSTR_EQUAL) &&
					(CompareString( LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 3, TEXT(".."), 2 ) != CSTR_EQUAL) )
				{
					StringCchCopy( szNextDir, MAX_PATH, sPath );
					StringCchCat( szNextDir, MAX_PATH, TEXT("\\") );
					StringCchCat( szNextDir, MAX_PATH, fd.cFileName );

					ParseDirectorySilent( szNextDir );
				}
			}
			else
			{
			}
		} while( FindNextFile( hFind, &fd ) != 0 );

		PrintValue( g_nTemp1, TEXT("%15s") );
		PrintValue( g_nTemp2, TEXT("%20s") );
		PrintValue( g_nTemp3, TEXT("%10s") );

		_tprintf( TEXT("  %s\n"), sPath );
	}

	g_nDepth--;

	FindClose( hFind );
}


// ----------------------------------------------------------------------------
//  Name: _tmain
//
//  Desc: Application entry point.
// ----------------------------------------------------------------------------
int _tmain( int argc, TCHAR** argv )
{
	if( 2 > argc )
	{
		_ftprintf( stderr, TEXT("Usage: %s startpath [clustersize]\n"), argv[0] );

		return ERROR_COMMANDLINE;
	}

	if( 2 < argc )
	{
		// Cluster size must be an integer and greater than 0.
		g_nClusterSize = _ttoi( argv[2] );

		if( 0 == g_nClusterSize )
		{
			_ftprintf( stderr, TEXT("Cluster size must be greater than 0 and a multiple of 512.\n") );

			return ERROR_CLUSTERSIZE;
		}
	}

	// Print out table header.
	_tprintf( TEXT("%15s%20s%10s  Directory\n"), TEXT("Size (b)"), TEXT("Size on Disk (b)"), TEXT("Files") );

	// Start walking the tree and calculating the sizes.
	ParseDirectory( argv[1] );

	// Print out the total values.
	PrintValue( g_nTotalSize, TEXT("%15s") );
	PrintValue( g_nTotalSizeOnDisk, TEXT("%20s") );
	PrintValue( g_nTotalCount, TEXT("%10s") );

	_tprintf( TEXT("  TOTAL\n\n") );
	_tprintf( TEXT("The numbers above are an approximation and may not reflect the real space\n") );
	_tprintf( TEXT("usage of the requested directory.\n") );

	if( g_bError )
	{
		_ftprintf( stderr, TEXT("\nOne or more errors occured while figuring out space usage. Possible errors\n") );
		_ftprintf( stderr, TEXT("could include an incorrectly specified directory, a path that was greater\n") );
		_ftprintf( stderr, TEXT("than 256 characters or a permissions issue.\n") );

		return ERROR_OTHER;
	}

	return 0;
}

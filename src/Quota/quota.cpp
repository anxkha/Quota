#include <windows.h>
#include <strsafe.h>

constexpr int ErrorCommandLine = -1;
constexpr int ErrorClusterSize = -2;
constexpr int ErrorOther = -3;

typedef unsigned _int64 uint64_t;

uint64_t g_nClusterSize = 4096;
uint64_t g_nTotalSize = 0;
uint64_t g_nTotalSizeOnDisk = 0;
uint64_t g_nTotalCount = 0;
uint64_t g_nDepth = 0;
uint64_t g_nTemp1 = 0;
uint64_t g_nTemp2 = 0;
uint64_t g_nTemp3 = 0;
BOOL g_bError = false;

void PrintValue(uint64_t, const wchar_t*);
void ParseDirectory(wchar_t*);
void ParseDirectorySilent(wchar_t*);

int wmain(int argc, wchar_t** argv)
{
	if (2 > argc)
	{
		fwprintf(stderr, L"Usage: %s startpath [clustersize]\n", argv[0]);
		return ErrorCommandLine;
	}

	if (2 < argc)
	{
		// Cluster size must be an integer and greater than 0.
		g_nClusterSize = _wtoi(argv[2]);

		if (0 == g_nClusterSize)
		{
			fwprintf(stderr, L"Cluster size must be greater than 0 and a multiple of 512.\n");
			return ErrorClusterSize;
		}
	}

	// Print out table header.
	wprintf(
		L"%20s%25s%15s  Directory\n",
		L"Size (B)",
		L"Size on Disk (B)",
		L"Files");

	// Start walking the tree and calculating the sizes.
	ParseDirectory(argv[1]);

	// Print out the total values.
	PrintValue(g_nTotalSize, L"%20s");
	PrintValue(g_nTotalSizeOnDisk, L"%25s");
	PrintValue(g_nTotalCount, L"%15s");

	wprintf(L"  TOTAL\n\n");
	wprintf(L"The numbers above are an approximation and may not reflect the exact space\n");
	wprintf(L"usage of the requested directory.\n");

	if (g_bError)
	{
		fwprintf(stderr, L"\nOne or more errors occured while figuring out space usage. Possible errors\n");
		fwprintf(stderr, L"could include an incorrectly specified directory, a path that was greater\n");
		fwprintf(stderr, L"than 256 characters or a permissions issue.\n");

		return ErrorOther;
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Calculates the space usage of files in the passed directory and recursively performs the same calculation on any
// subdirectories, only printing totals for the immediate subdirectories.
//
// Base code courtesy of Microsoft Corporation from: https://docs.microsoft.com/en-us/windows/win32/fileio/listing-the-files-in-a-directory
// --------------------------------------------------------------------------------------------------------------------
void ParseDirectory(wchar_t* sPath)
{
	WIN32_FIND_DATA fd;
	LARGE_INTEGER filesize;
	size_t PathLength;
	wchar_t szDir[MAX_PATH];
	wchar_t szNextDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	uint64_t nFileSize, nFileSizeTotal, nFileCount, temp;

	StringCchLengthW(sPath, MAX_PATH, &PathLength);
	if (PathLength > (MAX_PATH - 3))
	{
		fwprintf(stderr, L"The path, %s, is too long.\n", sPath);

		g_bError = true;
		return;
	}

	StringCchCopyW(szDir, MAX_PATH, sPath);
	StringCchCatW(szDir, MAX_PATH, L"\\*");

	hFind = FindFirstFileW(szDir, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		fwprintf(stderr, L"Cannot access the path, '%s'. It does not exist or you do not have permissions.\n", sPath);

		g_bError = true;
		return;
	}

	nFileSize = 0;
	nFileSizeTotal = 0;
	nFileCount = 0;

	do
	{
		// We only want files for this pass.
		if (FILE_ATTRIBUTE_DIRECTORY == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;
		
		nFileCount++;

		filesize.LowPart = fd.nFileSizeLow;
		filesize.HighPart = fd.nFileSizeHigh;

		temp = filesize.QuadPart;
		nFileSize += temp;

		if (temp >= g_nClusterSize)
		{
			nFileSizeTotal += temp + (temp % g_nClusterSize);
		}
		else
		{
			nFileSizeTotal += g_nClusterSize;
		}
	} while (FindNextFileW(hFind, &fd) != 0);

	FindClose(hFind);

	g_nTotalCount += nFileCount;
	g_nTotalSize += nFileSize;
	g_nTotalSizeOnDisk += nFileSizeTotal;

	hFind = FindFirstFileW(szDir, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		g_bError = true;
		return;
	}

	// g_nDepth is 0, we know we are in the root of the path initialize passed
	// to the program. We know we're in a subtree when g_nDepth is > 0.
	if (!g_nDepth)
	{
		g_nDepth++;

		PrintValue(nFileSize, L"%20s");
		PrintValue(nFileSizeTotal, L"%25s");
		PrintValue(nFileCount, L"%15s");

		wprintf(L"  %s\n", sPath);

		do
		{
			// We only want directories for this pass.
			if (FILE_ATTRIBUTE_DIRECTORY != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;
			
			// Obviously ignoring the . and .. directories.
			if ((CompareStringW(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 2, L".", 1) != CSTR_EQUAL)
				&& (CompareStringW(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 3, L"..", 2) != CSTR_EQUAL))
			{
				// Build the next path.
				StringCchCopyW(szNextDir, MAX_PATH, sPath);
				StringCchCatW(szNextDir, MAX_PATH, L"\\");
				StringCchCatW(szNextDir, MAX_PATH, fd.cFileName);

				// Start a new subtree and calculate its totals.
				ParseDirectory(szNextDir);
			}
		} while (FindNextFileW(hFind, &fd) != 0);
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
			// We only want directories for this pass.
			if (FILE_ATTRIBUTE_DIRECTORY != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;
			
			if ((CompareStringW(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 2, L".", 1) != CSTR_EQUAL)
				&& (CompareStringW(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 3, L"..", 2) != CSTR_EQUAL))
			{
				StringCchCopyW(szNextDir, MAX_PATH, sPath);
				StringCchCatW(szNextDir, MAX_PATH, L"\\");
				StringCchCatW(szNextDir, MAX_PATH, fd.cFileName);

				ParseDirectorySilent(szNextDir);
			}
		} while (FindNextFileW(hFind, &fd) != 0);

		PrintValue(g_nTemp1, L"%20s");
		PrintValue(g_nTemp2, L"%25s");
		PrintValue(g_nTemp3, L"%15s");

		wprintf(L"  %s\n", sPath);
	}

	g_nDepth--;

	FindClose(hFind);
}

// --------------------------------------------------------------------------------------------------------------------
//  See 'ParseDirectory' with the exception of it provides no console output.
// --------------------------------------------------------------------------------------------------------------------
void ParseDirectorySilent(wchar_t* sPath)
{
	WIN32_FIND_DATA fd;
	LARGE_INTEGER filesize;
	size_t PathLength;
	wchar_t szDir[MAX_PATH];
	wchar_t szNextDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	uint64_t nFileSize, nFileSizeTotal, nFileCount, temp;

	// Check the length of the path. It plus \* and a null char must be less
	// than MAX_PATH.
	StringCchLengthW(sPath, MAX_PATH, &PathLength);
	if (PathLength > (MAX_PATH - 3))
	{
		fwprintf(stderr, L"The path, %s, is too long.\n", sPath);

		g_bError = true;
		return;
	}

	// Tack a \* on the end for the directory search.
	StringCchCopyW(szDir, MAX_PATH, sPath);
	StringCchCatW(szDir, MAX_PATH, L"\\*");

	// Begin searching the directory for files and folders.
	hFind = FindFirstFileW(szDir, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		fwprintf(stderr, L"Cannot access the path, '%s'. It does not exist or you do not have permissions.\n", sPath);

		g_bError = true;
		return;
	}

	nFileSize = 0;
	nFileSizeTotal = 0;
	nFileCount = 0;

	do
	{
		// We only want to calculate the total for files in the current path.
		if (FILE_ATTRIBUTE_DIRECTORY == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;
		
		nFileCount++;

		// Get the raw file size.
		filesize.LowPart = fd.nFileSizeLow;
		filesize.HighPart = fd.nFileSizeHigh;

		temp = filesize.QuadPart;
		nFileSize += temp;

		if (temp >= g_nClusterSize)
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
	} while (FindNextFileW(hFind, &fd) != 0);

	// Done with this pass.
	FindClose(hFind);

	// These are used to calculate the totals for the current subtree.
	g_nTemp3 += nFileCount;
	g_nTemp1 += nFileSize;
	g_nTemp2 += nFileSizeTotal;

	// Increment the global totals.
	g_nTotalCount += nFileCount;
	g_nTotalSize += nFileSize;
	g_nTotalSizeOnDisk += nFileSizeTotal;

	// Begin second pass.
	hFind = FindFirstFileW(szDir, &fd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		g_bError = true;
		return;
	}

	do
	{
		// We now want to parse each directory in the path so we can calculate
		// their totals.
		if (FILE_ATTRIBUTE_DIRECTORY != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;
		
		// We only want folders that aren't . and ..
		if ((CompareStringW(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 2, L".", 1) != CSTR_EQUAL)
			&& (CompareStringW(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE, fd.cFileName, 3, L"..", 2) != CSTR_EQUAL))
		{
			// Build the next path.
			StringCchCopyW(szNextDir, MAX_PATH, sPath);
			StringCchCatW(szNextDir, MAX_PATH, L"\\");
			StringCchCatW(szNextDir, MAX_PATH, fd.cFileName);

			// Parse the next path.
			ParseDirectorySilent(szNextDir);
		}
	} while (FindNextFileW(hFind, &fd) != 0);

	FindClose(hFind);
}

// --------------------------------------------------------------------------------------------------------------------
//  Prints a right-justified integer with commas to the console.
// --------------------------------------------------------------------------------------------------------------------
void PrintValue(uint64_t value, const wchar_t* widthString)
{
	constexpr int bufferSize = 50;

	wchar_t inValue[bufferSize];
	wchar_t outValue[bufferSize];
	NUMBERFMT format;

	// Make sure our structures our clean.
	memset(&format, 0, sizeof(NUMBERFMT));
	memset(inValue, 0, sizeof(wchar_t) * bufferSize);
	memset(outValue, 0, sizeof(wchar_t) * bufferSize);

	// Convert the value to a string for conversion.
	_i64tow_s(value, inValue, bufferSize, 10);

	// Set up the format structure.
	format.NumDigits = 0;	// No decimal values.
	format.LeadingZero = 0;	// No leading zeroes.
	format.Grouping = 3;	// Group by thousands.
	format.lpDecimalSep = (wchar_t*)L".";
	format.lpThousandSep = (wchar_t*)L",";
	format.NegativeOrder = 1;	// Doesn't matter for this.
	
	// Create the number formatting.
	GetNumberFormatW(LOCALE_USER_DEFAULT, 0, inValue, &format, outValue, bufferSize);

	wprintf(widthString, outValue);
}

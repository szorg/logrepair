# LOGREPAIR README


## Basic Overview

1. open file a, file b from param
2. get file and timestamp metadata (used to reduce memory allocation, rather than pulling full file in, and reduce delay on disk - instead of reading whole file all at once, we can read a "block" from first to last instance of a particular timestamp to get all of its instances, sometimes this could be slow (if for example for some reason the same timestamp is very early and very late in the file) -- this could also be sped up by recording all of the instances of a timestamp, probably, will look in to that later.
3. using 'metadata', grab blocks of lines (from first to last instance) and record only the ones starting with the timestamp



## LIST OF FUNCTIONS:

1. housekeeping(int level)
2. getFileInfo(char *file)
3. getTSInfo(char *file, int fLines)
4. getLineByNum(int lineNumber, char *file)
5. findLineByStr(char *lineIn, char *file)
6. tsProcess(char *aFile, char *bFile, char *cFile, int aFirst, int aLast, int bFirst, int bLast, int aOcc, int bOcc, int aLLine, int bLLine)
    * // tsProcess - files (a/b/c), first instance (a/b), last instance (a/b), num of occurences (a/b), longest line (a/b)
7. main()

## MISC NOTES
tsProcess Variables - will need freed:
   linesFromA[aOcc][aLLine]
   linesFromB[bOcc][bLLine]
   linesWriteC[cOcc][cLLine]
                free((char*)&timeStamps[count]);

main 
    open files
    count instances of each timestamp
        # syslog ts = 15 char
        # apache ts = [27/Jan/2017:05:27:48 -0000]
        create ts_array with instances a + instances b # (excess but will never run out..)
    load_ts(ts_array)
    !!! NEED TO FREE ARRAYS USED FOR A and B FILES
return 0

tsLoad()
    load timestamps
        array: string (timestamp), int (line num), int (line num)...
    return 0

tsWrite()
    for first timestamp, count each unique line in files a and b
        new column: array line numbers (1-nnn, based on old line num)
        for each unique line
            if num of each line matches
                move on, using file a's order
            else
                find first difference
                    add missing line (from a or b) to array
                    reconfigure line numbers
            fi
            write timestamp array to file in order based on array line num
        end
        unload array
    end

vars
/* debug levels.
 * 1 = general debug info
 */
static int debug = 1;

/* other variables */
static int fALen = 0;
static int fBLen = 0;
static int fCLen = 0;
static int fLen = 0;
static char *fileA;
static char *fileB;
static char *fileC;
/* file counters and buffers */
static int curLineNum = 1;
static char curLine[1024];
static int comLineNum = 0;
static char comLine[1024];
static char line[1024];
static char lineOut[1024];
static char curTS[1024];
static char compTS[1024];
static char bufTS[1024];

/* ints for timestamp counting */ NEED MOVED!!!
static int tsFirst[86400]; // first instance of a timestamp
static int tsLast[86400]; // last instance of a timestamp
static int tsUnique = 0; // buffer for unique timestamps.

// slightly different naming for arrays
static char timeStamps[86400][1024];
static int timeStampCounts[86400]; // 0 = count, 1 = first, 2 = last

// slightly different naming for arrays
//char timeStamps[0][0];
//char timeStampsA[86400][1024];
//char timeStampsB[86400][1024];
//int timeStampCounts[86400];
//int timeStampCountsA[86400];
//int timeStampCountsB[86400];




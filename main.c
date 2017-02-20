/* Log repair program
 * Evan Miller
 * Feb 2017
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* debug levels.
 * 1 = general debug info
 */
static int debug = 0;

/* other variables */
static int fALen = 0;
static int fBLen = 0;
static int fCLen = 0;
static int fLen = 0;
static int longestLine = 0;
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
static int fileCOpened=0;
//static char bufTS[1024];

/* ints for timestamp counting */
static int tsFirst[86400]; // first instance of a timestamp
static int tsLast[86400]; // last instance of a timestamp
static int tsUnique = 0; 

// slightly different naming for arrays
char timeStamps[86400][1024];
int timeStampCounts[86400]; 

// level 0 = free memory
// level 1 = empty variables
void housekeeping(int level)
{
    // resetting some reusable vars just for good measure.
    int count = 0;
    if ( level == 0 )
    {
        if (fLen > fALen)
        {
            while (count > fALen)
            {
                free((char*)&tsFirst[count]);
                free((char*)&tsLast[count]);
                free((char*)&timeStamps[count]);
                free((char*)&timeStampCounts[count]); 
                count++;
            }
        } else if (fALen > fBLen)
        {
            while (count > fALen)
            {
                free((char*)&tsFirst[count]);
                free((char*)&tsLast[count]);
                free((char*)&timeStamps[count]);
                free((char*)&timeStampCounts[count]); 
                count++;
            }
        } else
        {
            while (count < fALen)
            {
                free((char*)&tsFirst[count]);
                free((char*)&tsLast[count]);
                free((char*)&timeStamps[count]);
                free((char*)&timeStampCounts[count]); 
                count++;
            }
        }
    }
    if ( level == 1 ) 
    { 
        if (fLen > fALen)
        {
            while (count < fALen)
            {
                tsFirst[count] = 0;
                tsLast[count] = 0;
                strcpy(timeStamps[count], "0");
                timeStampCounts[count] = 0; 
                count++;
            }
        } else if (fALen < fBLen)
        {
            while (count < fALen)
            {
                tsFirst[count] = 0;
                tsLast[count] = 0;
                strcpy(timeStamps[count], "0");
                timeStampCounts[count] = 0; 
                count++;
            }
        } else
        {
            while (count < fALen)
            {
                tsFirst[count] = 0;
                tsLast[count] = 0;
                strcpy(timeStamps[count], "0");
                timeStampCounts[count] = 0; 
                count++;
            }
        }
    }
    tsUnique = 0;
    fLen = 0;
    curLineNum = 0;
    comLineNum = 0;
    strcpy(curLine, "\0");
    strcpy(comLine, "\0");
    strcpy(line, "\0");
    strcpy(lineOut, "\0");
    strcpy(curTS, "\0");
    strcpy(compTS, "\0");
    //strcpy(bufTS, "\0");
    count = 0;
    return;
}

// get number of unique lines in array 
int getFileInfo(char *file)
{
    if (debug > 0) printf("running getFileInfo with argument: %s\n", file);
    // set and reset variables
    int count = 0;
    fLen = 0;
    longestLine = 0;
    // open file
    FILE *fp;
    fp = fopen(file, "r");
    if ( fp == 0 ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", file);
        exit(0); 
    }
    // count file length and longest line
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        fLen++;
        if (strlen(line) > longestLine) longestLine = strlen(line);
    }
    fclose(fp);
    return(0);
}

// for each line, check the rest of the file to see if any timestamps match.
// if a match is found, increase the timeStampCounts[tsUnique] counter.
// for each new unique timestamp, copy it in to timeStamps[tsUnique] 
// and increase the tsUnique counter
// and increase the timeStampCounts[tsUnique] counter 
// note: this process does not attempt to find out which timestamp is earliest.
void getTSInfo(char *file, int fLines)
{
    if (debug > 0) printf("running getTSInfo with arguments: %s :: %d\n", file, fLines);
    // set and reset variables
    FILE *fp;
    fp = fopen(file, "r");
    if ( fp == 0 ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", file);
        exit(0); 
    }
    int fLine = 0;
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        // copy line to buffered timestamp and truncate
        char bufTS[1024];
        strcpy(bufTS, line);
        bufTS[15] = '\0';
        int count = 0;
        int matchFound = 0;
        // first timestamp is unique so copy it over
        if (tsUnique == 0)
        {
            strcpy(timeStamps[tsUnique], bufTS);
            timeStamps[tsUnique][15] = '\0';
            if (debug > 1) printf("%s ::: %s\n", timeStamps[tsUnique], bufTS);
            tsFirst[tsUnique] = fLine;
            tsLast[tsUnique] = fLine;
            if (debug > 1) printf("tsFirst[tsUnique] : %d\n", tsFirst[tsUnique]);
            timeStampCounts[tsUnique]++;
            tsUnique++;
            count++;
            matchFound++;
        }
        // compare buffered timestamp with existing unique timestamps
        // if no match found, add it to the end and increase the unique counters
        // current tsUnique number is empty
        while (count < tsUnique) 
        {
            if (debug >= 1) printf("START count: %d, tsUnique:  %d\n", count, tsUnique);
            if (debug > 1) 
            { 
                printf("count: %d, tsUnique:  %d\n", count, tsUnique);
                printf("comparing next two lines: \n");
                printf("ts[tsU] --%s--\n", timeStamps[count]);
                printf("bufTS --%s-- fline: %d\n", bufTS, fLine);
            }
            if (strcmp(timeStamps[count], bufTS) == 0)
            {
                // buffered line = existing timestamp, increment counter
                if (debug > 1) printf("match found: %s, %s\n", timeStamps[count], bufTS);
                tsLast[count] = fLine;
                timeStampCounts[count]++;
                matchFound++;
                break;
            }
            if (debug >= 1) printf("END count: %d, tsUnique:  %d\n", count, tsUnique);
            count++;
        }
        if (matchFound == 0) 
        {   
            // if buffered line wasn't in array,
            // incrmeent unique, set first/last/inc counters, 
            // copy string over and truncate
            if (debug > 1 ) printf("match not found for: %s\n", bufTS);
            tsFirst[tsUnique] = fLine;
            tsLast[tsUnique] = fLine;
            timeStampCounts[tsUnique]++;
            strcpy(timeStamps[tsUnique], bufTS);
            timeStamps[tsUnique][15] = '\0';
            tsUnique++;
            if (debug > 1 ) printf("ts[tsU] set to: %s\n", timeStamps,timeStamps[tsUnique]);
        }
    fLine++;
    }
    if (debug > 1)
    {
        printf("recap so far:\n");
        int subCount;
        subCount = 0;
        printf(" tsUnique: %d\n", tsUnique);
        while (subCount < tsUnique) 
        {
            printf("unique ts :: %s :: instances: %d\n", timeStamps[subCount], timeStampCounts[subCount]);
            printf("first instance :: %d last instance :: %d\n", tsFirst[subCount], tsLast[subCount]);
            subCount++;
        }
    }
    if (debug > 0) printf("tsUnique in getTSInfo: %d\n", tsUnique);
    fclose(fp);
    return;
}

/* open file, error if not,
 * go through lines
 * stop once at correct number 
 * using variable lineOut as output. */
void getLineByNum(int lineNumber, char *file) 
{
    // open file
    FILE *fp;
    fp = fopen(file, "r");
    // exit if file open failed
    if ( fp == NULL ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", file);
        exit(0); 
    }
    // set up counter, reset line variable
    int count = 0;
    strcpy(line, ""); 
    // run through lines of file until you find the right one
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        if (count == lineNumber)
        {
            strcpy(lineOut, line);
            //lineOut[strlen(line)] = '\0';
        }
        count++;
    }
    fclose(fp);
    return;
}

/* take string and file, return true/false */
int findLineByStr(char *lineIn, char *file)
{
    // open file
    FILE *fp;
    fp = fopen(file, "r");
    // exit if file open failed
    if ( fp == NULL ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", file);
        exit(0); 
    }
    // set up counter, reset line variable
    int count = 0;
    strcpy(line, ""); 
    strcpy(lineOut, ""); 
    // run through lines of file until you find the right one
    size_t len = strlen(lineIn);
    // TO CUT OFF NEWLINES WHEN NEEDED:
    /*if (lineIn[len-1] == '\n') 
    {
        lineIn[len-1] = '\0';
    }*/
    printf("findlinebystr: lineIn: %s\n", lineIn);
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        if (strstr(line, lineIn)== 0)
        {
            printf("match found between line: %s\nAnd lineIn: %s\n", line,lineIn);
            strcpy(lineOut, line);
            fclose(fp);
            return 0;
        }
        count++;
    }
    fclose(fp);
    return 1;
}

/* 
 * load timestamps
 * variables: Files a+b first instance of timestamp, number of occurrences
 */
// tsProcess - files (a/b/c), first instance (a/b), last instance (a/b), num of occurences (a/b), longest line (a/b), useFA (1/0), useFB(1/0)
int tsProcess(char *aFile, char *bFile, char *cFile, int aFirst, int aLast, int bFirst, int bLast, int aOcc, int bOcc, int aLLine, int bLLine, int useFA, int useFB)
{
    if (debug > 1) printf("in tsProcess: aFile %s, bFile %s, cFile %s, aFirst %d, aLast %d, bFirst %d, bLast %d, aOcc %d, bOcc %d, aLLine %d, bLLine %d, aFile %s, bFile %s\n", aFile, bFile, cFile, aFirst, aLast, bFirst, bLast, aOcc, bOcc, aLLine, bLLine, aFile, bFile);
    // open and verify file
    int count = 0;
    int lineCountA = aLast - aFirst + 1;
    int lineCountB = bLast - bFirst + 1;
    int maxLinesC = lineCountA + lineCountB;
    FILE *fc;
    if (debug > 1) 
    {
            printf("opening fileC\n");
    }
    // check if file has been opened yet. If not, create a new EMPTY file for writing.
    // if it has been opened, open it for appending.
    if (fileCOpened == 0)
    {
        if (debug > 1) printf("\n *** %s BEING OPENED AS NEW FILE -- OVERWRITTEN *** \n\n", cFile);
        fc = fopen(cFile, "w");
        fileCOpened = 1;
    }
    else
    {
        fc = fopen(cFile, "a");
    }
    if ( fc == NULL ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", cFile);
        exit(0); 
}
    if (debug > 1 ) printf("file c open complete\n");
    // set up longest line for file C array
    int cLLine = 0;
    // cOcc must be total number of occurrences, in case they are somehow *all* separate
    // anything less could mean writing to unallocated memory/errors/badness
    int cOcc = (aOcc + bOcc);
    //cLLine, longest line, can be the highest of the two (a/b)
    aLLine++;
    bLLine++;
    if (aLLine >= bLLine) cLLine = aLLine;
    if (bLLine > aLLine) cLLine = bLLine;
    // set up arrays and malloc
    char **linesFromA = malloc(lineCountA * sizeof(char *));
    for (int i = 0; i < lineCountA; i++) linesFromA[i] = malloc(aLLine * sizeof(char));
    int *lnUsedA = ( int * ) malloc(lineCountA * sizeof( int ));
    char **linesFromB = malloc(lineCountB * sizeof(char *));
    for (int i = 0; i < lineCountB; i++) linesFromB[i] = malloc(bLLine * sizeof(char));
    int *lnUsedB = ( int * ) malloc(lineCountB * sizeof( int ));
    char **linesWriteC = malloc(maxLinesC * sizeof(char *));
    for (int i = 0; i < maxLinesC; i++) linesWriteC[i] = malloc(cLLine * sizeof(char));
    // just count lines in file and get longest line
    // we will run from first line to last and pull lines in to array
    while (count <= lineCountA) 
    {
        lnUsedA[count] = 0;
        count++;
    }
    count = 0;
    while (count <= lineCountB) 
    {
        lnUsedB[count] = 0;
        count++;
    }
    if (debug > 1)
    {
        //printf("TSA: %s\n", goldTSA);
        //printf("TSB: %s\n", goldTSB);
        int debugcount = 0;
        printf("--- DEBUG TSPROC lnUsed 1ST ---\n");
        while (debugcount < lineCountA)
        {
            printf("lnUsedA[%d] : %d : %s\n", debugcount, lnUsedA[debugcount], linesFromA[debugcount]);
            debugcount++;
        }
        printf("--- ON TO B ---\n");
        debugcount = 0;
        while (debugcount < lineCountB)
        {
            printf("lnUsedB[%d] : %d : %s\n", debugcount, lnUsedB[debugcount], linesFromB[debugcount]);
            debugcount++;
        }
        printf("--- END DEBUG TSPROC lnUsed ---\n");
    }
    // if timestamps match, grab the appropriate lines
    // actual number of lines that contain the timestamp..
    char goldTSA[aLLine];
    getLineByNum(aFirst, fileA);
    strcpy(goldTSA, lineOut);
    goldTSA[15] = '\0';
    char goldTSB[bLLine];
    getLineByNum(bFirst, fileB);
    strcpy(goldTSB, lineOut);
    goldTSB[15] = '\0';
    int actLinesA = 0;
    int actLinesB = 0;
    if (useFA == 0) lineCountA = -1;
    if (useFB == 0) lineCountB = -1;
    count = 0; 
    if ((strcmp(goldTSA, goldTSB) < 0) && (useFB == 1)) lineCountB = -1;
    if ((strcmp(goldTSA, goldTSB) > 0) && (useFA == 1)) lineCountA = -1;
    int lineGet = aFirst;
    if (debug > 1) printf("aLast %d aFirst %d inst %d\n", aLast, aFirst, aOcc);
    if (debug > 1) printf("bLast %d bFirst %d inst %d\n", bLast, bFirst, bOcc);
    // here we get the "gold" timestamps for use to verify timestamps going forward
    // "lineOut" is a global var, looking to fix that in the future.
    if (debug > 1) printf("timestamps are %s -- %s\n", goldTSA, goldTSB);
    if (debug > 1) printf("go from line %d to line %d\n", count, aLast);
    if (debug > 1) printf("number of lines: %d\n", lineCountA);
    while (lineGet <= aLast)
    {
        if (lineCountA < 0) break;
        if (debug > 1)printf("GET Agetting file line num: %d of %d\n", lineGet, aLast);
        getLineByNum(lineGet, fileA);
        
        if (strstr(lineOut, goldTSA) != NULL)
        {
            strcpy(linesFromA[actLinesA], lineOut);
            if (debug > 1) printf("lfa[%d] : %s", actLinesA, linesFromA[actLinesA]);
            actLinesA++;
        }
        if (actLinesA == aOcc) break;
        lineGet++;
    }
    lineGet = bFirst;
    while (lineGet <= bLast)
    {
        if (lineCountB < 0) break;
        if (debug > 1)printf("GET Bgetting line num: %d of %d\n", lineGet, bLast);
        getLineByNum(lineGet, fileB);
        
        if (strstr(lineOut, goldTSB) != NULL)
        {
            strcpy(linesFromB[actLinesB], lineOut);
            if (debug > 1) printf("lfb[%d] : %s", actLinesB, linesFromB[actLinesB]);
            actLinesB++;
        }
        if (actLinesB == bOcc) break;
        lineGet++;
    }
    // print some debug info.
    if ( debug > 1 )
    {
        count = 0;
        if (actLinesA > 0)
        {
            while (count < actLinesA)
            {
                printf("linesFromA[%d] : %s", count, linesFromA[count]);
                count++;
            }
        }
        count = 0;
        if (actLinesB > 0)
        {
            while (count < actLinesB)
            {
                printf("linesFromB[%d] : %s", count, linesFromB[count]);
                count++;
            }
        }
    }

    /*
    / SECTION GOAL: this section is what does the goal...:
    / "compare line by line..
    / if it doesn't match, forward (in the other file) until you find a match
    / (or timestamp differs)
    / ... if match not found, use the record.
    / ... if match not found, note the matched lines as being written and print once."
    /
    / this whole section is tedious in structure but it is to avoid segmentation faults
    / by way of comparing with an / non-existent array element
    / if they compared all at once, lnUsedA[count] could cause segfault
    / 
    / NOTE: we are relying on getting the lowest unused timestamp from findLowestUnused
    / and ONLY data from that timestamp via the lines above.
    */

    // get higher of the two lineCounts
    int lineCountHigh = 0;
    if (lineCountA >= lineCountB) lineCountHigh = lineCountA;
    if (lineCountB >= lineCountA) lineCountHigh = lineCountB;

    // while on a line number that exists in one of the files..
    count = 0;
    if (debug > 1)
    {
        printf("lineCountHigh: %d\n", lineCountHigh);
        int debugcount = 0;
        printf("--- DEBUG TSPROC lnUsed SECOND ---\n");
        while (debugcount < lineCountA)
        {
            printf("lnUsedA[%d] : %d : %s\n", debugcount, lnUsedA[debugcount], linesFromA[debugcount]);
            debugcount++;
        }
        printf("--- ON TO B ---\n");
        debugcount = 0;
        while (debugcount < lineCountB)
        {
            printf("lnUsedB[%d] : %d : %s\n", debugcount, lnUsedB[debugcount], linesFromB[debugcount]);
            debugcount++;
        }
        printf("--- END DEBUG TSPROC lnUsed ---\n");
    }
    while (count <= lineCountHigh)
    { 
        int bPrinted = 0;
        if (debug > 1) printf("TSPROCESS : WHILE l1: count %d <= %d lineCountHigh\n", count, lineCountHigh);
        // if count is less than the number of lines in FILE A
        if (count < lineCountA)
        {
            if (debug > 1) printf("TSPROCESS : WILE l2: count %d <= %d lineCountA\n", count, lineCountA);
            printf("lnUsedA[%d] == %d\n", count, lnUsedA[count]);
            // if line is unused in file A, it's getting printed.
            if (lnUsedA[count] == 0)
            {
                if (debug > 1) printf("lnUsedA[count] ( %d ) == 0\n", count);
                if (debug > 1) printf("printing line to file: %s\n", linesFromA[count]);
                fputs(linesFromA[count], fc);
                lnUsedA[count] = 1;
                int subcount = 0;
                // go through lines from the other file
                while (subcount < lineCountB)
                {
                    if (debug > 1) printf("subcount %d < %d lineCountB\n", subcount, lineCountB);
                    // if strings match, we're going to mark it printed in file B.
                    if (strcmp(linesFromA[count], linesFromB[subcount]) == 0 )
                    {
                        if (debug > 1)printf("SPROCESS: WHILE L5: strcmp A[count] B[subcount] MATCHED\n");
                        // if file is unused in file B, increment counts for file B and break loop
                        if (lnUsedB[subcount] == 0)
                        {   
                            lnUsedB[subcount] = 1;
                            bPrinted = 1;
                            // break (just this sub while loop) so we don't mark them all used
                            break;
                        }
                    }
                    subcount++;
                } 
            }
        }
        //if we haven't printed any lines from B yet (otherwise b gets 2 chances per round)
        if (bPrinted == 0)
        {
            if (debug > 1) printf("TSPROCESS : WHILE R2L2 : bPrinted found to be 0\n");
            if (count < lineCountB)
            {
                if (debug > 1) printf("count %d <= %d lineCountB : USED %d \nLINE %s\n", count, lineCountB, lnUsedB[count], linesFromB[count]);
                // if line is unused in file B, it's getting printed.
                if (lnUsedB[count] == 0)
                {
                    if (debug > 1) printf("lnUsedB[count] ( %d ) == 0\n", count);
                    if (debug > 1) printf("printing line to file: %s\n", linesFromB[count]);
                    fputs(linesFromB[count], fc);
                    lnUsedB[count] = 1;
                }
            }
        }
        count++;
    }
    fclose(fc);
    return(0);
}

// find the lowest unused timestamp in a given array set.
int findLowestUnused(char *array[], int used[], int end)
{
    int count = 0;
    int lowestUnused = 0;
    int updated = 0;
    // while in count loop
    while (count < end)
    {
        // if the first line is unused, that means we WILL have an earliest, so set updated.
        if ((count == 0) && (used[count] == 0))
        {
            if (debug > 1) printf("found count and used[count] both zero\nCHANGED lowestUnused to: %d\n which amounts to: %s", lowestUnused, array[lowestUnused]);
            updated = 1;
        }
        // if first line isn't used, find first unused line and and set it as lowestUnused
        if (updated == 0)
        {
            // iterate until we find an unused one..
            while (count < end)
            {
                // set it to the lowest unused and break loop
                if (used[count] == 0)
                {
                    lowestUnused = count;
                    updated = 1;
                    break;
                }
                count++;
            }
        }
        if (!(count < end)) break;
        if (debug > 1)
        {
            printf("end: %d\n", end);
            printf("comparing count %d\n", count);
            printf("with used[count] %d\n", used[count]);
    	    printf("relevant line: %s\n", array[count]);
        }
        // if current variable is unused
        if (used[count] == 0)
        {
            // and array at count is less than array at lowestUnused 
            if (strcmp(array[lowestUnused],array[count]) > 0)
            {
                // set new lowestUnused and mark as updated;
                lowestUnused = count;
                updated = 1;
            }
        }
        count++;
    }
    if (updated == 1)
    {
        if (debug > 1) printf("updated, so updating used[lU] to 1.\n");
        used[lowestUnused] = 1;
    }
    // if we didn't update, that means we have no unused timestamps. we will return -1.
    if (updated != 1) 
    {
        if (debug > 1) printf("NOT updated so setting lowestUnused to -1\n");
        lowestUnused = -1; 
    }
    if (debug > 1) 
    {
        printf("***FINDLOWESTUNUSED DEBUG FOOTER INFO***\n");
        printf("lowestUnused found to be: %d\n", lowestUnused);
        if (lowestUnused >= 0)
        {
            printf("relevant line array[lowestUnused]: %s\n", array[lowestUnused]);
            printf("used[lowestUnused] : %d\n", used[lowestUnused]);
        }
        printf("end %d\n", end);
        printf("***END FINDLOWESTUNUSED DEBUG FOOTER INFO***\n");
        //sleep(1);
    }
    return lowestUnused;
}

/*
 * process timestamp
*/
int tsWrite(int ts) 
{

    return 0;
}
/* 
 * get files,  timestamps 
 */
int main( int argc, char *argv[] ) {
    if ( debug > 0 ) {printf("argc: %d\n", argc);}
    /* argc should be 3 or 4 for correct execution, die otherwise */
    if ( argc == 3 ) /* if 3, create .repaired variable */
    {
        fileA = malloc(strlen(argv[1]) + 1);
        strcpy(fileA, argv[1]);
        fileB = malloc(strlen(argv[2]) + 1);
        strcpy(fileB, argv[2]);
        fileC = malloc(strlen(argv[1]) + 11);
        strcat(fileC, fileA);
        strcat(fileC, ".repair");
        if ( debug > 0 ) printf("Repair file: %s\n", fileC);
    }
    else if ( argc == 4 ) /* if 4, take argv[3] as repaired variable */
    {
        fileA = malloc(strlen(argv[1]) + 1);
        strcpy(fileA, argv[1]);
        fileB = malloc(strlen(argv[2]) + 1);
        strcpy(fileB, argv[2]);
        fileC = malloc(strlen(argv[3]) + 11);
        strcpy(fileC, argv[3]);
        if ( debug > 0 ) printf("Repair file: %s\n", fileC);
    }
    else
    {
        printf ( "ERROR: Params not supplied or supplied incorrectly. \n");
        printf ( "USAGE: : %s file_a file_b <optional: repaired file out>\n", argv[0] );
        printf ( "If repair file out is not specified, will create: file_a.repaired" );
        exit(0);
    }
    int count = 0;
    /* file length check and array creation  FOR FILE A */
    getFileInfo(fileA);
    fALen = fLen;
    getTSInfo(fileA, fALen);
    if (debug > 1) 
    {
        printf("IN MAIN ts0: %s\n", timeStamps[0]);
    }
    int longestLineA = longestLine;
    int tsUniqueA = tsUnique;
    int  *timeStampCountsA = ( int *) malloc(tsUniqueA * sizeof( int ));
    char **timeStampsA = malloc(tsUniqueA * sizeof(char *));
    for (int i = 0; i < tsUniqueA; i++) timeStampsA[i] = malloc(longestLineA * sizeof(char));
    int *tsFirstA = ( int * ) malloc(tsUniqueA * sizeof( int ));
    int *tsLastA = ( int * ) malloc(tsUniqueA * sizeof( int ));
    // copy the arrays because c... 
    count = 0;
    while (count < tsUniqueA)
    {
        tsFirstA[count] = tsFirst[count];
        tsLastA[count] = tsLast[count];
        strcpy(timeStampsA[count], timeStamps[count]);
        timeStampCountsA[count] = timeStampCounts[count];
        count++;
    }
    housekeeping(1);
    /* file length check and array creation FOR FILE B */
    getFileInfo(fileB);
    count = 0;
    fBLen = fLen;
    if (debug > 0) printf("tsUnique in main: %d\n", tsUnique);
    getTSInfo(fileB, fBLen);
    int longestLineB = longestLine;
    int tsUniqueB = tsUnique;
    if (debug > 0) printf("tsUnique in main post TSInfo: %d\n", tsUnique);
    int  *timeStampCountsB = ( int *) malloc(tsUniqueB * sizeof( int ));
    char **timeStampsB = malloc(tsUniqueB * sizeof(char *));
    for (int i = 0; i < tsUniqueB; i++) timeStampsB[i] = malloc(longestLineB * sizeof(char));
    int *tsFirstB = ( int * ) malloc(tsUniqueB * sizeof( int ));
    int *tsLastB = ( int * ) malloc(tsUniqueB * sizeof( int ));
    // copy the arrays because c... 
    while (count < tsUniqueB)
    {
        tsFirstB[count] = tsFirst[count];
        tsLastB[count] = tsLast[count];
        strcpy(timeStampsB[count], timeStamps[count]);
        timeStampCountsB[count] = timeStampCounts[count];
        count++;
    }
    // get date from first unique timestamp from file a
    //printf("tsa0 %s\n", timeStampsA[1]);
    char runDay[longestLineA];
    strcpy(runDay,timeStampsA[0]);
    runDay[6] = '\0';
    // get higher of two unique timestamp counts
    int tsUniqueHigh = 0;
    if (tsUniqueA >= tsUniqueB) tsUniqueHigh = tsUniqueA;
    if (tsUniqueB > tsUniqueA) tsUniqueHigh = tsUniqueB;
    if (debug > 0)
    {
        printf("\n\n\nlongestLineA %d\n", longestLineA);
        printf("runDay: %s\n", runDay);
	    printf("tsUniqueA: %d\n", tsUniqueA);
    	printf("tsUniqueB: %d\n", tsUniqueB);
        printf("tsUniqueHigh: %d\n\n\n", tsUniqueHigh);
    }
    if (debug > 1) printf("count: %d\n", count);
    count = 0;
    if (debug > 1) printf("count: %d\n", count);
    // create, malloc and assign values to tsUsed arrays - to track when timestamps get used
    int *tsUsedA = ( int * ) malloc(tsUniqueB * sizeof( int ));
    count = 0;
    while (count <= tsUniqueA) 
    {
        tsUsedA[count] = 0;
        count++;
    }
    int *tsUsedB = ( int * ) malloc(tsUniqueB * sizeof( int ));
    count = 0;
    while (count <= tsUniqueB) 
    {
        tsUsedB[count] = 0;
        count++;
    }
    if (debug > 1) printf("finished assigning values for used");
    // while in count loop, call necessary jobs to process and write timestamps.
    count = 0;
    while (count < tsUniqueHigh) 
    {
	    if (debug > 1) printf("\n\n\nentering big write loop\n");
	    if (debug > 1) printf("count: %d ---- tsUniqueHigh: %d\n\n\n", count, tsUniqueHigh);
        if (debug > 0) printf("run day: %s\n", runDay);
        // get lowest unused timestamps from arrays, verifying date 
        int lowUnuA = findLowestUnused(timeStampsA, tsUsedA, tsUniqueA);
        int lowUnuB = findLowestUnused(timeStampsB, tsUsedB, tsUniqueB);
        // if one is lower than the other, only the lower will get printed, so mark the higher as unused.
        if (strcmp(timeStampsA[lowUnuA], timeStampsB[lowUnuB]) < 0) tsUsedB[lowUnuB] = 0;  
        if (strcmp(timeStampsA[lowUnuA], timeStampsB[lowUnuB]) > 0) tsUsedA[lowUnuA] = 0;  
        if (debug > 1)
        {
            printf("LUA: %d, LUB: %d,\n", lowUnuA, lowUnuB);
            int debugcount = 0;
            printf("---DEBUG TABLE tsUsedA---\n");
            while (debugcount < tsUniqueA)
            {
                printf("tsUsedA[%d] :: %d\n", debugcount, tsUsedA[debugcount]);
                debugcount++;
            }
            printf("---END DEBUG TABLE    ---\n");
            debugcount = 0;
            printf("---DEBUG TABLE tsUsedB---\n");
            while (debugcount < tsUniqueB)
            {
                printf("tsUsedB[%d] :: %d\n", debugcount, tsUsedB[debugcount]);
                debugcount++;
            }
            printf("---END DEBUG TABLE    ---\n");
        }
            
        
        if ((lowUnuA < 0) && (lowUnuB < 0)) break;
        if ((lowUnuA > -1) && (lowUnuB > -1))
        {
            if (debug > 1) {
                printf("A not done && B not done\n");
                printf("starting tsProcesss with: %d tsFirstA[lowUnuA], %d tsLastA[lowUnuA], %d tsFirstB[lowUnuB], %d tsLastB[lowUnuB], %d timeStampCountsA[lowUnuA], %d timeStampCountsB[lowUnuB], %d longestLineA, %d longestLineB, 1, 1\n",tsFirstA[lowUnuA], tsLastA[lowUnuA], tsFirstB[lowUnuB], tsLastB[lowUnuB], timeStampCountsA[lowUnuA], timeStampCountsB[lowUnuB], longestLineA, longestLineB, 1, 1);
            }
            tsProcess(fileA, fileB, fileC, tsFirstA[lowUnuA], tsLastA[lowUnuA], tsFirstB[lowUnuB], tsLastB[lowUnuB], timeStampCountsA[lowUnuA], timeStampCountsB[lowUnuB], longestLineA, longestLineB, 1, 1);
        }
        else if (lowUnuA > -1)
        {
            if (debug > 1)
            {   
                printf("B done, A not done\n");
                printf("starting tsProcesss with: %d tsFirstA[lowUnuA], %d tsLastA[lowUnuA], %d tsFirstB[lowUnuB], %d tsLastB[lowUnuB], %d timeStampCountsA[lowUnuA], %d timeStampCountsB[lowUnuB], %d longestLineA, %d longestLineB, 1, 1(A), 0(B)\n",tsFirstA[lowUnuA], tsLastA[lowUnuA], 1, 1, timeStampCountsA[lowUnuA], 1, longestLineA, 1, 1, 0);
            }
            tsProcess(fileA, fileB, fileC, tsFirstA[lowUnuA], tsLastA[lowUnuA], 1, 1, timeStampCountsA[lowUnuA], 25, longestLineA, 1, 1, 0);
        }
        else if (lowUnuB > -1)
        {
            if (debug > 1)
            {
                printf("A done, B not done\n");
                printf("starting tsProcesss with: %d tsFirstA[lowUnuA], %d tsLastA[lowUnuA], %d tsFirstB[lowUnuB], %d tsLastB[lowUnuB], %d timeStampCountsA[lowUnuA], %d timeStampCountsB[lowUnuB], %d longestLineA, %d longestLineB 0(A), 1(B)\n", 1, 1, tsFirstB[lowUnuB], tsLastB[lowUnuB], 1, timeStampCountsB[lowUnuB], 1, longestLineB, 0, 1);
            }
            tsProcess(fileA, fileB, fileC, 1, 1, tsFirstB[lowUnuB], tsLastB[lowUnuB], 1, timeStampCountsB[lowUnuB], 25, longestLineB, 0, 1);
        }
        else 
        {
            if (debug > 1) printf("timestamps are fully processed\n");
        }
        count++;
    }
    
    //housekeeping(1);
    /* SOME GREBT DEBUG INFO HERE */
    /*if (debug > 1)
    {
        printf("recap so far FOR FILE A:\n");
        int subCount;
        subCount = 0;
        while (subCount < tsUniqueA) 
        {
            printf("unique ts :: %s :: instances: %d\n", timeStampsA[subCount], timeStampCountsA[subCount]);
            printf("first instance :: %d last instance :: %d\n", tsFirstA[subCount], tsLastA[subCount]);
            subCount++;
        }
        printf(" tsUniqueA: %d\n", tsUniqueA);
        printf(" fALen: %d\n", fALen);
        printf("END RECAP FOR FILE A\n");
    }*/

    /* count instances of each timestamp and
     * get first and last instances line num
     * Current Line = compare TO
     * Comparison Line = compare FROM */



    /* DEBUG BLOCK
     * get and print some info about stuff to show that things work. */
    if ( debug > 4) {
        curLineNum = 0;
        comLineNum = 12;
        getLineByNum(curLineNum, fileA);
        strcpy(curLine, lineOut);
        printf("curLine %d\n", curLineNum);
        printf("line text: %s\n", curLine);
        getLineByNum(comLineNum, fileB);
        strcpy(comLine, lineOut);
        printf("comLine %d\n", comLineNum);
        printf("line text: %s\n", comLine);
        strcpy(curTS, lineOut);
        curTS[15] = '\0';
        printf("timestamp: %s\n", curTS);
        /* get line counts */
        printf("Total file length (lines): %d\n", fALen);
        printf("Longest line length: %d\n", longestLineA);
        printf("Unique timestamps in File A: %d\n", tsUniqueA);
        printf("\n::: ON TO FILE B INFO :::\n\n");
        printf("Total file B length (lines): %d\n", fBLen);
        printf("Longest line length: %d\n", longestLineB);
        printf("Unique timestamps in File B: %d\n", tsUniqueB);
    }
            
            
    /* call timestamp LOAD w/ filea_firstinst, fileb_firstinst, totalnum  */
    //housekeeping(1);
    //housekeeping(0);
    return(0);
}

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
//static int tsFirstA[86400]; // first instance of a timestamp file A
//static int tsFirstB[86400]; // first instance of a timestamp file B
static int tsLast[86400]; // last instance of a timestamp
//static int tsLastA[86400]; // last instance of a timestamp file A
//static int tsLastB[86400]; // last instance of a timestamp file B
static int tsUnique = 0;
//static int tsUniqueA = 0; // unique timestamps in file A
//static int tsUniqueB = 0; // unique timestamps in file B

// slightly different naming for arrays
char timeStamps[86400][1024];
//char timeStampsA[86400][1024];
//char timeStampsB[86400][1024];
int timeStampCounts[86400]; 
//int timeStampCountsA[86400];
//int timeStampCountsB[86400];

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

void getTSInfo(char *file, int fLines)
{
    if (debug > 0) printf("running getTSInfo with argument: %s\n", file);
    // set and reset variables
    FILE *fp;
    fp = fopen(file, "r");
    if ( fp == 0 ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", file);
        exit(0); 
    }
    if ( debug > 1 ) printf("bout to start that while loop in getTSInfo\n");
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        if (debug > 1 ) printf("started that while loop in getTSInfo\n");
        // copy line to buffered timestamp and truncate
        char bufTS[1024];
        strcpy(bufTS, line);
        bufTS[15] = '\0';
        int count = 0;
        int matchFound = 0;
        if (tsUnique == 0)
        {
            strcpy(timeStamps[tsUnique], bufTS);
                printf("ts[0]: %s\n", timeStamps[0]);
                sleep(1);
            debug = 0;
            tsFirst[tsUnique] = count;
            tsUnique++;
            timeStampCounts[tsUnique]++;
            if (debug > 1) printf("tsUnique incremented in if zero statement to: %d\n", tsUnique);
        }
        while (count < fLines) {
            if (strcmp(bufTS, (char*)&timeStamps[tsUnique]) == 0)
            {
                printf("ts[0]: %s\n", timeStamps[0]);
                printf("ts[tsU]: %s\n", timeStamps[tsUnique]);
                sleep(1);
                /*printf("bufTS %s\n", bufTS);
                printf("ts[subcount] %s\n", timeStamps[subcount]);
                printf("subcount %d\n", subcount);
                printf("matching in strcmp in tsgetinfo\n");*/
                // if it exists, increment element at count
                timeStampCounts[count]++;
                tsLast[tsUnique] = count;
                //printf("tslast %d\n", tsLast[tsUnique - 1]);
                matchFound = 1;
                break;
            }
            /* ORIGINAL OF ABOVE
            if (strcmp(bufTS, (char*)&timeStamps[subcount]) == 0)
            {
                printf("matching in strcmp in tsgetinfo");
                // if it exists, increment element at count
                timeStampCounts[count]++;
                tsLast[tsUnique - 1] = count;
                printf("tslast %d\n", tsLast[tsUnique - 1]);
                matchFound = 1;
            }*/
            if (matchFound == 0) {
                strcpy(timeStamps[tsUnique], bufTS);
               // strcpy(timeStamps[tsUnique], (char*)&bufTS);
                printf(" matchfound ts[0]: %s\n", timeStamps[0]);
                if (tsFirst[tsUnique] == 0)
                {
                    tsFirst[tsUnique] = count;
                }
                tsLast[tsUnique] = count;
                tsUnique++;  
                if (debug > 1) printf("tsUnique incremented in if matchfound statement to: %d\n", tsUnique);
                timeStampCounts[tsUnique]++;
            }
            count++;
        }
    }
    /* SOME GREAT DEBUG INFO HERE */
    /*if (debug > 0)
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
    }*/
    if (debug > 0) printf("tsUnique in getTSInfo: %d\n", tsUnique);
    //if (debug > 0) sleep(2);
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
// tsProcess - files (a/b/c), first instance (a/b), last instance (a/b), num of occurences (a/b), longest line (a/b)
int tsProcess(char *aFile, char *bFile, char *cFile, int aFirst, int aLast, int bFirst, int bLast, int aOcc, int bOcc, int aLLine, int bLLine) 
{
    if (debug > 1) printf("in tsProcess: aFile %s, bFile %s, cFile %s, aFirst %d, aLast %d, bFirst %d, bLast %d, aOcc %d, bOcc %d, aLLine %d, bLLine %d\n", aFile, bFile, cFile, aFirst, aLast, bFirst, bLast, aOcc, bOcc, aLLine, bLLine);
    // open and verify file
    int count = 0;
    int lineCountA = aLast - aFirst + 1;
    int lineCountB = bLast - bFirst + 1;
    int maxLinesC = lineCountA + lineCountB;
    int lineCountHigh = 0;
    if (lineCountA >= lineCountB) lineCountHigh = lineCountA;
    if (lineCountB >= lineCountA) lineCountHigh = lineCountB;
    FILE *fc;
    if (debug > 1) printf("opening fileC\n");
    // check if file has been opened yet. If not, create a new EMPTY file for writing.
    // if it has been opened, open it for appending.
    if (fileCOpened == 0)
    {
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
    count = 0;
    // just count lines in file and get longest line
    // we will run from first line to last and pull lines in to array
    if (debug > 1) printf("aLast %d aFirst %d count %d\n", aLast, aFirst, count);
    count = aFirst;
    while (count <= aLast)
    {
        getLineByNum(count, fileA);
        strcpy(linesFromA[count], lineOut);
        count++;
    }
    count = bFirst;
    while (count <= bLast)
    {
        getLineByNum(count, fileB);
        strcpy(linesFromB[count], lineOut);
        count++;
    }
    count = 0;
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
    / TODO - why do we need to do lineCountA+1 when defining to avoid segfault in getlines, but now we need to do lineCountA-1 to avoid segfault here? c y u so weird, what am i missing?
    */


    // while on a line number that exists in one of the files..
    while (count <= lineCountHigh)
    { 
        // if count is less than the number of lines in FILE A
        if (count <= lineCountA-1)
        {
            // if line is unused in file A, it's getting printed.
            if (lnUsedA[count] == 0)
            {
                fputs(linesFromA[count], fc);
                lnUsedA[count] = 1;
                int subcount = 0;
                // go through lines from the other file
                while (subcount <= lineCountB-1)
                {
                    // if strings match, we're ready to print.
                    if (strcmp(linesFromA[count], linesFromB[subcount]) == 0 )
                    {
                        // if file is unused in file B, increment counts for file B and break loop
                        if (lnUsedB[subcount] == 0)
                        {   
                            lnUsedB[subcount] = 1;
                            break;
                        }
                    }
                    subcount++;
                } 
            }
        }
        if (count <= lineCountB-1)
        {
            // if line is unused in file B, it's getting printed.
            if (lnUsedB[count] == 0)
            {
                fputs(linesFromB[count], fc);
                lnUsedB[count] = 1;
                int subcount = 0;
                // go through lines from the other file
                while (subcount <= lineCountA-1)
                {
                    // if strings match, we're ready to print.
                    if (strcmp(linesFromB[count], linesFromA[subcount]) == 0 )
                    {
                        // if file is unused in file B, increment counts for file B and break loop
                        if (lnUsedA[subcount] == 0)
                        {   
                            lnUsedA[subcount] = 1;
                            break;
                        }
                    }
                    subcount++;
                } 
            }
        }
        count++;
    }
    return(0);
}

int findLowestUnused(char *array[], int end, char *day)
{
    if (debug > 1) printf("made it in to FLU\n");
    int count = 0;
    int lowestUnused = 0;
    int updated = 0;
    printf("array lowU%s\n", array[lowestUnused]);
    printf("end %d\n", end);
    printf("DAY %s\n", day);
    while (count < end)
    {
        //if (debug > 1) printf("comparing %s to %s\n", array[lowestUnused],array[count]);
        if (strcmp(array[lowestUnused],array[count]) > 0 )
        {
            lowestUnused = count;
            if (debug > 1) printf("lowestUnused CHANGED to: %d\n", lowestUnused);
            updated;
        }
        count++;
    }
    // if we didn't udpate, set lowestUnused back to 0 so we can test against 0.
    if (debug > 1) printf("lowestUnused post while: %d\n", lowestUnused);
    if (!strstr(day,(char *)&array[lowestUnused])==0)
    {
        lowestUnused = -1;
    }
    if (debug > 0) 
    {
        printf("lowestUnused found unique: %d\n", lowestUnused);
        printf("array[lowestUnused]: %s\n", array[lowestUnused]);
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
    printf("ts0: %s\n", timeStamps[0]);
    sleep(3);
    int longestLineA = longestLine;
    int tsUniqueA = tsUnique;
    int  *timeStampCountsA = ( int *) malloc(tsUniqueA * sizeof( int ));
    char **timeStampsA = malloc(tsUniqueA * sizeof(char *));
    for (int i = 0; i < tsUniqueA; i++) timeStampsA[i] = malloc(longestLineA * sizeof(char));
    int *tsFirstA = ( int * ) malloc(tsUniqueA * sizeof( int ));
    int *tsLastA = ( int * ) malloc(tsUniqueA * sizeof( int ));
    // copy the arrays because c... 
    while (count < tsUniqueA)
    {
        tsFirstA[count] = tsFirst[count];
        tsLastA[count] = tsLast[count];
        strcpy(timeStampsA[count], timeStamps[count]);
        timeStampCountsA[count] = timeStampCounts[count];
        count++;
    }
    housekeeping(1);
    //if (debug > 0) sleep(3);
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
    int aTSLine = 11;
    int bTSLine = 2;
    // get date from first unique timestamp from file a
    debug = 2;
    //printf("tsa0 %s\n", timeStampsA[1]);
    char runDay[longestLineA];
    strcpy(runDay,timeStampsA[1]);
    runDay[6] = '\0';
    // get higher of two unique timestamp counts
    int tsUniqueHigh = 0;
    if (tsUniqueA >= tsUniqueB) tsUniqueHigh = tsUniqueA;
    if (tsUniqueB > tsUniqueA) tsUniqueHigh = tsUniqueB;
    if (debug > 0)
    {
        printf("longestLineA %d\n", longestLineA);
        printf("runDay: %s\n", runDay);
        printf("aTSLine: %s\n", timeStampsA[aTSLine]);
        printf("bTSLine: %s\n", timeStampsB[bTSLine]);
        printf("tsUniqueHigh: %d\n", tsUniqueHigh);
    }
    printf("count: %d\n", count);
    count = 0;
    printf("count: %d\n", count);
    while (count < 1) 
    {
        printf("run day: %s\n", runDay);
        int lowUnuA = findLowestUnused(timeStampsA, tsUniqueA, runDay);
        int lowUnuB = findLowestUnused(timeStampsB, tsUniqueB, runDay);
        printf("lowest unused done\n");
        if (debug > 1) printf("LUA: %d, LUB: %d,", lowUnuA, lowUnuB);
        if ((lowUnuA > -1) && (lowUnuB > -1))
        {
            if (debug > 1) printf("both are't complete!!\n");
            tsProcess(fileA, fileB, fileC, tsFirstA[lowUnuA], tsLastA[lowUnuA], tsFirstB[lowUnuB], tsLastB[lowUnuB], timeStampCountsA[lowUnuA], timeStampCountsB[lowUnuB], longestLineA, longestLineB);
        }
        else if (lowUnuA > -1)
        {
            if (debug > 1) printf("a isn't complete, but b is\n");
        }
        else if (lowUnuB > -1)
        {
            if (debug > 1) printf("b isn't complete, but a is\n"); 
        }
        else 
        {
            if (debug > 1) printf("timestamps are fully processed\n");
        }
        count++;
    }
    
    // get rid of?
    /*if (strcmp(timeStampsA[aTSLine],timeStampsB[bTSLine])== 0)
    {
        printf("strcmp says equal\n");
    }
    else if (strcmp(timeStampsA[aTSLine],timeStampsB[bTSLine])< 0)
    {
        printf("strcmp says 1 is less than 2\n");
    }
    else if (strcmp(timeStampsA[aTSLine],timeStampsB[bTSLine])> 0)
    {
        printf("strcmp says 2 is less than 1\n");
    }*/
    //housekeeping(1);
    /* SOME GREBT DEBUG INFO HERE */
    if (debug > 1)
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
    }

    /* count instances of each timestamp and
     * get first and last instances line num
     * Current Line = compare TO
     * Comparison Line = compare FROM */



    /* DEBUG BLOCK
     * get and print some info about stuff to show that things work. */
    //debug = 2;
    if ( debug > 1) {
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
        aTSLine = 0;
        printf("Total file length (lines): %d\n", fALen);
        printf("Longest line length: %d\n", longestLineA);
        printf("Unique timestamps in File A: %d\n", tsUniqueA);
        printf("File A, Timestamp %d TS: %s\n", aTSLine, timeStampsA[aTSLine]);
        getLineByNum(tsFirstA[aTSLine], fileA);
        strcpy(curLine, lineOut);
        printf("Instances Of Timestamp: %d\n", timeStampCountsA[aTSLine]);
        printf("Associated First Instance: %d\n", tsFirstA[aTSLine]);
        printf("Associated Last Instance: %d\n", tsLastA[aTSLine]);
        printf("First Full: %s\n", curLine);
        printf("\n::: ON TO FILE B INFO :::\n\n");
        int bTSLine = 0;
        printf("Total file B length (lines): %d\n", fBLen);
        printf("Longest line length: %d\n", longestLineB);
        printf("Unique timestamps in File B: %d\n", tsUniqueB);
        printf("File B, Timestamp %d TS: %s\n", bTSLine, timeStampsB[bTSLine]);
        getLineByNum(tsFirstB[bTSLine], fileB);
        strcpy(curLine, lineOut);
        printf("Instances Of Timestamp: %d\n", timeStampCountsB[bTSLine]);
        printf("Associated First Instance: %d\n", tsFirstB[bTSLine]);
        printf("Associated Last Instance: %d\n", tsLastB[bTSLine]);
        printf("First Full: %s\n", curLine);
        if (findLineByStr(timeStampsA[aTSLine], fileB) == 0)
        {
            printf("FOUND line %s\n", lineOut);
            printf(" to match %s\n", timeStampsA[aTSLine]);
        }
        else
        {
            printf("couldn't find line %s\n", lineOut);
            printf(" to match %s\n", timeStampsA[aTSLine]);
        }
    }
            
            
    /* call timestamp LOAD w/ filea_firstinst, fileb_firstinst, totalnum  */
    //housekeeping(1);
    //housekeeping(0);
    return(0);
}

/* Log repair program
 * Evan Miller
 * Feb 2017
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

/* debug levels.
 * 1 = general debug info
 * 2 = extended debug info
 */
static int debug = 0;

//apache vars so we don't ahve to keep comparing strings
static int apache = 0;
static int apacheFull = 0;
static int apacheErr = 0;
const char colon = ':';
const char space = ' ';


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
static int fileCOpened = 0;
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
        syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "File %s unable to be accessed. Terminating.", file);
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
        syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: File %s unable to be accessed. Terminating.", file);
        exit(0); 
    }
    int fLine = 0; // file line counter
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        if (debug > 1) printf("fgets line: %s\n", line);
        // copy line to buffered timestamp and truncate
        char *ret;
        int lineLen = strlen(line);
        char bufTS[lineLen];
        // if filename has "apache" in it, check for either "full" or "error" as their timestamps are in different spots. if neither are present, terminate.
        // if "apache" isn't present, assume regular syslog and use start of file.
        if (strstr(file, "apache"))
        {
            apache = 1;
            if (strstr(file, "full"))
            {
                if (debug > 1) printf("apache FULL detected\n");
                apacheFull = 1;
                ret = strchr(line, colon);
                memmove(ret, ret+1, strlen(ret));
                ret[8] = '\0';
                strcpy(bufTS, ret);
            }
            else if (strstr(file, "error"))
            {
                apacheErr = 1;
                if (debug > 1) printf("apache ERROR detected\n");
                ret = strchr(line, space);
                memmove(ret, ret+1, strlen(ret));
                ret[8] = '\0';
                strcpy(bufTS, ret);
            }
            else
            {
                printf("ERROR: Filename contains \"apache\" but not \"full\" or \"error\"\n");
                syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: Filename contains \"apache\" but not \"full\" or \"error\". Terminating.");
                exit(0);
            }
        }
        else {
            if (debug > 1) printf("apache not detected\n");
            apache = 0;
            strcpy(bufTS, line);
            bufTS[15] = '\0';
        }
        if (debug > 1) printf("using timestamp: %s\n", bufTS);
        int count = 0;
        int matchFound = 0;
        // first timestamp is unique so copy it over
        if (tsUnique == 0)
        {
            char *ret;
            if (apache == 0) 
            {
                strcpy(timeStamps[tsUnique], bufTS);
                timeStamps[tsUnique][15] = '\0';
            }
            else 
            {
                strcpy(timeStamps[tsUnique], bufTS);
                timeStamps[tsUnique][8] = '\0';
            }
            if (debug > 1) printf("%s ::: %s\n", timeStamps[tsUnique], bufTS);
            tsFirst[tsUnique] = fLine;
            tsLast[tsUnique] = fLine;
            if (debug > 1) printf("tsFirst[tsUnique] : %d", tsFirst[tsUnique]);
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
            if (apache == 0) 
            {
                strcpy(timeStamps[tsUnique], bufTS);
                timeStamps[tsUnique][15] = '\0';
            }
            else 
            {
                strcpy(timeStamps[tsUnique], bufTS);
                timeStamps[tsUnique][8] = '\0';
            }
            tsUnique++;
            if (debug > 1 ) printf("ts[tsU] set to: %s\n", timeStamps[tsUnique]);
        }
    fLine++;
    }
    if (debug > 1)
    {
        printf("recap so far:\n");
        printf(" tsUnique: %d\n", tsUnique);
        printf("\n---\n---\n---\n");
        for (int i = 0; i < tsUnique; i++) 
        {
            printf("unique ts :: %s\n", timeStamps[i]);
            printf("instances: %d\n", timeStampCounts[i]);
            printf("first instance :: %d\n", tsFirst[i]);
            printf("last instance :: %d\n", tsLast[i]);
        }
        printf("\n---\n---\n---\n");
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
        syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: File %s unable to be accessed. Terminating.", file);
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
        syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: File %s unable to be accessed. Terminating.", file);
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
// tsProcess - char *cFile, int aFirst, int aLast, int aOcc, int aLLine, char *aTS, int bFirst, int bLast, int bOcc, int bLLine, char *bTS, int useFA, int useFB)
int tsProcess(char *cFile, int aFirst, int aLast, int aOcc, int aLLine, char *aTS, int bFirst, int bLast, int bOcc, int bLLine, char *bTS, int useFA, int useFB)
{
    if (debug > 1) printf("in tsProcess: cFile %s, aFirst %d, aLast %d, aOcc %d, aLLine %d, ts: %s, bFirst %d, bLas %d, bOcc %d, bLLine %d, bTS: %s, useFA %d, useFB %d\n", cFile, aFirst, aLast, aOcc, aLLine, aTS, bFirst, bLast, bOcc, bLLine, bTS, useFA, useFB);
    // open and verify file
    if (debug > 1) printf("aTS: %s, bTS: %s\n", aTS, bTS);
    int count = 0;
    int lineCountA = aLast - aFirst + 1;
    int lineCountB = bLast - bFirst + 1;
    int maxLinesC = lineCountA + lineCountB;
    if (debug > 1) printf("lineCounts and maxLines set up\n");
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
        if (debug > 1) printf("\n *** %s HAS BEEN OPENED *** \n\n", cFile);
        fileCOpened = 1;
    }
    else
    {
        fc = fopen(cFile, "a");
    }
    if ( fc == NULL ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", cFile);
        syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: File %s unable to be accessed. Terminating.", cFile);
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
    if (useFB == 0) bLLine = aLLine;
    if (useFA == 0) aLLine = bLLine;
    if (aLLine >= bLLine) cLLine = aLLine;
    if (bLLine > aLLine) cLLine = bLLine;
    // array creation and initialization
    // if we're getting a '0' for line count, we can't create our arrays with that, so we will temporarily set it to and longest line 1 so they are not null.
    int lLineHoldA = 0;
    if (debug > 1) printf("aLLine, bLLine, cLLine set up\n");
    /*if (lineCountA < 0) 
    {
        if (debug > 1) printf("LINECOUNTA LESS THAN ZERO\n");
        lcHoldA = lineCountA;
        lLineHoldA = aLLine;
        lineCountA = 1;
        aLLine = 1;
    }*/
    char **linesFromA = malloc(lineCountA * sizeof(char *));
    for (int i = 0; i < lineCountA; i++) linesFromA[i] = malloc(aLLine * sizeof(char));
    int *lnUsedA = ( int * ) malloc(lineCountA * sizeof( int ));
    count = 0;
    while (count < lineCountA) 
    {
        lnUsedA[count] = 0;
        count++;
    }
    // if we set lcHold, set lineCountA back and set lcHold back to 0
    /*if (!(lcHoldA == 0)) 
    {
        lineCountA = lcHoldA;
        aLLine = lLineHoldA;
    }
    int lLineHoldB = 0;
    if (lineCountB < 0) // NO LONGER USED I BELIEVE
    {
        if (debug > 1) printf("LINECOUNTB LESS THAN ZERO\n");
        lcHoldB = lineCountB;
        lLineHoldB = bLLine;
        lineCountB = 1;
        bLLine = 1;
    }*/
    char **linesFromB = malloc(lineCountB * sizeof(char *));
    for (int i = 0; i < lineCountB; i++) linesFromB[i] = malloc(bLLine * sizeof(char));
    int *lnUsedB = ( int * ) malloc(lineCountB * sizeof( int ));
    count = 0;
    while (count < lineCountB) 
    {
        lnUsedB[count] = 0;
        count++;
    }
    /*if (!(lcHoldB == 0)) 
    {
        lineCountB = lcHoldB;
        bLLine = lLineHoldB;
    }*/
    // just count lines in file and get longest line
    // we will run from first line to last and pull lines in to array
    count = 0;
    // if timestamps match, grab the appropriate lines
    // actual number of lines that contain the timestamp..
    int actLinesA = 0;
    int actLinesB = 0;
    int lcHoldA = 0;
    int lcHoldB = 0;
    if (useFA == 0) 
    {
            lcHoldA = lineCountA;
            lineCountA = -1;
    }
    if (useFB == 0) 
    {
        lcHoldB = lineCountB;
        lineCountB = -1;
    }
    count = 0; 
    if ((useFA == 1) && (useFB == 1))
    {
        if (strcmp(aTS, bTS) < 0) lineCountB = -1; // if b timestamp is greater, set lineCountB to -1 as we won't want to print any lines from B.
        if (strcmp(aTS, bTS) > 0) lineCountA = -1; // same for a timestamp
    }
    //debug 
    if (debug > 1) printf("aLast %d aFirst %d inst %d\n", aLast, aFirst, aOcc);
    if (debug > 1) printf("bLast %d bFirst %d inst %d\n", bLast, bFirst, bOcc);
    if (debug > 1) printf("timestamps are %s -- %s\n", aTS, bTS);
    if (debug > 1) printf("go from line %d to line %d\n", count, aLast);
    if (debug > 1) printf("number of lines: %d\n", lineCountA);
    if (useFA == 0)
    {
        aFirst = -1; 
        aLast = -2; //-2 so lineGet aFirst <= aLast fails
    }
    if (useFB == 0)
    {
        bFirst = -1;
        bLast = -2;
    }
    int lineGet = aFirst;
    if (debug > 1) printf("lineGet A: %d\n", lineGet);
    // if file A timestamps are to be used, grab lines with the current timestamp.
    if (useFA == 1)
    {
        while (lineGet <= aLast)
        {
            if (lineCountA < 0) break; // if the lineCount is less than 0, break loop
            getLineByNum(lineGet, fileA); // retrieve appropriate line
            
            if (strstr(lineOut, aTS) != NULL) //if retrieved line contains timestamp
            {
                strcpy(linesFromA[actLinesA], lineOut); // copy line from output to array
                actLinesA++;
            }
            if (actLinesA == aOcc) break; // if actual lines retrieved is equal to number of occurences, break loop.
            lineGet++;
        }
    }
    lineGet = bFirst;
    if (debug > 1) printf("lineGet B: %d\n", lineGet);
    // if file B is to be used
    if (useFB == 1)
    {
        while (lineGet <= bLast)
        {
            if (lineCountB < 0) break;
            getLineByNum(lineGet, fileB);
            
            if (strstr(lineOut, bTS) != NULL)
            {
                strcpy(linesFromB[actLinesB], lineOut);
                actLinesB++;
            }
            if (actLinesB == bOcc) break;
            lineGet++;
        }
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
    /*if (debug > 1)
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
    }*/
    int loopcount = 0;
    int allTSWritten = 0;
    while (allTSWritten < 1)
    { 
        int aPrinted = 0;
        int bPrinted = 0;
        // if count is less than the number of lines in FILE A
        if ((useFA == 1) && (count < actLinesA))
        {
            if (debug > 1) printf("TSPROCESS : WILE l2: count %d <= %d actLinesA\n", count, actLinesA);
            if (debug > 1)printf("lnUsedA[%d] == %d\n", count, lnUsedA[count]);
            // if line is unused in file A, it's getting printed.
            if (lnUsedA[count] == 0)
            {
                if (debug > 1) printf("lnUsedA[count] ( %d ) == 0\n", count);
                if (debug > 1) printf("printing line from A to file: %s\n", linesFromA[count]);
                fputs(linesFromA[count], fc);
                aPrinted = 1;
                lnUsedA[count] = 1;
                int subcount = 0;
                // go through lines from the other file
                while (subcount < actLinesB)
                {
                    if (debug > 1) printf("subcount %d < %d actLinesB\n", subcount, actLinesB);
                    // if strings match, we're going to mark it printed in file B.
                    if (debug > 1) printf("\n---COMPARING---\nA: %s B: %s---END COMPARE---\n", linesFromA[count], linesFromB[subcount]);
                    if (strcmp(linesFromA[count], linesFromB[subcount]) == 0 )
                    {
                        if (debug > 1)printf("SPROCESS: WHILE L5: strcmp A[count] B[subcount] MATCHED\n");
                        // if line is unused in file B, increment counts for file B and break loop
                        if ((useFB == 1) && (lnUsedB[subcount] == 0))
                        {   
                            lnUsedB[subcount] = 1;
                            if (debug > 1) printf("marked as used in B: %s", linesFromB[subcount]);
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
        if ((bPrinted == 0) && (useFB == 1) && (count < actLinesB))
        {
            if (debug > 1) printf("file B not yet printed, getting line");
            // if line is unused in file B, it's getting printed.
            if (lnUsedB[count] == 0)
            {
                if (debug > 1) printf("lnUsedB[count] ( %d ) == 0\n", count);
                if (debug > 1) printf("printing line from B to file: %s\n", linesFromB[count]);
                if (useFB == 1) fputs(linesFromB[count], fc);
                lnUsedB[count] = 1;
                // go through lines from the other file
                int subcount = 0;
                while (subcount < actLinesA)
                {
                    if (debug > 1) printf("subcount %d < %d actLinesA\n", subcount, actLinesA);
                    // if strings match, we're going to mark it printed in file A.
                    if (debug > 1) printf("\n---COMPARING---\nA: %s B: %s---END COMPARE---\n", linesFromA[subcount], linesFromB[count]);
                    if (strcmp(linesFromB[count], linesFromA[subcount]) == 0 )
                    {
                        if (debug > 1)printf("SPROCESS: WHILE L5: strcmp B[count] A[subcount] MATCHED\n");
                        // if line is unused in file A, increment counts for file A and break loop
                        if ((useFA == 1) && (lnUsedA[subcount] == 0))
                        {   
                            lnUsedA[subcount] = 1;
                            aPrinted = 1;
                            if (debug > 1) printf("marked as used in A: %s", linesFromA[subcount]);
                            // break (just this sub while loop) so we don't mark them all used
                            break;
                        }
                    }
                    subcount++;
                } 
            }
        }
        count++;
        // check if all timestamps are written, if unwritten found, set allTSWritten to 0.
        // if allTSWritten stays at '1', the loop will end. 
        allTSWritten = 1;
        if (useFA == 1)
        {
            for (int i = 0; i < actLinesA; i++)
            {
                if (lnUsedA[i] == 0) allTSWritten = 0;
                if (debug > 2) printf("found unused timestamp A: %d : %s\n", i, linesFromA[i]);
            }
        }
        if ((allTSWritten == 1) && (useFB == 1))
        {
            for (int i = 0; i < actLinesB; i++)
            {
                if (lnUsedB[i] == 0) allTSWritten = 0;
                if (debug > 2) printf("found unused timestamp B: %d : %s\n", i, linesFromB[i]);
            }
        }
        // if we've made our count above lineCountHigh, reset count.
        if (count > lineCountHigh) {
            count = 0;
            if (loopcount > lineCountHigh) {
                printf("ERROR: Looped through timestamps %d times and still both aren't printed.\n", lineCountHigh);
                syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: Looped through timestamps %d times and still both aren't printed.\n", lineCountHigh);
                exit(0);
            }
            loopcount++;
        } 
    }
    /* need to free allocated memory */
    // if we came with -1 for either lineCount, we need to set it to 1 as was done above.
    if (!(lcHoldA == 0)) lineCountA = lcHoldA;
    if (!(lcHoldB == 0)) lineCountB = lcHoldB;
    for (int i = 0; i < lineCountA; i++ )
    {
        free(linesFromA[i]);
    }
    free(linesFromA);
    for (int i = 0; i < lineCountB; i++ )
    {
        free(linesFromB[i]);
    }
    free(linesFromB);
    free(lnUsedA);
    free(lnUsedB);
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
        else {
            printf("lowestUnused found to be < 1: %d\n", lowestUnused);
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
    // TURN ON SETBUF FOR DEBUG ONLY
    setbuf(stdout, NULL);
    if ( debug > 0 ) {printf("argc: %d\n", argc);}
    /* argc should be 3 or 4 for correct execution, die otherwise */
    if ( argc > 2 )
    {
        fileA = malloc(strlen(argv[1]) + 1);
        strcpy(fileA, argv[1]);
        fileB = malloc(strlen(argv[2]) + 1);
        strcpy(fileB, argv[2]);
    }
    if ( argc == 3 ) /* if 3, create .repaired variable */
    {
        fileC = malloc(strlen(argv[1]) + 11);
        strcpy(fileC, fileA);
        strcat(fileC, ".repair");
        if ( debug > 0 ) printf("Repair file: %s\n", fileC);
    }
    else if ( argc == 4 ) /* if 4, take argv[3] as repaired variable */
    {
        fileC = malloc(strlen(argv[3]) + 11);
        strcpy(fileC, argv[3]);
        if ( debug > 0 ) printf("Repair file: %s\n", fileC);
    }
    else
    {
        printf ( "ERROR: Params not supplied or supplied incorrectly. \n");
        printf ( "USAGE: : %s file_a file_b <optional: repaired file out>\n", argv[0] );
        printf ( "If repair file out is not specified, will create: file_a.repaired" );
        syslog(LOG_MAKEPRI(LOG_SYSLOG, LOG_ERR), "ERROR: Incorrect parameters provided. <filea> <fileb> <opt: outputfile>\n");
        exit(0);
    }
    int count = 0;
    /* file length check and array creation  FOR FILE A */
    getFileInfo(fileA);
    if (debug > 0) printf("getSTInfo File A complete, starting var assignment\n");
    fALen = fLen;
    getTSInfo(fileA, fALen);
    int longestLineA = longestLine;
    int tsUniqueA = tsUnique;
    int  *timeStampCountsA = ( int *) malloc(tsUniqueA * sizeof( int ));
    char **timeStampsA = malloc(tsUniqueA * sizeof(char *));
    for (int i = 0; i < tsUniqueA; i++) timeStampsA[i] = malloc(longestLineA * sizeof(char));
    int *tsFirstA = ( int * ) malloc(tsUniqueA * sizeof( int ));
    int *tsLastA = ( int * ) malloc(tsUniqueA * sizeof( int ));
    // copy the arrays because c... 
    for (int i = 0; i < tsUniqueA; i++)
    {
        if (debug > 1) printf("i: %d\n");
        tsFirstA[i] = tsFirst[i];
        tsLastA[i] = tsLast[i];
        strcpy(timeStampsA[i], timeStamps[i]);
        if (debug > 1) printf("timeStampsA[%d] assigned:\n", timeStampsA[i]);
        timeStampCountsA[i] = timeStampCounts[i];
    }
    // do a bit of housekeeping for the reused vars bad form I know)
    // TODO: fix and move this in to housekeeping.
    for (int i = 0; i < tsUnique; i++) {
        strcpy(timeStamps[i], "");
        tsFirst[i] = 0;
        tsLast[i] = 0;
    }
    longestLine = 0;
    tsUnique = 0;
    /* file length check and array creation FOR FILE B */
    if (debug > 0) printf("getSTInfo and var assignment for File A complete, starting File B\n");
    getFileInfo(fileB);
    if (debug > 0) printf("getSTInfo File B complete, starting var assignment\n");
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
    if (debug > 0) printf("getSTInfo and var assignment for File B complete\n");
    // get date from first unique timestamp from file a
    //printf("tsa0 %s\n", timeStampsA[1]);
    // get higher of two unique timestamp counts
    int tsUniqueHigh = 0;
    if (tsUniqueA >= tsUniqueB) tsUniqueHigh = tsUniqueA;
    if (tsUniqueB > tsUniqueA) tsUniqueHigh = tsUniqueB;
    if (debug > 0)
    {
        printf("\n\n\nlongestLineA %d\n", longestLineA);
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
    while (count < tsUniqueA) 
    {
        tsUsedA[count] = 0;
        count++;
    }
    int *tsUsedB = ( int * ) malloc(tsUniqueB * sizeof( int ));
    count = 0;
    while (count < tsUniqueB) 
    {
        tsUsedB[count] = 0;
        count++;
    }
    if (debug > 1) printf("finished assigning values for used");
    // while in count loop, call necessary jobs to process and write timestamps.
    int allTSWritten = 0;
    char *noprint = "DONOTPRINTME";
    while (allTSWritten < 1) 
    {
	    if (debug > 1) printf("\n\n\nentering big write loop\n");
	    if (debug > 1) printf("count: %d ---- tsUniqueHigh: %d\n\n\n", count, tsUniqueHigh);
        // get lowest unused timestamps from arrays, verifying date 
        int lowUnuA = findLowestUnused(timeStampsA, tsUsedA, tsUniqueA);
        int lowUnuB = findLowestUnused(timeStampsB, tsUsedB, tsUniqueB);
        if (debug > 0)
        {
            printf("\n -- post lowUnus --\n A: %d, B: %d\n", lowUnuA, lowUnuB);
        }
        // if one is lower than the other, only the lower will get printed, so mark the higher as unused.
        // if lowUnu comes back negative, that means no unused left, so don't print it.
        // wrapped in 'for' loop for breaks, could do cases also
        for (int i = 0; i < 1; i++){
            if ((lowUnuA >= 0) && (lowUnuB >= 0))
            {
                // if A has lower timestamp, mark it used. Same for B. If timestamps match, mark both used.
                if (strcmp(timeStampsA[lowUnuA], timeStampsB[lowUnuB]) < 0) {
                    tsUsedA[lowUnuA] = 1;  
                    lowUnuB = -1;
                    if (debug > 1) printf("\n---\nA has lower timestamp than B\n");
                    if (debug > 1) printf("A : %s\n", timeStampsA[lowUnuA]);
                    break;
                }
                if (strcmp(timeStampsA[lowUnuA], timeStampsB[lowUnuB]) > 0) {
                    if (debug > 1) printf("B has lower timestamp than A\n");
                    if (debug > 1) printf("B : %s\n", timeStampsB[lowUnuB]);
                    tsUsedB[lowUnuB] = 1;  
                    lowUnuA = -1;
                    break;
                }
                if (strcmp(timeStampsA[lowUnuA], timeStampsB[lowUnuB]) == 0) 
                {
                    if (debug > 1) {
                        printf("Timestamps Equal\n");
                        printf("A : %s\n", timeStampsA[lowUnuA]);
                        printf("B : %s\n", timeStampsB[lowUnuB]);
                    }
                    tsUsedA[lowUnuA] = 1;  
                    tsUsedB[lowUnuB] = 1;  
                    break;
                }
            }
            else if (lowUnuA >= 0) {
                if (debug > 1) printf("A unused, B not unused.\n");
                if (debug > 1) printf("A : %s\n", timeStampsA[lowUnuA]);
                tsUsedA[lowUnuA] = 1; // if a is positive but b isn't, mark a as used
                lowUnuB = -1;
                break;
            }
            else if (lowUnuB >= 0) {
                tsUsedB[lowUnuB] = 1; // if a isn't positive but b is, mark b as used
                lowUnuA = -1;
                if (debug > 1) printf("B unused, A not unused.\n");
                if (debug > 1) printf("B : %s\n", timeStampsB[lowUnuB]);
                break;
            }
            else {
                if (debug > 0) printf("No unused timestamps remain.\n");
            }
        }
        if (debug > 1) printf("proceeding to call tsProcess to write.\n");    
        if ((lowUnuA >= 0) && (lowUnuB >= 0))
        {
            if (debug > 1) {
                printf("A and B to be written\n");
                printf("timeStampsA[lowUnuA]: %s\n", timeStampsA[lowUnuA]);
                printf("timeStampsB[lowUnuA]: %s\n", timeStampsB[lowUnuA]);
            }
            tsProcess(fileC, tsFirstA[lowUnuA], tsLastA[lowUnuA], timeStampCountsA[lowUnuA], longestLineA, timeStampsA[lowUnuA], tsFirstB[lowUnuB], tsLastB[lowUnuB], timeStampCountsB[lowUnuB], longestLineB, timeStampsB[lowUnuB], 1, 1);
        }
        else if (lowUnuA >= 0)
        {
            if (debug > 1)
            {   
                printf("A to be written, B NOT to be written\n");
                printf("timeStampsA[lowUnuA]: %s\n", timeStampsA[lowUnuA]);
            }
            tsProcess(fileC, tsFirstA[lowUnuA], tsLastA[lowUnuA], timeStampCountsA[lowUnuA], longestLineA, timeStampsA[lowUnuA], 1, 1, 1, 25, noprint, 1, 0 );
        }
        else if (lowUnuB >= 0)
        {
            if (debug > 1)
            {
                printf("B to be written, A NOT to be written\n");
                printf("timeStampsB[lowUnuB]: %s\n", timeStampsB[lowUnuB]);
            }
            tsProcess(fileC, 1, 1, 1, 25, noprint, tsFirstB[lowUnuB], tsLastB[lowUnuB], timeStampCountsB[lowUnuB], longestLineB, timeStampsB[lowUnuB], 0, 1);
        }
        else 
        {
            if (debug > 1) printf("timestamps are fully processed\n");
            break;
        }
        if (debug > 1) {
            printf("-\n-\n-\n");
            if ((lowUnuA >= 0) && (lowUnuB >= 0)) printf("printed timestamp A+B: %s\n", timeStampsA[lowUnuA]);

            if ((lowUnuA >= 0) && !(lowUnuB >= 0)) printf("printed timestamp A: %s\n", timeStampsA[lowUnuA]);
            if (!(lowUnuA >= 0) && (lowUnuB >= 0)) printf("printed timestamp B: %s\n", timeStampsB[lowUnuB]);
            printf("-\n-\n-\n");
        }
        // check if all timestamps are written, if unwritten found, set allTSWritten to 0.
        // if allTSWritten stays at '1', the loop will end.
        allTSWritten = 1;
        for (int i = 0; i < tsUniqueA; i++)
        {
            if (tsUsedA[i] == 0) allTSWritten = 0;
        }
        for (int i = 0; i < tsUniqueB; i++)
        {
            if (tsUsedB[i] == 0) allTSWritten = 0;
        }
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
        printf("Total file length (lines): %d\n", fALen);
        printf("Longest line length: %d\n", longestLineA);
        printf("Unique timestamps in File A: %d\n", tsUniqueA);
        printf("\n::: ON TO FILE B INFO :::\n\n");
        printf("Total file B length (lines): %d\n", fBLen);
        printf("Longest line length: %d\n", longestLineB);
        printf("Unique timestamps in File B: %d\n", tsUniqueB);
    }
            
    // free memory for A vars
    free(timeStampCountsA);
    for (int i = 0; i < tsUniqueA; i++)
    {
        free(timeStampsA[i]);
    }
    free(timeStampsA);
    free(tsFirstA);
    free(tsUsedA);
    free(tsLastA);
    // free memory for B vars
    free(timeStampCountsB);
    for (int i = 0; i < tsUniqueB; i++)
    {
        free(timeStampsB[i]);
    }
    free(timeStampsB);
    free(tsFirstB);
    free(tsUsedB);
    free(tsLastB);
    free(fileA);
    free(fileB);
    free(fileC); 
    return(0);
}

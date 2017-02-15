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

/* ints for timestamp counting */
static int tsFirst[86400]; // first instance of a timestamp
static int tsFirstA[86400]; // first instance of a timestamp file A
static int tsFirstB[86400]; // first instance of a timestamp file B
static int tsLast[86400]; // last instance of a timestamp
static int tsLastA[86400]; // last instance of a timestamp file A
static int tsLastB[86400]; // last instance of a timestamp file B
static int tsUnique = 0; // buffer for unique timestamps.
static int tsUniqueA = 0; // unique timestamps in file A
static int tsUniqueB = 0; // unique timestamps in file B

// slightly different naming for arrays
static char timeStamps[86400][1024];
static char timeStampsA[86400][1024];
static char timeStampsB[86400][1024];
static int timeStampCounts[86400]; // 0 = count, 1 = first, 2 = last
static int timeStampCountsA[86400]; // 0 = count, 1 = first, 2 = last
static int timeStampCountsB[86400]; // 0 = count, 1 = first, 2 = last

int housekeeping()
{
    // resetting some reusable vars just for good measure.
    int count = 0;
    if (fLen > fALen)
    {
        while (count > fALen)
        {
            tsFirst[count] = 0;
            tsLast[count] = 0;
            strcpy(timeStamps[count], "\0");
            timeStampCounts[count] = 0; 
            count++;
        }
    } else if (fALen > fBLen)
    {
        while (count > fALen)
        {
            tsFirst[count] = 0;
            tsLast[count] = 0;
            strcpy(timeStamps[count], "\0");
            timeStampCounts[count] = 0; 
            count++;
        }
    } else
    {
        while (count > fALen)
        {
            tsFirst[count] = 0;
            tsLast[count] = 0;
            strcpy(timeStamps[count], "\0");
            timeStampCounts[count] = 0; 
            count++;
        }
    }
    fLen = 0;
    curLineNum = 0;
    comLineNum = 0;
    strcpy(curLine, "\0");
    strcpy(comLine, "\0");
    strcpy(line, "\0");
    strcpy(lineOut, "\0");
    strcpy(curTS, "\0");
    strcpy(compTS, "\0");
    strcpy(bufTS, "\0");
    count = 0;
}
    
    
    

/* just count lines in file */
int getFileInfo(char *file)
{
    // set and reset variables
    int count = 0;
    tsUnique = 0;
    int lastUnique = 0; 
    fLen = 0;
     
    FILE *fp;
    fp = fopen(file, "r");
    if ( fp == 0 ) 
    {
        printf( "ERROR: File %s was not able to be accessed. Terminating. \n", file);
        exit(0); 
    }
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        printf("in while loop");
        // fLen is part of output and also used to count where we are in the file
        // copy line to buffered timestamp and truncate
        strcpy(bufTS, line);
        bufTS[15] = '\0';
        printf("bufts: %s", bufTS);
        // used for counter in do loop
        int count = 0;
        int matchFound = 0;
        if (tsUnique == 0)
        {
            strcpy(timeStamps[tsUnique], bufTS);
            tsFirst[tsUnique] = fLen + 1;
            tsUnique = tsUnique + 1;  
            timeStampCounts[tsUnique]++;
        }
        do {
            lastUnique = tsUnique - 1;
            if (strcmp(bufTS, (char*)&timeStamps[count]) == 0)
            {
                // if it exists, increment element at count
                timeStampCounts[count]++;
                matchFound++;
                break;
            }
            count++;
        } while (count < 86400);
        if (matchFound == 0) {
            strcpy(timeStamps[tsUnique], bufTS);
            if ( tsFirst[tsUnique] == 0)
            {
                tsFirst[tsUnique] = fLen;
            }
            tsLast[tsUnique] = fLen;
            tsUnique = tsUnique + 1;  
            timeStampCounts[tsUnique]++;
        }
        fLen++;
    }
    /* SOME GREAT DEBUG INFO HERE */
    if (debug == 1)
    {
        printf("recap so far:\n");
        int subCount;
        subCount = 0;
        while (subCount < tsUnique) 
        {
            printf("unique ts :: %s :: instances: %d\n", timeStamps[subCount], timeStampCounts[subCount]);
            printf("first instance :: %d last instance :: %d\n", tsFirst[subCount], tsLast[subCount]);
            subCount++;
        }
    }
    fclose(fp);
    return(0); 
}

/* open file, error if not,
 * go through lines
 * stop once at correct number 
 * using variable lineOut as output. */
int getLineByNum(int lineNumber, char *file) {
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
            printf("fount at count: %d\n", count);
            strcpy(lineOut, line);
        }
        count++;
    }
    fclose(fp);
    return(0);
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
    // run through lines of file until you find the right one
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        if (strcmp(line, lineIn) == 0)
        {
            fclose(fp);
            return 1;
        }
        count++;
    }
    fclose(fp);
    return 0;
}

/* count instances of each timestamp
 * variable tsArray as output */
int countTS(char *lineIn, char *file)
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
        if (strcmp(line, lineIn) == 0)
        {
            fclose(fp);
            return 1;
        }
        count++;
    }
    fclose(fp);
    return 0;
}

/* take text and file, spit out line number */

/* 
 * load timestamps
 * variables: Files a+b first instance of timestamp, number of occurrences
 */
int tsLoad(int aFirst, int aLast, int bFirst, int bLast, int aOcc, int bOcc) 
{
    /* call timestamp WRITE */

    return(0);
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
    if ( debug == 1 ) {printf("argc: %d\n", argc);}
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
        /*fA = fopen(fileA, "r");
        fB = fopen(fileB, "r");
        fC = fopen(fileC, "w+");*/
        if ( debug == 1 ) printf("Repair file: %s\n", fileC);
    }
    else if ( argc == 4 ) /* if 4, take argv[3] as repaired variable */
    {
        fileA = malloc(strlen(argv[1]) + 1);
        strcpy(fileA, argv[1]);
        fileB = malloc(strlen(argv[2]) + 1);
        strcpy(fileB, argv[2]);
        fileC = malloc(strlen(argv[3]) + 11);
        strcpy(fileC, argv[3]);
        /*fA = fopen(fileA, "r");
        fB = fopen(fileB, "r");
        fC = fopen(fileC, "w+");*/
        if ( debug == 1 ) printf("Repair file: %s\n", fileC);
    }
    else
    {
        printf ( "ERROR: Params not supplied or supplied incorrectly. \n");
        printf ( "USAGE: : %s file_a file_b <optional: repaired file out>\n", argv[0] );
        printf ( "If repair file out is not specified, will create: file_a.repaired" );
        exit(0);
    }
    /* file length check and array creation */
    getFileInfo(fileA);
    fALen = fLen;
    int count = 0;
    // copy the arrays because c... 
    tsUniqueA = tsUnique;
    while (count > fALen)
    {
        tsFirstA[count] = tsFirst[count];
        tsLastA[count] = tsLast[count];
        strcpy(timeStampsA[count], timeStamps[count]);
        timeStampCountsA[count] = timeStampCounts[count];
        count++;
    }    
    housekeeping();
    getFileInfo(fileB);
    tsUniqueB = tsUnique;
    // reset count and copy arrays for file B
    count = 0;
    fBLen = fLen;
    while (count > fBLen)
    {
        tsFirstB[count] = tsFirst[count];
        tsLastB[count] = tsLast[count];
        strcpy(timeStampsB[count], timeStamps[count]);
        timeStampCountsB[count] = timeStampCounts[count];
        count++;
    }    
    //char fileATS[fALen][3];
    //char fileBTS[fBLen][3];
    

    /* count instances of each timestamp and
     * get first and last instances line num
     * Current Line = compare TO
     * Comparison Line = compare FROM */



    /* DEBUG BLOCK
     * get and print some info about stuff to show that things work. */
    if ( debug == 1) {
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
        printf("Total file A length (lines): %d\n", fALen);
        printf("Unique timestamps in File A: %d\n", tsUniqueA);
        printf("Total file B length (lines): %d\n", fBLen);
        printf("Unique timestamps in File B: %d\n", tsUniqueB);
        if (findLineByStr(curLine, fileB) == 1)
        {
            printf("FOUND line:\n%s", curLine);
        }
        else
        {
            printf("couldn't find line:\n%s", curLine);
        }
    }
            
            
    /* call timestamp LOAD w/ filea_firstinst, fileb_firstinst, totalnum  */
    return(0);
}

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
static char bufTS[1024];

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
    strcpy(bufTS, "\0");
    count = 0;
    return;
}
    
    
    

/* just count lines in file and get longest line*/
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
    int count = 0;
    if (debug > 1) printf("tsUnique updated to: %d\n :in getTSInfo header", tsUnique);
    //if (debug > 0) sleep(1);
    int lastUnique = 0; 
    int lCount = 0;
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
        // lCount is part of output and also used to count where we are in the file
        // copy line to buffered timestamp and truncate
        strcpy(bufTS, line);
        bufTS[15] = '\0';
        //printf("bufts: %s", bufTS);
        // used for counter in do loop
        count = 0;
        int matchFound = 0;
        if (tsUnique == 0)
        {
            if (debug > 1) printf("tsUnique returned 0 in getTSInfo while loop\n");
            strcpy(timeStamps[tsUnique], (char*)&bufTS);
            tsFirst[tsUnique] = lCount + 1;
            tsUnique = tsUnique + 1;  
            if (debug > 1) printf("tsUnique incremented in if zero statement to: %d\n", tsUnique);
            timeStampCounts[tsUnique]++;
        }
        do {
            //if ( debug > 1 ) printf("started Do loop within while loop in getTSInfo\n");
            lastUnique = tsUnique - 1;
            if (strcmp(bufTS, (char*)&timeStamps[count]) == 0)
            {
                // if it exists, increment element at count
                timeStampCounts[count]++;
                matchFound++;
                break;
            }
            count++;
        } while (count < fLines);
        if (matchFound == 0) {
            strcpy(timeStamps[tsUnique], bufTS);
            if (tsFirst[tsUnique] == 0)
            {
                tsFirst[tsUnique] = lCount;
            }
            tsLast[tsUnique] = lCount;
            tsUnique++;  
            if (debug > 1) printf("tsUnique incremented in if matchfound statement to: %d\n", tsUnique);
            timeStampCounts[tsUnique]++;
        }
        lCount++;
    }
    if (debug > 1)
    {
        printf("post while loop info:\n");
        printf("tsUnique: %d\n", tsUnique);
        printf("lCount: %d\n", lCount);
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
int getLineByNum(int lineNumber, char *file) 
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
    printf("findlinebystr: lineIn: %s\n", lineIn);
    while (fgets(line, sizeof line, fp) !=NULL)
    {
        if (strstr(line, lineIn))
        {
            strcpy(lineOut, line);
            fclose(fp);
            return 1;
        }
        count++;
    }
    fclose(fp);
    return 0;
}

/* 
 * load timestamps
 * variables: Files a+b first instance of timestamp, number of occurrences
 */
int tsProcess(int aFirst, int aLast, int bFirst, int bLast, int aOcc, int bOcc, int aLLine, int bLLine) 
{
    char **timeStampsWriteA = malloc(aOcc * sizeof(char *));
    for (int i = 0; i < aOcc; i++) timeStampsWriteA[i] = malloc(aLLine * sizeof(char));
    char **timeStampsWriteB = malloc(bOcc * sizeof(char *));
    for (int i = 0; i < bOcc; i++) timeStampsWriteB[i] = malloc(bLLine * sizeof(char));

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
    //housekeeping(1);
    /* SOME GREBT DEBUG INFO HERE */
    /*if (debug > 1)
    {
        printf("recap so far FOR FILE B:\n");
        int subCount;
        subCount = 0;
        while (subCount < tsUniqueB) 
        {
            printf("unique ts :: %s :: instances: %d\n", timeStampsB[subCount], timeStampCountsB[subCount]);
            printf("first instance :: %d last instance :: %d\n", tsFirstB[subCount], tsLastB[subCount]);
            subCount++;
        }
        printf(" tsUniqueB: %d\n", tsUniqueB);
        printf(" fBLen: %d\n", fBLen);
        printf("END RECAP FOR FILE B\n");
    }*/

    /* count instances of each timestamp and
     * get first and last instances line num
     * Current Line = compare TO
     * Comparison Line = compare FROM */



    /* DEBUG BLOCK
     * get and print some info about stuff to show that things work. */
    debug = 2;
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
        int aTSLine = 4;
        printf("Total file length (lines): %d\n", fALen);
        printf("Longest line length: %d\n", longestLineA);
        printf("Unique timestamps in File A: %d\n", tsUniqueA);
        printf("File A, Timestamp %d TS: %s\n", aTSLine, timeStampsA[aTSLine]);
        getLineByNum(tsFirstA[aTSLine], fileA);
        strcpy(curLine, lineOut);
        printf("Associated Unique Timestamps: %d\n", timeStampCountsA[aTSLine]);
        printf("Associated First Instance: %d\n", tsFirstA[aTSLine]);
        printf("Associated Last Instance: %d\n", tsLastA[aTSLine]);
        printf("First Full: %s\n", curLine);
        printf("\n::: ON TO FILE B INFO :::\n\n");
        int bTSLine = 9;
        printf("Total file B length (lines): %d\n", fBLen);
        printf("Longest line length: %d\n", longestLineB);
        printf("Unique timestamps in File B: %d\n", tsUniqueB);
        printf("File B, Timestamp %d TS: %s\n", bTSLine, timeStampsB[bTSLine]);
        getLineByNum(tsFirstB[bTSLine], fileB);
        strcpy(curLine, lineOut);
        printf("Associated Unique Timestamps: %d\n", timeStampCountsB[bTSLine]);
        printf("Associated First Instance: %d\n", tsFirstB[bTSLine]);
        printf("Associated Last Instance: %d\n", tsLastB[bTSLine]);
        printf("First Full: %s\n", curLine);
        if (findLineByStr(timeStampsB[bTSLine], fileB))
        {
            printf("FOUND line %s\n", lineOut);
        }
        else
        {
            printf("couldn't find line %s\n", lineOut);
        }
    }
            
            
    /* call timestamp LOAD w/ filea_firstinst, fileb_firstinst, totalnum  */
    housekeeping(1);
    housekeeping(0);
    return(0);
}

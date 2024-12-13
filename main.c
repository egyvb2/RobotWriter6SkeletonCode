#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <windows.h>
#include "rs232.h"
#include "serial.h"

// Define struct to store font data G commands
typedef struct 
{
    int num1;
    int num2;
    int num3;
} fontData;

typedef struct
{
    float XOffset;
    float YOffset;
    int pen;
} GCodeData;


#define bdrate 115200               /* 115200 baud */


void SendCommands (char *buffer );

int readNumberOfWordsFromFile(char *textFileName);

char *readWordFromFile(char *textFileName, int nWord);

GCodeData writeLetter(fontData fontDataArray[], int targetASCII, GCodeData data, float scale);

int main()
{
    // Variables for reading font file
    FILE *file;                             // File pointer
    int maxLines = 1027;                    // Maximum number of lines set to number of lines in file
    fontData fontDataArray[maxLines];       // Struct array to store data in font file
    int i = 0;                              // Index for looping through array and file

    // Open the font file in read mode
    file = fopen("SingleStrokeFont.txt", "r");

    // Error checking for file opening
    if (file == NULL)
    {
        perror("File not found, exiting program");
        return 1;
    }

    // Read each line of the font file and store the numbers in the struct array
    while (fscanf(file, "%d %d %d", &fontDataArray[i].num1, &fontDataArray[i].num2, &fontDataArray[i].num3) == 3) 
    {
        i++;

        if (i >= maxLines)      // Break from while loop when at end of file
        {
            break;
        }
    }

    fclose(file);   //closes the file

    // Creates float for height and scale for text
    float height = 0;
    float scale = 0;

    // Asks user for height of text until value entered is between 4mm and 10mm
    while (height < 4 || height > 10)
    {
        printf("\nEnter height of text between 4mm and 10mm: ");
        scanf("%f", &height);
    }

    // Operation to create scale to apply correct height to text
    scale = height/18;

    //char mode[]= {'8','N','1',0};
    char buffer[100];

    // If we cannot open the port then give up immediately
    if ( CanRS232PortBeOpened() == -1 )
    {
        printf ("\nUnable to open the COM port (specified in serial.h) ");
        exit (0);
    }

    // Time to wake up the robot
    printf ("\nAbout to wake up the robot\n");

    // We do this by sending a new-line
    sprintf (buffer, "\n");
     // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    Sleep(100);

    // This is a special case - we wait  until we see a dollar ($)
    WaitForDollar();

    printf ("\nThe robot is now ready to draw\n");

    
    //These commands get the robot into 'ready to draw mode' and need to be sent before any writing commands

    sprintf (buffer, "G1 X0 Y0 F1000\n");
    SendCommands(buffer);
    sprintf (buffer, "M3\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);

    char textFileName;      // name of text file for text to be drawn
    int numWords = 0;       // number of words in file

    // User input file name until valid file is found
    while(numWords == 0)
    {
        printf("Enter name of text file containing text to be written by robot: ");
        scanf("%s", &textFileName);
        numWords = readNumberOfWordsFromFile(&textFileName);    // calls function returning number of words in file
    }

    int n = 0;
    GCodeData data;
    data.pen = 0;
    data.XOffset = 0;
    data.YOffset = 0;

    // Runs for each word in text file
    while(n < numWords)
    {    
        char *word = readWordFromFile(&textFileName, n);    // Reads nth word from file

        int wordLen = (int)strlen(word);    // length of word
        int characteri = 0;                 // index for character in word

        // Repeats until all letters in word have been passed into writeLetter functuin
        while (characteri < wordLen)
        {
            int ASCII = (int)word[characteri];      // find ASCII code for indexed character in word
            data = writeLetter(fontDataArray, ASCII, data, scale);      // calls function to write letter 
            characteri++;
        }
        n++;
    }

    // Before we exit the program we need to close the COM port
    CloseRS232Port();
    printf("Com port now closed\n");    

    return (0);
}

// Send the data to the robot - note in 'PC' mode you need to hit space twice
// as the dummy 'WaitForReply' has a getch() within the function.
void SendCommands (char *buffer )
{
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
    // getch(); // Omhit this once basic testing with emulator has taken place
}

// Function to read number of words from file
int readNumberOfWordsFromFile(char *textFileName)
{
    FILE *file;
    char word[100];     // array containing each letter of word
    int numWords = 0;   // number of words to be returned

    // Opens file
    file = fopen(textFileName, "r");

    // Error checking with file
    if (file == NULL)
    {
        printf("File not found or error with file\n");
        return numWords;
    }

    // Loops through file and for each word found, increment numWords by 1
    while(fscanf(file, "%99s", word) == 1)
    {
        numWords++;
    }

    fclose(file);       // closes the file

    return numWords;    // returns number of words
}

// Function to read word from file
char *readWordFromFile(char *textFileName, int nWord)
{
    FILE *file;             // array containing each letter of word
    int n = 0;              // number of words that have been read
    char word[100];         // word read from file
    char *nthWord = NULL;    // pointer to resultant word

    // Opens file
    file = fopen(textFileName, "r");

    // Loops through file until desired word is found
    while(fscanf(file, "%99s", word) == 1)
    {
        if(n == nWord)
        {
            fclose(file);
            break;
        }
        n++;
    }

    // Assign correct amount of memory and return the word
    nthWord = malloc(strlen(word) + 1);
    strcpy(nthWord, word);
    return nthWord;
}

// Function to send a single character G code commands to robot
GCodeData writeLetter(fontData fontDataArray[], int targetASCII, GCodeData data, float scale) 
{
    int commandNumber = 0;      // number of commands than need to be run
    int commandsRan = 1;        // current number of commands that have been run
    int index = 0;              // index for command in fontDataArray
    char buffer[100];           // buffer for sending commands to robot
    int n = 0;                  // index for where ASCII code was found in fontDataArray
    float X;                    // G code X position 
    float Y;                    // G code Y position

    // Looks through font data array to try find the appropriate character number
    for (int i = 0; i < 1027; i++)
    {
        if (fontDataArray[i].num1 == 999 && fontDataArray[i].num2 == targetASCII) 
        {
            commandNumber = fontDataArray[i].num3;
            n = i;
            break;
        }
    }

    // Loops for all commands ran
    while (commandsRan <= commandNumber)
    {  
        index = n + commandsRan;

        // If pen has changed, updates pen up or down depending on pen values specified for command
        if (fontDataArray[index].num3 != data.pen)
        {
            data.pen = fontDataArray[index].num3;
            if (data.pen == 1)
            {
                sprintf (buffer, "S1000\n");    // pen down
            }
            else
            {
                sprintf (buffer, "S0\n");       // pen up
            }
            SendCommands(buffer);
        }

        // Determines G code for X and Y direction by multiplying X command by scale and adding offset
        X = (float)fontDataArray[index].num1 * scale + data.XOffset;
        Y = (float)fontDataArray[index].num2 * scale + data.YOffset;

        // Sends commands
        sprintf(buffer, "G%d X%f Y%f\n", data.pen, X, Y);
        SendCommands(buffer);

        commandsRan++;
    }

    data.XOffset = X;

    return data; // Return data
}
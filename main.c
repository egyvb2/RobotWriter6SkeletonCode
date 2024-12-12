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

#define bdrate 115200               /* 115200 baud */


void SendCommands (char *buffer );

int readNumberOfWordsFromFile(char *textFileName);



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

    char textFileName;      // name of text file for text to be drawn
    int numWords = 0;       // number of words in file

    // User input file name until valid file is found
    while(numWords == 0)
    {
        printf("Enter name of text file containing text to be written by robot: ");
        scanf("%s", &textFileName);
        numWords = readNumberOfWordsFromFile(&textFileName);    // calls function returning number of words in file
    }

    //These commands get the robot into 'ready to draw mode' and need to be sent before any writing commands

    sprintf (buffer, "G1 X0 Y0 F1000\n");
    SendCommands(buffer);
    sprintf (buffer, "M3\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);


    // These are sample commands to draw out some information - these are the ones you will be generating.
    sprintf (buffer, "G0 X-13.41849 Y0.000\n");
    SendCommands(buffer);
    sprintf (buffer, "S1000\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41849 Y-4.28041\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41849 Y0.0000\n");
    SendCommands(buffer);
    sprintf (buffer, "G1 X-13.41089 Y4.28041\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);
    sprintf (buffer, "G0 X-7.17524 Y0\n");
    SendCommands(buffer);
    sprintf (buffer, "S1000\n");
    SendCommands(buffer);
    sprintf (buffer, "G0 X0 Y0\n");
    SendCommands(buffer);

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
/*
 * Copyright 2017 Dynamic Devices Ltd <info@dynamicdevices.co.uk>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * utilisation - Read in a stream of values from a text file of the format.
 *
 * Brief - "Consider an IOT device attached to machinery used to estimate its degree of usability.
 * The device measures its vibrations and stores the readings in a text file, one line for each reading. However, the device is flawed and reverses all the digits of the reading!  
 * The aim is to figure out the percentage time the machine is used, assuming the readings are taken at a fixed interval, and write it back to a text file.  Challenge 
 *
 * - Reads a file "data.txt" containing text representing floating point values which is reversed.
 * - Calculations a utilisation figure based on those input values
 * - Writes that utilisation to an output file.
 *
 * Assumptions (need clarification)
 *
 * - we've got a fairly standard full-fat C-runtime library available.
 * - input data format is one float value per line, standard line ending encoding.
 * - input data is valid as above. (no corrupt characters, no empty lines)
 * - any period present in the float is reversed along with the rest of the digits in the float.
 * - we don't want to dynamically allocate memory / we can spec. a limit on the maximum input data at any given time..
 * - not appending to output file.
 * - we're using doubles as Linux fscanf() expects doubles with %f (platform specifics).
 *   Assume supported or floats would be fine I would anticipate
 *
 * NOTE: In reality my expectation would be we'd probably be using uint8 or possibly uint16 values in the input
 *       In this case we'd most likely want to avoid floating point types where possibly as they are usually fairly
 *       heavy weight on IoT Sensor systems.
 *
 * - strrev isn't implemented out our platform
 *
 * Extras that could be added with time
 *  - command line option support, inputs, outputs, thresholds, algorithms, debugging levels
 *  - more error checking (particularly of input)
 *  - multiple test functions to determine utilisation.
 *  - basic unit testing, including fuzzing of input data.
 *  - discussion of checksums or alternative mechanisms to ensure data is more reliable
 *
 * Build Instructions:
 *
 * (Linux)
 *
 * gcc -o utilisation utilisation.c
 *
 * Date		Version		Author		Description
 * ====         =======         ======          ===========
 *
 * 19/03/18     0.1             AJL             Initial Rough Cut
 *
 */

// Includes

#include <stdio.h>
#include <string.h>

// Definitions

typedef unsigned char BOOL;

#define TRUE	1
#define FALSE 	!TRUE

#define VERSION "0.1"

#define MAX_STRING	 	64
#define MAX_INPUT_VALUES 	255

#define DFLT_DEBUG		TRUE

#define DFLT_IN_FILENAME 	"data.txt"
#define DFLT_OUT_FILENAME 	"results.txt"

// Arbitrary trigger level without knowledge of the physical system
// Would probably need to be configurable in the IoT device
#define DFLT_TRIGGER_LEVEL	10.0

// Ways to read in the input file (platform dependent)
//#define STANDARD_FLOAT_IMPLEMENTATION
#define REVERSED_FLOAT_IMPLEMENTATION

// Structs

// We're going to use this as a simple fixed size array to hold a
// a maximum number of input values read from file. We'll store
// that count in here

typedef struct STR_INPUT_DATA {
	int    iCount;
	double fInVibration[MAX_INPUT_VALUES];
} INPUT_DATA;

//  Globals
typedef struct STR_INPUT_DATA *PINPUT_DATA;

// (Keep potentially big things off the local stack just in case)
INPUT_DATA _InputData;

// Prototypes
char *strrev(char *str);

// Implementation

// Reverse a string in-situ
//
// @see: https://stackoverflow.com/questions/8534274/is-the-strrev-function-not-available-in-linux

char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

// Calculate a percentage utilisation over a range of input values
// Simplistic implemenation that iterates over each value and compares to a threshold
// Percentage utilisation is then calculated over total values where values are equal
// to or higher than threshold
double dCalculatePercentageUsage(PINPUT_DATA pInputData, double dTriggerLevel) {
  int i;
  int iTriggered = 0;
  double dPercentageUsage;

  for(i = 0; i < pInputData->iCount;i++) {
    if(pInputData->fInVibration[i] >= dTriggerLevel)
      iTriggered++;
  }
  dPercentageUsage = 100.0*(((double)iTriggered)/pInputData->iCount++);

  return dPercentageUsage;
}

// Main entry point
int main( int argc, char *argv ) {

  FILE *pInFile = NULL;
  FILE *pOutFile = NULL;
  BOOL bDebug = DFLT_DEBUG;
  BOOL bDumpValues = DFLT_DEBUG;
  int  i;
  char sInFileName[MAX_STRING];
  char sOutFileName[MAX_STRING];
  char sTmp[MAX_STRING];
  char sTmp2[MAX_STRING];
  double dTriggerLevel = DFLT_TRIGGER_LEVEL;
  double dPercentageUsage;

  sprintf(sInFileName, "%s", DFLT_IN_FILENAME);
  sprintf(sOutFileName, "%s", DFLT_OUT_FILENAME);

  // Could add in command line parsing here for
  // e.g. input / output files

  pInFile = fopen(sInFileName, "r");
  if(!pInFile) {
    fprintf(stderr, "Error: Can't open input file %s\n", sInFileName);
    return -1;
  }

  pOutFile = fopen(sOutFileName, "w");
  if(!pInFile) {
    fclose(pInFile);
    fprintf(stderr, "Error: Can't open output file %s\n", sOutFileName);
    return -1;
  }


  if(bDebug)
    printf("Size of INPUT_DATA structure is %lu\n", sizeof(_InputData));

  // Clear down data
  memset(&_InputData, 0, sizeof(_InputData));

  // Now read in the input file

  _InputData.iCount = 0;
#ifdef STANDARD_FLOAT_IMPLEMENTATION
  while( EOF != fscanf(pInFile, "%lf", &_InputData.fInVibration[_InputData.iCount++] ) )
    ;
#elif defined(REVERSED_FLOAT_IMPLEMENTATION)
  while( EOF != fscanf(pInFile, "%s", sTmp) ) {
    sscanf(strrev(sTmp), "%lf", &_InputData.fInVibration[_InputData.iCount++]);
  }
#else
#  error You must define a way of reading in the input data!
#endif

  // Debug - Let the user know how many values we think we read
  if(bDebug)
    printf("Read %d values\n", _InputData.iCount);

  // Debug - Dump out the values we think we read
  if(bDumpValues) {
    for(i = 0; i < _InputData.iCount;i++) {
      printf("%i\t%f\n", i, _InputData.fInVibration[i]);
    }
  }

  // Now work out the percentage usage
  dPercentageUsage = dCalculatePercentageUsage(&_InputData, dTriggerLevel);
  if(bDumpValues) {
    printf("\n\nPercentage Usage: %lf %%\n", dPercentageUsage);
  }

  // Write out the value
  fprintf(pOutFile, "%lf\n", dPercentageUsage);

  // All done - clean up
  fclose(pInFile);
  fclose(pOutFile);

  return 0;
}

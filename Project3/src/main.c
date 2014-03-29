/******************************************************************************
 * Rising Edge Histogram
 *
 * Description:
 *
 * This program calculates the interarrival times of each rising edge from
 * a pulse generator. It then outputs the results in a histogram.
 *
 * 
 * Author:
 *     Pratheep Joe Siluvai Iruthayaraj
 *     Chanel Matlock
 *
 *****************************************************************************/


// system includes
#include <hidef.h>      /* common defines and macros */
#include <stdio.h>      /* Standard I/O Library */
#include <stdlib.h>

// project includes
#include "types.h"
#include "derivative.h" /* derivative-specific definitions */

// Definitions

// Change this value to change the frequency of the output compare signal.
// The value is in Hz.
#define OC_FREQ_HZ    ((UINT16)10)

// Macro definitions for determining the TC1 value for the desired frequency
// in Hz (OC_FREQ_HZ). The formula is:
//
// Where:
//        Bus Clock Frequency     = 2 MHz
//        Prescaler Value         = 2 (Effectively giving us a 1 MHz timer)
//        2 --> Since we want to toggle the output at half of the period
//        Desired Frequency in Hz = The value you put in OC_FREQ_HZ
//
#define BUS_CLK_FREQ  ((UINT32) 2000000)   
#define PRESCALE      ((UINT16)  2)         

static unsigned list[1001];
static int index = 0;
static int buckets[101];
static unsigned store[1000];

// Initializes SCI0 for 8N1, 9600 baud, polled I/O
// The value for the baud selection registers is determined
// using the formula:
//
// SCI0 Baud Rate = ( 2 MHz Bus Clock ) / ( 16 * SCI0BD[12:0] )
//--------------------------------------------------------------
void InitializeSerialPort(void)
{
    // Set baud rate to ~9600 (See above formula)
    SCI0BD = 13;          
    
    // 8N1 is default, so we don't have to touch SCI0CR1.
    // Enable the transmitter and receiver.
    SCI0CR2_TE = 1;
    SCI0CR2_RE = 1;
}


// Initializes I/O and timer settings for the demo.
//--------------------------------------------------------------       
void InitializeTimer(void)
{
  // Set the timer prescaler to %2, since the bus clock is at 2 MHz,
  // and we want the timer running at 1 MHz
  TSCR2_PR0 = 1;
  TSCR2_PR1 = 0;
  TSCR2_PR2 = 0;
       
  // Enable Input Capture on Channel 1
  TIOS_IOS1 = 0;
  
  // Set up input capture to Rising Edge detection
  TCTL4_EDG1B = 0;
  TCTL4_EDG1A = 1;
  
  // Clear the Input Capture Interrupt Flag (Channel 1) 
  TFLG1 = TFLG1_C1F_MASK;
   
  //
  // Enable the timer
  // 
  TSCR1_TEN = 1;
   
  //
  // Enable interrupts via macro provided by hidef.h
  //
  EnableInterrupts;
}


// Input Capture Channel 1 Interrupt Service Routine
// Grabs the timer value and stores it in an array where 0 is 
// the first value, 1 is the second, etc. until all data points
// are collected
//          
// The first CODE_SEG pragma is needed to ensure that the ISR
// is placed in non-banked memory. The following CODE_SEG
// pragma returns to the default scheme. This is neccessary
// when non-ISR code follows. 
//
// The TRAP_PROC tells the compiler to implement an
// interrupt funcion. Alternitively, one could use
// the __interrupt keyword instead.
// 
// The following line must be added to the Project.prm
// file in order for this ISR to be placed in the correct
// location:
//		VECTOR ADDRESS 0xFFEC OC1_isr 
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------       
void interrupt 9 OC1_isr( void ) {
  unsigned temp = TCNT;
  if(index < 1001){ 
    list[index] = temp;
    index++;
  } else{
    TIE_C1I = 0;
  }
  TFLG1   =   TFLG1_C1F_MASK;  
}
#pragma pop


// This function is called by printf in order to
// output data. Our implementation will use polled
// serial I/O on SCI0 to output the character.
//
// Remember to call InitializeSerialPort() before using printf!
//
// Parameters: character to output
//--------------------------------------------------------------       
void TERMIO_PutChar(INT8 ch)
{
    // Poll for the last transmit to be complete
    do
    {
      // Nothing  
    } while (SCI0SR1_TC == 0);
    
    // write the data to the output shift register
    SCI0DRL = ch;
}


// Polls for a character on the serial port.
//
// Returns: Received character
//--------------------------------------------------------------       
UINT8 GetChar(void)
{ 
  // Poll for data
  do
  {
    // Nothing
  } while(SCI0SR1_RDRF == 0);
   
  // Fetch and return data from SCI0
  return SCI0DRL;
}

// Calculates the interval between data points in the array
// Checks if timer overflow happened and adds the offset.
void calculateBuckets(void){
  int i;
  unsigned temp; 
  for(i=0; i < 1000; i++){
    if(list[i] < list[i+1]){
      temp = list[i+1] - list[i];
      store[i]=temp;
      buckets[temp-950]++;
    } 
    else 
    {
      temp = (list[i+1] + (unsigned)(65535) ) - (list[i]);
      store[i]=temp;
      buckets[temp-950]++; 
    }
  }
}

// Minimum and maximum interval time calculation
void minmax(void) {
  
int min=0,max=0,i;

//for(i=0;i<1001;i++)
//printf("%d\t",store[i]);
//printf("\n");
for(i=0;i<1000;i++)
{
if(store[max]<store[i])
max=i;
}
for(i=0;i<1000;i++)
{
if(store[min]>store[i])
min=i;
}
printf("\r\nmax is: %d\r\n",store[max]);
printf("\r\nmin is: %d\r\n",store[min]);
  
}

//Initializes/Clears all data in the arrays used
void clearData(void){
  int i;
  index = 0;
  for(i=0; i < 101; i++){
      buckets[i] = 0;
  } 
  for(i=0; i < 1001; i++){
    list[i] = 0; 
  }
}


// Entry point of our application code
//--------------------------------------------------------------       
void main(void)
{
  int i;
  unsigned check1;
  unsigned check2;
  UINT8 userInput;
  
  InitializeSerialPort();
  InitializeTimer();
  
  // POST
  check1 = TCNT;
  for(i=0; i < 5; i++);
  check2 = TCNT;
  if(check2 - check1 == 0){
    (void)printf("POST FAILED\r\n");
  
  } else {
    (void)printf("POST PASSED\r\n");
    for(;;){
     (void)clearData();
      // Prompt User for Measurement
      (void)printf("\r\nPress any key to capture 1000 pulse measurements...\r\n");       
      userInput = GetChar();
      (void)printf("Capturing...\r\n");
            
      // Enable the input capture interrupt on Channel 1;
      TFLG1   =   TFLG1_C1F_MASK;
      TIE_C1I = 1;  
      // Collect Measurement
      while(index < 1001);
      
      // Disable the input capture interrupt on Channel 1;
      TIE_C1I = 0;  
      
      // Calculate interval times
      (void)printf("Calculating data received\r\n"); 
      (void)calculateBuckets();
      
      // Print Buckets, skipping any values that == 0    
      (void)printf("Printing Values, press a key to see next value\r\n\r\n");
      (void)printf("Value : Frequency\r\n"); 
      for(i=0; i < 101; i++){
        if(buckets[i] != 0){
          (void)printf("%d : %d\r\n", i+950, buckets[i]); 
          userInput = GetChar();
        }
      }
      minmax(); 
    }
   
  }
    
}
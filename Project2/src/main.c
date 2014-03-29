/******************************************************************************
 * Contol of 2 servo's simultaneously
 *
 * Description:
 *
 * This program controls 2 servo's independedntly through user commands and 
 * glows LED's for four different states of the servo's. 
 *
 * 
 * Author:
 *      Pratheep Joe Siluvai Iruthayaraj
 *      Amedeo Cristillo
 *
 *****************************************************************************/


// system includes
#include <hidef.h>      /* common defines and macros */
#include <stdio.h>      /* Standard I/O Library */
#include <time.h>

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
#define TC1_VAL       ((UINT16)  (((BUS_CLK_FREQ / PRESCALE) / 2) / OC_FREQ_HZ))

//#define NTIMES 1250 


// Initializes SCI0 for 8N1, 9600 baud, polled I/O
// The value for the baud selection registers is determined
// using the formula:
//
// SCI0 Baud Rate = ( 2 MHz Bus Clock ) / ( 16 * SCI0BD[12:0] )
//--------------------------------------------------------------

#define MOV(x) (0x20|x)

#define pos(x) (MOV(x)&31)        // calculates the position of the motor
#define LOOP(x) (0x80|x)
#define times(x) (LOOP(x)&31)     // number of times to loop
#define WAIT(x) (0x40|x)
#define wait_time(x) (WAIT(x) & 31)  // number of times to wait  
#define END_LOOP (160)
#define DUM (192)                     // Error OpCode1
#define DUM1 (224)
#define JMP(x) (0x60|x)               // Error Opcode2
#define jmp_value(x) (JMP(x) & 31)    // number of steps to skip or jump
#define RECIPE_END (0)

//ASCII Values
#define asciip 112
#define asciiP 80 
#define asciic 99 
#define asciiC 67
#define asciir 114
#define asciiR 82
#define asciil 108
#define asciiL 76
#define asciin 110
#define asciiN 78
#define asciib 98
#define asciiB 66
#define asciix 120
#define asciiX 88

char j,k;
int m,i,error2,error1,l_flag2,l_flag1;

int temp1, temp2, track1, track2, pause_flag, pause_flag2;

UINT8 Buffer1[100];
UINT8 Buffer2[100];

UINT8 *Buffer_inst1 = &Buffer1;
UINT8 *Buffer_inst2 = &Buffer2;

UINT8 *loop_position1;
UINT8 *loop_position2;

int wait_flag1=-1;
int wait_flag2=-1;


UINT8 *mov_position1;
UINT8 *mov_position2;

int counter1;
int counter2;

int wait_counter1;
int wait_counter2;

int cur_move1=0,cur_move2=0,pre_move1=5,pre_move2=5; 

int right1 = 0;
int right2 = 0;

int left1 = 0;
int left2 = 0;




void InitializeLED() {


  DDRA = 0xFF;


}

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
    
  // Enable output compare on Channel 1
  TIOS_IOS1 = 1;
  
  // Set up output compare action to toggle Port T, bit 1
  TCTL2_OM1 = 0;
  TCTL2_OL1 = 1;
  
  // Set up timer compare value
  TC1 = TC1_VAL;
  
  // Clear the Output Compare Interrupt Flag (Channel 1) 
  TFLG1 = TFLG1_C1F_MASK;
  
  // Enable the output compare interrupt on Channel 1;
  TIE_C1I = 1;  
  
  //
  // Enable the timer
  // 
  TSCR1_TEN = 1;
   
  //
  // Enable interrupts via macro provided by hidef.h
  //
  EnableInterrupts;
 
}

 
// Output Compare Channel 1 Interrupt Service Routine
// Refreshes TC1 and clears the interrupt flag.
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


// Entry point of our application code
//--------------------------------------------------------------       

void InitializePWM0() {

 PWMCLK_PCLK0=1;
 PWMPOL_PPOL0=1;
 PWMSCLA=0x50;
 PWMPER0=0xFF; 
 PWME_PWME0=1;
}

void InitializePWM2() {
 
 PWMCLK_PCLK2=1;
 PWMPOL_PPOL2=1;   
 PWMSCLB=0x50;
 PWMPER2=0xFF;
 PWME_PWME2=1;
}

// Do nothing - just loop 

void NOP1(){
           if(wait_flag1<-5000) wait_flag1 = -1;
         
  //stall for 1 ms.  The bs frequency is 2MHz, so 50000 cycles corresponds to a time of 1 ms
}

// Do nothing - just loop
void NOP2(){
           if(wait_flag2<-5000) wait_flag2 = -1;
         
  //stall for 1 ms.  The bs frequency is 2MHz, so 50000 cycles corresponds to a time of 1 ms
}



void snippet(){

// motor 1 
 
// robust pair
*Buffer_inst1 = MOV(0);
Buffer_inst1++;
*Buffer_inst1 = MOV(1);
Buffer_inst1++;
*Buffer_inst1 = MOV(2);
Buffer_inst1++;
*Buffer_inst1 = MOV(3);
Buffer_inst1++;
*Buffer_inst1 = MOV(4);
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
 
//s1
Buffer_inst1++;
*Buffer_inst1 = MOV(0);
Buffer_inst1++;
*Buffer_inst1 = WAIT(5);
Buffer_inst1++;
*Buffer_inst1 = MOV(0);
Buffer_inst1++;
//s2
*Buffer_inst1 = MOV(3);
Buffer_inst1++; 
*Buffer_inst1 = LOOP(0);
Buffer_inst1++;
*Buffer_inst1 = MOV(1);
Buffer_inst1++;
*Buffer_inst1 = MOV(4);
Buffer_inst1++;
*Buffer_inst1 = END_LOOP;
Buffer_inst1++;
*Buffer_inst1 = MOV(0);
//s3
Buffer_inst1++;
*Buffer_inst1 = MOV(2);
Buffer_inst1++; 
*Buffer_inst1 = WAIT(0);
Buffer_inst1++;
*Buffer_inst1 = MOV(3);
//s4
Buffer_inst1++;
*Buffer_inst1 = MOV(2);
Buffer_inst1++; 
*Buffer_inst1 = MOV(3);
Buffer_inst1++;
*Buffer_inst1 = WAIT(31);
Buffer_inst1++;
*Buffer_inst1 = WAIT(31);
Buffer_inst1++;
*Buffer_inst1 = WAIT(31);
Buffer_inst1++;
*Buffer_inst1 = MOV(4);
//s5
Buffer_inst1++;
*Buffer_inst1 = MOV(0);
Buffer_inst1++;
*Buffer_inst1 = MOV(1);
Buffer_inst1++;
*Buffer_inst1 = MOV(2);
Buffer_inst1++;
*Buffer_inst1 = MOV(3);
Buffer_inst1++;
*Buffer_inst1 = MOV(4);
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
// nested loop test
Buffer_inst1++;
*Buffer_inst1 = MOV(2);
Buffer_inst1++; 
*Buffer_inst1 = LOOP(0);
Buffer_inst1++;
*Buffer_inst1 = MOV(1);
Buffer_inst1++;
*Buffer_inst1 = LOOP(1);           // nested loop
Buffer_inst1++;
*Buffer_inst1 = END_LOOP;
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
Buffer_inst1++;
//jump opcode
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
Buffer_inst1++;
*Buffer_inst1 = JMP(3);            // jump
Buffer_inst1++;
*Buffer_inst1 = WAIT(31);
Buffer_inst1++;
*Buffer_inst1 = WAIT(31);
Buffer_inst1++;
*Buffer_inst1 = WAIT(31);
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
//s7
Buffer_inst1++;
*Buffer_inst1 = MOV(0);
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
Buffer_inst1++;
*Buffer_inst1 = RECIPE_END;
Buffer_inst1++;
*Buffer_inst1 = MOV(0);
Buffer_inst1++;  
*Buffer_inst1 = DUM1;
Buffer_inst1++;
*Buffer_inst1 = MOV(5);
Buffer_inst1++;
*Buffer_inst1 = RECIPE_END;


*Buffer_inst1 = RECIPE_END;
// -----------------


// motor 2 

//robust pair
*Buffer_inst2 = MOV(5);
Buffer_inst2++;
*Buffer_inst2 = MOV(4);
Buffer_inst2++;
*Buffer_inst2 = MOV(3);
Buffer_inst2++;
*Buffer_inst2 = MOV(2);
Buffer_inst2++;
*Buffer_inst2 = MOV(1);
Buffer_inst2++;
//s1
*Buffer_inst2 = MOV(0);
Buffer_inst2++;
*Buffer_inst2 = WAIT(5);
Buffer_inst2++;
*Buffer_inst2 = MOV(0);
Buffer_inst2++;
//s2
*Buffer_inst2 = MOV(3);
Buffer_inst2++; 
*Buffer_inst2 = LOOP(0);
Buffer_inst2++;
*Buffer_inst2 = MOV(1);
Buffer_inst2++;
*Buffer_inst2 = MOV(4);
Buffer_inst2++;
*Buffer_inst2 = END_LOOP;
Buffer_inst2++;
*Buffer_inst2 = MOV(0);
//s3
Buffer_inst2++;
*Buffer_inst2 = MOV(2);
Buffer_inst2++; 
*Buffer_inst2 = WAIT(0);
Buffer_inst2++;
*Buffer_inst2 = MOV(3);
//s4
Buffer_inst2++;
*Buffer_inst2 = MOV(2);
Buffer_inst2++; 
*Buffer_inst2 = MOV(3);
Buffer_inst2++;
*Buffer_inst2 = WAIT(31);
Buffer_inst2++;
*Buffer_inst2 = WAIT(31);
Buffer_inst2++;
*Buffer_inst2 = WAIT(31);
Buffer_inst2++;
*Buffer_inst2 = MOV(4);
//s5
Buffer_inst2++;
*Buffer_inst2 = MOV(0);
Buffer_inst2++;
*Buffer_inst2 = MOV(1);
Buffer_inst2++;
*Buffer_inst2 = MOV(2);
Buffer_inst2++;
*Buffer_inst2 = MOV(3);
Buffer_inst2++;
*Buffer_inst2 = MOV(4);
Buffer_inst2++;
*Buffer_inst2 = MOV(5);
//illegal opcode
Buffer_inst2++;
*Buffer_inst2 = MOV(0);
Buffer_inst2++;
*Buffer_inst2 = MOV(5);
Buffer_inst2++;
*Buffer_inst2 = RECIPE_END;
Buffer_inst2++;
*Buffer_inst2 = MOV(0);
Buffer_inst2++;  
*Buffer_inst2 = DUM1;      // dummy opcode
Buffer_inst2++;
*Buffer_inst2 = MOV(5);
Buffer_inst2++;
*Buffer_inst2 = RECIPE_END;    // 
//Jump opcode
Buffer_inst2++;
*Buffer_inst2 = MOV(1);
Buffer_inst2++;
*Buffer_inst2 = JMP(3);
Buffer_inst2++;
*Buffer_inst2 = WAIT(31);
Buffer_inst2++;
*Buffer_inst2 = WAIT(31);
Buffer_inst2++;
*Buffer_inst2 = WAIT(31);
Buffer_inst2++;
*Buffer_inst2 = MOV(5);
Buffer_inst2++;
*Buffer_inst2 = RECIPE_END;
// nested loop test
Buffer_inst2++;
*Buffer_inst2 = MOV(2);
Buffer_inst2++; 
*Buffer_inst2 = LOOP(0);
Buffer_inst2++;
*Buffer_inst2 = MOV(1);
Buffer_inst2++;
*Buffer_inst2 = LOOP(1);
Buffer_inst2++;
*Buffer_inst2 = END_LOOP;
Buffer_inst2++;
*Buffer_inst2 = MOV(5);
Buffer_inst2++;
*Buffer_inst2 = RECIPE_END;


 Buffer_inst1=&Buffer1;
 Buffer_inst2=&Buffer2;

}  

UINT8 command(char* in, char* in2){

char c = *in; 
char c2 = *in2;
UINT8 output = 0x00;

(void)printf("\n\r");

  switch (c) {
  
    case asciic: 
    if(error1!=1){
       wait_flag1 = 0;
       pause_flag = 0;
       PORTA = PORTA & 0xF0;
    }
       break;
    case asciiC:
    if(error1!=1){
       wait_flag1 = 0;
       pause_flag = 0; 
       PORTA = PORTA & 0xF0;
    }
       break;
       
       
    case asciip: 
       wait_flag1 = -1;
       pause_flag = 1;
       PORTA = PORTA | 0x01;
       break;
    case asciiP: 
       wait_flag1 = -1;
       pause_flag = 1;
       PORTA = PORTA | 0x01;
       break;
       
       
    case asciir: 
    if(pause_flag == 1){
       wait_flag1=0;
        if(pre_move1==track1){
          temp1--;
        }else{
          
          
       temp1=pre_move1-1; 
        }
     if(temp1<0){
      temp1 = 0;
     }
        switch (temp1){
    case 0:
      PWMDTY0=0x01;
      break;
    case 1:
      PWMDTY0=0x0A;
      break;
    case 2:
      PWMDTY0=0x0F;
      break;
    case 3:
      PWMDTY0=0x14;
      break;
    case 4:
      PWMDTY0=0x18;
      break;
    case 5:
      PWMDTY0=0x20;
      break;
      
   
   } 
   wait_flag1=-1;
   
   track1=pre_move1;
          }
       break;
    case asciiR: 
    if(pause_flag == 1){
       wait_flag1=0;
        if(pre_move1==track1){
          temp1--;
        }else{
          
          
       temp1=pre_move1-1; 
        }
       if(temp1<0){
      temp1 = 0;
     }
        switch (temp1){
    case 0:
      PWMDTY0=0x01;
      break;
    case 1:
      PWMDTY0=0x0A;
      break;
    case 2:
      PWMDTY0=0x0F;
      break;
    case 3:
      PWMDTY0=0x14;
      break;
    case 4:
      PWMDTY0=0x18;
      break;
    case 5:
      PWMDTY0=0x20;
      break;
      
   
   } 
   wait_flag1=-1;
   
   track1=pre_move1;
          }
       break;
       
       
    case asciil: 
    if(pause_flag == 1){
       wait_flag1=0;
        if(pre_move1==track1){
          temp1++;
        }else{
          
          
       temp1=pre_move1+1; 
        }
      if(temp1>5){
      temp1 = 5;
     }
        switch (temp1){
    case 0:
      PWMDTY0=0x01;
      break;
    case 1:
      PWMDTY0=0x0A;
      break;
    case 2:
      PWMDTY0=0x0F;
      break;
    case 3:
      PWMDTY0=0x14;
      break;
    case 4:
      PWMDTY0=0x18;
      break;
    case 5:
      PWMDTY0=0x20;
      break;
      
   
   } 
   wait_flag1=-1;
   
   track1=pre_move1;
          } 
       break;
    case asciiL: 
    if(pause_flag == 1){
       wait_flag1=0;
        if(pre_move1==track1){
          temp1++;
        }else{
          
          
       temp1=pre_move1+1; 
        }
     if(temp1>5){
      temp1 = 5;
     } 
        switch (temp1){
    case 0:
      PWMDTY0=0x01;
      break;
    case 1:
      PWMDTY0=0x0A;
      break;
    case 2:
      PWMDTY0=0x0F;
      break;
    case 3:
      PWMDTY0=0x14;
      break;
    case 4:
      PWMDTY0=0x18;
      break;
    case 5:
      PWMDTY0=0x20;
      break;
      
   
   } 
   wait_flag1=-1;
   
   track1=pre_move1;
          } 
       break;


    case asciin:
       wait_flag1 = 1; 
       break;
    case asciiN: 
       wait_flag1 = 1; 
       break;

    case asciib:
       
       Buffer_inst1=&Buffer1;
       wait_flag1=0;
       l_flag1=0;
       error1 = 0;
       PORTA = PORTA & 0xF0;
       break;
    case asciiB:
       
       Buffer_inst1=&Buffer1;
       wait_flag1=0;
       l_flag1=0;
       error1 = 0;
       PORTA = PORTA & 0xF0;
       break;
       
  }

  switch (c2) {
  
    case asciic:
    if(error2!=1){
       wait_flag2 = 0;
       pause_flag2 = 0;
       PORTA = PORTA & 0x0F;
    }
       break;
    case asciiC:
    if(error2!=1){ 
       wait_flag2 = 0;
       pause_flag2 = 0;
       PORTA = PORTA & 0x0F;
    }
       break;
       
       
    case asciip:
       wait_flag2 = -1;
       pause_flag2 = 1;
       PORTA = PORTA | 0x10;
       break;
    case asciiP: 
       wait_flag2 = -1;
       pause_flag2 = 1;
       PORTA = PORTA | 0x10;
       break;
       
       
    case asciir:
    if(pause_flag2 == 1){
       wait_flag2=0;
        if(pre_move2==track2){
          temp2--;
        }else{
          
          
       temp2=pre_move2-1; 
        }
      if(temp2<0){
      temp2 = 0;
     }
        switch (temp2){
    case 0:
      PWMDTY2=0x01;
      break;
    case 1:
      PWMDTY2=0x0A;
      break;
    case 2:
      PWMDTY2=0x0F;
      break;
    case 3:
      PWMDTY2=0x14;
      break;
    case 4:
      PWMDTY2=0x18;
      break;
    case 5:
      PWMDTY2=0x20;
      break;
      
   
   } 
   wait_flag2=-1;
   
   track2=pre_move2;
          }
       break;
    case asciiR: 
    if(pause_flag2 == 1){
       wait_flag2=0;
        if(pre_move2==track2){
          temp2--;
        }else{
          
          
       temp2=pre_move2-1; 
        }
      if(temp2<0){
      temp2 = 0;
     }
        switch (temp2){
    case 0:
      PWMDTY2=0x01;
      break;
    case 1:
      PWMDTY2=0x0A;
      break;
    case 2:
      PWMDTY2=0x0F;
      break;
    case 3:
      PWMDTY2=0x14;
      break;
    case 4:
      PWMDTY2=0x18;
      break;
    case 5:
      PWMDTY2=0x20;
      break;
      
   
   } 
   wait_flag2=-1;
   
   track2=pre_move2;
          }
       break;
       
       
    case asciil:
    if(pause_flag2 == 1){
       wait_flag2=0;
        if(pre_move2==track2){
          temp2++;
        }else{
          
          
       temp2=pre_move2+1; 
        }
     if(temp2>5){
      temp2 = 5;
     } 
        switch (temp2){
    case 0:
      PWMDTY2=0x01;
      break;
    case 1:
      PWMDTY2=0x0A;
      break;
    case 2:
      PWMDTY2=0x0F;
      break;
    case 3:
      PWMDTY2=0x14;
      break;
    case 4:
      PWMDTY2=0x18;
      break;
    case 5:
      PWMDTY2=0x20;
      break;
      
   
   } 
   wait_flag2=-1;
   
   track2=pre_move2;
          } 
       break;
    case asciiL: 
    if(pause_flag2 == 1){
       wait_flag2=0;
        if(pre_move2==track2){
          temp2++;
        }else{
          
          
       temp2=pre_move2+1; 
        }
       if(temp2>5){
      temp2 = 5;
      }       
        switch (temp2){
    case 0:
      PWMDTY2=0x01;
      break;
    case 1:
      PWMDTY2=0x0A;
      break;
    case 2:
      PWMDTY2=0x0F;
      break;
    case 3:
      PWMDTY2=0x14;
      break;
    case 4:
      PWMDTY2=0x18;
      break;
    case 5:
      PWMDTY2=0x20;
      break;
      } 
   wait_flag2=-1;
   
   track2=pre_move2;
          } 
       break;


    case asciin: 
       wait_flag2 = 1; 
       break;
    case asciiN: 
       wait_flag2 = 1; 
       break;


    case asciib: 
         
       Buffer_inst2=&Buffer2;
      wait_flag2=0;
      l_flag2=0;
      error2 = 0;       
      PORTA = PORTA & 0x0F;
       break;
    case asciiB: 
        
       Buffer_inst2=&Buffer2; 
       wait_flag2=0;
       l_flag2=0;
       error2 = 0;      
       PORTA = PORTA & 0x0F;
       break;
       
  }

return output;
}


void run1(int z){

   InitializePWM0();
   switch (z){
    case 32:
      PWMDTY0=0x01;
      break;
    case 33:
      PWMDTY0=0x0A;
      break;
    case 34:
      PWMDTY0=0x0F;
      break;
    case 35:
      PWMDTY0=0x14;
      break;
    case 36:
      PWMDTY0=0x18;
      break;
    case 37:
      PWMDTY0=0x20;
      break;
   }
}


void run2(int z){

  InitializePWM2();
   switch (z){
    case 32:
      PWMDTY2=0x01;
      //delay(25);
      break;
    case 33:
      PWMDTY2=0x0A;
      break;
    case 34:
      PWMDTY2=0x0F;
      break;
    case 35:
      PWMDTY2=0x14;
      break;
    case 36:
      PWMDTY2=0x18;
      break;
    case 37:
      PWMDTY2=0x20;
      //delay(25);
      break;
   }
   
}



void Process_Buffer2()
{

    int i = 0;
    if (wait_flag2 == 0) {

    
	// check for end of snippet


	if (*Buffer_inst2 != 0) {


	    //Check to see if current mnemonic in buffer is a LOOP  
	    if ((*Buffer_inst2 >= 128) && (*Buffer_inst2 < 160)) {
	
		
		  if (l_flag2==0){

		loop_position2 = Buffer_inst2;
		loop_position2++;

		counter2 = times(*Buffer_inst2);

     

		Buffer_inst2++;
		

		  }

		 else{
   (void) printf("\r\nNESTED LOOP ERROR\r\n");
      PORTA = PORTA | 0x40;         

		  error2 = 1;
		  wait_flag2 = -1;
		  pause_flag2 = 1;
		 
		 }
		 l_flag2 = 1;
	    }
//Check to see if current mnemonic in buffer is a MOV 
	    else if ((*Buffer_inst2 > 31 && *Buffer_inst2 < 38)) {

		    cur_move2 = pos(*Buffer_inst2);

        // 
        
		    wait_flag2 = (cur_move2 - pre_move2) * 4;
	

		    if (wait_flag2 < 0) {
			wait_flag2 = -wait_flag2;
			
		    }

		    run2(*Buffer_inst2);

       
		    pre_move2 = pos(*Buffer_inst2);
		    Buffer_inst2++;
	    }

	else if (*Buffer_inst2 >= 38 && *Buffer_inst2 <= 63) {
	    
	 (void) printf("\r\nOut of range value : RECIPE COMMAND ERROR\r\n");    
	    // Out of range error state
	    error2 = 1;
	    wait_flag2 = -1;
	    pause_flag2 = 1;
	    
	   	}
//Check to see if current mnemonic in buffer is a WAIT 
		else if (*Buffer_inst2 >= 64 && *Buffer_inst2 <= 95) {
		
     
		wait_counter2 = wait_time(*Buffer_inst2);
		wait_flag2 = wait_counter2++;	//increment the wait time because WAIT(0) should wait 1 ms on its own

		while (wait_counter2 != 0) {
		    
		    NOP2();
		    wait_counter2--;
		}
			Buffer_inst2++;
		}
		
	    else if (*Buffer_inst2 == 192 || *Buffer_inst2 == 224 ){
	    
	   (void) printf("\r\nInvalid opcode : RECIPE COMMAND ERROR\r\n"); 

	    PORTA = PORTA | 0x80;
      error2 = 1;
	    wait_flag2 = -1;
	    pause_flag2 = 1;
	    
	    }
	    else if (*Buffer_inst2 >= 96 && *Buffer_inst2 <= 101){
	  
	   Buffer_inst2 +=jmp_value(*Buffer_inst2);
	  
	    }
	    else if (*Buffer_inst2 == 160) {
//If current mnemonic in buffer is none of the above, END LOOP.
	    
		
		if (counter2 != 0) {

		  	  
		    Buffer_inst2 = loop_position2;

		    counter2--;
		    
	        l_flag2=0;

		}
		else {

		 
		    Buffer_inst2++;

		}
	    }

	    if (*Buffer_inst2 == 0) {

  (void) printf("\r\nEND OF RECIPE2\r\n");
    
      wait_flag2 = -1;
       pause_flag2 = 1 ;     
     PORTA = PORTA | 0x20;
		Buffer_inst2++;

	    }



	}
    }
   else {
   
	NOP2();
	wait_flag2--;
    }

}
 



void Process_Buffer1()
{

    int i = 0;
    if (wait_flag1 == 0) {


	// check for end of snippet

  
	if (*Buffer_inst1 != 0) {


	    //Check to see if current mnemonic in buffer is a LOOP  
	    if ((*Buffer_inst1 >= 128) && (*Buffer_inst1 < 160)) {
		
		
		  if (l_flag1==0){

		loop_position1 = Buffer_inst1;
		loop_position1++;

		counter1 = times(*Buffer_inst1);

    

		Buffer_inst1++;

		  }

		 else{
  
   (void) printf("\r\nNESTED LOOP ERROR\r\n");             
	
		PORTA = PORTA | 0x04;
		  error1 = 1;
		  wait_flag1 = -1;
		  pause_flag = 1;
		 }
		 l_flag1 = 1;
	    }
//Check to see if current mnemonic in buffer is a MOV 
	    else if ((*Buffer_inst1 > 31 && *Buffer_inst1 < 38)) {

		    cur_move1 = pos(*Buffer_inst1);

		    wait_flag1 = (cur_move1 - pre_move1) * 4;
	

		    if (wait_flag1 < 0) {
			wait_flag1 = -wait_flag1;
			
		    }

		    run1(*Buffer_inst1);

       
		    pre_move1 = pos(*Buffer_inst1);
		    Buffer_inst1++;
	    }

	else if (*Buffer_inst1 >= 38 && *Buffer_inst1 <= 63) {
	    
	   (void) printf("\r\nOut of range value : RECIPE COMMAND ERROR\r\n"); 
	   
	     error1 = 1;
	    wait_flag1 = -1;
	    pause_flag = 1;
	    
	}
//Check to see if current mnemonic in buffer is a WAIT 
		else if (*Buffer_inst1 >= 64 && *Buffer_inst1 <= 95) {
		
    wait_counter1 = wait_time(*Buffer_inst1);
		wait_flag1 = wait_counter1++;	//increment the wait time because WAIT(0) should wait 1 ms on its own

		while (wait_counter1 != 0) {
		    // (void)printf("wait_counter1=%drn",wait_counter1); 
		    NOP1();
		    wait_counter1--;
		}
			Buffer_inst1++;
		}
		
	    else if (*Buffer_inst1 == 192 || *Buffer_inst1 == 224){
	    
	    (void) printf("\r\nInvalid opcode : RECIPE COMMAND ERROR\r\n");
	    
	    
	         PORTA = PORTA | 0x08;
            error1 = 1;
	    wait_flag1 = -1;
	    pause_flag = 1;
	    
	    }
	    else if (*Buffer_inst1 >= 96 && *Buffer_inst1 <= 101){
	  
	   Buffer_inst1 +=jmp_value(*Buffer_inst1);
	  
	    }
	    else if (*Buffer_inst1 == 160) {
//If current mnemonic in buffer is none of the above, END LOOP.
	     
		
		if (counter1 != 0) {

		  	 
		    Buffer_inst1 = loop_position1;

		    counter1--;
	
         l_flag1=0; 
		}
		else {

		 
		    Buffer_inst1++;

		}
	    }

	    if (*Buffer_inst1 == 0) {

  (void) printf("\r\nEND OF RECIPE1\r\n");
   PORTA = PORTA | 0x02;
		Buffer_inst1++;

	    }



	}
    }
   else {
        
	NOP1();
	wait_flag1--;
    }

}
 
 
   
     
#pragma push
#pragma CODE_SEG __SHORT_SEG NON_BANKED
//--------------------------------------------------------------       
void interrupt 9 OC1_isr( void )
 { 
 
 Process_Buffer1();
 Process_Buffer2();
 
 TC1     +=  TC1_VAL;      
 TFLG1   =   TFLG1_C1F_MASK;
   
#pragma pop

 
 }
  
 
void main(void)
{

  UINT8 userInput, userInput2, CR;
  char c, c2, cr;
  unsigned check1;
  unsigned check2;
  

  InitializeSerialPort();
  
  InitializeLED();
  
  
   snippet();
   
   
  InitializeTimer();
   
 
  // POST
  check1 = TCNT;
  for(i=0; i < 5; i++);
  check2 = TCNT;
  if(check2 - check1 == 0){
    (void)printf("POST FAILED\r\n");
  
  } else {
    (void)printf("POST PASSED\r\n");
  }

 while(1){
  
  //Get input for Servos 1 & 2
  (void)printf("Command> "); 
  userInput = GetChar();
  c = userInput;
  (void)printf("%c", c); 
 
  userInput2 = GetChar();
  c2 = userInput2;
  (void)printf("%c", c2); 
  
  CR= GetChar();
  cr = CR;
  (void)printf("%c", cr);
  

  //Got input
  //Check 3rd character was a Carriage Return
  if(CR==13){
  
  //Check for cancel character
    if((asciix==c)||(asciiX==c)||(asciix==c2)||(asciiX==c2)){
      (void)printf("\n\r***Cancel Character detected.  Current command nullified.\n\r");
    }else{
    
    //If 3rd  char==CR & no cancel character detected, run command  
    command(& c, & c2);
    }
  }else{
  (void)printf("\n\r***ERROR: 3rd character was not a Carriage Return\n\r");
  }
 }
  


  
 
}
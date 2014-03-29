#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>
#include <math.h>


/* The Neutrino IO port used here corresponds to a single register, which is
 * one byte long */
#define PORT_LENGTH 1

/*Digital input output port configuration,*/
#define DIO_BASE_ADDR 0x280                //Base Address for Ports
#define DIO_PORTC_ADDR 0x0A                                   //Base Address for Port_B
#define DIO_PORTB_ADDR 0x09                                   //Base Address for Port_B

#define DIO_CTL_ADDR 0x0B
#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

 /* bit 2 = printer initialisation (high to initialise)
  * bit 4 = hardware IRQ (high to enable) */
#define ENABLE_IN 0xFF
#define ENABLE_OUT 0x00

#define LOW 0x00                                          //Values to give pulse.
#define HIGH 0x01


int user_input = 0;
int start_time = 0;
int end_time = 0;
int diff =0;
int first_reading = 0;

/* Handlers for port-B and port-C*/
uintptr_t ctrl_handle_portB;
uintptr_t ctrl_handle_portC;

/*Structures for timers used.*/
struct timespec my_timer_value1;
struct timespec my_timer_value2;

/*Threads functions.*/
void *generate_pulse_servoA( void *ptr );
void *generate_pulse_servoB( void *ptr );

void *out_pulse( void *ptr );
int step_size_servoA = 20;
int step_size_servoB = 20;

typedef union {
        struct _pulse   pulse;
} my_message_t;  //This union is for timer module.

// Define some booleans for code readability.
#define TRUE 1
#define FALSE 0

// These are used to hold the user input.
char servo1UserInput = 'c';
char servo2UserInput = 'c';

// These are used to extract the command and
// any parameters attached to those commands.
#define firstThree(x) ((x>>5)<<5)
#define lastFive(y) (y&31)

const unsigned int servoPositionTicks[6] = {1, 7, 10, 13, 16, 20};

// buffers to hold the recipies for each servo.
unsigned int bufferServoA[100] = {0};  // Commands buffer for ServoA
unsigned int bufferServoB[100] = {0};  // Commands buffer for ServoB

// Possible Task Statuses.
enum TASKSTATUS
{
  ready = 0,
  running,
  error,
  paused,
  donothing,
};

// the order of these commands match the op codes
// listed in the assignment.
enum COMMANDS
{
  RECIPE_END = 0,
  MOV = 32,
  WAIT = 64,
  TBD1,
  LOOP_START = 128,
  END_LOOP = 160,
  BREAK_LOOP = 96,
  TBD3
};

// Holds the information for each task.
struct TaskControlBlock
{
   enum TASKSTATUS status;
   unsigned int * currentCommand;      // points to the current command in a bufferServo

   // Loop bookkeeping stuff.
   unsigned int loopFlag;              // True if we're in a loop, otherwise false.
   unsigned int loopCounter;           // Loop will run n+1 times.
   unsigned int* firstLoopInstruction; // Pointer to the instructon after the LOOP_START command.

   // MOV bookkeeping stuff
   unsigned int currentServoPosition;  // 0-5
   unsigned int expectedServoPosition; // 0-5

   // MOV and WAIT bookkeeping stuff
   unsigned int timeLeftms;            // timeleft to execute the current
                                // command.
};

// Look Ma TCBS!!!
struct TaskControlBlock servoA;
struct TaskControlBlock servoB;

// Function definitions
unsigned int GetChar(void);
void getUserInput(void);
void initializeServos(void);
void initializeCommands(void);
void processCommand(struct TaskControlBlock* servo, enum COMMANDS command, int commandContext);
void processUserCommand(void);
void runTasks(void);
void updateTaskStatus(struct TaskControlBlock* servo);

// Flags to show the reciepe end.
unsigned int reciepeEndServoA =0;
unsigned int reciepeEndServoB =0;


//*****************************************************************************
// This unmitigated piece of crap holds the recipes to be run by each servo.
//
// Parameters: NONE
//
// Return: None
//*****************************************************************************
void initializeCommands(void)
{

    enum COMMANDS myCommand, myCommand2, myCommand3, myCommand4, myCommand5;

    myCommand = MOV;
    myCommand2 = WAIT;
    myCommand3 = LOOP_START;
    myCommand4 = END_LOOP;
    myCommand5 = BREAK_LOOP;

    // Fill in the commands for servo A
    *(servoA.currentCommand) = myCommand+5;
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+0;
    servoA.currentCommand++;
    //Simple move command check
   *(servoA.currentCommand) = myCommand+2;  // Test-3
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand + 4;    // Test-3
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+3;  // Test-3
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+1;
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+3;
    servoA.currentCommand++;
    //This test case is loop check.
    *(servoA.currentCommand) = myCommand+5;      //Test-2
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand3+0;     //Test-2
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+1;      //Test-2
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+5;      //Test-2
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+0;      //Test-2
    servoA.currentCommand++;
     *(servoA.currentCommand) = myCommand4;      //Test-2
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand;        //Test-2
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand2 + 20;
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+1;
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand;
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+5;
    servoA.currentCommand++;
    // this test case is for Break command check.
    *(servoA.currentCommand) = myCommand+3;      //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand3+2;     //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+1;      //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand5;      //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+4;      //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+0;      //Test-6
    servoA.currentCommand++;
     *(servoA.currentCommand) = myCommand4;      //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+5;        //Test-6
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+2;
    servoA.currentCommand++;
    *(servoA.currentCommand) = myCommand+3;
    servoA.currentCommand++;
    *(servoA.currentCommand) = RECIPE_END;


    // set the pointer back to the beginning of the buffer.
    servoA.currentCommand = &bufferServoA;

    // Fill in the commands for servo B
    *(servoB.currentCommand) = myCommand+0;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+4;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+1;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;
    servoB.currentCommand++;
    // this is MOV command test.
    *(servoB.currentCommand) = myCommand;     //Test1
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;   //Test1
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand;     //Test1
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand;
    servoB.currentCommand++;
    //WAIT command test.
    *(servoB.currentCommand) = myCommand+2;        //Test4
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+3;        //Test4
    servoB.currentCommand++;
     *(servoB.currentCommand) = myCommand2 + 31;   //Test4
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand2 + 31;    //Test4
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand2 + 31;    //Test4
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+4;        //Test4
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand;
    servoB.currentCommand++;
    //Test for LOOP error.
    *(servoB.currentCommand) = myCommand+3;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand3+2;     //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+1;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+4;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand3+1;     //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+1;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand4;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+0;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand4;      //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand;        //Test-5
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand+5;
    servoB.currentCommand++;
    *(servoB.currentCommand) = myCommand;
    servoB.currentCommand++;
    *(servoB.currentCommand) = RECIPE_END;

    // set the pointer back to the beginning of the buffer.
    servoB.currentCommand = &bufferServoB;
}



//*****************************************************************************
// This unmitigated piece of crap holds the recipes to be run by each servo.
//
// Parameters: NONE
//
// Return: None
//*****************************************************************************
void initializeServos(void)
{
  const unsigned int TwentymsTicks = 250; // There are 250 ticks in 20ms.

  // Initialize the Task Control Blocks.
  servoA.status  = paused;
  servoA.currentCommand = &bufferServoA;
  servoA.loopFlag = FALSE;
  servoA.loopCounter = 0;
  servoA.firstLoopInstruction = 0;
  servoA.currentServoPosition = 255;  // These are 255 so that if the first command is to
                                      // go to position 0 it will go there.
  servoA.expectedServoPosition = 255; // These are 255 so that if the first command is to
                                      // go to position 0 it will go there.
  servoA.timeLeftms = 0;

  servoB.status = paused;
  servoB.currentCommand = &bufferServoB;
  servoB.loopFlag = FALSE;
  servoB.loopCounter = 0;
  servoB.firstLoopInstruction = 0;
  servoB.currentServoPosition = 255;  // These are 255 so that if the first command is to
                                      // go to position 0 it will go there.
  servoB.expectedServoPosition = 255; // These are 255 so that if the first command is to
                                      // go to position 0 it will go there.
  servoB.timeLeftms = 0;

}


//*****************************************************************************
// This unmitigated piece of crap will process the commands that make up a
// recipe.
//
// Parameters:  servo          Holds a pointer to the servos Task Control Block.
//              command        The Command to be executed by the servo.
//              commandContext The context of the comamnd extracted fromt the one
//                             byte command.
//
// Return: None
//*****************************************************************************
void processCommand (struct TaskControlBlock* servo, enum COMMANDS command, int commandContext)
{
  int positionChange = 0;
  const unsigned int PerPositionIncrementms = 200;
  const unsigned int waitTimeIncrementms = 100;

  switch(command)
  {
     case RECIPE_END:
          //printf("\r\n processCommand: RECIPE_END\r\n");
          // Turn off the servos
           if(servo == &servoA)
              {
        	     servo1UserInput = 'e';

                 // Flag is set for reciepe end so that the servo will not process any more commands.
                 reciepeEndServoA = 1;
              }
              else if(servo == &servoB)
              {
            	  servo1UserInput = 'e';

                 // Flag is set for reciepe end so that the servo will not process any more commands.
                 reciepeEndServoB = 1;
              }
          break;
     case MOV:
        if(commandContext < 6)
        {
              // if the servo position in the command is different to the
              // the current position proces it.
              // update the expected servo position
              servo->expectedServoPosition = commandContext;

              // Calculate out the amount of time it will take the command
              // to run.
              if(servo->currentServoPosition == 255)
              {
                 positionChange =  commandContext;
              }
              else if(servo->expectedServoPosition < servo->currentServoPosition)
              {
                 positionChange = servo->currentServoPosition - servo->expectedServoPosition;
              }
              else
              {
                 positionChange = servo->expectedServoPosition - servo->currentServoPosition;
              }

              //printf("\r\nprocessCommand: setting positionChange %u\r\n", positionChange);

              servo->timeLeftms = positionChange * PerPositionIncrementms;
             // printf("\r\nprocessCommand: setting processCommand %u\r\n", servo->timeLeftms);

              // Send the commands down the PWM channel.
              // figure out which channel to send it down on.
              if(servo == &servoA)
              {
                 //printf("\r\nprocessCommand: setting servoA\r\n");
                 step_size_servoA = servoPositionTicks[servo->expectedServoPosition];
              }
              else if(servo == &servoB)
              {
                 //printf("\r\nprocessCommand: setting servoB\r\n");
                 step_size_servoB = servoPositionTicks[servo->expectedServoPosition];
              }
              else {
                 printf("\r\nprocessCommand: undefined servo\r\n");
              }

              // Update the Task Control Block Status.
              servo->status = running;
              servo->currentCommand++;
        }

        break;
     case WAIT:
        // The wait lengths are 0-31

        //printf("\r\n processCommand: WAIT %d\r\n", commandContext);
        if(commandContext < 32)
        {
            // Calculate out the amount of time it will take the command
            // to run.
            servo->timeLeftms = (commandContext) * waitTimeIncrementms;

            // Update the Task Control Block Status.
            if(servo == &servoA || servo == &servoB)
            {
              // increment the command buffer;
              servo->currentCommand++;
            }
            else
            {
              printf("\r\nprocessCommand: undefined servo\r\n");
            }

            servo->status = running;
        }

        break;

     case LOOP_START:
        //printf("\r\n processCommand: LOOPSTART %d\r\n", commandContext);

        // if we do not have a nested loop set things up for
        // a loop.
        if(servo->loopFlag == FALSE)
        {
           servo->loopFlag = TRUE;

           // Set the number of iterations.
           servo->loopCounter = commandContext;

           // Set a pointer to the instruction after the
           // LOOPSTART command and move onto the next
           // instruction.
           servo->firstLoopInstruction = servo->currentCommand + 1;

           // Increment the instruction pointer.
           servo->currentCommand++;
         }
         else
         {
            // place the task in an error state.
            servo->status = error;

            // indicate an error for that servo.
            if(servo == &servoA)
            {
              printf("\r\nprocessCommand: Nested Loop Error for servoA\r\n");
            } else if(servo == &servoB){
              printf("\r\nprocessCommand: Nested Loop Error for servoB\r\n");
            }
         }

         break;

     case END_LOOP:
        //printf("\r\n processCommand: END_LOOP\r\n");

        // if we are not on our last iteration of the loop
        if(servo->loopCounter > 0)
        {
           // Go back to the instruction after the LOOP_START command.
           servo->currentCommand = servo->firstLoopInstruction;
           // deincrement the loop counter.
           --(servo->loopCounter);
        }
        else
        {
           // Ok we're done with the loop.  Clean up the TCB and
           // go to the next instruction.
           servo->loopFlag = FALSE;
           servo->firstLoopInstruction = 0;
           if(servo == &servoA || servo == &servoB)
           {
             // increment the command buffer;
             servo->currentCommand++;
           }
           else
           {
             printf("\r\nprocessCommand: undefined servo\r\n");
           }
        }

        break;

     case BREAK_LOOP :
        //printf("\r\n processCommand: BREAK_LOOP\r\n");

        //While loop will shift the pointer to the end of current loop.
        while(*servo->currentCommand != END_LOOP) {
          servo->currentCommand++;
        }

        // clean up the TCB since were out of the loop.
        servo->loopFlag = FALSE;
        servo->firstLoopInstruction = 0;

        servo->currentCommand++; //finally we are pointing to next command in reciepe.

        break;

     default:

        // set the status lights to indicate a recipe command error.
        if(servo == &servoA)
        {
           printf("\r\nprocessCommand: undefined command for servoA\r\n");
        } else if(servo == &servoB){
           printf("\r\nprocessCommand: undefined command for servoB\r\n");
        }
        else
        {
           printf("\r\nprocessCommand: Unknown error occured.\r\n");
        }
  }
}

//*****************************************************************************
// This unmitigated piece of crap will process the commands input from the user.
//
// Parameters: NONE
//
// Return: None.
//*****************************************************************************
void processUserCommand(void)
{
   // process command
   // process the continue command.
   if((servo1UserInput == 'c' || servo1UserInput == 'C') &&
       servoA.status != error && (firstThree(*servoA.currentCommand)) != RECIPE_END)
   {
	   printf("It is continue.\n");
      servoA.status  = running;
      //printf("\r\n processUserCommand: servoA.status = running\r\n");
   }

   if((servo2UserInput == 'c' || servo2UserInput == 'C') &&
      servoB.status != error && (firstThree(*servoB.currentCommand)) != RECIPE_END)
   {
      servoB.status  = running;
     // printf("\r\n processUserCommand: servoB.status = running\r\n");
   }

      // process the pause command.
   if((servo1UserInput == 'p' || servo1UserInput == 'P') &&
       servoA.status != error  && (firstThree(*servoA.currentCommand)) != RECIPE_END)
   {
	   printf("It is continue.\n");
      printf("\r\nYou have following options: \n\r l = Move left \n\r r = Move Right\n\r s = Switch Reciepe\n\r c = Continue reciepe\n\r n = no-op\n\r b = Restart reciepe");
      servoA.status  = paused;
      //printf("\r\n processUserCommand: servoA.status = paused\r\n");
   }

   if((servo2UserInput == 'p' || servo2UserInput == 'P') &&
      servoB.status != error && (firstThree(*servoB.currentCommand)) != RECIPE_END)
   {
      servoB.status = paused;
      //printf("\r\n processUserCommand: servoB.status = paused\r\n");
   }

       // process the restart command.
   if((servo1UserInput == 'b' || servo1UserInput == 'B'))
   {
      servoA.currentCommand = &bufferServoA;
      servoA.status  = ready;
      reciepeEndServoA = 0;
      servoA.loopFlag = FALSE;
      //printf("\r\n processUserCommand: B is pressed for ServoA.\r\n");
   }

    if((servo2UserInput == 'b' || servo2UserInput == 'B'))
   {
      servoB.currentCommand = &bufferServoB;
      servoB.status = ready;
      reciepeEndServoB = 0;
      servoB.loopFlag = FALSE;
      //printf("\r\n processUserCommand: B is pressed for ServoB\r\n");
   }

        // process the no-op command.
   if((servo1UserInput == 'n' || servo1UserInput == 'N') &&
       servoA.status != error )
   {
      servoA.status  = donothing;
      //printf("\r\n processUserCommand: B is pressed for ServoA.\r\n");
   }

    if((servo2UserInput == 'n' || servo2UserInput == 'N') &&
      servoB.status != error )
   {
      servoB.status = donothing;
      //printf("\r\n processUserCommand: B is pressed for ServoB\r\n");
   }

   // process the run Right command.
   if((servo1UserInput == 'r' || servo1UserInput == 'R') &&
       servoA.status != error )
   {
      if(servoA.currentServoPosition != 0){
        processCommand(&servoA,MOV, --servoA.currentServoPosition );
      }
      servoA.status = donothing;
   }

    if((servo2UserInput == 'r' || servo2UserInput == 'R') &&
      servoB.status != error )
   {
      if(servoB.currentServoPosition != 0){
      processCommand(&servoB,MOV, --servoB.currentServoPosition);
      }
      servoB.status = donothing;
   }

    // process the run Left command.
   if((servo1UserInput == 'l' || servo1UserInput == 'L') &&
       servoA.status != error )
   {
      if(servoA.currentServoPosition != 5){
          processCommand(&servoA,MOV, ++servoA.currentServoPosition);
      }
      servoA.status = donothing;
   }

    if((servo2UserInput == 'l' || servo2UserInput == 'L') &&
      servoB.status != error )
   {
      if(servoB.currentServoPosition != 5){
          processCommand(&servoB,MOV, ++servoB.currentServoPosition);
      }
      servoB.status = donothing;
   }

     // Process the swap Command which changes the reciepe for the servos.
   if((servo1UserInput == 's' || servo1UserInput == 'S') &&
       servoA.status != error )
   {
      servoA.currentCommand = servoB.currentCommand;
   }

    if((servo2UserInput == 's' || servo2UserInput == 'S') &&
      servoB.status != error )
   {
      servoB.currentCommand = servoA.currentCommand;
   }

   // set global variables to 0 so we know we have new input
   servo1UserInput = 0;
   servo2UserInput = 0;
}


//*****************************************************************************
// This unmitigated piece of crap process the user commands and then either
// processes a new command or updates the status on a current command for
// each servo.
//
// Parameters: NONE
//
// Return: None.
//*****************************************************************************
void runTasks(void)
{
   // first process the user commands
   processUserCommand();
   // then run the recipies based on the changes from the processUserCommand
   // function.
   if(servoA.status  == ready && reciepeEndServoA != 1)
   {
     // get the next command and process it.
     processCommand(&servoA,firstThree(*servoA.currentCommand), lastFive(*servoA.currentCommand));
   }
   else if(servoA.status  == running)
   {
     updateTaskStatus(&servoA);
   }

   if(servoB.status  == ready && reciepeEndServoB != 1)
   {
     processCommand(&servoB, firstThree(*servoB.currentCommand), lastFive(*servoB.currentCommand));
   }
   else if (servoB.status  == running)
   {
     updateTaskStatus(&servoB);
   }
}

//*****************************************************************************
// This unmitigated piece of crap updates the amount of time left for a command
// running on a servo.  If the amount of time has expired then it sets the task
// status to ready.
//
// Parameters: NONE
//
// Return: None.
//*****************************************************************************
void updateTaskStatus(struct TaskControlBlock* servo)
{
   // Right now all we need to do is deincrement the timers
   // and update the task status to ready once the timer
   // for a thread has run out.

   // We are processing a command
   if(servo->status  == running) {
      //printf("\r\nprocessCommand: updateTaskStatus servo->timeLeftms %u\r\n", servo->timeLeftms);
      if(servo->timeLeftms > 0)
      {
        //printf("\r\n updateTaskStatus: updateTime == TRUE && servo->timeLeftms > 0\r\n");
        servo->timeLeftms -=100;
      }
      else
      {
        // update time is 0.  set the servo postion to the expected
        // position and update the task status to ready.
        servo->currentServoPosition = servo->expectedServoPosition;
        servo->status = ready;
        //printf("\r\n updateTaskStatus: servostatus = ready\r\n");
      }
   }
}
int fetch_counter = 0;

void *generate_pulse_servoA( void *ptr )
{
     printf("Timer Thread.\n");
	int privity_err;
    privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
    if ( privity_err == -1 )
    {
        fprintf( stderr, "can't get root permissions\n" );
        pthread_exit(NULL);
     }
    struct sigevent         event;
             struct itimerspec       itime;
             timer_t                 timer_id;
             int                     chid, rcvid;
             my_message_t            msg;

             chid = ChannelCreate(0);

             event.sigev_notify = SIGEV_PULSE;
             event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
                                                chid,
                                                _NTO_SIDE_CHANNEL, 0);
             event.sigev_priority = getprio(0);
             event.sigev_code = MY_PULSE_CODE;
             timer_create(CLOCK_REALTIME, &event, &timer_id);

             itime.it_value.tv_sec = 0;
             /* 100 ms = .1 secs */
             itime.it_value.tv_nsec = 20000000;
             itime.it_interval.tv_sec = 0;
             /* 100 ms = .1 secs */
             itime.it_interval.tv_nsec = 20000000;
             timer_settime(timer_id, 0, &itime, NULL);
             // This for loop will update the global_time for every 100 ms which is 1 minute in simulation time.
             for (;;) {
                 rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
                 if (rcvid == 0) { /* we got a pulse */
                      if (msg.pulse.code == MY_PULSE_CODE) {
                    	  fetch_counter++;
                    	  if(fetch_counter == 20 /*&& user_input == 1*/)
                    	  {
                    		  runTasks();
                    		  fetch_counter = 0;
                    	  }
                    	  if(servo1UserInput != 'p' || servo1UserInput != 'e'){
                    		  my_timer_value1.tv_nsec = step_size_servoA * 100000;
                    		  out8( ctrl_handle_portB, HIGH );
                    		  //ctrl_handle_portB = ctrl_handle_portB | 0x01;
                    		  nanospin(&my_timer_value1);
                    		  out8( ctrl_handle_portB, LOW );
                    		  //ctrl_handle_portB = ctrl_handle_portB & 0xFE;
                    		  }

                    	  if (servo1UserInput == 'x')
                    	  {
                    		  printf("\n Pulse Generator : Exiting\n");
                    		  pthread_exit(NULL);//Rajeev
                    	  }
                      } /* else other pulses ... */
                 } /* else other messages ... */
        }
   printf("\n Pulse Generator : Exiting\n");    //Debug log
   pthread_exit(NULL);
}

void *generate_pulse_servoB( void *ptr )
{
     printf("Timer Thread.\n");

	int privity_err;
    privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
    if ( privity_err == -1 )
    {
        fprintf( stderr, "can't get root permissions\n" );
        pthread_exit(NULL);
     }
    struct sigevent         event;
             struct itimerspec       itime;
             timer_t                 timer_id;
             int                     chid, rcvid;
             my_message_t            msg;

             chid = ChannelCreate(0);

             event.sigev_notify = SIGEV_PULSE;
             event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
                                                chid,
                                                _NTO_SIDE_CHANNEL, 0);
             event.sigev_priority = getprio(0);
             event.sigev_code = MY_PULSE_CODE;
             timer_create(CLOCK_REALTIME, &event, &timer_id);

             itime.it_value.tv_sec = 0;
             /* 100 ms = .1 secs */
             itime.it_value.tv_nsec = 20000000;
             itime.it_interval.tv_sec = 0;
             /* 100 ms = .1 secs */
             itime.it_interval.tv_nsec = 20000000;
             timer_settime(timer_id, 0, &itime, NULL);
             // This for loop will update the global_time for every 100 ms which is 1 minute in simulation time.
             for (;;) {
                 rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
                 if (rcvid == 0) { /* we got a pulse */
                      if (msg.pulse.code == MY_PULSE_CODE) {
                    	  fetch_counter++;
                    	  if(fetch_counter == 20 /*&& user_input == 1*/)
                    	  {
                    		  runTasks();
                    		  fetch_counter = 0;
                    	  }
                    	  if(servo2UserInput != 'p' || servo2UserInput != 'e'){
                    		  my_timer_value2.tv_nsec = step_size_servoB * 100000;
                    		  out8( ctrl_handle_portC, HIGH );
                    		  nanospin(&my_timer_value2);
                    		  out8( ctrl_handle_portC, LOW );}
                    	  if (servo1UserInput == 'x')
                    	  {
                    		  printf("\n Pulse Generator : Exiting\n");
                    		  pthread_exit(NULL);//Rajeev
                    	  }
                      } /* else other pulses ... */
                 } /* else other messages ... */
        }

   printf("\n Pulse Generator : Exiting\n");    //Debug log
   pthread_exit(NULL);
}

int main( )
{
	int privity_err, servoAthread, servoBthread;
    uintptr_t ctrl_handle_portCTL;
    pthread_t thread1, thread2;

    //my_timer_value1.tv_nsec = 1000;
    my_timer_value1.tv_nsec = 1500000;
    my_timer_value1.tv_sec = 0;

    my_timer_value2.tv_nsec = 1500000;
    my_timer_value2.tv_sec = 0;

    /* Give this thread root permissions to access the hardware */
    privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
    if ( privity_err == -1 )
    {
        fprintf( stderr, "can't get root permissions\n" );
        return -1;
    }

    /* Get a handle to the DIO port's Control register */
    ctrl_handle_portB = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_PORTB_ADDR );
    ctrl_handle_portC = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_PORTC_ADDR );
    ctrl_handle_portCTL = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_CTL_ADDR);

    /* Initialize the DIO port */
    out8( ctrl_handle_portCTL, 0x80 );
    out8( ctrl_handle_portB, LOW );
    out8( ctrl_handle_portC, LOW );
    servoAthread = pthread_create( &thread1, NULL, generate_pulse_servoA, NULL);
    servoBthread = pthread_create( &thread2, NULL, generate_pulse_servoB, NULL);

    // This function has to be before the InitializeTimer function.
    initializeServos();
    initializeCommands();
    char user_input[5];
    // Show initial prompt
    while(1){
    	   printf("\nCommand for both Servos: ");
    	   gets(user_input);
    	   (void)printf("\r\nServoA Command: %c", user_input[0]);
    	   (void)printf("\r\nServoB Command: %c", user_input[1]);

    	   servo1UserInput = user_input[0];
    	   servo2UserInput = user_input[1];
    }
    printf("Out of loop.\n");
    pthread_join(thread1, NULL);
    return 0;
}


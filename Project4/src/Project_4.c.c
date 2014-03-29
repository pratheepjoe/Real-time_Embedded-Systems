#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>

void *customer_enqueue( void *ptr );
void *teller1_function( void *ptr );
void *teller2_function( void *ptr );
void *teller3_function( void *ptr );
void *timer_thread( void *ptr);


static int bank_time =420;
int i;
int queue[1000];
int front = 0;
int rear = -1;
int customer_no=1;
int queue_empty;
int service_time1, service_time2, service_time3;
int customer_time[1000];
int in_time[1000];
int total_customers, sum_of_times;
float avg_waiting_time;
static int up_time;
int time_with_teller;
float avg_time_with_teller;
int no_of_teller_waits;
int wait_time;
int entry_time1, entry_time2, entry_time3, exit_time1, exit_time2, exit_time3;
float avg_teller_wait_time;
int max_customer_wait;
int max_wait_time_array[1000];
int max_teller_wait;
float max_service_time;
int max_queue_depth;
int time_between_breaks1, time_between_breaks2, time_between_breaks3;
int break_time1, break_time2, break_time3;



pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

typedef union {
        struct _pulse   pulse;

} my_message_t;



struct Customer
{
	int arrival_time;

};


void *customer_enqueue(void *ptr){


	while(bank_time>0){
		pthread_mutex_lock(&lock);
		struct Customer *c1;
		c1 = (struct Customer *) ptr;                      //type cast to a pointer to Customer
		c1->arrival_time = (rand() % 4) +1;

		usleep(c1->arrival_time*100000);                  //arrival time

		rear++;
		queue[rear] = customer_no;                        //Customer IN
		queue_empty=0;

		if(rear > max_queue_depth){			              	 //....
			max_queue_depth = rear;				                 //....steps for metric calc.
		}										                             //....

		in_time[customer_no] = bank_time;		            //storing in_time
		printf("Customer coming IN : %d\n", queue[rear]);
		customer_no++;
		pthread_mutex_unlock(&lock);
	}


}

void customer_dequeue(void){

		pthread_mutex_lock(&lock);

		if(queue[front]!=0){
			printf("customer OUT : %d\n", queue[front]);
		}
		customer_time[(queue[front])] = in_time[(queue[front])] - bank_time;	//calculating time spent in queue

		for(i=0;i<999;i++){			                                            	//Dequeue and decrement rear.
			queue[i]=queue[i+1];
		}
		if(rear>=0){
			rear--;
		}

		if(rear==-1){
			queue_empty=1;
		}else{
			queue_empty=0;
		}


	pthread_mutex_unlock(&lock);

}


void *teller1_function( void *ptr )
{
	time_between_breaks1 = (rand() % 31)+30;

	while((bank_time>0) || (queue_empty==0)){
		if(queue_empty==0){

			entry_time1 = bank_time;                            					               //....
			if((exit_time1 - entry_time1)>0){ 									                  	     //....
				max_wait_time_array[no_of_teller_waits] = (exit_time1 - entry_time1);     //....steps for metric calc.
				no_of_teller_waits++;											                              	//....
				wait_time = wait_time + (exit_time1 - entry_time1); 		               		//....
			}

			service_time1 = (((rand() % 12) +1)/2);
			time_with_teller = time_with_teller + service_time1;                        //summation of customer's time spent with teller
			if(service_time1 > max_service_time){  			                              	//....
				max_service_time = service_time1;			                                	//....steps for metric calc.
			}												                                                 	//....
			usleep(service_time1*100000);
			customer_dequeue();

			//SNIPPET FOR BREAKS - FOR GRADUATES-----------------------
			if(up_time >= time_between_breaks1){

				break_time1 = (rand() % 4) +1;
				usleep(break_time1*100000);
				printf("\nTeller 1 took a break of %d mins\n\n", break_time1);
				time_between_breaks1 = up_time + ((rand() % 31)+30);
			}
			//-------------------------------------------------------
			exit_time1=bank_time;

		}

	}
}


void *teller2_function( void *ptr )
{
	time_between_breaks2 = (rand() % 31)+30;

	while((bank_time>0) || (queue_empty==0)){
		if(queue_empty==0){

			entry_time2 = bank_time;                            					               //....
			if((exit_time2 - entry_time2)>0){  							                      			//....
				max_wait_time_array[no_of_teller_waits] = (exit_time2 - entry_time2);     //....steps for metric calc.
				no_of_teller_waits++;											                              	//....
				wait_time = wait_time + (exit_time2 - entry_time2); 			              	//....
			}
			service_time2 = (((rand() % 12) +1)/2);
			time_with_teller = time_with_teller + service_time2;                        //summation of customer's time spent with teller
			if(service_time2 > max_service_time){  			                              	//....
				max_service_time = service_time2;				                                  //....steps for metric calc.
			}												                                                  	//....
			usleep(service_time2*100000);
			customer_dequeue();

			//SNIPPET FOR BREAKS - FOR GRADUATES-----------------------
			if(up_time >= time_between_breaks2){

				break_time2 = (rand() % 4) +1;
				usleep(break_time2*100000);
				printf("\nTeller 2 took a break of %d mins\n\n", break_time2);
				time_between_breaks2 = up_time + ((rand() % 31)+30);
			}
			//---------------------------------------------------------

			exit_time2=bank_time;

		}
	}
}


void *teller3_function( void *ptr )
{
	time_between_breaks3 = (rand() % 31)+30;

	while((bank_time>0) || (queue_empty==0)){
		if(queue_empty==0){

			entry_time3 = bank_time;                                 			              	//....
			if((exit_time3 - entry_time3)>0){  									                        	//....
				max_wait_time_array[no_of_teller_waits] = (exit_time3 - entry_time3);       //....steps for metric calc.
				no_of_teller_waits++;											                                	//....
				wait_time = wait_time + (exit_time3 - entry_time3);  			                	//....
			}

			service_time3 = (((rand() % 12) +1)/2);
			time_with_teller = time_with_teller + service_time3;                          //summation of customer's time spent with teller
			if(service_time3 > max_service_time){  			                                 	//....
				max_service_time = service_time3;			                                    	//....steps for metric calc.
			}												                                                    	//....
			usleep(service_time3*100000);
			customer_dequeue();

			//SNIPPET FOR BREAKS - FOR GRADUATES-----------------------
			if(up_time >= time_between_breaks3){

				break_time3 = (rand() % 4) +1;
				usleep(break_time3*100000);
				printf("\nTeller 3 took a break of %d mins\n\n", break_time3);
				time_between_breaks3 = up_time + ((rand() % 31)+30);
			}
			//---------------------------------------------------------

			exit_time3=bank_time;

		}
	}
}

void *timer_thread( void *ptr )
{
		struct sigevent         event;
	   struct itimerspec       itime;
	   timer_t                 timer_id;
	   int                     chid;
	   int                     rcvid;
	   my_message_t            msg;

	   chid = ChannelCreate(0);

	   event.sigev_notify = SIGEV_PULSE;
	   event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
	                                    chid,
	                                    _NTO_SIDE_CHANNEL, 0);
	   event.sigev_priority = getprio(0);
	   event.sigev_code = MY_PULSE_CODE;
	   timer_create(CLOCK_REALTIME, &event, &timer_id);

	   itime.it_value.tv_sec = 1;
	   /* 100 million nsecs = .1 secs */
	   itime.it_value.tv_nsec = 100000000;
	   itime.it_interval.tv_sec = 0.1;
	   /* 100 million nsecs = .1 secs */
	   itime.it_interval.tv_nsec = 100000000;
	   timer_settime(timer_id, 0, &itime, NULL);

	   /*
	    * As of the timer_settime(), we will receive our pulse
	    * in 1.5 seconds (the itime.it_value) and every 1.5
	    * seconds thereafter (the itime.it_interval)
	    */


	   while(bank_time>0){
	       rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
	       bank_time --;
	       up_time++;
	   }

}



main()
{
     pthread_t enqueue, teller1, teller2, teller3, timer;
     struct Customer *c;

     printf("BANK OPEN\n\n");

     pthread_create( &timer, NULL, timer_thread, NULL);
     pthread_create( &enqueue, NULL, customer_enqueue, (void *) &c);
     pthread_create( &teller1, NULL, teller1_function, NULL);
     pthread_create( &teller2, NULL, teller2_function, NULL);
     pthread_create( &teller3, NULL, teller3_function, NULL);

     pthread_join( enqueue, NULL);
     pthread_join( teller1, NULL);
     pthread_join( teller2, NULL);
     pthread_join( teller3, NULL);
     pthread_join( timer, NULL);

     printf("\n\nBANK CLOSED\n\n");
     //SNIPPETS FOR METRICS---------------------------------------------------------------------------

     //calculating Total Number of Customers
     total_customers = customer_no-1;
     printf("\n\nTotal Number Of Customers = %d\n", total_customers);


     //calculating Average Waiting time of each customer in queue
     for(i=0;i<1000;i++){
    	 sum_of_times = sum_of_times + customer_time[i];
     }
     avg_waiting_time = sum_of_times / total_customers;
     printf("Average Waiting Time of each customer in the queue = %f minutes\n", avg_waiting_time);


     //calculating average time customer spends with teller
     avg_time_with_teller = time_with_teller / total_customers;
     printf("Average time each customer spends with the teller = %f minutes\n", avg_time_with_teller);


     //calculating average time tellers wait for customers
     avg_teller_wait_time = wait_time / no_of_teller_waits;
     printf("Average time tellers wait for customers = %f minutes\n", avg_teller_wait_time);


     //calculating maximum customer wait time in the queue
     for(i=0;i<1000;i++){
    	 if(customer_time[i]>max_customer_wait){
    		 max_customer_wait = customer_time[i];
    	 }
     }
     printf("Maximum customer wait time in the queue = %d minutes\n", max_customer_wait);


     //calculating maximum time tellers wait for customers
     for(i=0;i<1000;i++){
         	 if(max_wait_time_array[i]>max_teller_wait){
         		 max_teller_wait = max_wait_time_array[i];
         	 }
     }
     printf("Maximum time tellers wait for customers = %d minutes\n", max_teller_wait);


     //the maximum transaction time for tellers
     printf("Maximum transaction time for tellers = %f minutes\n", max_service_time);


     //The maximum depth of the queue
     printf("The maximum depth of the queue = %d\n", max_queue_depth);

     //---------------------------------------------------------------------------------------------

     exit(0);
}









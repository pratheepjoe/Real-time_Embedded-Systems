Design and implement an embedded, stand-alone QNX Neutrino program to simulate 
the workflow in a typical banking environment (single queue, multiple server queuing 
problem.) 
 
Analysis: 
Customers: Customers enter the bank to transact business on a regular basis. Each 
new customer arrives every one to four minutes, based on a uniform random 
distribution. Each new customer enters a single queue of all customers. 
Tellers: Three tellers are available to service customers in the queue. As tellers 
become available, customers leave the queue, approach the teller and transact their 
business. Each customer requires between 30 seconds and 6 minutes to complete their 
transaction with the teller. The time required for each transaction is based on a uniform 
random distribution. 
Bank: The bank is open for business between the hours of 9:00am and 4:00pm. 
Customers begin entering when the bank opens in the morning, and stop entering when 
the bank closes in the afternoon. Customers in the queue at closing time remain in the 
queue until tellers are available to complete their transactions. 
Metrics: To monitor the performance of the business, metrics are gathered and 
reported at the end of the day. The metrics are: 
? the total number of customers serviced during the day, 
? the average time each customer spends waiting in the queue, 
? the average time each customer spends with the teller, 
? the average time tellers wait for customers, 
? the maximum customer wait time in the queue, 
? the maximum wait time for tellers waiting for customers, 
? the maximum transaction time for the tellers, and 
? the maximum depth of the queue. 
 
 
Design Constraints: 
Each customer is represented as an application task (i.e. TCB). 
Each teller is modeled as an independent thread. 
The simulation parameters are variable and assigned at program startup. 
The simulation time is scaled such that 100 milliseconds of absolute clock time 
represents 1 minute of simulation clock time. The output should be presented in 
simulation clock time. 
 Graduate Students: 
Include random breaks for each of the tellers. Each teller will take a break every 30 to 
60 minutes for a duration of 1 to 4 minutes each. A break can only occur after the 
completion of customer transactions. The break timing and duration is based on a 
random uniform distribution. 
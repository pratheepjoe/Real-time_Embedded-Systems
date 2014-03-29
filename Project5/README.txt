Design and implement an embedded, stand-alone QNX Neutrino program to measure 
the distance between the rear bumper of your car and any objects behind the vehicle 
while parking. 
 
Analysis: 
Driver: When the vehicle is placed into reverse, the parking sensor is activated, 
providing a continuous stream of data on the distance of any objects behind the vehicle. 
Car: The parking sensor is mounted on the rear bumper of the car. 
Parking Sensor: When activated, the parking sensor measures the distance between 
itself (as located on the rear bumper) and any objects within its field of view. The 
distance is reported to the driver on a continuous basis. 
 
Design Constraints: 
? The distance is measured at a rate of 10 times per second. 
? The results of the measurement is displayed on the console so that the value 
does not scroll. 
? The measured results are rounded to the nearest inch and displayed as integers 
values only. 
? Out-of-range measurements are represented as a flashing asterisk. 
? The measuring process is started when the user selects a key, and ended when 
the user selectsanother. 
? After measurements are ended, display the maximum and minimum distances 
measured. 
? In your report, include test cases and explicit results indicating the practical range 
of your ultrasound sensor. 
 
 
Technical Notes: 
To measure distance with the ultrasound sensor, generate a positive “ping” pulse that 
remains high for at least 10 microseconds. The output from the ultrasound sensor 
returns a positive pulse, the duration of which will be from 100 microseconds to 18 
milliseconds. The duration of the return pulse equals the round-trip propagation time of 
the “ping” pulse between the sensor and the target. The distance is determined by 
measuring the duration of the return pulse between the rising and falling edges, and 
calculating the distance based on the speed of sound in air at room temperature 
(769.55 mph). Please refer to the technical data sheet for the ultrasound sensor for 
more information. 
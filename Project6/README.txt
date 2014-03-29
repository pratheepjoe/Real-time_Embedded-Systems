Design and implement an embedded, real-time stand-alone system to provide a rough 
indication of voltage using servo motors. 
 
Analysis: 
Signal Generator: A signal generator produces a waveform with a long frequency and 
limited amplitude. Multiple waveforms are possible, include a sine wave, a square wave, 
and a triangle wave. 
Voltage Indicator: The voltage indicator provides a rough (non-calibrated) indication of 
the input voltage. The indicator movement reflects the voltage changes of the signal 
generator. Two indicators are required, one for negative voltage, one for positive 
voltage. 
 
Design Constraints: 
? The signal generator voltage is measured with the A/D convertor on the “purple 
box” using a QNX Neutrino program. 
? The voltage of the signal generator must be constrained to -5 volts to +5 volts 
(see warning below.) 
? The measured voltage is indicated with servo motors driven by a program on the 
Freescale prototype board. 
? The frequency is low enough to allow the servo to respond in real-time. 
? The sampling frequency of the signal generator should be sufficient to provide 
smooth operation of the servo. 
? A push-button on the Freescale prototype board is used to start the servo 
motors. 
? No user interface is required nor permitted. 
? The communication mechanism between the two platforms is at your discretion. 
Please see the instructor if you’d like any suggestions. 
? Each platform must provide a visual or audio indication of a fault condition 
(communication link is down, out-of-range voltage, etc.) 
 
WARNING: 
The “purple box” A/D converter accepts voltages between -10 and +10 volts DC, 
referenced to ground on the DIO port. Voltages beyond this specification will damage 
the hardware! Please limit your signal generator voltage to -5 volts to +5 volts DC. You 
MUST verify the output voltage of the signal generator with a ‘scope or voltmeter prior to 
connecting to the A/D port. If the signal generator configuration is changed or is 
unknown, please disconnect the signal and re-measure. 
# ADF4351_Loader
Simple Arduino program to initialise an ADF4351 and drive a pin with 1kHz modulation.

This variant works well as a signal source for the HP 415E "SWR meter" ... ideal as a battery powered widget on an antenna test range.

8 bands are catered for, in an array.  Selecting bands 0~7 on a HEX thumbwheel will select the appropriate band with 1kHz mod, selecting 8~15 will select the same set of bands but CW. 

The code takes some of the pain out of configuring an ADF4351 chip via an Arduino.  Simply, it takes in the frequency you desire, the reference frequency you are using and calculates the appropriate register values on the fly and sends them out over SPI.  No need to use the factory tools to calculate values and copy them over.



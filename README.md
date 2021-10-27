# ADF4351_Loader
Simple Arduino program to initialise an ADF4351

This code takes some of the pain out of configuring an ADF4351 chip via an Arduino.  Simply, it takes in the frequency you desire, the reference frequency you are using and calculates the appropriate register values on the fly and sends them out over SPI.  No need to use the factory tools to calculate values and copy them over.

Can drive multiple latch eneable lines for systems with more than one chip to configure.

After loading the ADF4351 with the values, the Arduino goes to sleep to minimise any chance of sprogs on receive.

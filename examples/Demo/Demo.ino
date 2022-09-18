/*******************************************************************************
* Software I2C Library designed for devices without hardware I2C like the ATTiny
* devices, or for multiple identical devices on one system where their addresses
* would otherwise clash and fail
*
* This is a demo of the most useful funtions, but this ultimately has no
* usefulness.
*
* ADBeta 18 Sep 2022
* Version 0.1.6
*******************************************************************************/

#include "SoftwareI2C.h"

//SDA, SCL, ADDR
SoftwareI2C SI2C(8, 9, 0xD0);

//Array to read or write from
uint8_t demoArray[4] = {0xaa, 0xbb, 0xcc, 0xdd};

void setup() {
	/* Make the SI2C Library verbose with errors, or listing etc. Not advised
	   for use on ATTiny devices or any device with low RAM or no Serial */
	SI2C.setVerbosity(true);
	Serial.begin(9600);
	
	//Set the I2C Bus speed to a speed in KHz. If not set, defaults unlimited
	SI2C.setSpeed(100);
	
	//Lists all the devices on the bus.
	SI2C.listDevices();
	
	//Write a single byte to the bus
	SI2C.writeCommand(0x50);
	
	//Write an array to the bus from register 00
	SI2C.writeArray(0x00, demoArray, 4);
	
	//Read an array from the bus from register 00
	SI2C.readArray(0x00, demoArray, 4);
}

void loop() {

}

/*
	This library is still a work in progress. It is perfectly usable and 
	functions as inteded but it needs a lot of clean-up and tidying.
	
	Please note - This example is just hypothetical, I was using it to 
	communicate with a I2C card reader, so ignore any specific addresses,
	This is just to demonstrate the syntax.
	
	ADBeta
	5 Nov 2021
*/

#include <SoftwareI2C.h>

SoftwareI2C SI2C;

void setup() {
	Serial.begin(9600);
	
	//SI2C.setup(8, 9, 0x1E); //Mag
	//SI2C.setup(8, 9, 0x68); //Clock
	//SI2C.setup(8, 9, 0x76); //Barometer
	SI2C.setup(8, 9, 0x28); //Card reader
	
	//Set the I2C Bus speed to a speed in KHz. If not set, defaults to 
	//fastest possible speed
	SI2C.setSpeed(100);
	
	//Lists all the devices on the bus. Slow function but useful for debugging
	//Unknown devices
	SI2C.listDevices();
	
	//Write a single command to the bus
	SI2C.writeCommand(0x50);
	
	//Write data to the bus, REG ADDR, DATA, BYTE_COUNT - 4 bytes maximum 
	SI2C.writeData(0x00, 0x55AABBCC, 4);
	
	//Read data and print to monitor. REG ADDR, BYTE_COUNT - 4 bytes maximum
	Serial.println(SI2C.readData(0x31, 3));
}

void loop() {

}

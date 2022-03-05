/*
	Software I2C Library designed for devices without hardware I2C like ATTiny
	devices, or for multiple clashing I2C addresses.
	
	Still a work in progress.
	ADBeta
	Last Modified 12 Dec 2021
*/

#include "Arduino.h"

#ifndef SoftwareI2C_h
#define SoftwareI2C_h

class SoftwareI2C {
	public:
	void setup(uint8_t, uint8_t, uint8_t);
	void setSpeed(int); //Communication speed in kHz. Defaults to no delay.
	void listDevices(); //Find and print any connected devices.
	
	/* High level functions for faster/easier development */ 
	uint8_t writeCommand(uint8_t); //Write single byte to bus
	
	//LSB First multi-byte functions
	uint8_t writeData(uint8_t, uint32_t, uint8_t = 1); //Register addr, data, number of bytes. 4 max. (default 1)
	uint32_t readData(uint8_t, uint8_t = 1); //Register addr, number of bytes. returns 4 bytes max (default 1)
	
	/* Low level functions to somewhat emulate wire.h */
	uint8_t I2CAddress;
	void startI2C();
	void stopI2C();
	
	uint8_t rxByte(bool);
	bool txByte(uint8_t);
	
	private:
	/* Low level functions */
	uint8_t _SCL; //Want to make asm converter and have asm versions of the pins for speed
	uint8_t _SDA;
	
	void assertPin(uint8_t); //Asserts a LOW to the bus
	void releasePin(uint8_t); //Lets bus go HIGH 
	
	void wait(int = 0); //Wait x amount of microseconds (defined by setSpeed) or override
	
	bool rxBit();
	void txBit(bool);
	
	/* Safety and error functions */
	uint8_t transmitError(uint8_t, const char* = ""); //Returns the Error ID and (optional) error message
	uint8_t clockStretch(); //Function to do clock stretching, allows for error handling
	
	/* Internal use vars */
	bool active = false;
	int delayMicros = 0;
	
};
#endif

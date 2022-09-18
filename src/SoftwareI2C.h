/*******************************************************************************
* Software I2C Library designed for devices without hardware I2C like the ATTiny
* devices, or for multiple identical devices on one system where their addresses
* would otherwise clash and fail
*
* ADBeta 18 Sep 2022
* Version 0.1.6
*******************************************************************************/

#include "Arduino.h"

#ifndef SoftwareI2C_h
#define SoftwareI2C_h

//TODO Make progmem list of possible error messages

class SoftwareI2C {
	public:
	//Constructor. SDA, SCL, Address.
	SoftwareI2C(uint8_t sda, uint8_t scl, uint8_t addr);
	
	/*** Setup and configure **************************************************/
	void setVerbosity(bool); //Configures if serial printing is enabled
	void setSpeed(int); //Communication speed in kHz. Defaults to no delay.
	void listDevices(); //Find and print any connected devices.
	
	/*** High Level Functions *************************************************/ 
	//Write a single byte to the bus (returns exit status)
	bool writeByte(uint8_t dat);
	
	//Write a byte to a register (returns exit status)
	bool writeRegister(uint8_t reg, uint8_t dat);
	//Read a byte from a register
	uint8_t readRegister(uint8_t reg);
	
	//From start register, read to or write from an array, n number of bytes
	//Return the exit status
	bool writeArray(uint8_t reg, uint8_t *, uint8_t n);
	bool readArray(uint8_t reg, uint8_t *, uint8_t n);
	
	/*** Transmission control *************************************************/
	void startI2C();
	void stopI2C();
	
	//Acknowledge modes. aliased to the actual value needed
	enum ACK_t : uint8_t{
		ACK = 0,
		NACK = 1
	};
	
	//Read a byte from bus, pass ACK type 
	uint8_t rxByte(ACK_t);
	bool txByte(uint8_t);
	
	private:	
	/*** Hardware pins and ports ***/
	uint8_t bm_SDA, bm_SCL; //ASM 'pin' aka bitmask
	volatile uint8_t *ir_SDA, *ir_SCL; //ASM INPUT port
	volatile uint8_t *dr_SDA, *dr_SCL; //ASM Direction Register
	
	/*** Enum & Macros ********************************************************/
	//Enum pin names, used for assertPin and releasePin
	enum i2c_pin {
		sda,
		scl
	};
	
	/*** Transmission control ***/
	void assertPin(i2c_pin); //Asserts a LOW to the bus
	void releasePin(i2c_pin); //Lets bus pullup to HIGH 
	
	bool rxBit();
	void txBit(bool);
	
	//Delay based on speed set. Can override (useconds). Default is unlimted
	void wait(int overrideDelay = 0); 
	
	//Terminates a protcol action, and prints a message if verbosity is enabled
	void protocolError(const char* = "");
	
	//Clock Stretching allows for processing time of the slave before 
	//receiving the bytes
	uint8_t clockStretch();
	
	/*** Configuration variables **********************************************/
	bool verbose = false;
	int delayMicros = 0; //Micros to delay during transmission. 
	uint8_t I2CAddress; //The address of the slave deivce from constructor
	
	//Keeps track of if the bus is active
	bool active = false;
};
#endif

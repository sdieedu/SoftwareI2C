//Still to-do
//Arbitration detection, if SDA or SCL is not what we expect after asserts
//conditional serial printing depending on settings (if serial.begin was called)
//Read to user-defined array - limiting + defined size + offset?
//ASM version of funstions to allow for higher speed

#include "Arduino.h"
#include "SoftwareI2C.h"

//Hardware related functions//
void SoftwareI2C::setup(uint8_t SDAPin, uint8_t SCLPin, uint8_t deviceAddress) {
	//Define the class pins and address
	_SDA = SDAPin;
	_SCL = SCLPin;
	I2CAddress = deviceAddress;
	
	//Set pins low so they can actively pull lines low
	digitalWrite(SDAPin, LOW);
	digitalWrite(SCLPin, LOW);	
	stopI2C(); //Ensure the I2C pins are in a known state
}

void SoftwareI2C::assertPin(uint8_t pin) {
	//Equivelant of digitalWrite LOW
	pinMode(pin, OUTPUT);
}

void SoftwareI2C::releasePin(uint8_t pin) {
	//Equivelant of digitalWrite HIGH
	pinMode(pin, INPUT);
}

void SoftwareI2C::setSpeed(int kHz) {
	//Get period (in seconds) 
	float period = 1 / (float)kHz;
	//*1000 to quickly get it into Microsecond range
	period *= 1000;
	//Round up to nearest int, then cast to int
	delayMicros = (int)ceil(period);
}

uint8_t SoftwareI2C::transmitError(uint8_t errorID, const char* errorMsg) {
	//Reset bus pins to idle state
	stopI2C();
	
	//Check if Serial has been initialised / is being used
	//TODO
	
	//If the string is not empty
	if(errorMsg[0] != '\0') {
		Serial.print("ERR " + (String)errorID + ": ");
		Serial.println(errorMsg);
	}
	
	return errorID;
}

uint8_t SoftwareI2C::clockStretch() {
	uint8_t clockWaits = 0;
	while(digitalRead(_SCL) == 0) {
		wait();
		++clockWaits;
		
		//Error state, upon 10 attempts
		if(clockWaits == 10) {
			Serial.println("error clk stretching");
			return 1;
		}
	}
	//Success State
	return 0;
}

void SoftwareI2C::listDevices() {
	bool response = 1;
	
	for(uint8_t cID = 0; cID < 128; ++cID) {
		startI2C();
		
		//Current ID shifted by 1 to alight to 7bit address
		response = txByte(cID << 1);
		
		stopI2C();
		
		//If device responds, print info
		if(response == 0) {
			Serial.print("Device resp: 0x");
			//Non shifted to match how the software accepts address'
			Serial.println(cID, HEX); 
		}
	}
}

void SoftwareI2C::wait(int delayOverride) {
	//Override option added for safety if stop and start become a problem later
	if(delayOverride == 0) {
		delayMicroseconds(delayMicros);
	} else {
		delay(delayOverride);
	}
}

void SoftwareI2C::startI2C() {
	//Start condition is defined by SDA going low while SCL is high
	//If repeat start is needed, first 
	if(active) {
		//SDA high
		releasePin(_SDA);
		//Slight delay
		wait();
		//SCL high
		releasePin(_SCL);
		//Exit if clock stretching fails
		if(clockStretch() != 0) return;
		//If success then delay
		wait();
	}
	//Do normal start
	//SDA low
	assertPin(_SDA);
	wait();
	//SCL low
	assertPin(_SCL); 
	
	active = true;
}

void SoftwareI2C::stopI2C() {
	//Stop condition is defined by SDA going high while SCL is high
	//SDA low
	assertPin(_SDA);
	wait();
	//SCL high
	releasePin(_SCL);
	//Exit if clock stretching fails
	if(clockStretch() != 0) return;
	//If success then delay
	wait();
	
	//Set SDA high while SCL is high
	releasePin(_SDA);
	wait();
	
	//If SDA is not what we expect, something went wrong
	active = false;
}

void SoftwareI2C::txBit(bool bit) {
	if(bit == 0) {
		assertPin(_SDA);
	} else {
		releasePin(_SDA);
	}
	
	//Toggle SCL pin
	wait();
	releasePin(_SCL);
	wait();
	
	//Exit if clock stretching fails
	if(clockStretch() != 0) return;
	//If success then delay
	wait();
	
	assertPin(_SCL);
}

bool SoftwareI2C::rxBit() {
	bool bit = 0;
	
	//Let the slave device drive the data line
	releasePin(_SDA);
	wait();
	
	//Set clock line high to request new bit
	releasePin(_SCL);
	
	//Exit if clock stretching fails
	if(clockStretch() != 0) return;
	//If success then delay
	wait();
	
	wait();
	
	bit = digitalRead(_SDA);
	//Set clock line low
	assertPin(_SCL);
	
	return bit;
}

bool SoftwareI2C::txByte(uint8_t byte) {
	//transmit bits from MSB First
	for(uint8_t bit = 0; bit < 8; ++bit) {
		//Transmit MSB 
		uint8_t cBit = (byte & 0x80) != 0;
		txBit(cBit);
		//Shift byte up by one
		byte = byte << 1;
	}
	
	bool ack = rxBit();
	return ack;
}

uint8_t SoftwareI2C::rxByte(bool nackBit) {
	uint8_t byte = 0;
	
	//Read bits MSB first
	for(uint8_t bit = 0; bit < 8; ++bit) {
		byte = (byte << 1) | rxBit();
	}
	
	//Write NACK bit. ACK (LOW) = ready for more data; NACK (HIGH) = stop sending
	txBit(nackBit);
	
	return byte;
}

uint8_t SoftwareI2C::writeCommand(uint8_t cmdAddr) {
	uint8_t deviceAddr = (I2CAddress << 1);
	
	startI2C();
	
	//I2C Address in Write mode
	if(txByte(deviceAddr)) {
		return transmitError(1, "Adr");
	}
	//Command Address
	if(txByte(cmdAddr)) {
		return transmitError(2, "Cmd");
	}
	
	stopI2C();
	
	return 0;
}

uint8_t SoftwareI2C::writeData(uint8_t regAddr, uint32_t data, uint8_t bytes) {
	uint8_t deviceAddr = (I2CAddress << 1);
	
	//Overflow protection
 	if(bytes > 4) bytes = 4;
	
	startI2C();
	
	//I2C Address in Write mode
	if(txByte(deviceAddr)) {
		return transmitError(1, "Adr");
	}
	//Register Address
	if(txByte(regAddr)) {
		return transmitError(2, "Reg");
	}
	
	uint8_t cByte = 0; //Current byte
	while(bytes >= 1) {
		//Current byte = the first byte of data
		cByte = (data & 0x000000FF);
		
		//Transmit that byte
		txByte(cByte);
		
		//Shift the data for the next round
		data = data >> 8;
		
		//Move to next byte
		--bytes;
	}
	
	stopI2C();
	
	return 0;
}

uint32_t SoftwareI2C::readData(uint8_t regAddr, uint8_t bytes) {
 	uint8_t deviceAddr = (I2CAddress << 1);
 	
 	//Overflow protection
 	if(bytes > 4) bytes = 4;
 	
	startI2C();
	
	//I2C Address in Write mode
	if(txByte(deviceAddr)) {
		return transmitError(1, "Adr");
	}
	//Register Address
	if(txByte(regAddr)) {
		return transmitError(2, "Reg");
	}
	
	//Repeat start
	startI2C();
	
	//I2C Address in Read mode
	deviceAddr |= 0x01;
	if(txByte(deviceAddr)) {
		return transmitError(3, "Adr-R");
	}
	
	uint32_t receivedData = 0;
	//NACK bit for signaling end of request
	bool NACK = 0;
	
	while(bytes >= 1) {
		//On the last byte, set NACK to 1 to signal EOR
		if(bytes == 1) {
			NACK = 1;
		}
		
		// Shift data before reading data in.   
		receivedData = receivedData << 8;
		
		//Read data in 
		receivedData |= rxByte(NACK);
		
		//Move to next byte
		--bytes;
	}
	
	stopI2C();
	
	return receivedData;
}

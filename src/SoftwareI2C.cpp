//Still to-do
//Arbitration detection, if SDA or SCL is not what we expect after asserts
//TODO

#include "Arduino.h"
#include "SoftwareI2C.h"

/*** Constructor **************************************************************/
SoftwareI2C::SoftwareI2C(uint8_t sda, uint8_t scl, uint8_t addr) {
	//Convert the arduino pin numbering into ASM variables
	bm_SDA = digitalPinToBitMask(sda); //Bit mask
	bm_SCL = digitalPinToBitMask(scl);
	////
	ir_SDA = portInputRegister(digitalPinToPort(sda)); //Input register
	ir_SCL = portInputRegister(digitalPinToPort(scl)); 
	////
	dr_SDA = portModeRegister(digitalPinToPort(sda)); //Direction Register
	dr_SCL = portModeRegister(digitalPinToPort(scl));
	
	//Set the internal address
	I2CAddress = addr;
	
	/* Set pins low so they can actively pull lines low. Do this via the passed
	   values to avoid creating another volatile set for SDA and SCL - output
	   register isn't used anywhere else */
	digitalWrite(sda, LOW);
	digitalWrite(scl, LOW);
	
	stopI2C(); //Ensure the I2C pins are in a known state
}

/*** Configuraiton ************************************************************/
void SoftwareI2C::setVerbosity(bool v) {
	this->verbose = v;
}

void SoftwareI2C::setSpeed(int kHz) {
	//Get period (in seconds) 
	float period = 1 / (float)kHz;
	//*1000 to quickly get it into Microsecond range
	period *= 1000;
	//Round up to nearest digit, then cast to int
	delayMicros = (int)ceil(period);
}

/*** Hardware Control *********************************************************/
void SoftwareI2C::assertPin(i2c_pin pin) {
	//Signal LOW - Get the Host to pull down the line (set mode to OUTPUT)
	if(pin == sda) *dr_SDA |= bm_SDA; //SDA OUTPUT
	if(pin == scl) *dr_SCL |= bm_SCL; //SCL OUTPUT
}

void SoftwareI2C::releasePin(i2c_pin pin) {
	//Signal HIGH - Release the Host pulldown (set mode to INPUT)
	if(pin == sda) *dr_SDA &= ~bm_SDA; //SDA INPUT
	if(pin == scl) *dr_SCL &= ~bm_SCL; //SCL INPUT
}

/*** Status control ***********************************************************/
void SoftwareI2C::protocolError(const char* errorMsg) {
	//Stop the current transmission and get to a blank state
	stopI2C();

	//Check if verbosity is enabled
	if(this->verbose) {
		//If the string is not empty
		if(errorMsg[0] != '\0') {
			Serial.println(errorMsg);
		}
	}
}

/*** Transmission Control *****************************************************/
void SoftwareI2C::txBit(bool bit) {
	if(bit == 0) {
		assertPin(sda);
	} else {
		releasePin(sda);
	}
	
	//Toggle SCL pin
	wait();
	releasePin(scl);
	wait();
	
	//Exit if clock stretching fails
	if(clockStretch() != 0) return;
	//If success then delay
	wait();
	
	assertPin(scl);
}

bool SoftwareI2C::rxBit() {
	bool bit = 0;
	
	//Let the slave device drive the data line
	releasePin(sda);
	wait();
	
	//Set clock line high to request new bit
	releasePin(scl);
	
	//Exit if clock stretching fails
	if(clockStretch() != 0) return 0;
	//If success then delay
	wait();
	
	//TODO why is there a wait laying here unused? arbitration?
	//wait();
	
	//read the SDA line and set bit accordingly
	if(*ir_SDA & bm_SDA) {
		bit = 1;
	} else {
		bit = 0;
	}
	
	assertPin(scl);
	
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

uint8_t SoftwareI2C::rxByte(ACK_t ack) {
	uint8_t byte = 0;
	
	//Read bits MSB first
	for(uint8_t bit = 0; bit < 8; ++bit) {
		byte = (byte << 1) | rxBit();
	}
	
	//Write ACK bit. ACK(LOW) = ready for more data; NACK(HIGH) = stop sending
	txBit(ack);
	return byte;
}

void SoftwareI2C::wait(int delayOverride) {
	//Override option added for safety if stop and start become a problem later
	if(delayOverride == 0) {
		delayMicroseconds(delayMicros);
	} else {
		delayMicroseconds(delayOverride);
	}
}

uint8_t SoftwareI2C::clockStretch() {
	uint8_t clockWaits = 0;
	
	//While SCL is being pulled LOW, wait
	while((*ir_SCL & bm_SCL) == 0) {
		wait();
		++clockWaits;
		
		//Error state, upon 10 attempts
		if(clockWaits == 10) {
			protocolError("Clock Stretch Error");
			return 1;
		}
	}
	//Success State
	return 0;
}

void SoftwareI2C::startI2C() {
	//Start condition is defined by SDA going low while SCL is high
	//If repeat start is needed, first do this 
	if(active) {
		//SDA high
		releasePin(sda);
		//Slight delay
		wait();
		//SCL high
		releasePin(scl);
		//Exit if clock stretching fails
		if(clockStretch() != 0) return;
		//If success then delay
		wait();
	}
	//Do normal start
	//SDA low
	assertPin(sda);
	wait();
	//SCL low
	assertPin(scl); 
	
	active = true;
}

void SoftwareI2C::stopI2C() {
	//Stop condition is defined by SDA going high while SCL is high
	//SDA low
	assertPin(sda);
	wait();
	//SCL high
	releasePin(scl);
	//Exit if clock stretching fails
	if(clockStretch() != 0) return;
	//If success then delay
	wait();
	
	//Set SDA high while SCL is high
	releasePin(sda);
	wait();
	
	//If SDA is not what we expect, something went wrong
	//TODO
	
	active = false;
}


/*** High level functions *****************************************************/
void SoftwareI2C::listDevices() {
	//If verbosity is disabled, just exit
	if(this->verbose == false) return;
	
	bool response;
	
	for(uint8_t cID = 1; cID < 128; ++cID) {
		startI2C();
		
		//Current ID shifted by 1 to alight to 7bit address
		response = txByte(cID << 1);
		
		stopI2C();
		
		//If device responds, print info
		if(response == 0) {
			Serial.print("Device resp: 0x");
			//Shift the cID to represent the real value
			Serial.println(cID << 1, HEX); 
		}
	}
}

bool SoftwareI2C::writeByte(uint8_t dat) {
	startI2C();
	
	//Send I2C Address in Write mode
	if(txByte(I2CAddress)) {
		protocolError("Adr");
		return 1;
	}
	
	//Send single byte
	if(txByte(dat)) {
		protocolError("Cmd");
		return 1;
	}
	
	stopI2C();
	
	return 0;
}

uint8_t SoftwareI2C::readRegister(uint8_t reg) { 	
	startI2C();
	
	//Send I2C Address in Write mode
	if(txByte(I2CAddress)) {
		protocolError("Adr");
		return 0;
	}
	//Send Register Byte
	if(txByte(reg)) {
		protocolError("Reg");
		return 0;
	}
	
	//Repeat start sequence
	startI2C();
	
	//Send I2C Address in Read mode
	if(txByte(I2CAddress | 0x01)) {
		protocolError("Adr-R");
		return 0;
	}
	
	//Read data in, Send NACK bit to end read request
	uint8_t receivedData = rxByte(NACK);

	stopI2C();
	
	return receivedData;
}

bool SoftwareI2C::writeRegister(uint8_t reg, uint8_t dat) { 	
	startI2C();
	
	//Send I2C Address in Write mode
	if(txByte(I2CAddress)) {
		protocolError("Adr");
		return 1;
	}
	//Send Register Byte
	if(txByte(reg)) {
		protocolError("Reg");
		return 1;
	}
	//Send Data Byte
	if(txByte(dat)) {
		protocolError("Reg");
		return 1;
	}
	
	stopI2C();
	
	//If successful, return 0
	return 0;
}

bool SoftwareI2C::readArray(uint8_t reg, uint8_t *arr, uint8_t n) {
	startI2C();
	
	//Send I2C Address in Write mode
	if(txByte(I2CAddress)) {
		protocolError("Adr");
		return 1;
	}
	//Send Register Byte
	if(txByte(reg)) {
		protocolError("Reg");
		return 1;
	}
	
	//Repeat start sequence
	startI2C();
	
	//Send I2C Address in Read mode
	if(txByte(I2CAddress | 0x01)) {
		protocolError("Adr-R");
		return 1;
	}
	
	//Acknowledge, will be switched at last byte read
	ACK_t readACK = ACK;
	
	//Copy read bytes to the array
	for(uint8_t b = 0; b < n; b++) {
		//If the current byte is the last to be read (n - 1), change ACK to NACK
		if(b == (n-1)) readACK = NACK;
		
		//Read data in, Send ACK or NACK.
		arr[b] = rxByte(readACK);
	}
	
	//End sequence
	stopI2C();
}

bool SoftwareI2C::writeArray(uint8_t reg, uint8_t *arr, uint8_t n) {
	startI2C();
	
	//Send I2C Address in Write mode
	if(txByte(I2CAddress)) {
		protocolError("Adr");
		return 1;
	}
	//Send Register Byte
	if(txByte(reg)) {
		protocolError("Reg");
		return 1;
	}
	
	//Copy read bytes to the array
	for(uint8_t b = 0; b < n; b++) {
		//Send data in the current array index.
		txByte(arr[b]);
	}
	
	//End sequence
	stopI2C();
}


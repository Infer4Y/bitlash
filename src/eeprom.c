/***
	eeprom.c:	minimal eeprom interface

	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2012 Bill Roy

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.

***/
#include "bitlash.h"

#if (defined(EEPROM_MICROCHIP_24XX32A)) || (defined(EEPROM_MICROCHIP_24XX256))

	#include "Wire.h"
	// A cache to speed up eeprom reads
	//uint8_t cache_eeprom[ENDEEPROM];


	// read a 32 byte page from eeprom
	// source: https://github.com/IngloriousEngineer/Arduino
	void extEEPROMreadPage(int EEPROM_addr, int addr, uint8_t* data_target, int amount, int offset)
	{
	  Wire.beginTransmission(EEPROM_addr);            
	  Wire.write(highByte(addr));                     
	  Wire.write(lowByte(addr));                      
	  Wire.endTransmission(true);                     
	  Wire.requestFrom(EEPROM_addr, amount, true);    
	  
	  while(Wire.available() == 0)
	  	{}                 
	  
	  for(int i = 0; i<amount; i++)
	    data_target[offset + i] = Wire.read();
	}  


	// initializes I2C bus and loads eeprom contents into cache
	void eeinit(void) {
		Wire.begin();
		
		//for(int offset=0; offset<=ENDEEPROM; offset+=32) {
		//extEEPROMreadPage(EEPROM_ADDRESS, offset, cache_eeprom, 32, offset); 
		//}
	}

	// write a single byte to eeprom
	// source: https://github.com/IngloriousEngineer/Arduino
	void eewrite(int addr, uint8_t value) { 
		
		// update cache first
		//cache_eeprom[addr] = value;

		// write back to eeprom
		Wire.beginTransmission(EEPROM_ADDRESS);
		Wire.write(highByte(addr));           
		Wire.write(lowByte(addr));            
		Wire.write((byte) value);             
		Wire.endTransmission(true);
		delay(6);  
	}

	//In order to favor small memspaces for large eeproms this'll do well.
	uint8_t eeread(int addr) { 
		Wire.beginTransmission(addr);            
	  	Wire.write(highByte(addr));                     
	  	Wire.write(lowByte(addr));                      
	  	Wire.endTransmission(true);                     
	  	Wire.requestFrom(EEPROM_ADDRESS, 1, true);  

		while(Wire.available() == 0)
	  	{} ; 
		
		return Wire.read();; //cache_eeprom[addr]; 
	}

#elif defined(EEPROM_MICROCHIP_25XX128)

	#define EDATAOUT 11//MOSI
	#define EDATAIN  12//MISO 
	#define ESPICLOCK  13//sck
	#define ESLAVESELECT 10//ss

	//opcodes
	#define EEWREN  6
	#define EEWRDI  4
	#define EERDSR  5
	#define EEWRSR  1
	#define EEREAD  3
	#define EEWRITE 2

	// A cache to speed up eeprom reads
	//uint8_t cache_eeprom[ENDEEPROM];

	byte spi_transfer(volatile byte data) {
		SPDR = data;                    // Start the transmission

		while (!(SPSR & (1<<SPIF)))     // Wait the end of the transmission
		{};

		return SPDR;                    // return the received byte
	}


	// read a 32 byte page from eeprom
	// source: https://www.arduino.cc/en/Tutorial/SPIEEPROM
	void extEEPROMreadPage(int EEPROM_addr, int addr, uint8_t* data_target, int amount, int offset) {
		digitalWrite(SLAVESELECT,LOW);

	  	for(int i = 0; i<amount; i++) {
			spi_transfer(READ); //transmit read opcode

			spi_transfer(highByte((EEPROM_addr+i));   //send MSByte address first

			spi_transfer(lowByte(EEPROM_addr+i));      //send LSByte address
	    	data_target[offset + i] = spi_transfer(0xFF); //get data byte
		
		}

		digitalWrite(SLAVESELECT,HIGH); //release chip, signal end transfer  
	}  


	// initializes SPI bus and loads eeprom contents into cache
	void eeinit(void) {
		pinMode(DATAOUT, OUTPUT);

  		pinMode(DATAIN, INPUT);

  		pinMode(SPICLOCK,OUTPUT);

		pinMode(SLAVESELECT,OUTPUT);

		digitalWrite(SLAVESELECT,HIGH);
		
		for(int offset=0; offset<=ENDEEPROM; offset+=32) {
			extEEPROMreadPage(EEPROM_ADDRESS, offset, cache_eeprom, 32, offset); 
		}
	}

	// write a single byte to eeprom
	// source: https://www.arduino.cc/en/Tutorial/SPIEEPROM
	void eewrite(int addr, uint8_t value) { 
		
		// update cache first
		cache_eeprom[addr] = value;

		digitalWrite(SLAVESELECT,LOW);
  		spi_transfer(WREN); //write enable
  		digitalWrite(SLAVESELECT,HIGH);

  		delay(10);

  		digitalWrite(SLAVESELECT,LOW);
  		spi_transfer(WRITE); //write instruction
		spi_transfer(highByte(addr>>8));   //send MSByte address first
  		spi_transfer(lowByte(addr));      //send LSByte address

		spi_transfer((char)(value));

		digitalWrite(SLAVESELECT,HIGH); //release chip, signal end transfer 
		delay(6);  
	}

	uint8_t eeread(int addr) { 
		digitalWrite(SLAVESELECT,LOW);
		spi_transfer(READ); //transmit read opcode

		spi_transfer(highByte((EEPROM_addr+i));   //send MSByte address first

		spi_transfer(lowByte(EEPROM_addr+i));      //send LSByte address

		uint8_t value = spi_transfer(0xFF);
		
  		digitalWrite(SLAVESELECT,HIGH);

		return value; 
	}

#elif (defined(AVR_BUILD)) || ( (defined(ARM_BUILD)) && (ARM_BUILD==2))
	// AVR or Teensy 3
	#include "avr/eeprom.h"
	void eewrite(int addr, uint8_t value) { eeprom_write_byte((unsigned char *) addr, value); }
	uint8_t eeread(int addr) { return eeprom_read_byte((unsigned char *) addr); }
	#if defined(ARM_BUILD)
		// Initialize Teensy 3 eeprom
		void eeinit(void) { eeprom_initialize(); }
	#endif
#elif defined(ARM_BUILD)
	#if ARM_BUILD!=2
		// A little fake eeprom for ARM testing
		char virtual_eeprom[E2END];

		void eeinit(void) {
			for (int i=0; i<E2END; i++) virtual_eeprom[i] = 255;
		}

		void eewrite(int addr, uint8_t value) { virtual_eeprom[addr] = value; }
		uint8_t eeread(int addr) { return virtual_eeprom[addr]; }
	#endif
#endif

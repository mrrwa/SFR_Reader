#ifndef SFR_READER_INCLUDED
#define SFR_READER_INCLUDED

#include "Wire.h"

/*  This library provides a class object to use with the Starfish Rail I2C RFID modules
 *  Up to 4 readers can be connected (limited by I2C bus address range for these devices)
 *  Avoids 'blocking' caused by 'delay' using a timer and a flag for each instance and can scan 4 
 *  devices at an interval of 10 to 16mS when using a minimum wait time of 8mS
 *  
 *  3 calls for each device are needed
 *  
 *  Create the class object
 *
 *  	detector1(i2cBaseAdd);
 *  
 *  In setup function
 *  Initalise I2C RFID readers
 *
 *  	detector1.init ();
 *  
 *  In the loop function
 *  Call for a Scan of the i2c RFID readers
 *
 *  	detector1.scan();
 *
 *  To get an ASCII String of the UID
 *
 *  	detector1.srtUID();
 *
 *  Copyright (C) 2022 MGB
 *  Copyright (C) 2022 Martin Snashall
 *  Copyright (C) 2024 Alex Shepherd
 *
 * 	This library is free software; you can redistribute it and/or
 * 	modify it under the terms of the GNU Lesser General Public
 * 	License as published by the Free Software Foundation; either
 * 	version 2.1 of the License, or (at your option) any later version.
 *
 * 	This library is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * 	Lesser General Public License for more details.
 *
 * 	You should have received a copy of the GNU Lesser General Public
 * 	License along with this library; if not, write to the Free Software
 * 	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 */

// #define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
 #define DebugPrint(...)	Serial.print(__VA_ARGS__)
 #define DebugPrintln(...)	Serial.println(__VA_ARGS__)
#else 
 #define DebugPrint(...)
 #define DebugPrintln(...)
#endif

// This enumeration represents the 4 different status code returned by the scan() method
enum SRF_Read_Status { SFR_INIT, 
                       SFR_NO_TAG,
                       SFR_TAG_EXIT,
                       SFR_TAG_ENTER,
                       SFR_TAG_REPEAT};

class SFR_Reader {
  int i2cAdd;             // Reader I2C Address
  byte IRQ0;              // IRQ0 Status Bits
  byte lenFIFO;           // Number of bytes in the FIFO register
  int inFIFO[9];          // Array to hold bytes read from FIFO
  int prevFIFO[9];        // Array to hold the previous bytes read from FIFO
  byte errorFlags;        // Byte with value read from the Error Flages register
  byte led = 0;           // Value to be written to the LED control register
  char UIDStrBuffer[20];  // Local string buffer for Str conversions
  SRF_Read_Status UID_Status = SFR_INIT;
  bool waiting = false;               // TRUE while waiting for tag to read - Waiting time is normally in the range 8 to 12 mS
  uint8_t noReadCount = 0;				  
  unsigned long startMillis = 0;      // start time of wait
  unsigned long waitMillis = 12;      // time to wait for a tag to come in range and be read, in mS
  unsigned long loopstartMillis = 0;  // Timing check
  unsigned long loopMillis = 0;
  
  TwoWire* i2cInterface;  // I2C Interface of the Arduino to use

  PROGMEM const char* SFR_INIT_Str PROGMEM = "Initial";
  PROGMEM const char* SFR_NO_TAG_Str PROGMEM = "No Tag";
  PROGMEM const char* SFR_TAG_EXIT_Str PROGMEM = "Tag Exit";
  PROGMEM const char* SFR_TAG_ENTER_Str PROGMEM = "Tag Enter";
  PROGMEM const char* SFR_TAG_REPEAT_Str PROGMEM = "Tag Repeat";
  PROGMEM const char* SRF_Read_Status_Str[5] = { SFR_INIT_Str, SFR_NO_TAG_Str, SFR_TAG_EXIT_Str, SFR_TAG_ENTER_Str, SFR_TAG_REPEAT_Str };

  // Constructor - creates a RFID scan object
  // and initializes the member variables and state
public:
  SFR_Reader(int i2cAddress)  // *** INITALISE class object ***
  {
    i2cAdd = i2cAddress;
  }

  void init(TwoWire* newi2cInterface)  // *** Setup RFID registers ***
  {
    i2cInterface = newi2cInterface;

    // Soft Reset
    i2cInterface->beginTransmission(i2cAdd);
    i2cInterface->write(0x00);
    i2cInterface->write(0x1F);
    i2cInterface->endTransmission();

    //FIFO Control
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x02);                // device address is specified in datasheet
    i2cInterface->write(0xb0);                // sends value byte
    i2cInterface->endTransmission();

    //FIFO Data and IRQ0 Control
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x05);                // device address is specified in datasheet
    i2cInterface->write(0x00);                // sends value byte
    i2cInterface->write(0x00);                // sends value byte
    i2cInterface->endTransmission();

    //Command
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x00);                // device address is specified in datasheet
    i2cInterface->write(0x0d);                // sends value byte
    i2cInterface->endTransmission();

    //Clear FIFO
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x02);                // device address is specified in datasheet
    i2cInterface->write(0xb0);                // sends value byte
    i2cInterface->endTransmission();

    //  Configure RF mode
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x05);                // device address is specified in datasheet
    i2cInterface->write(0x01);                // sends value byte
    i2cInterface->write(0x94);                // new
    i2cInterface->write(0x28);                // new
    i2cInterface->write(0x11);                // new
    i2cInterface->endTransmission();

    // Load Registers
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x00);                // device address is specified in datasheet
    i2cInterface->write(0x0c);                // sends value byte
    i2cInterface->endTransmission();
    delay(1);

    // Set RF on part 1
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x00);                // device address is specified in datasheet
    i2cInterface->write(0x06);                // sends value byte
    i2cInterface->endTransmission();

    // Set RF on part 2
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x28);                // device address is specified in datasheet
    i2cInterface->write(0x8f);                // sends value byte
    i2cInterface->endTransmission();

    // Set Sensitivity
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x37);                // device address is specified in datasheet
    i2cInterface->write(0x14);                // sends value byte
    i2cInterface->endTransmission();

    // Set pin 22 to output for indicator LED and turn it OFF
    i2cInterface->beginTransmission(i2cAdd);  // transmit to device
    i2cInterface->write(0x44);                // device address is specified in datasheet
    i2cInterface->write(0x40);                // sends value byte
    i2cInterface->endTransmission();

    led = 0x40;  //led=0x40= OFF
    i2cInterface->beginTransmission(i2cAdd);
    i2cInterface->write(0x45);
    i2cInterface->write(led);  // set LED OFF
    i2cInterface->endTransmission();
  }  // *** END INITALISE ***

  SRF_Read_Status scan()  // Scan for RFID tag
  {
#ifdef ENABLE_DEBUG
    DebugPrint("scan() waiting: ");
    DebugPrint(waiting);
    DebugPrint("  millis: ");
    DebugPrintln(millis());
#endif
    // Wait time has passed
    if (!waiting)  // Not waiting for the timer to expire so initalise a scan
    {
      loopstartMillis = millis();  // Loop timer
      // Flush FIFO
      i2cInterface->beginTransmission(i2cAdd);  // transmit to device
      i2cInterface->write(0x02);                // device address is specified in datasheet
      i2cInterface->write(0xb0);                // sends value byte  was 0x80, changed to 0xb0
      i2cInterface->endTransmission();

      // Write FIFO with ASK UID
      i2cInterface->beginTransmission(i2cAdd);  // transmit to device
      i2cInterface->write(0x05);                // device address is specified in datasheet
      i2cInterface->write(0x76);                // sends value byte
      i2cInterface->write(0xa0);                // sends value byte
      i2cInterface->write(0x04);                // sends value byte
      i2cInterface->write(0x00);                // sends value byte
      i2cInterface->write(0x00);                // sends value byte
      i2cInterface->write(0x00);                // sends value byte
      i2cInterface->write(0x00);                // sends value byte
      i2cInterface->endTransmission();

      // Set to transceive
      i2cInterface->beginTransmission(i2cAdd);  // transmit to device
      i2cInterface->write(0x00);                // device address is specified in datasheet
      i2cInterface->write(0x07);                // sends value byte
      i2cInterface->endTransmission();

      startMillis = millis();  // set the start time to current millis value
      waiting = true;
    }

    else if (waiting)
    {
      if (millis() < (startMillis + waitMillis))  // Wait time has passed
      {
#ifdef ENABLE_DEBUG
        //Read register 06 : IRQ0 Status
        i2cInterface->beginTransmission(i2cAdd);
        i2cInterface->write(0x06);
        i2cInterface->requestFrom(i2cAdd, 1);
        IRQ0 = i2cInterface->read();
        i2cInterface->endTransmission();

        DebugPrint("IRQ0: ");
        DebugPrint(IRQ0, HEX);
        DebugPrint(" ");
        DebugPrintln(IRQ0, BIN);
#endif
      } 
      else
      {
        waiting = false;

        // Dummy - Read register 04 : length of data in FIFO
        i2cInterface->beginTransmission(i2cAdd);
        i2cInterface->write(0x04);
        i2cInterface->endTransmission();

        //Read register 04 : length of data in FIFO
        i2cInterface->beginTransmission(i2cAdd);
        i2cInterface->write(0x04);
        i2cInterface->requestFrom(i2cAdd, 1);
        lenFIFO = i2cInterface->read();
        i2cInterface->endTransmission();

		if(lenFIFO)
		{
          // Dummy - Read values from FIFO
          i2cInterface->beginTransmission(i2cAdd);
          i2cInterface->write(0x05);
          i2cInterface->endTransmission();

          // Read values from FIFO
          i2cInterface->beginTransmission(i2cAdd);
          i2cInterface->write(0x05);
          i2cInterface->requestFrom(i2cAdd, 9);  // read the byte values from FIFO
          int byteCounter = 0;                   // Index for array of bytes read from FIFO
          while (i2cInterface->available() && (byteCounter < 9))      //
          {
            inFIFO[byteCounter] = i2cInterface->read();  // Capture received byte
            byteCounter++;                               // Increment index counter
          }
		}

        // Read ERROR flags - register 0a
        i2cInterface->beginTransmission(i2cAdd);
        i2cInterface->write(0x0a);
        i2cInterface->requestFrom(i2cAdd, 1);  // read the current GPIO output latches
        errorFlags = i2cInterface->read();     // receive a byte as character

#ifdef ENABLE_DEBUG
        DebugPrint("errorFlags: ");
        DebugPrint(errorFlags);
        DebugPrint("  lenFIFO: ");
        DebugPrintln(lenFIFO);
#endif

        // Add missing Command identified by Olaf Krahmer
        i2cInterface->beginTransmission(i2cAdd);
        i2cInterface->write(0x02);
        i2cInterface->write(0xFF);
        i2cInterface->write(0x00);
        i2cInterface->endTransmission();

        // Work out STATUS
        if (errorFlags == 0)
        {
          if(lenFIFO == 13)  // Valid tag detected
		  {
			if (memcmp(inFIFO, prevFIFO, sizeof(inFIFO)) == 0)
			{
			  UID_Status = SFR_TAG_REPEAT;
			}  // Valid tag but same UID as last time
	
			else
			{
			  UID_Status = SFR_TAG_ENTER;  // Valid tag but different UID
			  memcpy(prevFIFO, inFIFO, sizeof(inFIFO));
			}
            noReadCount = 0;
          }
          else   // Invalid or no tag present
          {
          	if(++noReadCount >= 2)
          	{
              if (UID_Status > SFR_TAG_EXIT)
              {
                UID_Status = SFR_TAG_EXIT;
              }
              else
              {
                UID_Status = SFR_NO_TAG;
                memset(inFIFO, 0, sizeof(inFIFO));
                memset(prevFIFO, 0, sizeof(prevFIFO));
              }
            }
          }
        }
        
        // Set LED ON / OFF
        if ((errorFlags == 0) && (lenFIFO == 13)) // Valid tag detected)
        {
          led = 0x00;                            //led=0x00= ON
        }
        else
        {
          led = 0x40;  //led=0x40= OFF
        }

        i2cInterface->beginTransmission(i2cAdd);
        i2cInterface->write(0x45);
        i2cInterface->write(led);  // set LED ON or OFF as required
        i2cInterface->endTransmission();
      }

      loopMillis = millis() - loopstartMillis;  // Loop timing check
    }                                           // still waiting for the timer to expire so do nothing.

    return UID_Status;
  }

  char* strUID() {
    int chrIndex = 0;
    int nibble[2];
    char chr_nibble;
    for (int i = 8; i > 0; i--) {
      div_t divresult;
      divresult = div(inFIFO[i], 16);
      nibble[0] = divresult.quot;  // Get most significant nibble
      nibble[1] = divresult.rem;   // Get least significant nibble

      for (int nib = 0; nib < 2; nib++) {
        if (nibble[nib] < 10) { chr_nibble = 48 + nibble[nib]; }  // 0 to 9 add to ASCII 0
        else {
          chr_nibble = 55 + nibble[nib];
        }  // A TO F, add to 55-ASCII 7 saves doing -10

        UIDStrBuffer[chrIndex++] = chr_nibble;
      }
    }

    UIDStrBuffer[chrIndex] = 0;

    return UIDStrBuffer;
  }

  const char binToHexAscii[17] PROGMEM = {"0123456789ABCDEF"};	


  char* strMERG() {
    int chrIndex = 0;
    int nibble[2];
    char chr_nibble;

    for (int i = 5; i > 0; i--) {
      uint8_t byteVal = inFIFO[i];
      UIDStrBuffer[chrIndex++] = binToHexAscii[byteVal >> 4];
      UIDStrBuffer[chrIndex++] = binToHexAscii[byteVal & 0x0F];
    }

	// Calculate the Checksum
    uint8_t checksum = inFIFO[5];
    for (int i = 4; i > 0; i--)
    	checksum ^= inFIFO[i];

    UIDStrBuffer[chrIndex++] = binToHexAscii[checksum >> 4];
    UIDStrBuffer[chrIndex++] = binToHexAscii[checksum & 0x0F];

    UIDStrBuffer[chrIndex] = 0;

    return UIDStrBuffer;
  }

  const char* StatusStr(void) {
    return StatusStr(UID_Status);
  }

  const char* StatusStr(SRF_Read_Status Status) {
    return SRF_Read_Status_Str[Status];
  }
};

#endif
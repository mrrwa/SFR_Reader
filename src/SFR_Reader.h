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

// This enumeration represents the 5 different status code returned by the scan() method
enum SRF_Read_Status { SFR_INIT,
                       SFR_NO_TAG,
                       SFR_TAG_EXIT,
                       SFR_TAG_ENTER,
                       SFR_TAG_REPEAT
                     };

class SFR_Reader
{
private:
    uint8_t i2cAdd;             // Reader I2C Address
    uint8_t IRQ0;              // IRQ0 Status Bits
    uint8_t lenFIFO;           // Number of bytes in the FIFO register
    uint8_t inFIFO[9];          // Array to hold bytes read from FIFO
    uint8_t prevFIFO[9];        // Array to hold the previous bytes read from FIFO
    uint8_t errorFlags;        // Byte with value read from the Error Flages register
    uint8_t led = 0;           // Value to be written to the LED control register
    char UIDStrBuffer[20];  // Local string buffer for Str conversions
    SRF_Read_Status UID_Status = SFR_INIT;
    bool waiting = false;               // TRUE while waiting for tag to read - Waiting time is normally in the range 8 to 12 mS
    uint8_t noReadCount = 0;
    unsigned long startMillis = 0;      // start time of wait
    unsigned long waitMillis = 12;      // time to wait for a tag to come in range and be read, in mS
    unsigned long loopstartMillis = 0;  // Timing check
    unsigned long loopMillis = 0;

    PROGMEM const char* SFR_INIT_Str PROGMEM = "Initial";
    PROGMEM const char* SFR_NO_TAG_Str PROGMEM = "No Tag";
    PROGMEM const char* SFR_TAG_EXIT_Str PROGMEM = "Tag Exit";
    PROGMEM const char* SFR_TAG_ENTER_Str PROGMEM = "Tag Enter";
    PROGMEM const char* SFR_TAG_REPEAT_Str PROGMEM = "Tag Repeat";
    PROGMEM const char* SRF_Read_Status_Str[5] = { SFR_INIT_Str, SFR_NO_TAG_Str, SFR_TAG_EXIT_Str, SFR_TAG_ENTER_Str, SFR_TAG_REPEAT_Str };
    const char binToHexAscii[17] PROGMEM = {"0123456789ABCDEF"};

public:
    SFR_Reader (int i2cAddress);
    void init (TwoWire* i2cInterface);
    SRF_Read_Status scan (TwoWire* i2cInterface);
    char* strUID();
    char* strMERG();
    const char* StatusStr (void);
    const char* StatusStr (SRF_Read_Status Status);
};

#define MAX_READERS 4   // Maximum Number of I2C RFID Readers

class SFR_ReaderGroup
{
    using ReaderEventHandler = void (*) (int readerGroupId, int readerId, SRF_Read_Status readStatus, SFR_Reader * pReader);

public:
    SFR_ReaderGroup (TwoWire* twiInterface);
    void registerReaderEventHandler (ReaderEventHandler inReaderEventHandler);
    void begin (uint32_t clockFrequency = 200000);
    uint8_t scanForReaders (void);
    void process();

private:
    TwoWire* i2cInterface;  // I2C Interface
    uint8_t	i2cBaseAdd = 0x28;  // Base Address of I2C RFID Readers.
    uint8_t	numReaders = 0;
    uint8_t	readerIndex = 0;

    uint8_t	groupId;
    static uint8_t instanceNum;

    SFR_Reader * readers[MAX_READERS];
    SRF_Read_Status lastStatus[MAX_READERS];

    ReaderEventHandler readerEventHandler;
};

#endif
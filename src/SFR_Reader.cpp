#include "SFR_Reader.h"

/*
 *****************************************************************************
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

// #define ENABLE_SCANNING_DEBUG
// #define ENABLE_STATUS_DEBUG
// #define ENABLE_ERROR_LEN_DEBUG
// #define ENABLE_READER_SCAN_DEBUG
// #define ENABLE_HANDLER_DEBUG

#if defined(ENABLE_SCANNING_DEBUG) || defined(ENABLE_STATUS_DEBUG) || defined(ENABLE_ERROR_LEN_DEBUG) || defined(ENABLE_READER_SCAN_DEBUG) || defined(ENABLE_HANDLER_DEBUG)
    #define ENABLE_DEBUG
#endif

#ifdef ENABLE_DEBUG
    #define DebugPrint(...)	Serial.print(__VA_ARGS__)
    #define DebugPrintln(...)	Serial.println(__VA_ARGS__)
#else
    #define DebugPrint(...)
    #define DebugPrintln(...)
#endif

SFR_Reader::SFR_Reader (int i2cAddress) // *** INITALISE class object ***
{
    i2cAdd = i2cAddress;
}

void SFR_Reader::init (TwoWire* i2cInterface) // *** Setup RFID registers ***
{
    // Soft Reset
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x00);
    i2cInterface->write (0x1F);
    i2cInterface->endTransmission();

    //FIFO Control
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x02);
    i2cInterface->write (0xb0);
    i2cInterface->endTransmission();

    //FIFO Data and IRQ0 Control
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x05);
    i2cInterface->write (0x00);
    i2cInterface->write (0x00);
    i2cInterface->endTransmission();

    //Command
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x00);
    i2cInterface->write (0x0d);
    i2cInterface->endTransmission();

    //Clear FIFO
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x02);
    i2cInterface->write (0xb0);
    i2cInterface->endTransmission();

    //  Configure RF mode
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x05);
    i2cInterface->write (0x01);
    i2cInterface->write (0x94);
    i2cInterface->write (0x28);
    i2cInterface->write (0x11);
    i2cInterface->endTransmission();

    // Load Registers
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x00);
    i2cInterface->write (0x0c);
    i2cInterface->endTransmission();
    delay (1);

    // Set RF on part 1
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x00);
    i2cInterface->write (0x06);
    i2cInterface->endTransmission();

    // Set RF on part 2
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x28);
    i2cInterface->write (0x8f);
    i2cInterface->endTransmission();

    // Set Sensitivity
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x37);
    i2cInterface->write (0x14);
    i2cInterface->endTransmission();

    // Set pin 22 to output for indicator LED and turn it OFF
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x44);
    i2cInterface->write (0x40);
    i2cInterface->endTransmission();

    led = 0x40;  //led=0x40= OFF
    i2cInterface->beginTransmission (i2cAdd);
    i2cInterface->write (0x45);
    i2cInterface->write (led); // set LED OFF
    i2cInterface->endTransmission();
}  // *** END INITALISE ***

SRF_Read_Status SFR_Reader::scan (TwoWire* i2cInterface) // Scan for RFID tag
{
    #ifdef ENABLE_SCANNING_DEBUG
    DebugPrint ("scan() waiting: ");
    DebugPrint (waiting);
    DebugPrint ("  millis: ");
    DebugPrintln (millis());
    #endif
    // Wait time has passed
    if (!waiting)  // Not waiting for the timer to expire so initalise a scan
    {
        loopstartMillis = millis();  // Loop timer
        // Flush FIFO
        i2cInterface->beginTransmission (i2cAdd);
        i2cInterface->write (0x02);
        i2cInterface->write (0xb0);
        i2cInterface->endTransmission();

        // Write FIFO with ASK UID
        i2cInterface->beginTransmission (i2cAdd);
        i2cInterface->write (0x05);
        i2cInterface->write (0x76);
        i2cInterface->write (0xa0);
        i2cInterface->write (0x04);
        i2cInterface->write (0x00);
        i2cInterface->write (0x00);
        i2cInterface->write (0x00);
        i2cInterface->write (0x00);
        i2cInterface->endTransmission();

        // Set to transceive
        i2cInterface->beginTransmission (i2cAdd);
        i2cInterface->write (0x00);
        i2cInterface->write (0x07);
        i2cInterface->endTransmission();

        startMillis = millis();  // set the start time to current millis value
        waiting = true;
    }

    else if (waiting)
    {
        if (millis() < (startMillis + waitMillis))  // Wait time has passed
        {
            #ifdef ENABLE_STATUS_DEBUG
            //Read register 06 : IRQ0 Status
            i2cInterface->beginTransmission (i2cAdd);
            i2cInterface->write (0x06);
            i2cInterface->endTransmission();
            i2cInterface->requestFrom (i2cAdd, 1);
            IRQ0 = i2cInterface->read();

            DebugPrint ("IRQ0: ");
            DebugPrint (IRQ0, HEX);
            DebugPrint (" ");
            DebugPrintln (IRQ0, BIN);
            #endif
        }
        else
        {
            waiting = false;

            //Read register 04 : length of data in FIFO
            i2cInterface->beginTransmission (i2cAdd);
            i2cInterface->write (0x04);
            i2cInterface->endTransmission();

            i2cInterface->requestFrom (i2cAdd, 1);
            lenFIFO = i2cInterface->read();

            if (lenFIFO)
            {
                // Read values from FIFO
                i2cInterface->beginTransmission (i2cAdd);
                i2cInterface->write (0x05);
                i2cInterface->endTransmission();

                i2cInterface->requestFrom (i2cAdd, 9); // read the byte values from FIFO
                int byteCounter = 0;                   // Index for array of bytes read from FIFO
                while (i2cInterface->available() && (byteCounter < 9))      //
                {
                    inFIFO[byteCounter] = i2cInterface->read();  // Capture received byte
                    byteCounter++;                               // Increment index counter
                }
            }

            // Read ERROR flags - register 0a
            i2cInterface->beginTransmission (i2cAdd);
            i2cInterface->write (0x0a);
            i2cInterface->endTransmission();

            i2cInterface->requestFrom (i2cAdd, 1); // read the current GPIO output latches
            errorFlags = i2cInterface->read();     // receive a byte as character

            #ifdef ENABLE_ERROR_LEN_DEBUG
            DebugPrint ("errorFlags: ");
            DebugPrint (errorFlags);
            DebugPrint ("  lenFIFO: ");
            DebugPrintln (lenFIFO);
            #endif

            // Work out STATUS
            if (errorFlags == 0)
            {
                if (lenFIFO == 13) // Valid tag detected
                {
                    if (memcmp (inFIFO, prevFIFO, sizeof (inFIFO)) == 0)
                    {
                        UID_Status = SFR_TAG_REPEAT;
                    }  // Valid tag but same UID as last time

                    else
                    {
                        UID_Status = SFR_TAG_ENTER;  // Valid tag but different UID
                        memcpy (prevFIFO, inFIFO, sizeof (inFIFO));
                    }
                    noReadCount = 0;
                }
                else   // Invalid or no tag present
                {
                    if (++noReadCount >= 2)
                    {
                        if (UID_Status > SFR_TAG_EXIT)
                        {
                            memset (inFIFO, 0, sizeof (inFIFO));
                            UID_Status = SFR_TAG_EXIT;
                        }
                        else
                        {
                            UID_Status = SFR_NO_TAG;
                            memset (inFIFO, 0, sizeof (inFIFO));
                            memset (prevFIFO, 0, sizeof (prevFIFO));
                        }
                    }
                }
            }

            // Set LED ON / OFF
            if ( (errorFlags == 0) && (lenFIFO == 13)) // Valid tag detected)
            {
                led = 0x00;                            //led=0x00= ON
            }
            else
            {
                led = 0x40;  //led=0x40= OFF
            }

            i2cInterface->beginTransmission (i2cAdd);
            i2cInterface->write (0x45);
            i2cInterface->write (led); // set LED ON or OFF as required
            i2cInterface->endTransmission();
        }

        loopMillis = millis() - loopstartMillis;  // Loop timing check
    }                                           // still waiting for the timer to expire so do nothing.

    return UID_Status;
}

char* SFR_Reader::strUID()
{
    int chrIndex = 0;
    int nibble[2];
    char chr_nibble;
    for (int i = 8; i > 0; i--)
    {
        div_t divresult;
        divresult = div (inFIFO[i], 16);
        nibble[0] = divresult.quot;  // Get most significant nibble
        nibble[1] = divresult.rem;   // Get least significant nibble

        for (int nib = 0; nib < 2; nib++)
        {
            if (nibble[nib] < 10)
            {
                chr_nibble = 48 + nibble[nib];    // 0 to 9 add to ASCII 0
            }
            else
            {
                chr_nibble = 55 + nibble[nib];
            }  // A TO F, add to 55-ASCII 7 saves doing -10

            UIDStrBuffer[chrIndex++] = chr_nibble;
        }
    }

    UIDStrBuffer[chrIndex] = 0;

    return UIDStrBuffer;
}

char* SFR_Reader::strMERG()
{
    int chrIndex = 0;
    int nibble[2];
    char chr_nibble;

    for (int i = 5; i > 0; i--)
    {
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

const char* SFR_Reader::StatusStr (void)
{
    return StatusStr (UID_Status);
}

const char* SFR_Reader::StatusStr (SRF_Read_Status Status)
{
    return SRF_Read_Status_Str[Status];
}

uint8_t SFR_ReaderGroup::instanceNum = 0;

SFR_ReaderGroup::SFR_ReaderGroup (TwoWire* twiInterface)
{
    groupId = instanceNum++;
    i2cInterface = twiInterface;
}

void SFR_ReaderGroup::registerReaderEventHandler (ReaderEventHandler inReaderEventHandler)
{
    readerEventHandler = inReaderEventHandler;
}

void SFR_ReaderGroup::begin (uint32_t clockFrequency)
{
    i2cInterface->begin ();
    i2cInterface->setClock (clockFrequency);
}

uint8_t SFR_ReaderGroup::scanForReaders (void)
{
    numReaders = 0;

    for (readerIndex = 0; readerIndex < MAX_READERS; readerIndex++)
    {
        int i2cAddr = i2cBaseAdd + readerIndex;

        #ifdef ENABLE_READER_SCAN_DEBUG
        DebugPrint ("Detecting Reader at address: ");
        DebugPrint (i2cAddr, HEX);
        #endif
        i2cInterface->beginTransmission (i2cAddr);
        uint8_t error = i2cInterface->endTransmission();

        if (error)
        {
            #ifdef ENABLE_READER_SCAN_DEBUG
            DebugPrintln (" Not Found");
            #endif
        }

        else
        {
            #ifdef ENABLE_READER_SCAN_DEBUG
            DebugPrintln (" Found");
            #endif

            readers[numReaders] = new SFR_Reader (i2cAddr); // index with numReaders instead of readerIndex to avoid a crash
            readers[numReaders]->init (i2cInterface);
            numReaders++;
        }
    }

    readerIndex = 0;

    return numReaders;
}

void SFR_ReaderGroup::process()
{
    if (readerIndex >= numReaders)
        readerIndex = 0;

    SRF_Read_Status status = readers[readerIndex]->scan (i2cInterface);
    if ( (status > SFR_INIT) && (lastStatus[readerIndex] != status))
    {
        #ifdef ENABLE_HANDLER_DEBUG
        DebugPrint ("scanForTags: readerIndex: ");
        DebugPrint (readerIndex);
        DebugPrint ("  status: ");
        DebugPrint (status);
        DebugPrint ("  EID: ");
        DebugPrintln (readers[readerIndex]->strUID());
        #endif

        lastStatus[readerIndex] = status;
        if (readerEventHandler)
            readerEventHandler (groupId, readerIndex, status, readers[readerIndex]);
    }

    readerIndex++;
}

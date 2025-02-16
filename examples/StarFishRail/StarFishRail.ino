/*  This code provides an example of how to use the Starfish Rail I2C RFID Reader Library
 *  Up to 4 readers can be connected (limited by i2c bus address range for these devices)
 *  Avoids 'blocking' caused by 'delay' using a timer and a flag for each instance and can scan 4 readers
 *  As each reader state changes, the SFR_Reader library call the Event Handler, providing access to the current state
 *
 *  This example prints the current state of the reader, including any EID tag identifier string to the serial monitor 
 */

// Uncomment the line below to enable monitor output for debugging purposes
#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
#define DebugPrint(...) Serial.print(__VA_ARGS__)
#define DebugPrintln(...) Serial.println(__VA_ARGS__)
#else
#define DebugPrint(...)
#define DebugPrintln(...)
#endif

#include <SFR_Reader.h>

// The SFR_ReaderGroup object takes the Arduino TwoWire (I2C) interface as a parameter and manages all the SFR_Reader objects for eacho f the readers connected to the I2C bus.
SFR_ReaderGroup tagReaders = SFR_ReaderGroup(&Wire);

// The readerEventHandler function below is an Event Handler that is called when the state of any SFR_Reader changes. Typically this is when an EID tag is detected as coming in/out of range
// The paramters passed into the EventHandler are:
// - readerGroupId: an incrementing value starting at 0, for each instance of the SFR_ReaderGroup object, for each TwoWire (I2C) Bus.
// - readerId: an index value starting at 0, for each reader detected on the I2c Bus
// - readStatus: a SRF_Read_Status enumeration of the current state of the reader
// - pReader: a pointer to the SFR_Reader object, to allow method calls to access the UID and other data and status values.

void readerEventHandler(int readerGroupId, int readerId, SRF_Read_Status readStatus, SFR_Reader* pReader) {
  DebugPrint("Reader Group: ");
  DebugPrint(readerGroupId);
  DebugPrint("  Reader: ");
  DebugPrint(readerId);
  DebugPrint("  UID: ");
  DebugPrint(pReader->strUID());
  DebugPrint("  Status: ");
  DebugPrint(readStatus);
  DebugPrint("-");
  DebugPrintln(pReader->StatusStr());
}

void setup() {
  // Setup the Serial interface to 115200 Baud and wait 3 sec for a USB interface to enumerate and activate
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
  while (!Serial && millis() < 3000)
    delay(10);
#endif

  DebugPrintln("Starfish Rail RFID example");

  // Register the readerEventHandler function to be called when any SFR_Reader object changes state
  tagReaders.registerReaderEventHandler(readerEventHandler);

  // Initial;ises the TwoWire (I2C) Bus interface
  tagReaders.begin();

  // This loop scans for readers on the I2C Bus and creates SFR_Reader object for each and returns the number of readers found on the I2C Bus
  // This loops around until at least one reader is detected
  int numReadersFound;
  while ((numReadersFound = tagReaders.scanForReaders()) == 0) {
    DebugPrintln("No readers found. Attach readers");
    delay(1000);
  }

  // Print the number of readers found
  DebugPrint("Found ");
  DebugPrint(numReadersFound);
  DebugPrintln(" readers");
}

void loop() {
  // Each call to the SFR_ReaderGroup::process() methos handles all the internal processing needed to scan for EID tags and manage the interface
  // It needs to be called regularly (every loop) to ensure timely detection and processing of EID tags
  tagReaders.process();
}

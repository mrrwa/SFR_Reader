/*  This code provides an example of how to use the Starfish Rail I2C RFID Reader Library
 *  Up to 4 readers can be connected (limited by i2c bus address range for these devices)
 *  Avoids 'blocking' caused by 'delay' using a timer and a flag for each instance and can scan 4 
 *  devices at an interval of 10 to 16mS when using a minimum wait time of 8mS
 *  
 *  3 calls for each device are needed
 *  
 *  Create the class object
 *  detector1(i2cBaseAdd);
 *  
 *  In setup function
 *  Initalise i2c RFID readers
 *  detector1.init ();
 *  
 *  In the loop function
 *  Call for a Scan of the i2c RFID readers
 *  detector1.Scan();
 *
 *  If using less devices remove / comment out those not needed
 * 
 */

#include <SFR_Reader.h>

#define NUM_READERS 4   // Maximum Number of I2C RFID Readers
int i2cBaseAdd = 0x28;  // Base Address of I2C RFID Readers.

SFR_Reader * readers[NUM_READERS];
SRF_Read_Status lastStatus[NUM_READERS];

int        numReaders = 0;
int        readerIndex = 0;

void setup() {
  Wire.begin();
  Wire.setClock(200000);  // Set i2c bus speed
  Serial.begin(115200);   // 9600 default for Arduino serial monitor
  while(!Serial && millis() < 3000)
    delay(10);

  Serial.println("Starfish Rail RFID via I2C Class version");

  while (numReaders == 0) { 
    for(readerIndex = 0; readerIndex < NUM_READERS; readerIndex++)
    {
      int i2cAddr = i2cBaseAdd + readerIndex;

      Serial.print("Detecting Reader at address: ");
      Serial.print(i2cAddr, HEX);

      Wire.beginTransmission(i2cAddr);
      uint8_t error = Wire.endTransmission();

      if(error)
        Serial.println(" Not Found");

      else {  // Device Found
        Serial.println(" Found");

        readers[numReaders] = new SFR_Reader(i2cAddr); // index with numReaders instead of readerIndex to avoid a crash
        readers[numReaders]->init(&Wire);
        numReaders++;
      } 
    }
    int sec = 0;
    if (numReaders == 0) {
      Serial.println("No readers found. Attach readers");
      Serial.print("Trying again in 10 seconds ");
      for (sec = 0; sec < 10; sec++) {
        Serial.print(".");
        delay(1000);
      }
      Serial.println();
    }
  }
  readerIndex = 0;
}

void loop() {
  if( readerIndex >= numReaders)
    readerIndex = 0;

  SRF_Read_Status status = readers[readerIndex]->scan();
  if((status > SFR_INIT) && (lastStatus[readerIndex] != status)){
    lastStatus[readerIndex] = status;

    Serial.print("Reader: ");
    Serial.print(readerIndex);
    Serial.print("  Status: ");
    Serial.print(readers[readerIndex]->StatusStr(status));
    Serial.print("  UID: ");
    Serial.println(readers[readerIndex]->strUID());
  }

  readerIndex++;
}

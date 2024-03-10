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
#include <Keyboard.h>

int i2cBaseAdd = 0x28;  // base address of i2c RFID devices.

// Uncomment the line below for the number of RFID Reader Chips on the board
// #define NUM_READERS 1
#define NUM_READERS 2
// #define NUM_READERS 4

// Inital SF detector objects

SFR_Reader detector1(i2cBaseAdd);
#if NUM_READERS > 1 
SFR_Reader detector2(i2cBaseAdd + 1);
#endif
#if NUM_READERS > 2 
SFR_Reader detector3(i2cBaseAdd + 2);
SFR_Reader detector4(i2cBaseAdd + 3);
#endif

void setup() {
  Wire.begin();
  Wire.setClock(200000);  // Set i2c bus speed
  Serial.begin(115200);   // 9600 default for Arduino serial monitor
  while(!Serial && millis() < 3000)
    delay(10);

  Serial.println("Starfish Rail RFID via I2C Class version");

  // Initalise i2c RFID readers
  detector1.init(&Wire);
#if NUM_READERS > 1 
  detector2.init(&Wire);
#endif
#if NUM_READERS > 2 
  detector3.init(&Wire);
  detector4.init(&Wire);
#endif

  Keyboard.begin();
}

void loop() {
  //  *** main ***
  // Call for a Scan of the i2c RFID readers
  SRF_Read_Status status = detector1.scan();
  if(status)
  {
    Serial.print("Status 1: ");
    Serial.print(status);
    Serial.print(" UID: ");
    Serial.println(detector1.strUID());
  }

#if NUM_READERS > 1 
  status = detector2.scan();
  if(status)
  {
    Serial.print("Status 2: ");
    Serial.print(status);
    Serial.print(" UID: ");
    Serial.println(detector2.strUID());
  }
#endif
#if NUM_READERS > 2 
  status = detector3.scan();
  if(status)
  {
    Serial.print("Status 3: ");
    Serial.print(status);
    Serial.print(" UID: ");
    Serial.println(detector3.strUID());
  }

  status = detector4.scan();
  if(status)
  {
    Serial.print("Status 4: ");
    Serial.print(status);
    Serial.print(" UID: ");
    Serial.println(detector4.strUID());
  }
#endif
}

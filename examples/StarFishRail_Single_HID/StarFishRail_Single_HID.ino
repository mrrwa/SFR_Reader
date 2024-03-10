/*  This code provides an example that writes the RFID Reader Tag info to the keyboard
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


// Inital SF detector objects

SFR_Reader detector1(i2cBaseAdd);

void setup() {
  Keyboard.begin();

  Wire.begin();
  Wire.setClock(200000);  // Set i2c bus speed

  // Initalise i2c RFID readers
  detector1.init(&Wire);
}

void loop() {
  //  *** main ***
  // Call for a Scan of the i2c RFID readers
  SRF_Read_Status status = detector1.scan();
  if(status == SFR_TAG_ENTER)
  {
    Keyboard.println(detector1.strUID());
  }
}

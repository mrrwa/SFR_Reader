// Starfish Class version v1
// MGB  12/02/22
// Starfish_Class_v1

/*  This code provides a class object to use with the Starfish Rail i2c RFID modules
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
 *  detector1.init (i2cBaseAdd);
 *  
 *  In the loop function
 *  Call for a Scan of the i2c RFID readers
 *  detector1.Scan(i2cBaseAdd);
 *
 *  If using less devices remove / comment out those not needed
 * 
 */

#include "Wire.h"
int i2cBaseAdd=0x28;  // base address of i2c RFID devices.

// *** Start of Class definition ***
class SF   // Starfish
{
  int i2cAdd;
  byte lenFIFO; // Number of bytes in the FIFO register
  int inFIFO[6]; // Array to hold bytes read from FIFO
  byte errorFlags; // Byte with value read from the Error Flages register
  byte led=0; // Value to be written to the LED control register
  String newTagUID="";  // Most recent Tag data as a string in HEX
  String oldTagUID="";  // Previously captured Tag data as a string in HEX
  int UID_Status=0;  // 0= No tag detected  1= Tag entered field  2= Tag still in field  3= Tag exited field
  bool waiting=false;  // TRUE while waiting for tag to read - Waiting time is normally in the range 8 to 12 mS 
  unsigned long startMillis=0;  // start time of wait 
  unsigned long waitMillis=8;   // time to wait for a tag to come in range and be read, in mS
  unsigned long loopstartMillis=0;  // Timing check
  unsigned long loopMillis=0;

  // Constructor - creates a RFID scan object 
  // and initializes the member variables and state
  public:
  SF(int i2cAddress)   // *** INITALISE class object ***
  {i2cAdd=i2cAddress;
  }
  
  init(int i2cAddress)   // *** Setup RFID registers ***
  {
    i2cAdd=i2cAddress;
    
    Serial.println("Starfish Rail RFID setup start");
  //FIFO Control

    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x02);             // device address is specified in datasheet
    Wire.write(0xb0);             // sends value byte  
    Wire.endTransmission();
    
  //FIFO Data and IRQ0 Control
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x05);             // device address is specified in datasheet
    Wire.write(0x00);             // sends value byte  
    Wire.write(0x00);             // sends value byte  
    Wire.endTransmission();
  
  //Command
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x00);             // device address is specified in datasheet
    Wire.write(0x0d);             // sends value byte  
    Wire.endTransmission();

  //Clear FIFO
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x02);             // device address is specified in datasheet
    Wire.write(0xb0);             // sends value byte  
    Wire.endTransmission();
    
  //  Configure RF mode
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x05);             // device address is specified in datasheet
    Wire.write(0x01);             // sends value byte  
    Wire.write(0x94);    // new
    Wire.write(0x28);   // new
    Wire.write(0x11);   // new
    Wire.endTransmission();
  
  // Load Registers
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x00);             // device address is specified in datasheet
    Wire.write(0x0c);             // sends value byte  
    Wire.endTransmission();
    delay(1);
  
  // Set RF on part 1
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x00);             // device address is specified in datasheet
    Wire.write(0x06);             // sends value byte  
    Wire.endTransmission();
  
    // Set RF on part 2
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x28);             // device address is specified in datasheet
    Wire.write(0x8f);             // sends value byte  
    Wire.endTransmission();
  
    // Set Sensitivity
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x37);             // device address is specified in datasheet
    Wire.write(0x14);             // sends value byte  
    Wire.endTransmission();
  
    // Set pin 22 to output for indicator LED and turn it OFF
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x44);             // device address is specified in datasheet
    Wire.write(0x40);             // sends value byte  
    Wire.endTransmission();

    led=0x40;   //led=0x40= OFF
    Wire.beginTransmission(i2cAdd); 
    Wire.write(0x45);             
    Wire.write(led);             // set LED OFF
    Wire.endTransmission(); 


    Serial.println("Starfish Rail RFID setup end");
  }  // *** END INITALISE ***

  void Scan(int i2cAdd)  // Scan for RFID tag
  {
  if (waiting==false)  // Not waiting for the timer to expire so initalise a scan
    {
    loopstartMillis=millis();  // Loop timer
    // Flush FIFO
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x02);             // device address is specified in datasheet
    Wire.write(0xb0);             // sends value byte  was 0x80
    Wire.endTransmission();
  
    // Write FIFO with ASK UID
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x05);             // device address is specified in datasheet
    Wire.write(0x76);             // sends value byte  
    Wire.write(0xa0);             // sends value byte 
    Wire.write(0x04);             // sends value byte 
    Wire.write(0x00);             // sends value byte 
    Wire.write(0x00);             // sends value byte 
    Wire.write(0x00);             // sends value byte 
    Wire.write(0x00);             // sends value byte 
    Wire.endTransmission();
    //Serial.print("Write FIFO with ASK UID ");
    //Serial.println(x);
  
    // Set to transceive
    Wire.beginTransmission(i2cAdd); // transmit to device
    Wire.write(0x00);             // device address is specified in datasheet
    Wire.write(0x07);             // sends value byte  
    Wire.endTransmission();
    //Serial.print("Set to transceive ");
    //Serial.println(x);
    startMillis = millis(); // set the start time to current millis value
    waiting =true;
    }

  else if (waiting==true && (millis() >= startMillis + waitMillis))  // Wait time has passed
    //delay(8);  // ***** DELAY
    {
    // Dummy - Read register 04 : length of data in FIFO
    Wire.beginTransmission(i2cAdd);    
    Wire.write(0x04);
    Wire.endTransmission();
    //Read register 04 : length of data in FIFO
    Wire.beginTransmission(i2cAdd);
    Wire.write(0x04);
    Wire.requestFrom(i2cAdd, 1);
    lenFIFO  = Wire.read();
    Wire.endTransmission();
    //Serial.print("Len FIFO ");
    //Serial.println(lenFIFO);
  
    // Dummy - Read values from FIFO
    Wire.beginTransmission(i2cAdd);    
    Wire.write(0x05);
    Wire.endTransmission();
    // Read values from FIFO
    Wire.beginTransmission(i2cAdd);    
    Wire.write(0x05);
    Wire.requestFrom(i2cAdd, 6);  // read the byte values from FIFO
    int byteCounter=0;            // Index for array of bytes read from FIFO 
    while(Wire.available())       // 
      {
      inFIFO[byteCounter] = Wire.read();  // Capture received byte
      byteCounter++;                      // Increment index counter
      }
  
  // Read ERROR flags - register 0a
    Wire.beginTransmission(i2cAdd);    
    Wire.write(0x0a);
    Wire.endTransmission();
  // Read ERROR flags
    Wire.beginTransmission(i2cAdd);    
    Wire.write(0x0a);
    Wire.requestFrom(i2cAdd, 1);  // read the current GPIO output latches
    errorFlags = Wire.read();    // receive a byte as character
  
  // Work out STATUS
  // 0= No tag detected (0)   1= Tag entered field (R)  2= Tag still in field (C)  3= Tag exited field (U)
  
    if(errorFlags==0 && lenFIFO==13){  // Valid tag detected
  
        int nibble[2];
        char chr_nibble;
        newTagUID="";
        for (int i = 5; i >0; i--)  {  
           div_t divresult;
           divresult = div (inFIFO[i],16);
           nibble[0]=divresult.quot; // Get most significant nibble
           nibble[1]=divresult.rem;  // Get least significant nibble
  
        for(int nib=0;nib<2;nib++)
           {
           if (nibble[nib]<10)    {chr_nibble= 48 + nibble[nib];}  // 0 to 9 add to ASCII 0
           else   {chr_nibble= 55 + nibble[nib];}  // A TO F, add to 55-ASCII 7 saves doing -10
      
        newTagUID += chr_nibble;
      }
    }   
      
        if (newTagUID == oldTagUID){
        UID_Status=2;  }   // Valid tag but same UID as last time
    
        else {
         UID_Status=1;     // Valid tag but different UID
         oldTagUID=newTagUID;
         }
    }     
    else              // Invalid or no tag present
  
      if(UID_Status==0 && lenFIFO==0)
        {UID_Status=0;}
      else
        {UID_Status=3;}
  
      Serial.print("Reader=");
      Serial.print(i2cAdd,HEX);
      
      Serial.print(",Status=");
      Serial.print(UID_Status);
      Serial.print(",UID=");
      Serial.print(newTagUID);
      
    Serial.println();
  // Set LED ON / OFF  
    if(errorFlags==0 && lenFIFO==13){  // Valid tag detected)
      led=0x00;   //led=0x00= ON
    }
    else{
      led=0x40;   //led=0x40= OFF
    }
    
    Wire.beginTransmission(i2cAdd); 
    Wire.write(0x45);             
    Wire.write(led);             // set LED ON or OFF as required
    Wire.endTransmission(); 

    waiting=false;
    
    loopMillis=millis()-loopstartMillis;  // Loop timing check
    Serial.println(loopMillis);
    }
    else {} // still waiting for the timer to expire so do nothing.
  }
};

//    *** END of Class ***

// Inital SF detector objects

SF detector1(i2cBaseAdd);
SF detector2(i2cBaseAdd+1);
SF detector3(i2cBaseAdd+2);
SF detector4(i2cBaseAdd+3);


void setup() {
  Wire.begin();
  Wire.setClock(200000);  // Set i2c bus speed
  Serial.begin(115200);   // 9600 default for Arduino serial monitor
  Serial.println("Starfish Rail RFID via I2C Class version");

  // Initalise i2c RFID readers
  detector1.init (i2cBaseAdd);
  detector2.init (i2cBaseAdd+1);
  detector3.init (i2cBaseAdd+2);
  detector4.init (i2cBaseAdd+3);
  
}

void loop() {
  //  *** main *** 
  // Call for a Scan of the i2c RFID readers
detector1.Scan(i2cBaseAdd);
detector2.Scan(i2cBaseAdd+1);
detector3.Scan(i2cBaseAdd+2);
detector4.Scan(i2cBaseAdd+3);


}

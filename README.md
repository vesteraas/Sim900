# Arduino Sim900 Library #
This library is written for any Arduino shield equipped with a SIM900 Quad-band GSM/GPRS module.  Events from the unit is sendt to your sketch via callback functions.  Sending AT commands to the unit should be done with the **sendATCommand()** function.  This function will only return true if the unit responds with **OK** as its first response.  The function should not be used with commands that doesn't return **OK** or **ERROR** as its first response.

### Examples ###
You may need to modify **mySerial.begin()** under **setup()** to match your SIM900's current baud rate.

#### echo ####
A SMS sent to the unit will be echoed back to the sender.  See the **smsHandler()** callback function.

#### callback ####
A SMS sent to the unit will trigger a call to the sender.  See the **smsHandler()** callback function.

## Download ##
Click the **Download Zip** button to the right

## Installation ##
* Uncompress the downloaded zip-file
* Rename the uncompressed folder to Sim900
* Place the Sim900 library folder in your <arduinosketchfolder>/libraries/ folder
* Restart the Arduino IDE

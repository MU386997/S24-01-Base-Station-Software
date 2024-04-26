S24-01 Software
===============

This repository contains the Personal Locator Beacon (PLB), Range Extender, and Base Station code for the Major Design Experience Project.

### Demo Code
The demo code was used at the expo to demonstrate the project. The Base Station **does not** send acknowledgements to show the double packets being received from the Range Extender and PLB.

### Final Code
The final code is handed off to the customer. 

PLB
---
### Operation
- The power switch is located on the bottom.
- A flashing Red LED indicates there was an error when intiializing the PLB.
- A flashing Green LED indicates the PLB is looking for a GPS lock.
- A solid Green LED indicates the PLB is ready.
- A solid Yellow LED indicates the PLB is in active mode and is attempting to transmit every ~10 seconds (no transmission is sent if there is no GPS lock)
- A solid Red LED indicates the PLB is in the panic state. If the panic state is activated while the PLB is not in active mode, a single transmission is sent on the rising edge. 

### Packet Structure
The PLB broadcasts the following packet in big endian encoding:
```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               |P|             |               |
|           Radio ID            |A|  Message ID |   GPS Lat     |
|                               |N|             |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          GPS Latitude (continued)             |   GPS Long    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          GPS Longitude (continued)            | Battery Life  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Unix Time                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

- Radio ID: an unsigned 16-bit integer unique to each PLB. Can be changed with the RADIO_ID constant
- Panic State: the sign bit of the Message ID. True if the panic switch is pressed. 
- Message ID: a signed 8-bit integer that increments for each message. Rolls over to 0 after 127 to prevent overflow from affecting Panic State
- GPS Latitude: 32-bit float
- GPS Longitude: 32-bit float
- Battery Life: an unsigned 8-bit integer for the battery percent. Only values 0-100 are used but the packet must be a whole number of bytes
- Unix Time: a signed 32-bit integer of the number of ms since January 1st, 1970

### Useful Constants
- RF95_FREQ: the floating point frequency to use in MHz
- RF95_TX_POWER: the power to use when transmitting in decibels. The PLB was kept at a low power for testing to simulate a long range
- RADIO_ID: the unique ID for each PLB (Range Extenders do not have an ID)
- SLEEP_TIME: the time in milliseconds between transmissions in active mode
- SLEEP_TIME_VARIANCE: the maximum time in milliseconds added to or subtracted from the sleep time to decrease the likelihood of a transmission collision
- GPS_TIME_ALLOWABLE_AGE: how old in milliseconds the GPS timestamp is allowed to be for it to be considered valid
- DEBUG_BAUD: the baud rate of the USB serial port used for debugging
- GPS_BAUD: the baud rate of the GPS module (note some of our modules use 38400 baud)
- SERIAL_DEBUG: whether or not to print debug information on the serial port
- DEBUG_START_DELAY_SEC: how long to wait before starting the PLB (useful since debug messages can be missed if the serial monitor is not connected in time)
- BATTERY_MIN_THRESHOLD: minimum (in ADC counts 0-1024) for linear mapping of battery voltage
- BATTERY_MAX_THRESHOLD maximum (in ADC counts 0-1024) for linear mapping of battery voltage

Range Extender
--------------
### Operation
The Range Extender operates as a relay for packets. It keeps an array of all messages received from PLBs and retransmits them after one second. Any acknoweledgement is used to search the array and remove any messages matching the Radio ID and Message ID, thus preventing the retransmission.

### Useful Constants
- RF95_FREQ: the floating point frequency to use in MHz
- RF95_TX_POWER: the power to use when transmitting in decibels
- SEND_INTERVAL: the time to wait before retransmitting a packet

Base Station
------------
### Description
The Base Station runs GnuRadio and an associated python GUI that communicate over TCP. There are scripts available for emulating each end of the TCP connection.
Simply plug in the SDR run both the GnuRadio program (Docker Recommended) and the python script.

### Dependencies
- Make sure the python GUI requirements in requirements.txt are satisifed
- (unecessary if using docker) install GnuRadio from https://github.com/ryanvolz/radioconda/
- (unecessary if using docker) install USRP Drivers from https://files.ettus.com/manual/page_install.html
- If on linux, make sure to add the appropriate UDEV rules to access the SDR over USB

### Docker Operation
Build and run the Docker image
`docker build -t basestation {path to dockerfile}`
`docker run -it --privileged --network=host basestation`

### GnuRadio Operation
1. Open GnuRadio Companion
2. Open the basestation.grc file
3. Run 


### Packet Structure
The basestation broadcasts the following packet in big endian encoding:
```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               |P|             |
|           Radio ID            |A|  Message ID |
|                               |N|             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

- Radio ID: an unsigned 16-bit integer unique to each PLB. Can be changed with the RADIO_ID constant
- Panic State: the sign bit of the Message ID. True if the panic switch is pressed. 
- Message ID: a signed 8-bit integer that increments for each message. Rolls over to 0 after 127 to prevent overflow from affecting Panic State
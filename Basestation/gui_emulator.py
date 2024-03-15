## imports
import socket
from datetime import *
import struct


## constants
BUFFER_SIZE = 2**12
HOST = "127.0.0.1"
PORT = 8080


## main
if __name__ == "__main__":
    # connect
    print(f"Connecting to {HOST}:{PORT} ...")
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serverSocket.connect((HOST, PORT))               

    # get data
    while (True):
        try:
            received_data = serverSocket.recv(BUFFER_SIZE)
            received_data = received_data[4:]

            radioID = int.from_bytes(received_data[0:2])
            panicState = int(received_data[2]) < 0
            messageID = int(received_data[2] & 0b0111111)
            gpsLat = struct.unpack('>f', received_data[3:7])[0]
            gpsLong = struct.unpack('>f', received_data[7:11])[0]
            batteryLife = int(received_data[11])
            utc = int.from_bytes(received_data[12:16], signed=False)

            print("Radio ID:", radioID)
            print("Panic State:", panicState)
            print("Message ID:", messageID)
            print("GPS Latitude:", gpsLat)
            print("GPS Longitude:", gpsLong)
            print("Battery Life:", batteryLife)
            print("Timestamp:", utc)
        except:
            print("Error Decoding")
        finally:
            print("\n")
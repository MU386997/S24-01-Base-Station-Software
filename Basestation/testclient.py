## imports
import socket
import sys


## constants
BUFFER_SIZE = 2**12
HOST = "127.0.0.1"
PORT = 8080


## main
if __name__ == "__main__":
    try:
        # connect
        print(f"Connecting to {HOST}:{PORT} ...")
        serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        serverSocket.connect((HOST, PORT))               

        # get data
        while (True):
            received_data = serverSocket.recv(BUFFER_SIZE)
            print(received_data)

    except OSError as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)

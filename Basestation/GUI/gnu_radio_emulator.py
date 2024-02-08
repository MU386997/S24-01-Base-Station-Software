"""
This file is meant to emulate GNU Radio for the purpose of testing
"""

import time
import socket
import struct

# GNU Radio should listen on this port address
GNURADIO_ADDR = ("localhost", 8080)
BUFFER_SIZE = 2**12

if __name__ == "__main__":
    # Set up the socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(GNURADIO_ADDR)
    server_socket.listen()

    # Accept peer connections
    while peer_connection := server_socket.accept():
        peer_socket, _ = peer_connection
        with peer_socket:
            # Construct an arbitrary packet
            radio_id = 42
            message_id = -99
            latitude = 37.22788
            longitude = -80.42212
            battery_life = 200  # Battery life is from 0 to 255
            unix_time = int(time.time())  # Get the current Unix time
            packet = struct.pack(
                "!HbffBI",
                radio_id,
                message_id,
                latitude,
                longitude,
                battery_life,
                unix_time,
            )
            # Send the packet once per connection
            peer_socket.send(packet)

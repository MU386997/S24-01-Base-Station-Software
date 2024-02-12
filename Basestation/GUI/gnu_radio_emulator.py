"""
This file is meant to emulate GNU Radio for the purpose of testing
"""

import time
import socket
import struct
import random

# GNU Radio should listen on this port address
GNURADIO_ADDR = ("localhost", 8080)
BUFFER_SIZE = 2**12

def get_random_packet():
    # Construct an arbitrary packet
    radio_id = random.randint(0, 65535)
    message_id = random.randint(-128, 127)
    latitude = random.uniform(37, 38)
    longitude = random.uniform(-81, -80)
    battery_life = random.randint(0, 255)
    unix_time = int(time.time())  # Get the current Unix time
    return struct.pack(
        "!HbffBI",
        radio_id,
        message_id,
        latitude,
        longitude,
        battery_life,
        unix_time,
    )


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
            # Send the packet once per connection
            for _ in range(7):
                time.sleep(2)
                peer_socket.send(get_random_packet())

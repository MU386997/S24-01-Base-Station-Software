"""
This file is meant to emulate GNU Radio for the purpose of testing
"""

import time
import socket
import struct
import random

# GNU Radio should listen on this port address
GNURADIO_SEND_ADDR = ("localhost", 8080)
GNURADIO_RECV_ADDR = ("localhost", 8081)
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
        "!xxxxHbffBI",
        radio_id,
        message_id,
        latitude,
        longitude,
        battery_life,
        unix_time,
    )


if __name__ == "__main__":
    # Set up the send socket
    server_send_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_send_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_send_socket.bind(GNURADIO_SEND_ADDR)
    server_send_socket.listen()
    # Set up the receive socket
    server_recv_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_recv_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_recv_socket.bind(GNURADIO_RECV_ADDR)
    server_recv_socket.listen()

    # Accept peer connections
    while send_connection := server_send_socket.accept():
        peer_send_socket, _ = send_connection
        recv_connection = server_recv_socket.accept()
        peer_recv_socket, _ = recv_connection
        with peer_send_socket:
            with peer_recv_socket:
                # Send the packet once per connection
                for _ in range(7):
                    time.sleep(2)
                    packet = get_random_packet()
                    peer_send_socket.send(packet)
                    print(f"Packet sent: {packet}")

                    data = peer_recv_socket.recv(BUFFER_SIZE)
                    print(f"Data received: {data}")

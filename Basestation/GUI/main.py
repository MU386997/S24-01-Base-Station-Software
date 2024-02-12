"""
This file contains the code for running the GUI which receives packets from
GNU Radio and plots the data on an interactive map running in Qt.
"""

from datetime import UTC, datetime
import io
import socket
import struct
import sys
import threading

from PyQt5 import QtCore, QtWebEngineWidgets, QtWidgets
import folium

# The GUI is a client that connects to GNURadio
GNURADIO_ADDR = ("localhost", 8080)
BUFFER_SIZE = 2**12


class PacketLengthError(Exception):
    """
    Creates a new error to throw if the packet is not the right length
    """
    pass


class MapManager(QtCore.QObject):
    # Qt signal for transmitting html back to the GUI thread
    htmlChanged = QtCore.pyqtSignal(str)
    # Allows closing of the main window in case the GUI is disconnected from GNURadio
    closeWindow = QtCore.pyqtSignal()

    def __init__(self):
        """
        Creates a MapManager QObject which can receive data from GNURadio
        and add it to a folium map and update the GUI
        """
        super().__init__()
        # Make a socket to connect to GNURadio
        self.radioSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.radioSocket.connect(GNURADIO_ADDR)
        # Creates a folium map to store markers
        self.map = folium.Map(location=[37.227779, -80.422289], zoom_start=13)
        # Arrays used for calculating map bounds later
        self.latitudes = []
        self.longitudes = []
        # Starts a separate thread that manages the map
        threading.Thread(target=self.exec, daemon=True).start()

    def exec(self):
        """
        Main exec loop for the worker
        Runs in a separate thread
        """
        with self.radioSocket:
            try:
                # Continuously recieve data from GNURadio
                while data := self.radioSocket.recv(BUFFER_SIZE):
                    try:
                        # Decode and add the point to the map
                        self.add_point(self.decode(data))
                        # Update the map in the GUI by emitting a signal
                        self.htmlChanged.emit(self.load_HTML())
                    except PacketLengthError as err:
                        # Print out any error with packet length
                        print(f"Error: {err}", file=sys.stderr)
                        # Continue receiving packets
            except OSError as err:
                # Print out any error with receiving data
                print(f"Error communicating with GNURadio: {err}", file=sys.stderr)
                # Close the window
                self.closeWindow.emit()

    def load_HTML(self):
        """
        Loads the html from the map
        """
        # Create data variable to output to
        data = io.BytesIO()
        # Save the map to the data variable
        self.map.save(data, close_file=False)
        # Return the html as a string
        return data.getvalue().decode()

    def add_point(self, point):
        """
        Adds a point to the map
        """
        # Unpack the tuple with the decoded packet data
        (
            radio_id,
            message_id,
            panic_state,
            latitude,
            longitude,
            battery_life,
            utc_time,
        ) = point
        # Append point to list of latitudes and longitudes
        self.latitudes.append(latitude)
        self.longitudes.append(longitude)
        # Format a string for the map marker
        popup_string = (
            f"Radio ID: {radio_id}<br>"
            f"Message ID: {message_id}<br>"
            f"Panic State: {panic_state}<br>"
            f"Latitude: {latitude:.4f}<br>"
            f"Longitude: {longitude:.4f}<br>"
            f"Battery Life: {battery_life:.1f}%<br>"
            f"Time: {utc_time} UTC"
        )
        # Print packet to console
        print(f"\nPacket Received:\n{popup_string.replace("<br>", "\n")}")
        # Make a popup with all the packet data
        iframe = folium.IFrame(popup_string)
        popup = folium.Popup(iframe, min_width=250, max_width=250)
        # Add the marker to the folium map
        folium.Marker(location=(latitude, longitude), popup=popup).add_to(self.map)
        # Adjust map bounds so all points can be seen
        southwest_point = (min(self.latitudes) - 0.01, min(self.longitudes) - 0.01)
        northeast_point = (max(self.latitudes) + 0.01, max(self.longitudes) + 0.01)
        self.map.fit_bounds((southwest_point, northeast_point))

    def decode(self, received_data: bytes):
        """
        Decodes the data packet coming from GNURadio

        Packet Structure:
         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
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
        """
        # Expected packet length in bytes
        expected_length = 16
        # Raise exception if packet is not expected length
        if (packet_length := len(received_data)) != expected_length:
            raise PacketLengthError(
                f"Expected packet length of {expected_length} bytes. Received {packet_length} bytes"
            )

        # First 2 bytes are radio id (unsigned short: H)
        # Second byte holds the message id and panic state (signed char: b)
        # Bytes 4-7 are GPS latitude (float: f)
        # Bytes 8-11 are GPS longitude (float: f)
        # Bytes 13-16 are the time in Unix time format (unsigned int: I)
        radio_id, message_byte, latitude, longitude, unix_time = struct.unpack("!HbffxI", received_data)
        # Message id is the absolute value of the message id byte
        message_id = abs(message_byte)
        # Panic state is determined by the first bit of the message id which also determines the sign
        panic_state = message_byte < 0
        # Byte 12 is battery life from 0 to 255
        battery_life = received_data[11] * 100 / 255
        # Convert unix time to UTC string
        utc_time = datetime.fromtimestamp(unix_time, UTC).strftime("%m-%d-%Y %H:%M:%S")

        # Return a tuple with all the necessary info
        return (
            radio_id,
            message_id,
            panic_state,
            latitude,
            longitude,
            battery_life,
            utc_time,
        )


class BaseStationGUI(QtWidgets.QWidget):
    """
    Creates a GUI which has a map for displaying markers
    """
    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        """
        Initialize the UI for the GUI
        """
        # Create a window for the GUI
        self.webEngineView = QtWebEngineWidgets.QWebEngineView()

        try:
            # Create a worker for processing signals
            self.mapManager = MapManager()
        except OSError as err:
            # Print any connection error to the command line
            print(f"Error: {err}. Make sure GNURadio is open", file=sys.stderr)
            sys.exit(1)

        # Load initial map to the GUI
        self.webEngineView.setHtml(self.mapManager.load_HTML())
        # Connect htmlChanged signal to setHtml slot
        self.mapManager.htmlChanged.connect(self.webEngineView.setHtml)
        # Allows the mapManager to close the main window
        self.mapManager.closeWindow.connect(self.close)

        # Create layout and add web engine view
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self.webEngineView)
        layout.setContentsMargins(0, 0, 0, 0)

        # Set the window size
        self.resize(1280, 720)
        # Set the window title
        self.setWindowTitle("Base station GUI")
        # Show the window to the screen
        self.show()


if __name__ == "__main__":
    """
    Main function
    """
    # Creates a QApplication to display
    app = QtWidgets.QApplication(sys.argv)
    # Creates the GUI
    gui = BaseStationGUI()
    # Exit is handled by Qt
    sys.exit(app.exec_())

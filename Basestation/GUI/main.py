import io
import sys
import threading
import socket

import folium
from PyQt5 import QtWidgets, QtWebEngineWidgets, QtCore

# The GUI is a client that connects to GNURadio
GNURADIO_ADDR = ("localhost", 8080)
BUFFER_SIZE = 2**12


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
                    # For now, just print the data to the console
                    print(data)
            except OSError as err:
                # Print out any error with receiving data
                print(f"Error communicating with GNURadio: {err}", file=sys.stderr)
                # Close the window
                self.closeWindow.emit()

    def loadHtml(self):
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
        # Add the marker to the folium map
        folium.Marker(location=point, popup=str(point)).add_to(self.map)


class BaseStationGUI(QtWidgets.QWidget):
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
        self.webEngineView.setHtml(self.mapManager.loadHtml())
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

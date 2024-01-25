import io
import sys

import folium
from PyQt5 import QtWidgets, QtWebEngineWidgets

def add_point(point, window):
    """
    Adds a point to the map and displays it in the GUI
    """
    # Add the marker to the folium map
    folium.Marker(location=point, popup=str(point)).add_to(map)

    # Create data variable to output to
    data = io.BytesIO()
    # Save the map to the data variable
    map.save(data, close_file=False)
    # Input the new data into the GUI window
    window.setHtml(data.getvalue().decode())


if __name__ == "__main__":
    """
    Main function
    """
    # Creates a QApplication to display
    app = QtWidgets.QApplication(sys.argv)
    # Creates a folium map to store markers
    map = folium.Map(
        location=[37.227779, -80.422289], zoom_start=13
    )

    # Create data variable to output to
    data = io.BytesIO()
    # Save the map to the data variable
    map.save(data, close_file=False)

    # Create a window for the GUI
    window = QtWebEngineWidgets.QWebEngineView()
    # Input the data into the GUI window
    window.setHtml(data.getvalue().decode())
    # Set the window size
    window.resize(1280, 720)
    # Set the window title
    window.setWindowTitle("Base station GUI")
    # Show the window to the screen
    window.show()

    # Some example points
    points = [(37.7749, -122.4194), (34.0522, -118.2437), (40.7128, -74.0060)]
    # Plot each point on the map
    for point in points:
        add_point(point, window)

    # Exit is handled by the Qt window
    sys.exit(app.exec_())

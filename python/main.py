import io
import sys

import folium
from PyQt5 import QtWidgets, QtWebEngineWidgets

def add_point(point, window):
    folium.Marker(location=point, popup=str(point)).add_to(map)

    data = io.BytesIO()
    map.save(data, close_file=False)
    window.setHtml(data.getvalue().decode())


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    map = folium.Map(
        location=[37.227779, -80.422289], zoom_start=13
    )

    data = io.BytesIO()
    map.save(data, close_file=False)

    window = QtWebEngineWidgets.QWebEngineView()
    window.setHtml(data.getvalue().decode())
    window.resize(1280, 720)
    window.setWindowTitle("Base station GUI")
    window.show()

    points = [(37.7749, -122.4194), (34.0522, -118.2437), (40.7128, -74.0060)]
    # Plot each point on the map
    for point in points:
        add_point(point, window)


    sys.exit(app.exec_())
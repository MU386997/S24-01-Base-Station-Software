FROM archlinux/archlinux:latest

# install dependencies
RUN pacman --noconfirm -Sy python python-pyqt5 python-pyqt5-sip python-pyqt5-webengine
WORKDIR /gui
COPY requirements.txt .
COPY main.py .
RUN python -m venv .venv 
RUN . .venv/bin/activate && pip install -r requirements.txt

CMD . .venv/bin/activate && exec python main.py
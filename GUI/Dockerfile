FROM python:latest

WORKDIR /gui
COPY requirements.txt .
RUN pip install -r requirements.txt

COPY main.py .
CMD ["python", "main.py"]
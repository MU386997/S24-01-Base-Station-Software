FROM archlinux/archlinux:latest

# setup gnuradio
RUN pacman --noconfirm -Sy gnuradio python-pyqt5 cmake make libvolk boost libuhd gcc git pybind11
RUN python /usr/lib/uhd/utils/uhd_images_downloader.py

# install lora sdr
RUN git clone https://github.com/tapparelj/gr-lora_sdr.git && mkdir /gr-lora_sdr/build
WORKDIR /gr-lora_sdr/build/
RUN  cmake .. && make install -j$(nproc)
ENV PYTHONPATH=/usr/lib64/python3.11/site-packages:/gr-lora_sdr/include/:$PYTHONPATH
ENV LD_LIBRARY_PATH=/usr/lib64:$LD_LIBRARY_PATH

# generate python file
WORKDIR /basestation
COPY ./* .
RUN grcc basestation.grc

CMD ["python", "./basestation.py"]
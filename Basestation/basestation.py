#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: PLB Basestation
# Author: caleb
# GNU Radio version: 3.10.9.2

from gnuradio import blocks
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import gr, pdu
from gnuradio import network
from gnuradio import uhd
import time
import gnuradio.lora_sdr as lora_sdr




class basestation(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "PLB Basestation", catch_exceptions=True)

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 500000
        self.listen_address = listen_address = '::'
        self.freq = freq = 915000000
        self.data_port = data_port = 8080
        self.ack_port = ack_port = 8081

        ##################################################
        # Blocks
        ##################################################

        self.uhd_usrp_source_0 = uhd.usrp_source(
            ",".join(("", '')),
            uhd.stream_args(
                cpu_format="fc32",
                args='',
                channels=list(range(0,1)),
            ),
        )
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        # No synchronization enforced.

        self.uhd_usrp_source_0.set_center_freq(freq, 0)
        self.uhd_usrp_source_0.set_antenna("TX/RX", 0)
        self.uhd_usrp_source_0.set_gain(30, 0)
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
            ",".join(("", '')),
            uhd.stream_args(
                cpu_format="fc32",
                args='',
                channels=list(range(0,1)),
            ),
            "",
        )
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        # No synchronization enforced.

        self.uhd_usrp_sink_0.set_center_freq(freq, 0)
        self.uhd_usrp_sink_0.set_antenna("TX/RX", 0)
        self.uhd_usrp_sink_0.set_gain(30, 0)
        self.pdu_tagged_stream_to_pdu_0 = pdu.tagged_stream_to_pdu(gr.types.byte_t, 'packet_len')
        self.network_tcp_source_0 = network.tcp_source.tcp_source(itemsize=gr.sizeof_char*1,addr=listen_address,port=ack_port,server=True)
        self.network_tcp_sink_0 = network.tcp_sink(gr.sizeof_char, 1, '127.0.0.1', 8080,2)
        self.lora_tx_0 = lora_sdr.lora_sdr_lora_tx(
            bw=125000,
            cr=1,
            has_crc=True,
            impl_head=False,
            samp_rate=samp_rate,
            sf=7,
         ldro_mode=2,frame_zero_padd=1280 )
        self.lora_rx_0 = lora_sdr.lora_sdr_lora_rx( bw=125000, cr=1, has_crc=True, impl_head=False, pay_len=255, samp_rate=samp_rate, sf=7, soft_decoding=True, ldro_mode=2, print_rx=[False,False])
        self.blocks_stream_to_tagged_stream_0 = blocks.stream_to_tagged_stream(gr.sizeof_char, 1, 3, "packet_len")


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.pdu_tagged_stream_to_pdu_0, 'pdus'), (self.lora_tx_0, 'in'))
        self.connect((self.blocks_stream_to_tagged_stream_0, 0), (self.pdu_tagged_stream_to_pdu_0, 0))
        self.connect((self.lora_rx_0, 0), (self.network_tcp_sink_0, 0))
        self.connect((self.lora_tx_0, 0), (self.uhd_usrp_sink_0, 0))
        self.connect((self.network_tcp_source_0, 0), (self.blocks_stream_to_tagged_stream_0, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.lora_rx_0, 0))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)

    def get_listen_address(self):
        return self.listen_address

    def set_listen_address(self, listen_address):
        self.listen_address = listen_address

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.uhd_usrp_sink_0.set_center_freq(self.freq, 0)
        self.uhd_usrp_source_0.set_center_freq(self.freq, 0)

    def get_data_port(self):
        return self.data_port

    def set_data_port(self, data_port):
        self.data_port = data_port

    def get_ack_port(self):
        return self.ack_port

    def set_ack_port(self, ack_port):
        self.ack_port = ack_port




def main(top_block_cls=basestation, options=None):
    tb = top_block_cls()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    try:
        input('Press Enter to quit: ')
    except EOFError:
        pass
    tb.stop()
    tb.wait()


if __name__ == '__main__':
    main()

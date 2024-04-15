"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
import pmt
from gnuradio import gr


class blk(gr.basic_block):
    """Embedded Python Block example - a simple multiply const"""

    def __init__(self, packet_len=16, ack_len=3):
        """arguments to this function show up as parameters in GRC"""
        gr.basic_block.__init__(
            self,
            name='Generate ACK', 
            in_sig=[np.byte],
            out_sig=[np.byte]
        )
        self.packet_len = packet_len
        self.ack_len = ack_len
        self.message_port_register_out(pmt.intern('ack_out'))

    def general_work(self, input_items, output_items):
        tagTuple = self.get_tags_in_window(0, 0, len(input_items[0]))
        relativeOffsetList = []
        for tag in tagTuple:
            if (pmt.to_python(tag.key) == "frame_info" and pmt.to_python(tag.value)["pay_len"] == self.packet_len):
                relativeOffsetList.append(tag.offset - self.nitems_read(0))
        relativeOffsetList.sort()

        print(relativeOffsetList)

        return 0
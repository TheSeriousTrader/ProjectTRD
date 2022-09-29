#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# """
#
# """

# import basesvc
import urwid
import pydim
import logging
from trdmon.dimwid import dimwid as dimwid
# from trdmon.basesvc import basesvc as basesvc

#
# class trdbox_daq_run(basesvc.basesvc):
#     def __init__(self):
#         super().__init__("trdbox/RUN_NUMBER", "I")
#
#     def refresh(self):
#         if self.value >= 0:
#             self.set_text(("fsm:ready", f"Run: {self.value:5d}"))
#         else:
#             self.set_text(("fsm:static", f"- no run -"))
#
# class trdbox_daq_bytes_read(basesvc.basesvc):
#     def __init__(self):
#         super().__init__("trdbox/DAQ/BYTES_READ", "I")
#
#     def refresh(self):
#         self.set_text(("bg", f"{self.value:9d} B"))
#

class trigger(urwid.Pile):
    def __init__(self):

        self.reg = []

        self.pre_conf = 0
        self.pre_dgg = 0
        self.pre_cnt = 0
        self.pre_stat = 0

        self.dis_dac = 0
        self.dis_freq = 0
        self.dis_led = 0
        self.dis_conf = 0
        self.dis_time = 0

        self.txt = [ urwid.Text("") for i in range(9) ]
        super().__init__(self.txt)

        pydim.dic_info_service(f"trdbox/registers", "I:9", self.cb)

        dimwid.register_callback(self)

    def cb(self, r0, r1, r2, r3, r4, r5, r6, r7, r8):
        self.pre_conf = r0
        self.pre_dgg  = r1
        self.pre_cnt  = r2
        self.pre_stat = r3

        self.dis_dac  = r4
        self.dis_freq = r5
        self.dis_led  = r6
        self.dis_conf = r7
        self.dis_time = r8

        dimwid.request_callback(self)


    def refresh(self):

        self.txt[0].set_text("trg conf: %08x" % self.pre_conf )
        self.txt[1].set_text("     dgg: %08x" % self.pre_dgg  )
        self.txt[2].set_text("     cnt: %08x" % self.pre_cnt  )
        self.txt[3].set_text("    stat: %08x" % self.pre_stat )

        self.txt[4].set_text("disc dac: %08x" % self.dis_dac  )
        self.txt[5].set_text("    freq: %08x" % self.dis_freq )
        self.txt[6].set_text("     led: %08x" % self.dis_led  )
        self.txt[7].set_text("    conf: %08x" % self.dis_conf )
        self.txt[8].set_text("    time: %08x" % self.dis_time )


class daq(urwid.Pile):
    def __init__(self):

        self.run = -999
        self.rd = 0
        self.wr = 0
        self.ev = 0

        self.w_run = urwid.Text("run")
        self.w_ev = urwid.Text("ev")
        self.w_rd = urwid.Text("bytes R")

        super().__init__([
        urwid.Text("TRDbox : DAQ"),
        self.w_run, self.w_ev, self.w_rd])

        pydim.dic_info_service(f"trdbox/RUN_NUMBER", "I", self.cb_run)
        pydim.dic_info_service(f"trdbox/DAQ/EVENT_NUMBER", "I", self.cb_ev)
        pydim.dic_info_service(f"trdbox/DAQ/BYTES_WRITTEN", "I", self.cb_wr)
        pydim.dic_info_service(f"trdbox/DAQ/BYTES_READ", "I", self.cb_rd)

        dimwid.register_callback(self)

    def cb_run(self, x):
        self.run = x
        dimwid.request_callback(self)

    def cb_ev(self, x):
        self.ev = x
        dimwid.request_callback(self)

    def cb_wr(self, x):
        self.wr = x
        dimwid.request_callback(self)

    def cb_rd(self, x):
        self.rd = x
        dimwid.request_callback(self)

    def refresh(self):

        if self.run >= 0:
            self.w_run.set_text(("fsm:ready", f"Run: {self.run:5d}"))
        else:
            self.w_run.set_text(("fsm:static", f"- no run -"))

        self.w_ev.set_text(f"{self.ev:10d} events")
        self.w_rd.set_text(f"{self.rd:10d} bytes")


class daq2(urwid.Pile):
    def __init__(self):

        super().__init__([
            # basesvc( "trdbox/RUN_NUMBER", "I", fmt = trdbox_daq2.fmt_run),
            # basesvc( "trdbox/RUN_NUMBER", "I", fmt = trdbox_daq2.fmt_run),
            basesvc( "trdbox/DAQ/BYTES_READ", "I",
                     fmt = lambda x: ("bg",f"{x} B")),
            #          # fmt = lambda x: ("bg",f"{float(x)/1024.:5f} kB")),
            # trdbox_daq_run()
        ])

    def fmt_run(runno):
        if runno >= 0:
            return ("fsm:ready", f"Run: {runno:5d}")
        else:
            return ("fsm:static", f"- no run -")

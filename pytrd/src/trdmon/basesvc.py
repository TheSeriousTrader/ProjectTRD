#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""

"""

# import sys
# import time
# import curses
import urwid
# import proc
# import dimmon
import pydim
import logging

class basesvc(urwid.Text):

    def __init__(self, name, desc=None, default=0, fmt=None, timeout=10):
        super().__init__("---")
        pydim.dic_info_service(name, desc, self.callback, timeout)
        dimwid.register_callback(self)
        self.value = default
        self.format = fmt

    def callback(self, x):
        self.value = x
        dimwid.request_callback(self)

    def refresh(self):
        if self.format is None:
            self.set_text( str(self.value) )
        elif callable(self.format):
            self.set_text( self.format(self.value) )
        else:
            raise(ValueError(f"Unknown format type: {type(self.format)}"))

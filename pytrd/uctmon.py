#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""

"""

import urwid
import logging

from trdmon.dimwid import dimwid as dimwid
import trdmon.trdbox
import trdmon.roc
import trdmon

logging.basicConfig(filename='/tmp/trdmon.log', level=logging.DEBUG,
          format='%(asctime)s %(levelname)s %(funcName)s: %(message)s')
logger=logging.getLogger(__name__)

# ===========================================================================

palette = [
    ('state', 'light green', 'black'),

    ('fsm:off',     'dark gray',  'black'),
    ('fsm:static',  'light blue', 'black'),
    ('fsm:ready',   'light green', 'black'),
    ('fsm:trans',   'yellow',     'black'),
    ('fsm:error',   'white',      'light red'),

    ('bg', 'light gray', 'black'),
    ('foo', 'light red', 'black'),
    ]


top_widget = urwid.Frame(
    header=urwid.Text(("bg", "HEADER")),
    body =
     urwid.AttrMap(urwid.Filler(urwid.Columns([
      urwid.LineBox(trdmon.trdbox.trdbox_daq()),
      urwid.Pile([
        # urwid.LineBox(trdmon.roc.state(0,2,0)),
        urwid.LineBox(trdmon.roc.info(0,2,0))
      ]),
      # trdbox_daq2(),
      # trdbox_daq_run(),
      # trdbox_daq_bytes_read(),
    ])), 'bg'),
    focus_part='header')


trdmon.start(top_widget, palette)

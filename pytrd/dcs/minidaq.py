#
#   tristan/pytrdt/src/dcs/minidaq.py
#

# imports

import struct
import logging
import click
import time
import zmq
import numpy as np

from datetime import datetime

# configure the logging module

logging.basicConfig(filename = 'log1.log',   
                    filemode = 'a',   
                    level = logging.INFO,   
                    format = '%(created)s,%(message)s')  


# creates the zmq environment (opens relative REQ channels to talk to the subevent builders)

class zmq_env:
    def __init__(self):

        self.context = zmq.Context()

        self.trdbox = self.context.socket(zmq.REQ)
        self.trdbox.connect('tcp://localhost:7766')

        self.sfp0 = self.context.socket(zmq.REQ)
        self.sfp0.connect('tcp://localhost:7750')

        self.sfp1 = self.context.socket(zmq.REQ)
        self.sfp1.connect('tcp://localhost:7751')


# a command line tool to run the zmq_env class

@click.group()
@click.pass_context
def minidaq(ctx):
    ctx.obj = zmq_env()

# varous functions to do crap
def get_pretrigger_count(trdbox):
    trdbox.send_string("read 0x102")
    #logging.info('minidaq,trdboxdReadRequestSent,')   # new logs
    cnt = int(trdbox.recv_string(), 16)
    #logging.info('minidaq,trdboxdResponseReceived,')
    return cnt

def wait_for_pretrigger(trdbox):
    cnt = get_pretrigger_count(trdbox)
    while get_pretrigger_count(trdbox) <= cnt:
        #logging.info('minidaq,awaitingTrigger')
        #time.sleep(0.1)
        continue

        # why are we sleeping here? can python not handle a open while loop?

def gen_event_header(payloadsize):
    """Generate MiniDaq header"""
    ti = time.time()
    tis = int(ti)
    tin = ti-tis
    return struct.pack("<LBBBBBBHLL", 
        0xDA7AFEED, # magic
        1, 0, # equipment 1:0 is event
        0, 1, 0, # reserved / header version / reserved
        20, payloadsize, # header, payload sizes
        tis, int(tin) # time stamp
    )


# the readevent command line tool

@minidaq.command()
@click.pass_context
@click.option("--nevents", "-n", default=2, help='Number of events.')
def readevent(ctx, nevents):

    nowtime = datetime.now()

    #outfile = open("data.bin", "wb")


    for ievent in range(nevents):

        # unblock trigger
        ctx.obj.trdbox.send_string("write 0x103 1") 
        #logging.info('minidaq,unblockTriggerSent,')   # new logs 
        print(ctx.obj.trdbox.recv_string(), ievent, "unblocked")
        #logging.info('minidaq,unblockTriggerConfirmed,')

        # log event number
        logging.info(str(ievent))

        # wait for pre-trigger
        wait_for_pretrigger(ctx.obj.trdbox)

        # log pretrigger "found" and take timedelta from time the trdbox is next unblocked
        logging.info('minidaq,preTrigger,')

        # define the equipments that should be read out
        eqlist = [ctx.obj.sfp0, ctx.obj.sfp1]
        
        # send query for data to all equipments
        i = 0
        for eq in eqlist:
            eq.send_string("read")
            #logging.info(f'minidaq,sfp{i}dataRequestSent,')
            i+=1
        
        # receive data
        #data = list(eq.recv() for eq in eqlist)
        #logging.info('minidaq,dataReceived,')

        data

        # build event
        #evdata = bytes()   # save bytes size of data
        #for segment in data:
        #    evdata += segment

        # write to file
        #outfile.write(evdata)

        # fetch scope data
        
        
        #logging.info('minidaq,dataWrittenToFile,')
        

        time.sleep(0.1)
    

    click.echo(f"Runtime: {np.round((datetime.now()-nowtime).total_seconds(), 5)}s")


    #outfile.close()

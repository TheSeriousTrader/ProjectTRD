# imports
import zmq
import logging
import time

# logging config
logging.basicConfig(filename = 'log.csv',   
                    filemode = 'a',   
                    level = logging.INFO,   
                    format = '%(created)s,%(message)s')  

# server function
def trdmon():
    
    # open a REP socket on port 5555 to talk to the control computer
    context = zmq.Context()
    controlSocket = context.socket(zmq.REP)
    controlSocket.bind('tcp://*:5555')
    #controlSocket.bind('ipc:///tmp/test')
    print('trd - running')
    
    # run
    while True:
      
        # wait for trigger from control computer
        print('trd - waiting for trigger from control computer')
        size = controlSocket.recv_string()
        
        # generate data
        print('trd - trigger received from trdbox')
        data = bytes(int(size))
        
        # send data to control computer
        logging.info('trdbox,dataSent')
        controlSocket.send(data)
     
        
   

        



        

        
# execute
trdmon()


# imports
import zmq
import logging
import time

# logging config
logging.basicConfig(filename = 'log.csv',   
                    filemode = 'a',   
                    level = logging.INFO,   
                    format = '%(created)s,%(message)s')  

# function
def requestData(datasize):
    
    # open a REQ socket on port 5555 to talk to trdbox
    context = zmq.Context()
    controlSocket = context.socket(zmq.REQ)
    controlSocket.connect('tcp://localhost:5555')
    #controlSocket.connect('ipc:///tmp/test')
    print('control - connected to TRD')
    
    for i in range(1000):
        
        # send a trigger to the trdbox
        controlSocket.send_string(datasize)
        print('control - trigger sent to TRD')
            
        # receive data from trdbox
        controlSocket.recv()
        logging.info('control,dataReceived')
        
        time.sleep(0.1)
   


        
# execute
requestData('80000')

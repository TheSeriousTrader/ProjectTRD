#include "logicbox/su738.hh"
#include <Csu736.h>
#include <Csu704_pre.h>
#include <scsn_class.h>

#include <CLogicBox.h>
#include "command.hh"
#include "logging.hh"
// #include "data_acquisition.hh"
// #include "ack_channel.hh"
// #include "trigger.hh"
#include "lbox_addr.h"
// #include "trdbox_zmq.hh"
#include </usr/include/dim/dis.hxx>
#include <Poco/RegularExpression.h>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <ratio>

// #include <boost/date_time/posix_time/posix_time.hpp>


using namespace std;

// namespace bptime = boost::posix_time;

// typedef std::chrono::time_point<std::chrono::high_resolution_clock> timestamp_t;
typedef Poco::RegularExpression regex;

// instatiate logging functionality
Poco::Logger& logger = Poco::Logger::get("TRDbox");
Poco::LogStream logs(logger);

// Acknowledge channel
ostringstream tack;




// =========================================================================
// LogicBox USB access thread
// =========================================================================

class logicbox_proxy
{
public:
  logicbox_proxy(zmq::context_t &zmqcontext);
  void run();

  std::thread spawn() {
    logs.notice() << "lbproxy" << 99 << endl;
    return std::thread( [this] { this->run(); } );
  }

protected:
    CLogicBox *pMyLogicBox;
    CCBUS *pMyCBus;

    zmq::socket_t cmdsock;

    trdbox::su738 *sfp[2];
};

logicbox_proxy::logicbox_proxy(zmq::context_t &zmqcontext)
: pMyLogicBox(new CLogicBox) //, CCBUS(pMyLogicBox->getCBusHandel())
, cmdsock (zmqcontext, ZMQ_REP)
{

  // CLogicBox *pMyLogicBox = new CLogicBox;
  pMyCBus = pMyLogicBox->getCBusHandel();
  LOGIC_BOX_LIST* pList = pMyLogicBox->getDevices();

  if (pList->nDevices == 0) {
    logs.critical() << "no TRDbox USB device found" << endl;
    throw 0;
  } else if (pList->nDevices == 1) {
    logs.debug() << "Trying to open device "
                 << pList->pLogicBoxInfo[0].serialNumber << endl;
    int ret = pMyLogicBox->openDevice(pList->pLogicBoxInfo[0].deviceNumber);
    if (ret) {
      logs.critical() << "error opening TRDbox USB device" << endl;
      throw 1;
    }
  } else if (pList->nDevices>1) {
    logs.critical() << "more than one logicbox found - not supported" << endl;
    throw 2;
  }

    sfp[0] = new trdbox::su738( pMyCBus, SU738_BASE_A);
    sfp[1] = new trdbox::su738( pMyCBus, SU738_BASE_B);

    for (int s=0; s<2; s++) {
        int err = sfp[s]->opticalTest(1);
        if (err > 0) {
            logs.information() << err
             << " errors in optical test, reprogramming SFP"
             << endl;

            sfp[s]->program_FPGA();
            sfp[s]->RX_phase();
        }
    }

    logs.notice() << "tests completed, binding to ZeroMQ sockets" << endl;

    // Prepare socket for incoming commands
    // zmq::socket_t cmdsock (zmqcontext, ZMQ_REP);
    // cmdsock.bind ("tcp://localhost:7765");
    cmdsock.bind ("tcp://*:7766");
    cmdsock.bind("inproc://ccbus");
}

void logicbox_proxy::run()
{

    // buffers for the data from the SFPs
    zmq_buffer_t sfpbuffer[] = { zmq_buffer_t(0xF0, 0), zmq_buffer_t(0xF0, 1) };

    // Poco::RegularExpression::MatchVec match;
    std::vector<std::string> match;

    while (true) {
        zmq::message_t request_msg;

        //  Wait for next request from client
        cmdsock.recv (&request_msg);
        std::string request(request_msg.data<char>(), request_msg.size());

        logs.information() << "ccbus received command: " << request << endl;

        char result[1000];

        // variables for sscanf
        int s;
        uint32_t addr,data;

        // ----- mem_dump -----
        if (sscanf(request.c_str(), "dump sfp%i", &s) == 1) {

            if ( ! sfp[s]->mem_dump(sfpbuffer[s])) {
                // we did not read anything from this SFP, so
                sprintf(result,"0");
                // cmdsock.send(result,strlen(result));
                cmdsock.send(NULL,0);
            } else if (uint32_t size = sfpbuffer[s].get_payload_size()) {
                sfpbuffer[s].set_timestamp();
                // uint32_t size = sfpbuffer[s].get_payload_size();
                // logs.notice() << "send " << size << " bytes from SFP " << s << " eq"
                              // << int(sfpbuffer[s].get_header().equipment_id) << endl;
                sprintf(result,"", sfpbuffer[s].get_payload_size());
                cmdsock.send(sfpbuffer[s].get_payload_ptr(),sfpbuffer[s].get_payload_size());
                sfpbuffer[s].set_payload_size(0);


            } else {
                sprintf(result,"0");
                // cmdsock.send(result,strlen(result));
                cmdsock.send(NULL,0);
            }

        // ----------------------------------------------------------------
        // TRIGGER commands
        } else if (strncmp(request.c_str(), "trg ", 4)==0) {

          if (strcmp(request.c_str()+4, "unblock")==0) {
            pMyCBus->write32(SU704_PRE_BASE_A+3, 1);
            sprintf(result,"trigger unblocked");
            cmdsock.send(result,strlen(result));

          } else if (sscanf(request.c_str()+4, "send %d", &s)==1) {
            pMyCBus->write32(SU707_SCSN_BASE_A+8, s);
            sprintf(result,"pretrigger %d sent", s);
            cmdsock.send(result,strlen(result));

          } else {
            logs.warning() << "unknown command: " << request << endl;
            sprintf(result,"???");
            cmdsock.send(result,strlen(result));
          }

        // ----------------------------------------------------------------
        // general SFP commands
        } else if (regex("sfp([01]) reset").split(request, match)) {
            int s = atoi(match[0].c_str());
            sfp[s]->clear_tx();
            sfp[s]->clear_rx();
            logs.information() << "SFP" << s << ": status after RX/TX clear\n" << endl;
            sfp[s]->opt_io_print_status();
            sprintf(result,"reset done");
            cmdsock.send(result,strlen(result));

        } else if (regex("sfp([01]) test1").split(request, match)) {
            int s = atoi(match[0].c_str());
            sfp[s]->opticalTest(1);
            sprintf(result,"optical test done");
            cmdsock.send(result,strlen(result));

        } else if (regex("sfp([01]) test2").split(request, match)) {
            int s = atoi(match[0].c_str());
            sfp[s]->opticalTest(2);
            sprintf(result,"optical test done");
            cmdsock.send(result,strlen(result));

        } else if (regex("sfp([01]) phase").split(request, match)) {
            sfp[s]->RX_phase();
            sprintf(result,"RX phase scan done");
            cmdsock.send(result,strlen(result));

        } else if (regex("sfp([01]) prog").split(request, match)) {
            sfp[s]->program_FPGA();
            sprintf(result,"FPGA programming done");
            cmdsock.send(result,strlen(result));

            // ----- write command -----
        } else if (sscanf(request.c_str(), "write %i %i", &addr, &data) == 2) {
            pMyCBus->write32(addr, data);
            sprintf(result,"OK");
            cmdsock.send(result,strlen(result));

            // ----- read command -----
        } else if (sscanf(request.c_str(), "read %i", &addr) == 1) {
            pMyCBus->read32(addr, &data);
            sprintf(result,"%08X", data);
            // logs.notice() << "read " << data << " from " << addr << endl;
            cmdsock.send(result,strlen(result));

        } else {
            logs.warning() << "unknown command: " << request << endl;
            sprintf(result,"???");
            cmdsock.send(result,strlen(result));
        }

    }

}


// =========================================================================
// Main Function
// =========================================================================

int main()
{

  //bool daemonize = true;
  bool daemonize = false;


  if (daemonize) {
    // =========================================================================
    // fork to background
    // =========================================================================
    switch (fork()) {
    case 0: break; // child process, keep running
    case -1:
      std::cerr << "fork() failed" << endl;
      exit(-1);
    default:
      exit(0);
    }

    Poco::SyslogChannel* syslogch = new Poco::SyslogChannel("trdbox");
    Poco::Logger::root().setChannel(syslogch);
    logger.setChannel(syslogch);

  } else {

    Poco::Logger::root().setChannel(new Poco::ConsoleChannel);
    logger.setChannel(new Poco::ConsoleChannel);

  }

  logger.setLevel(Poco::Message::PRIO_INFORMATION);
  //logger.setLevel(Poco::Message::PRIO_DEBUG);

  logger.notice("starting TRDbox DIM server");



    // =========================================================================
    // ZeroMQ setup
    // =========================================================================
    zmq::context_t zmqcontext (1);

  // =========================================================================
  // Main loop
  // =========================================================================

    // cmdhandler;
    logger.notice("create LogicBox proxy");
    logicbox_proxy lbproxy(zmqcontext);

    sleep(10);

    // logger.notice("start LogicBox proxy");
    lbproxy.run();

    return 0;
}

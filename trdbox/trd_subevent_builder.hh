#ifndef TRDBOX_SUBEVENT_BUILDER_HH
#define TRDBOX_SUBEVENT_BUILDER_HH

#include "trdbox_zmq.hh"
#include <thread>
#include <Poco/Task.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/MapConfiguration.h>
#include <Poco/Util/LayeredConfiguration.h>

//! TRD Subevent Builder
/*!
  The subevent builder merges data fragments read from the optical links into
  subevents.

  The subevent builder listens for requests to read an event on a ZeroMQ
  socket. Once a readout is started, it requests memory dumps via ZeroMQ from
  the LogicBox proxy, concatenates data fragments from the same SFP into a
  buffer, searches for data end markers in the concatenated buffers to split
  subevents, and publishes the subevents on a ZeroMQ socket.
*/

class trd_subevent_builder : public Poco::Task
{

public:
  trd_subevent_builder(zmq::context_t &zmqctx, int sfp, 
                       Poco::Util::AbstractConfiguration* cfg);

  void runTask();

protected:
  void process_fragment(void *ptr, size_t size);
  size_t find_dword(uint32_t marker = 0x00000000);

  zmq::context_t &zmqcontext;
  // std::string query; // command to be sent to ccbus for reading
  // int sfp_index;
  Poco::AutoPtr<Poco::Util::MapConfiguration> defaults;
  Poco::AutoPtr<Poco::Util::LayeredConfiguration> config;

  // data buffer for data from SFP: header + payload
  static const size_t max_payload_size = 16*1024*sizeof(uint32_t);
  size_t buffer_size; // number of read bytes in buffer
  struct buffer_t {
    buffer_t(uint8_t eqclass, uint8_t eqid) : header(eqclass, eqid) {}

    struct header_t header;
    uint8_t payload[max_payload_size];
  } buffer;

};

#endif

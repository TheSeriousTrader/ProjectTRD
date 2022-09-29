// Tristan's Clone

#include "trd_subevent_builder.hh"

#include <Poco/Util/ServerApplication.h>
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>
#include <Poco/Format.h>

#include <sstream>
#include <iostream>
#include <cstdio>

using Poco::format;

using namespace std;

trd_subevent_builder::trd_subevent_builder(zmq::context_t &zmqctx, int sfp,
      Poco::Util::AbstractConfiguration* cfg)
    : Poco::Task(format("sfp%d", sfp))
    , zmqcontext(zmqctx)
    , defaults(new Poco::Util::MapConfiguration)
    , config(new Poco::Util::LayeredConfiguration)
    , buffer(0x10,sfp)
    , buffer_size(0)

{
  defaults->setBool   ("enable",  false);
  defaults->setInt    ("id",      sfp);
  defaults->setString ("name",    format("SFP%d",sfp));
  defaults->setString ("cmdsock", format("tcp://*:%d",7750+sfp));
  defaults->setString ("ccbus",   "tcp://localhost:7766");
  defaults->setString ("query",   format("dump sfp%d",sfp));
  defaults->setInt    ("timeout", 1000000); // in microseconds

  config->add(cfg,10,false);
  config->add(defaults,20,true);
}

void trd_subevent_builder::runTask() {

  freopen("log.log", "w", stderr);

  auto& logger = Poco::Util::Application::instance().logger();
  std::stringstream connstr;

  // set up command socket to receive data
  zmq::socket_t cmdsock(zmqcontext, ZMQ_REP);
  cmdsock.bind(config->getString("cmdsock"));

  // connect to the LogicBox proxy for access to the TRDbox
  zmq::socket_t ccbus(zmqcontext, ZMQ_REQ);
  ccbus.connect(config->getString("ccbus"));

  string query = config->getString("query");
  Poco::Timestamp::TimeDiff timeout = config->getInt("timeout");

  poco_notice_f2(logger, "started subevent builder SFP%d on %s",
    config->getInt("id"), config->getString("cmdsock"));

  while (true) {

    //------------------------------------------------------------------------------
    zmq::message_t request_msg;
    //zmq::message_t event_num;

    // Wait for next request from client
    cmdsock.recv (&request_msg);
    std::string request(request_msg.data<char>(), request_msg.size());

    // Log the time the request arrives
    Poco::Timestamp rxtime;
    poco_information_f3(logger, "%lu,subevd,sfp%d%sCommandReceived,", 
      uint64_t(rxtime.epochMicroseconds()), config->getInt("id"), request);

    //------------------------------------------------------------------------------

    // If the request is a read request (which is what minidaq is sending)
    if (request=="read") {

      buffer.header.payload_size = 0;
      buffer.header.status = 0;

      do {

        zmq::message_t reply_msg;
        ccbus.send(query.begin(), query.end());
        Poco::Timestamp totime;

        // log query send

        poco_information_f2(logger, "%lu,subevd,sfp%dtoTRDBOXD,", 
          uint64_t(totime.epochMicroseconds()), config->getInt("id"));

        ccbus.recv(&reply_msg);
        Poco::Timestamp fromtime;
        poco_information_f2(logger, "%lu,subevd,sfp%dfromTRDBOXD,", 
          uint64_t(fromtime.epochMicroseconds()), config->getInt("id"));

        //poco_information_f2(logger, "received %lu bytes from TRDbox after %d us", reply_msg.size(), int(tbox_txtime.elapsed()));

        if (reply_msg.size() > 0) {
          process_fragment(reply_msg.data(), reply_msg.size());
        }

        if (rxtime.elapsed() > timeout) {
          buffer.header.payload_size = 0;
          buffer.header.status = 1;
          break;
        }

      } 
      
      //------------------------------------------------------------------------------
      
      while (buffer.header.payload_size == 0);
      
        // Send the data to minidaq
        zmq::message_t msg(&buffer, sizeof(header_t)+buffer.header.payload_size);
        cmdsock.send(msg);
        //poco_information_f2(logger, "sent subevent with %lu bytes after %d us", sizeof(header_t)+buffer.header.payload_size, int(rxtime.elapsed()));


        // Log the time the data was sent
        Poco::Timestamp senttime;
        poco_information_f3(logger, "%lu,subevd,sfp%dSentSubEvent,%lu",
          uint64_t(senttime.epochMicroseconds()), config->getInt("id"), sizeof(header_t)+buffer.header.payload_size);
          

      if ( buffer_size > buffer.header.payload_size ) {
        //poco_warning(logger, "partial event");

        memmove(buffer.payload, buffer.payload+buffer.header.payload_size,
                buffer_size - buffer.header.payload_size);
      }
      buffer_size -= buffer.header.payload_size;
    }

    // If the request isn't a read request
    else {
      poco_error_f1(logger, "unknown command '%s'", request);

      buffer.header.payload_size = 0;
      buffer.header.status = 1;

      zmq::message_t msg(&buffer, sizeof(header_t));
      cmdsock.send(msg);
    }

  }
}








size_t trd_subevent_builder::find_dword(uint32_t marker) {

  auto payload = reinterpret_cast<const uint32_t *>(buffer.payload);

  // sub-event building
  size_t payload_size = buffer_size / sizeof(uint32_t);

  for (int i = 0; i < payload_size; i++) {
    // check for endmarker
    if (payload[i] == marker) {
      // move through remaining end markers or until end of data
      while (payload[i] == marker && i < payload_size) {
        i++;
      }
      // we found the EOD marker and moved through all copies
      return i * sizeof(uint32_t);
    }
  }

  return 0;
}




void trd_subevent_builder::process_fragment(void *ptr, size_t size)
{

  auto& logger = Poco::Util::Application::instance().logger();
  Poco::Timestamp foundtime;

  if (size > ( max_payload_size - buffer_size ) ) {
    poco_notice_f1(logger, "buffer overflow on SFP%d - reset buffer",
    config->getInt("id"));
    buffer_size = 0;
    return;
  }

  memcpy(buffer.payload + buffer_size, ptr, size);
  buffer_size += size;

  // TODO: this should only search the new data, not the entire buffer
  size_t event_size = find_dword(0x00000000);

  if (event_size == 0) {
    // no EOD marker found -> wait for rest
    return;
  }

  // We found an event -> copy it to the subevent buffer, send it out,
  poco_information_f3(logger, "%lu,subevd,sfp%dFoundSubEvent,%lu", uint64_t(foundtime.epochMicroseconds()), config->getInt("id"), event_size);
  //poco_information_f1(logger, "subevent found at %lu", uint64_t(foundtime.epochMicroseconds()));


  buffer.header.payload_size = event_size;

}

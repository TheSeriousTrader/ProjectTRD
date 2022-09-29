#include <sstream>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <Poco/Logger.h>
#include <Poco/LogStream.h>
#include </usr/include/dim/dis.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "logicbox/su738.hh"
#include "Csu704_pre.h"
#include "CLogicBox.h"

#include "command.hh"
#include "logging.hh"
#include "ack_channel.hh"
#include "data_acquisition.hh"
#include "trigger.hh"
#include "lbox_addr.h"

#define TX_BASE             0x000
#define RX_FIFO_SIZE        32768
#define RX_BASE             0x008


using namespace std;
// =========================================================================
// DIM Commands
// =========================================================================
commmand::commmand(CLogicBox* lbox,
		   trdbox::su738* sfp0, trdbox::su738*sfp1, su704_pre* io,
		   trigger* TRG, data_acquisition* DAQ)
  : DimCommand("trdbox/command","C"),
    pMyLogicBox(lbox),
    my_su704_pre(io),
    trg(TRG),
    daq(DAQ),
    ack("trdbox/acknowledge")
{
  my_su738[0] = sfp0;
  my_su738[1] = sfp1;
};


void commmand::programSFP(int ccard)
{
  uint32_t lSize;
  uint8_t * buffer;
  size_t result;
  int16_t  rx_phase, tx_phase;
  char  in_file[512];
  strcpy(in_file,":");

  FILE* f_in;

  //Poco::Logger& logger = Poco::Logger::get("TRDbox");
  Poco::LogStream logs(Poco::Logger::get("TRDbox"));

  strcpy(in_file, "/usr/share/trdbox/sfpprog.bin");
  if (strcmp(in_file, ":") != 0)
  {
    // get all content of the inputfile into a uint8_t*
    logs.debug() << "Reading from input file " << in_file << endl;
    f_in = fopen(in_file, "rb");
    if (f_in == NULL)
      logs.error() << "failure to open " << in_file << endl;

    // obtain file size:
    fseek(f_in, 0, SEEK_END);
    lSize = ftell(f_in);
    rewind(f_in);
    // allocate memory to contain the whole file:
    buffer = (uint8_t*) malloc(sizeof(uint8_t) * lSize);
    if (buffer == NULL)
      logs.error() << "memory allocation failed" << endl;
    //printf("Memory error");

    // copy the file into the buffer:
    result = fread(buffer, 1, lSize, f_in);
    if (result != lSize)
      logs.error() << "reading from of SFP data from file failed" << endl;
    //printf("Reading error");

    // and call my_su738->program_FPGA
    int a = my_su738[ccard]->program_FPGA(buffer, lSize);

    if (a == 0)
      logs.information() << "SFP programming completed" << endl;

    fclose(f_in);
    free(buffer);
    strcpy(in_file, ":");
  }
}

int commmand::opticalTest(int i, int ccard)
{
  uint8_t  mode = 1;
  uint8_t  show_mess = 0;
  uint32_t runs = 15;
  uint32_t wrk32, err, spi_cnf[4];

  spi_cnf[0] = 0x1e | 0x030100; // fpga and tlk loopback
  spi_cnf[1] = 0x0e | 0x030200; // tlk loopback
  spi_cnf[2] = 0x06 | 0x030300; // ext loopback, same config for normal operation and after power up except for the LEDs
  spi_cnf[3] = 0x06 | 0x000000; // normal operation and after power up

  if (i==1)
    logs.notice() << "Testing SFP " << ccard
		  << " with FPGA-loopback..." << endl;

  else if (i==2)
    logs.notice() << "Testing SFP " << ccard
		  << " with TLK-loopback..." << endl;

  else if (i==3)
    logs.notice() << "Testing SFP " << ccard
		  << " with external loopback..." << endl;

  else
    logs.error() << "Invalid test for SFP " << ccard
		 << " requested" << endl;

  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[i-1], &wrk32);
  if (i == 3) sleep(2);
  logs.debug() << "clear RX/TX" << endl;
  my_su738[ccard]->clear_tx();
  my_su738[ccard]->clear_rx();

  logs.debug() << "start testing..." << endl;
  err = 0;
  for (mode=0; mode<4; mode++)
      err += my_su738[ccard]->test_opt_io_burst(mode, runs, show_mess, 3);

  if (err > 0) {
    logs.error() << "test completed with " << err << " errors" << endl;
    printf("%d errors found!\n",err);
  } else {
    logs.information() << "test competed without error" << endl;
  }

  my_su738[ccard]->opt_io_print_status();
  sleep(1);
  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[3], &wrk32);
  return err;
}

void commmand::RX_phase(int ccard)
{
  uint8_t  mode = 1;
  uint8_t  show_mess = 0;
  uint32_t runs = 15;
  uint32_t wrk32, spi_cnf[4];

  spi_cnf[0] = 0x1e | 0x030100; // fpga and tlk loopback
  spi_cnf[1] = 0x0e | 0x030200; // tlk loopback
  spi_cnf[2] = 0x06 | 0x030300; // ext loopback, same config for normal operation and after power up except for the LEDs
  spi_cnf[3] = 0x06 | 0x000000; // normal operation and after power up

  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[0], &wrk32);
    logs.notice() << "Testing SFP " << ccard
		  << " rx_phase with FPGA-loopback..." << endl;

  logs.debug() << "clear RX/TX" << endl;
  my_su738[ccard]->clear_tx();
  my_su738[ccard]->clear_rx();

  logs.debug() << "start testing..." << endl;
  my_su738[ccard]->test_rxtx_ph(0, 0);
  my_su738[ccard]->opt_io_print_status();

  int err = 0;
  for (mode=0; mode<4; mode++)
      err += my_su738[ccard]->test_opt_io_burst(mode, runs, show_mess, 3);

  if (err > 0) {
    logs.error() << "test completed with " << err << " errors" << endl;
    printf("%d errors found!\n",err);
  } else {
    logs.information() << "test competed without error" << endl;
  }

  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[3], &wrk32);
}

void commmand::opticalWrite(int SFP, uint32_t Data)
{
  if (SFP == 0)
  {
    pMyLogicBox->write32(SU738_BASE_A + TX_BASE, Data);
    pMyLogicBox->write32(SU738_BASE_A + TX_BASE, ~Data);
  }
  else
    pMyLogicBox->write32(SU738_BASE_B + TX_BASE, Data);
    pMyLogicBox->write32(SU738_BASE_B + TX_BASE, ~Data);
}


//==============================================================================
//  commmand Handler
//==============================================================================
void commmand::commandHandler()
{

  string cmd = getString();
  istringstream args(cmd);

  // parse 0x... and 0... as properly as hex and dec
  // https://stackoverflow.com/questions/13196589/getting-hex-through-cin
  args.unsetf(std::ios::dec);
  args.unsetf(std::ios::hex);
  args.unsetf(std::ios::oct);

  logs.notice() << "TrdBox: received command: '" << cmd << "'" << endl;

  // reset ACK channel
  tack.str("");
  tack << "<trdbox>" << endl;

  // -------------------------------------------------------------------------
  // delegation to other processing commands

  if ( cmd.find("trg") == 0 || cmd.find("dscr") == 0) {
    trg->process(args);
    return;
  }



  // -------------------------------------------------------------------------
  // split cmd into arguments
  string cmdopt[4];
  int j = 0;

  do
  {
    if (j >= 4)
    {
      return;
    }
      string sub;
      args >> sub;
      cmdopt[j] = sub;
      j++;
  } while (args);

  // =========================================================================
  // ROB light
  // =========================================================================
  if ( cmdopt[0] == "rob" )
  {
    if ( cmdopt[1] == "on") {
      my_su704_pre->set_rob_power(1);
      printf("ROB powered ON\n");
    }
    else
    if ( cmdopt[1] == "off") {
      my_su704_pre->set_rob_power(0);
      printf("ROB powered OFF\n");
    }
    else
    if ( cmdopt[1] == "reset") {
        my_su704_pre->rob_reset();
        printf("ROB reset sent\n");
    }
  }

  //// =========================================================================
  //// TRIGGER
  //// =========================================================================
  //else if ( cmdopt[0] == "trg" )
  //{
  //
  //  if ( cmdopt[1] == "single") {
  //
  //    logs.debug() << "send SINGLE trigger" << endl;
  //
  //    trg->set_source(trigger::single);
  //    trg->send_pcmd(1);
  //
  //  } else if ( cmdopt[1] == "burst") {
  //
  //    trg->set_burst_counter(atoi(cmdopt[2].c_str()));
  //    logs.debug() << "send " <<  trg->get_burst_counter()
  //		   << " triggers in BURST mode" << endl;
  //
  //    trg->set_source(trigger::burst);
  //    trg->send_pcmd(1);
  //
  //    printf("trigger BURST %d\n", trg->get_burst_counter());
  //
  //  } else if ( cmdopt[1] == "off") {
  //
  //    logs.debug() << "trigger OFF" << endl;
  //    trg->set_source(trigger::off);
  //
  //  } else {
  //    logs.error() << "invalid trigger command '"
  //		   << cmdopt[1] << "'" << endl;
  //  }
  //}


  // =========================================================================
  // Optical Tests
  // =========================================================================
  else if ( cmdopt[0] == "OPTtest")
  {
    logs.notice() << "running optical tests" << endl;
    int i = atoi(cmdopt[2].c_str());
    int card = atoi(cmdopt[1].c_str());
    if (card == 2) {
      for (int j=0; j<2; j++) {
        switch (i) {
          case 1:
          case 2:
          case 3:
            opticalTest(i, j);
            break;
          case 4:
            for (int k=1; k<4; k++) {
              opticalTest(k, j);
            }
            break;
        }
      }
    }
    else
    if (card == 0 | card == 1) {
      switch (i) {
        case 1:
        case 2:
        case 3:
          opticalTest(i, card);
          break;
        case 4:
          for (int k=1; k<4; k++) {
            opticalTest(k, card);
          }
          break;
      }
    }
    else  printf("Invalid SFP");
  }
// =========================================================================
// Optical Write
// =========================================================================
  else
  if ( cmdopt[0] == "OPTwrite")
    {
      int card = atoi(cmdopt[1].c_str());
      int data = atoi(cmdopt[2].c_str());
      if (card == 2)
      {
        for (int j=0; j<2; j++)
        {
          opticalWrite(j, 0xDEADBEEF);
        }
      }
      else
      if (card == 0 | card == 1)
      {
        opticalWrite(card, 0xDEADBEEF);
      }
      else printf("Invalid SFP");
    }

  // =========================================================================
  // start/stop runs
  // =========================================================================
  else if (cmdopt[0] == "run") {

    if (cmdopt[1] == "start") {
      trg->set_burst_counter(0); // disable triggers
      trg->clear_busy(); // clear the busy, but trigger are disabled
      daq->start_run();

    } else if (cmdopt[1] == "stop" || cmdopt[1] == "end" ) {
      daq->stop_run();
    }
  }

  // =========================================================================
  // reset LogicBox design and SFP buffers
  // =========================================================================
  else if (cmdopt[1] == "sfp") {
    pMyLogicBox->reset();

    for (int card=0; card<2; card++) {
      my_su738[card]->clear_tx();
      my_su738[card]->clear_rx();
      printf("Card %d: Status of rx-tx after clear of tx\n", card);
      my_su738[card]->opt_io_print_status();
    }

  }

  // =========================================================================
  // reset design and SFP buffers
  // =========================================================================
  else if (cmdopt[0] == "sfp") {

    if (cmdopt[1] == "reset") {

      //pMyLogicBox->reset();

      for (int card=0; card<2; card++) {
	my_su738[card]->clear_tx();
	my_su738[card]->clear_rx();
	logs.information() << "Card " << card
			   << ": status after RX/TX clear\n" << endl;
	my_su738[card]->opt_io_print_status();
      }

    } else if (cmdopt[1] == "arm") {

      for (int ccard=0; ccard<2; ccard++) {
	int err = opticalTest(1, ccard);
	if (err > 0) {
	  logs.information() << err
			     << " errors in optical test, reprogramming SFP"
			     << endl;

	  programSFP(ccard);
	  RX_phase(ccard);
	}
      }

    } else if (cmdopt[1] == "prog") {
      for (int card=0; card<2; card++) {
	programSFP(card);
	RX_phase(card);
      }
    }
  }

  // =========================================================================
  // Other
  // =========================================================================
  else
  {
    printf("Incorrect Command\n");
  }

  tack << "</trdbox>" << endl;
  ack = tack.str();
  ack.updateService();
}

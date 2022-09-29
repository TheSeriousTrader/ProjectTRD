#include <sstream>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <Poco/Logger.h>
#include </usr/include/dim/dis.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Csu704_pre.h"
#include "CLogicBox.h"
#include "data_acquisition.hh"
#include "logicbox/su738.hh"
#include "trigger.hh"
#include "lbox_addr.h"
#include "logging.hh"

#define TX_BASE             0x000
#define RX_FIFO_SIZE        32768
#define RX_BASE             0x008


using namespace std;

// =========================================================================
// DAQ Interface
// =========================================================================
data_acquisition::data_acquisition( trdbox::su738* sfp0, trdbox::su738*sfp1,
				    //su704_pre* io,
				    trigger* trig,
				    string data_dir)
  : trg(trig),
    //pMyLogicBox(lbox),
    //my_su704_pre(io),
    run(norun),
    lastrun(norun),
    datadir(data_dir),
    out_file(NULL),
    svc_run(0),
    evno("trdbox/DAQ/EVENT_NUMBER"),
    rdbytes("trdbox/DAQ/BYTES_READ"),
    wrbytes("trdbox/DAQ/BYTES_WRITTEN")
{
  my_su738[0] = sfp0;
  my_su738[1] = sfp1;

  ifstream run_file ( (datadir+"/run_no.txt").c_str() );
  if(run_file.is_open()) {
    run_file >> lastrun;
  } else {
    Poco::Logger::get("TRDbox").error("run file is not accessible");
  }

  svc_run = new DimService("trdbox/RUN_NUMBER", run);
  svc_run->updateService();

  //// start the reader thread
  //pthread_attr_t attr;
  //pthread_attr_init(&attr);
  //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  //
  //pthread_create(&reader_thread_handle, &attr,
  //		 data_acquisition::reader_thread_function, (void*)this);
};


void* data_acquisition::reader_thread_function(void* arg)
{
  data_acquisition* daq = (data_acquisition *)arg;

  logs.information() << "starting reader thread" << endl;

  while(1) {
    daq->read_event();
  }

  return NULL;
}

///*
//void data_acquisition::programSFP(int ccard)
//{
//  uint32_t lSize;
//  uint8_t * buffer;
//  size_t result;
//  int16_t  rx_phase, tx_phase;
//  char  in_file[512];
//  strcpy(in_file,":");
//
//  FILE* f_in;
//
//  strcpy(in_file, "data/byte.bin");
//  if (strcmp(in_file, ":") != 0)
//  {
//// get all content of the inputfile into a uint8_t*
//    printf("Reading from input file %s\n", in_file);
//    f_in = fopen(in_file, "rb");
//    if (f_in == NULL)
//        printf("opening not successfull");
//// obtain file size:
//    fseek(f_in, 0, SEEK_END);
//    lSize = ftell(f_in);
//    rewind(f_in);
//// allocate memory to contain the whole file:
//    buffer = (uint8_t*) malloc(sizeof(uint8_t) * lSize);
//    if (buffer == NULL)
//        printf("Memory error");
//// copy the file into the buffer:
//    result = fread(buffer, 1, lSize, f_in);
//    if (result != lSize)
//        printf("Reading error");
//
//// and call my_su738->programm_FPGA
//    int a = my_su738[ccard]->programm_FPGA(buffer, lSize);
//
//    if (a == 0)
//        printf("Programming done.\n");
//    fclose(f_in);
//    free(buffer);
//    strcpy(in_file, ":");
//  }
//}
//
//int data_acquisition::opticalTest(int i, int ccard)
//{
//  uint8_t  mode = 1;
//  uint8_t  show_mess = 0;
//  uint32_t runs = 15;
//  uint32_t wrk32, err, spi_cnf[4];
//
//  spi_cnf[0] = 0x1e | 0x030100; // fpga and tlk loopback
//  spi_cnf[1] = 0x0e | 0x030200; // tlk loopback
//  spi_cnf[2] = 0x06 | 0x030300; // ext loopback, same config for normal operation and after power up except for the LEDs
//  spi_cnf[3] = 0x06 | 0x000000; // normal operation and after power up
//
//  printf("************ >>>>>> SFP");
//  cout << ccard;
//  if (i==1)
//  printf("   Testing with FPGA-loopback...\n");
//  else if (i==2)
//  printf("   Testing with TLK-loopback...\n");
//  else if (i==3)
//  printf("   Testing with external-loopback...\n");
//  else
//  printf("Invalid\n");
//
//  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[i-1], &wrk32);
//  if (i == 3) sleep(2);
//  printf("clear rx-tx\n");
//  my_su738[ccard]->clear_tx();
//  my_su738[ccard]->clear_rx();
//  printf("start testing...\n");
//  err = 0;
//  for (mode=0; mode<4; mode++)
//      err += my_su738[ccard]->test_opt_io_burst(mode, runs, show_mess, 3);
//  printf("Done. ");
//  if (err > 0) printf("%d errors found!\n",err);
//  else
//      printf(" OK!\n");
//  my_su738[ccard]->opt_io_print_status();
//  sleep(1);
//  printf("\n");
//  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[3], &wrk32);
//  return err;
//}
//
//void data_acquisition::RX_phase(int ccard)
//{
//  uint8_t  mode = 1;
//  uint8_t  show_mess = 0;
//  uint32_t runs = 15;
//  uint32_t wrk32, spi_cnf[4];
//
//  spi_cnf[0] = 0x1e | 0x030100; // fpga and tlk loopback
//  spi_cnf[1] = 0x0e | 0x030200; // tlk loopback
//  spi_cnf[2] = 0x06 | 0x030300; // ext loopback, same config for normal operation and after power up except for the LEDs
//  spi_cnf[3] = 0x06 | 0x000000; // normal operation and after power up
//
//  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[0], &wrk32);
//  printf("************ >>>>>> SFP");
//  cout << ccard;
//  printf("    Testing rx_phase with FPGA-loopback...\n");
//
//  printf("clear rx-tx\n");
//  my_su738[ccard]->clear_tx();
//  my_su738[ccard]->clear_rx();
//  printf("start testing...\n");
//
//  my_su738[ccard]->test_rxtx_ph(0, 0);
//  my_su738[ccard]->opt_io_print_status();
//
//  int err = 0;
//  for (mode=0; mode<4; mode++)
//      err += my_su738[ccard]->test_opt_io_burst(mode, runs, show_mess, 3);
//  printf("Errors found:");
//  cout << err;
//  printf("\n");
//  my_su738[ccard]->spi_rw_lb(SPI_A_CNF, 1, spi_cnf[3], &wrk32);
//}
//
//void data_acquisition::opticalWrite(int SFP, uint32_t Data)
//{
//  if (SFP == 0)
//  {
//    pMyLogicBox->write32(SU738_BASE_A + TX_BASE, Data);
//    pMyLogicBox->write32(SU738_BASE_A + TX_BASE, ~Data);
//  }
//  else
//    pMyLogicBox->write32(SU738_BASE_B + TX_BASE, Data);
//    pMyLogicBox->write32(SU738_BASE_B + TX_BASE, ~Data);
//}
//
//*/


void data_acquisition::start_run()
{
  // lock out_file mutex
  //pthread_mutex_lock(&mutex_file);

  run = lastrun+1;
  lastrun = run;

  evno = 0;
  rdbytes = 0;
  wrbytes = 0;

  char filename[1000];
  sprintf(filename, "%s/%04d", datadir.c_str(), run);
  out_file = fopen(filename,"a");
  if (!out_file) {
    poco_error(Poco::Logger::get("TRDbox"), "unable to open output file");
  }

  ofstream run_file ((datadir+"/run_no.txt").c_str());
  if(run_file.is_open()) {
    run_file << run;
  }

  // unlock out_file mutex
  //pthread_mutex_unlock(&mutex_file);

  svc_run->updateService();
  poco_notice_f1(Poco::Logger::get("TRDbox"), "started run %d", lastrun);
}

void data_acquisition::stop_run()
{
  run = norun;

  //pthread_mutex_lock(&mutex_file);

  fclose(out_file);
  out_file = NULL;

  //pthread_mutex_unlock(&mutex_file);

  svc_run->updateService();
  poco_notice_f1(Poco::Logger::get("TRDbox"), "stopped run %d", lastrun);
}



bool data_acquisition::read_event()
{
  if ( run == norun || !out_file) {
    usleep(100);
    return false;
  }

  boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();

  const int nsfp = 2;
  const size_t buf_size = 16000;
  uint32_t buffer[nsfp*buf_size]; // reserve space for (nsfp) data blocks

  uint32_t* buf[nsfp];    // start of buffer for sfp
  size_t    curr[nsfp];   // last read dword
  size_t    eodm[nsfp];   // end-of-data marker (0x00000000)

  bool readflag = false;
  bool compflag = false;

  // initialize pointers
  for (int sfp=0; sfp<nsfp;sfp++) {
    buf[sfp]  = buffer + sfp*buf_size;
    curr[sfp] = 0;
    eodm[sfp] = 0;
  }

	// buf[0] = NULL; // disable reading from SFP0

  // try reading
  while (compflag == false) {

    compflag = true;

    for (int sfp=0; sfp<nsfp;sfp++) {

      if ( ! buf[sfp] ){
				continue; //
			}

			size_t maxlen = buf_size-(curr[sfp]);
      curr[sfp] += my_su738[sfp]->mem_dump( buf[sfp]+curr[sfp], maxlen );

      if ( curr[sfp] > 0 ) {
				// poco_notice_f1(Poco::Logger::get("TRDbox"), "read from SFP %d", sfp);
				readflag = true;

				if ( buf[sfp][curr[sfp]-1] != 0x00000000 ) {
					compflag = false;

				}
      }
    }

    if (readflag==false) {
      // nothing was read so far
      return false;
    }

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    if ( (now-start).total_seconds() > 30 ) {
      Poco::Logger::get("TRDbox").error("timeout while reading event");
      break;
    }

  }


  rdbytes += (curr[0] + curr[1]);

  // reading is done, let's clear the busy while we write the event...
  trg->clear_busy();

  // TODO: should so some sanity checks of data
  //pthread_mutex_lock(&mutex_file);

  if (!out_file) {
    //pthread_mutex_unlock(&mutex_file);
    return true;
  }

  int wrsize = 0;

  // write data segments to file
  wrsize += fprintf(out_file, "# EVENT\n");
  wrsize += fprintf(out_file, "# format version:    1.0\n");
  wrsize += fprintf(out_file, "# time stamp:        %s\n",
		    to_iso_extended_string(start).c_str() );
  //to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()).c_str() );
  wrsize += fprintf(out_file, "# data blocks:       %d\n", nsfp);

  for (int sfp=0; sfp<nsfp;sfp++) {

    wrsize += fprintf(out_file, "## DATA SEGMENT\n");
    wrsize += fprintf(out_file, "## sfp:            %d\n", sfp);
    wrsize += fprintf(out_file, "## size:           %d\n", curr[sfp]);

    for (int i=0; i<curr[sfp]; i++) {
      wrsize += fprintf(out_file, "0x%08x\n", buf[sfp][i]);
    }
  }

  wrbytes += wrsize;
  evno++;

  //pthread_mutex_unlock(&mutex_file);

  //trg->clear_busy();

  //if ( run != norun && trg->get_source() == trigger::burst ) {
  //  trg->send_pcmd(1);
  //}

  return true;
}

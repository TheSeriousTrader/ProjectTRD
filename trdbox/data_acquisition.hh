#ifndef DATA_ACQUISITION_H
#define DATA_ACQUISITION_H

#include "logicbox/su738.hh"
//#include "Csu704_pre.h"
#include "CLogicBox.h"
#include </usr/include/dim/dis.hxx>
#include <pthread.h>
#include <string>

#include "dim_services.hh"

using namespace std;
// =========================================================================
// DIM Commands
// =========================================================================

class trigger;

class data_acquisition
{
public:
  data_acquisition(//CLogicBox* lbox,
		   trdbox::su738* sfp0, trdbox::su738*sfp1,
		   //su704_pre* io,
		   trigger* trig,
		   std::string data_dir="/data/raw");

  //void openDevice();
  //void programSFP(int ccard);
  //void readEvent(int card);
  bool read_event();
  //int  opticalTest(int i, int ccard);
  //void RX_phase(int ccard);
  //void opticalWrite(int SFP, uint32_t Data);
  void start_run();
  void stop_run();

	int get_run_number() { return run; }

protected:

  void commandHandler();
  //CCBUS *pMyCBus;
  //CLogicBox *pMyLogicBox;
  //LOGIC_BOX_LIST* pList;
  trdbox::su738 *my_su738[2];
  //su704_pre *my_su704_pre;

  trigger*              trg;

  int run;
  int lastrun;

  std::string datadir;
  FILE* out_file;

  DimService* svc_run;

  dim_int evno;
  dim_int rdbytes;
  dim_int wrbytes;

  static const int norun = -99; // avoid misinterpretation of -1 in DID

  // keep the reading / saving of data in a separtate thread
  static void* reader_thread_function(void* arg);
  pthread_t reader_thread_handle;

  // there is still a mutex around - not sure if it is actually used
  pthread_mutex_t mutex_file;


};

#endif

#ifndef COMMAND_H
#define COMMAND_H

// #include "logicbox/su738.h"
#include "Csu704_pre.h"
#include "CLogicBox.h"
#include </usr/include/dim/dis.hxx>

#include "dim_services.hh"

using namespace std;
// =========================================================================
// DIM Commands
// =========================================================================

class trigger;
class data_acquisition;

namespace trdbox {
	class su738;
};

class commmand: public DimCommand
{
public:
  commmand(CLogicBox* lbox,
	   trdbox::su738* sfp0, trdbox::su738*sfp1, su704_pre* io,
	   trigger* trig,
	   data_acquisition* daq);

  void openDevice();
  void programSFP(int ccard);

  int  opticalTest(int i, int ccard);
  void RX_phase(int ccard);
  void opticalWrite(int SFP, uint32_t Data);


protected:

  void commandHandler();

  // low-level hardware access -> should be phased out in favor of
  // higher level interfaces, e.g. DAQ, TRG
  CLogicBox *pMyLogicBox;
  trdbox::su738 *my_su738[2];
  su704_pre *my_su704_pre;

  // high-level interfaces
  trigger*              trg;
  data_acquisition*     daq;

  // acknowledge channel
  dim_string ack;
};

#endif

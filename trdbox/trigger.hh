#ifndef TRDBOX_TRIGGER_HH
#define TRDBOX_TRIGGER_HH

#include <scsn_class.h>
#include <Csu704_pre.h>
#include <Csu736.h>

#include <sstream>

class trigger
{
public:
  trigger(scsn_bus *sc, su704_pre* p, su736* d);

  void process(std::istringstream& args);
  
  void send_trigger();

  enum src_t {trgoff=0,  trglut1=1, trglut2=2, trglut3=3,
	      trglut4=4, trglut5=5, trglut6=6, trglut7=7,
	      trgsoft=8 };
  
  void set_source(src_t s);
  src_t get_source() { return src; }
  
  void set_burst_counter(int n) { burstcnt = n; }
  int get_burst_counter() { return burstcnt; }

  // busy handling
  void set_busy() { busy = true; }
  void clear_busy();
  bool is_busy() { return busy; }

  //// discriminator configuration
  //void set_threshold  (int n, int t) { dscr->set_thr(n,t); }
  //void set_hysteresis (int n, int h) { dscr->set_hys(n,h); }
  
  
  
protected:
  scsn_bus*  scsn;
  su704_pre* ptrg;
  su736*     dscr;
  
  src_t src;
  int burstcnt;

  bool busy;
};
 

#endif


#include "trigger.hh"
#include "logging.hh"

#include <iomanip>
#include <string>

using namespace std;

trigger::trigger(scsn_bus* sc, su704_pre* p, su736* d)
  : scsn(sc), ptrg(p), dscr(d), src(trgoff), burstcnt(0)
{}

union ptrg_conf_t {
  uint32_t raw;
  struct {
    unsigned int  ext_width : 8;
    unsigned int  ext_delay : 8;
    unsigned int  trg_lut   : 8;
    unsigned int  mon_out   : 4;
    unsigned int  rob_power : 1;
    unsigned int  pre_ena   : 1;
    unsigned int  pre_inv   : 1;
    unsigned int  autoblock : 1;
  };

  ptrg_conf_t(uint32_t other)
  { this->raw = other; }
  
  //union ptrg_conf_t&  operator=(uint32_t other)
  //{ this->raw = other; }
};

ostream& operator<<(ostream& os, ptrg_conf_t pc)
{
  os << "0x" << hex << setfill('0') << setw(8) << pc.raw
     << setfill(' ') << dec;
  return os;
}


void trigger::process(std::istringstream& args)
{
  string cmd, subcmd;
  args >> cmd;
  
  if ( cmd == "trg" ) {
    
    args >> subcmd;
    
    if ( subcmd == "show") {

      logs.notice() << "foo" << endl;
      
      ptrg_conf_t ptrcfg = ptrg->get_pre_conf();
      logs.notice() << "bar" << endl;
      //uint32_t dggcfg = ptrg->get_dgg_conf();
      logs.notice() << "baz" << endl;

      
      logs.notice() << "ptrg conf: "       
		    << hex << setfill('0')
		    << "0x" << setw(8) << ptrcfg.raw
		    << dec << setfill(' ')
		    << endl;
      
    } else if ( subcmd == "single") {

      logs.notice() << "sending single trigger " << endl;
      set_burst_counter(1);

      // if we are not busy, we need to send the first trigger
      if (!is_busy()) {
	logs.notice() << "call send_trigger()" << endl;
	send_trigger();
      } else {
	logs.notice() << "waiting for clear_busy() to send trigger" << endl;
      }
      
    } else if ( subcmd == "on") {

      logs.notice() << "start sending triggers " << endl;
      set_burst_counter(-1);

      // if we are not busy, we need to send the first trigger
      if (!is_busy()) send_trigger();
      
    } else if ( subcmd == "burst") { 

      int ntrg;
      args >> ntrg;

      logs.notice() << "send " <<  ntrg
		   << " triggers in BURST mode" << endl;

      //set_source(trgsoft);
      set_burst_counter(ntrg);

      // if we are not busy, we need to send the first trigger
      if (!is_busy()) send_trigger();
      
    } else if ( subcmd == "off") {
      
      logs.notice() << "trigger OFF" << endl;
      set_burst_counter(0);
      set_source(trgoff);
      
    } else if ( subcmd == "lut") {
      
      int lutval;
      args >> lutval;

      logs.notice() << "trigger LUT <- " << lutval << endl;

      // we need an explicit conversion:
      set_source( static_cast<src_t>(lutval) );
      
    } else if ( subcmd == "unblock") {
      logs.notice() << "trigger UNBLOCK " << endl;
      ptrg->pre_unblock();

    } else if ( subcmd == "clr") {
      logs.notice() << "trigger CLR " << endl;
      ptrg->clr_pre_counter();

    } else if ( subcmd == "read") {
      logs.notice() << "trigger READ " << endl;
      ptrg->print_pre_conf();
      ptrg->print_pre_status();
      
    } else if ( subcmd == "conf") {
      
      int cfgreg; args >> cfgreg;

      logs.notice() << "trigger CONF <- " << cfgreg << endl;

      ptrg->set_pre_conf( (cfgreg & 0xFF000000) >> 24,  // ext_width
			  (cfgreg & 0x00FF0000) >> 16,  // ext_delay
			  (cfgreg & 0x0000FF00) >>  8,  // LUT
			  (cfgreg & 0x000000F0) >>  4,  // mon out
			  (cfgreg & 0x00000001) >>  0,  // autoblock
			  (cfgreg & 0x00000002) >>  1,  // pre inv
			  (cfgreg & 0x00000004) >>  2,  // pre enable
			  (cfgreg & 0x00000008) >>  3); // ROB power

    } else if ( subcmd == "send" ) {

      int ptcmd; args >> ptcmd;

      logs.notice() << "trigger send " << ptcmd << endl;
      scsn->scsn_pretr(ptcmd);

    } else {
      logs.error() << "invalid trigger command '"
		   << subcmd << "'" << endl;
    }

  } else if ( cmd == "dscr" ) {

    args >> subcmd;
    
    if ( subcmd == "threshold" || subcmd == "thr" ) {
      int n; args >> n;
      int t; args >> t;

      logs.information() << "set discriminator " << n
			 << " threshold to " << t << endl;

      dscr->set_thr(n,t);
      
    } else if ( subcmd == "hystresis" || subcmd == "hys" ) {
      int n; args >> n;
      int h; args >> h;

      logs.information() << "set discriminator " << n
			 << " hysteresis to " << h << endl;

      dscr->set_hys(n,h);
      
    } else if ( subcmd == "enainv" || subcmd == "ena" ) {
      int n; args >> n;

      logs.notice() << "set discriminator enable/invert flags to "
		    << n << endl;
      
      dscr->set_conf(n);

    } else if ( subcmd == "dgg" ) {
      int d1,w1,d2,w2;
      args >> w1 >> d1 >> w2 >> d2;

      logs.notice() << "set discriminator delays" << endl;
      
      ptrg->set_dgg_conf(w2,d2,w1,d1);

    } else {
      logs.error() << "invalid discriminator command '"
		   << subcmd << "'" << endl;
    }
  }
  
}
  



void trigger::clear_busy()
{
  // clear the busy flag
  busy = false;

  send_trigger();
}

void trigger::send_trigger()
{

  if ( is_busy() ) {
    logs.error() << "send trigger requqested while busy" << endl;
    return;
  }

  if ( burstcnt < -1 ) {
    logs.error() << "invalid value of burstcnt (" 
                 << burstcnt << ")" << endl;
    return;
  }

  if ( burstcnt == 0 ) {
    logs.notice() << "no more triggers to be sent" << endl;
    return;
  }


  // all checks complete -> we will send the trigger

  // decrease the counter
  if (burstcnt > 0) {
    logs.notice() << "sending trigger, " << burstcnt << " to go" << endl;
    burstcnt--;
  } else {
    logs.notice() << "sending trigger" << endl;
  }

  set_busy();

  // enable sending of triggers again
  ptrg->pre_unblock();
  
  // if we are using software triggers, send one my hand.
  if (get_source()==trgsoft) {
    scsn->scsn_pretr(1);
  }

}


void trigger::set_source(src_t s)
{
  src = s;

  if (src<8) {
    ptrg->set_LUT(src);
  }
}

//
// else
//   if ( strcmp(argv[idx],"--dis_ena_inv") ==0 )
//     {
//       idx++;
//       read_hex_dec(argv[idx], &dac_value);
//       my_su736->set_conf(dac_value);
//     }
//   else
//        if ( strcmp(argv[idx],"--dis_dgg") ==0 )
//        {
//            idx++;
//            read_hex_dec(argv[idx], &dgg_delay);
//            idx++;
//            read_hex_dec(argv[idx], &dgg_width);
//            my_su704_pre->set_dgg_conf(dgg_width, dgg_delay, dgg_width, dgg_delay);
//        }
//        else
//        if ( strcmp(argv[idx],"--LUT") ==0 )
//        {
//            idx++;
//            read_hex_dec(argv[idx], &lut_trg);
//        }
//        else
//        if ( strcmp(argv[idx],"--monout") ==0 )
//        {
//            idx++;
//            read_hex_dec(argv[idx], &mon_out);
//            my_su704_pre->set_mon_mux(mon_out);
//        }
//        else
//        if ( strcmp(argv[idx],"--ext_dgg") ==0 )
//        {
//            idx++;
//            read_hex_dec(argv[idx], &dgg_inv);
//            idx++;
//            read_hex_dec(argv[idx], &dgg_delay);
//            idx++;
//            read_hex_dec(argv[idx], &dgg_width);
//            my_su704_pre->set_dgg_ext(dgg_width, dgg_delay, dgg_inv, 1);
//
//

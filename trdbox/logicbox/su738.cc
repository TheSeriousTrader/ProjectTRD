// $Id: Csu738.cpp 344 2015-03-04 15:55:14Z angelov $:
#include <iomanip>
#include <sstream>

#include "logicbox/su738.hh"
#include "Logging.h"

using namespace std;

namespace trdbox {

su738::su738(CCBUS *pCBus, uint32_t new_base_addr)
{
    m_pCBus = pCBus;
    base_addr = new_base_addr;

    // debug = new_debug;

    cpu0s = 0x8100;
    cpu1s = 0x8200;
    cpu2s = 0x8300;
    cpu3s = 0x8400;
//    set_dcm(DCM_LB_RX, 0, 0);
//    set_dcm(DCM_LB_TX, 1, 0);
}

su738::~su738()
{
    // do nothing
}

uint32_t su738::get_fifo_cnt(void)
{
    uint32_t cnt_rx;
    //usb_box->USB_RD_A4_D4(RX_BASE_A + 5, &cnt_rx);
    m_pCBus->read32(base_addr + RX_BASE_A + 5, &cnt_rx);

    if (logger.debug()) {
        logs.debug() << "RX Fifo status: "
                     << setw(6) << (cnt_rx & 0xFFFF) << " = 0x"
                     << setfill('0') << setw(4) << hex << (cnt_rx & 0xFFFF)
                     << " 32-bit words, "
                     << setfill(' ') << dec
                     << "empty=" << ((cnt_rx >> 28) & 1)
                     << ", full=" << ((cnt_rx >> 24) & 1)
                     << ", hfull=" << ((cnt_rx >> 20) & 1)
                     << endl;

        //printf("RX Fifo status : %6d = 0x%04x 32-bit words, empty=%d, full=%d, hfull=%d\n",
        //       cnt_rx & 0xFFFF,
        //       cnt_rx & 0xFFFF,
        //       (cnt_rx >> 28) & 1,
        //       (cnt_rx >> 24) & 1,
        //       (cnt_rx >> 20) & 1);
    }
    return cnt_rx;
}

uint32_t su738::opt_io_print_status(void)
{
    uint32_t cnt_tx, fifo_par, cnt_rx[4], fifo_size, fifo_hfull; //, cnt_rx_e, cnt_rx_ev, cnt_rx_fifo;

    m_pCBus->read32(base_addr + TX_BASE_A + 4, &cnt_tx);


    // bits 7..0 are log2 of FIFO size in 32-bit words
    // bits 31..16 are the hfull threshold (not used)
    m_pCBus->read32(base_addr + RX_BASE_A - 1, &fifo_par);

    fifo_size = 1;
    fifo_size <<= fifo_par & 0xFF;
    fifo_hfull =  fifo_par >> 16;

    logs.information() << "RX FIFO size is " << fifo_size
                       << ", Half-full threshold (not in use) is "
                       << fifo_hfull
                       << endl;

    m_pCBus->readBurst(base_addr + RX_BASE_A + 4, cnt_rx, (uint32_t)4, 4, 1);

    // 0 -> cnt_rx, 1 -> cnt_rx_fifo, 2 -> cnt_rx_e, 3 -> cnt_rx_ev

    logs.notice() << "Counter TX: " << setw(10) << cnt_tx << " = 0x"
                  << setfill('0') << hex << setw(2) << cnt_tx
                  << setfill(' ') << dec << endl;

    // only in loop mode!
    if (cnt_rx[0] != cnt_tx) {
        logs.warning() << "Warning, cnt_rx = " << cnt_rx[0] << " = 0x"
                       << setfill('0') << hex << setw(2) << cnt_rx[0]
                       << " NOT EQUAL cnt_tx!!!"
                       << setfill(' ') << dec << endl;
    }

    // this counter is 16-bit
    logs.information() << "RX Fifo status: "
                       << setw(6) << (cnt_rx[1] & 0xFFFF) << " = "
                       << hex16 << (cnt_rx[1] & 0xFFFF)
                       << " 32-bit words, "
                       << setfill(' ') << dec
                       << "empty=" << ((cnt_rx[1] >> 28) & 1)
                       << ", full=" << ((cnt_rx[1] >> 24) & 1)
                       << ", hfull=" << ((cnt_rx[1] >> 20) & 1)
                       << endl;

    // these counters are 32-bit
    if (cnt_rx[2] > 0) {
        logs.error() << "Error: cnt_rx with ER=1 =" <<  cnt_rx[2]
                     << hex16 << cnt_rx[2] << endl;
    }

    if (cnt_rx[3] > 0) {
        logs.error() << "Error: cnt_rx with ER=1 and DV=1 =" <<  cnt_rx[3]
                     << hex32 << cnt_rx[3] << endl;
    }
    //printf("Error: cnt_rx with ER=1 and DV=1 = %10d = 0x%08x\n",cnt_rx[3], cnt_rx[3]);

    return cnt_rx[0];
}

int su738::opticalTest(int loopback)
{
  // uint8_t  mode = 1;
  uint8_t  show_mess = 0;
  uint32_t runs = 15;
  uint32_t wrk32, err, spi_cnf[4];

  spi_cnf[0] = 0x1e | 0x030100; // fpga and tlk loopback
  spi_cnf[1] = 0x0e | 0x030200; // tlk loopback
  spi_cnf[2] = 0x06 | 0x030300; // ext loopback, same config for normal operation and after power up except for the LEDs
  spi_cnf[3] = 0x06 | 0x000000; // normal operation and after power up

  switch(loopback) {
    case 1: logs.notice() << "Testing SFP with FPGA-loopback..." << endl; break;
    case 2: logs.notice() << "Testing SFP with TLK-loopback..." << endl; break;
    case 3: logs.notice() << "Testing SFP with external loopback..." << endl; break;
    default: logs.error() << "Invalid test for SFP requested" << endl; return 0;
  }

  spi_rw_lb(SPI_A_CNF, 1, spi_cnf[loopback-1], &wrk32);

  if (loopback == 3) sleep(2);
  logs.debug() << "clear RX/TX" << endl;
  clear_tx();
  clear_rx();

  logs.debug() << "start testing..." << endl;
  err = 0;
  for (uint8_t mode=0; mode<4; mode++) {
	  err += test_opt_io_burst(mode, runs, show_mess, 3);
  }

  if (err > 0) {
	  logs.error() << "test completed with " << err << " errors" << endl;
  } else {
	  logs.information() << "test competed without error" << endl;
  }

  opt_io_print_status();
  sleep(1);
  spi_rw_lb(SPI_A_CNF, 1, spi_cnf[3], &wrk32);
  return err;
}

void su738::RX_phase()
{
  uint8_t  show_mess = 0;
  uint32_t runs = 15;
  uint32_t wrk32, spi_cnf[4];

  spi_cnf[0] = 0x1e | 0x030100; // fpga and tlk loopback
  spi_cnf[1] = 0x0e | 0x030200; // tlk loopback
  spi_cnf[2] = 0x06 | 0x030300; // ext loopback, same config for normal operation and after power up except for the LEDs
  spi_cnf[3] = 0x06 | 0x000000; // normal operation and after power up

  spi_rw_lb(SPI_A_CNF, 1, spi_cnf[0], &wrk32);
  logs.notice() << "Testing SFP rx_phase with FPGA-loopback..." << endl;

  logs.debug() << "clear RX/TX" << endl;
  clear_tx();
  clear_rx();

  logs.debug() << "start testing..." << endl;
  test_rxtx_ph(0, 0);
  opt_io_print_status();

  int err = 0;
  for (uint8_t mode=0; mode<4; mode++) {
    err += test_opt_io_burst(mode, runs, show_mess, 3);
  }

  if (err > 0) {
    logs.error() << "test completed with " << err << " errors" << endl;
  } else {
    logs.information() << "test competed without error" << endl;
  }

  spi_rw_lb(SPI_A_CNF, 1, spi_cnf[3], &wrk32);
}


uint32_t su738::gen_first(uint16_t nbits, uint16_t mode, uint16_t run)
{

    uint32_t mask;

    if (nbits>32) nbits=32;
    if (nbits<1 ) nbits= 1;
    if (nbits==32)
        mask=0xFFFFFFFF;
    else
        mask = (1 << (nbits-1)) - 1;

    switch (mode)
    {
    case 0 : {return run;}                   // count
    case 1 : {return 1 << (run % nbits);}    // shift
    case 2 : { switch (run & 3)
                { case 0: return 0;
                  case 1: return mask;
                  case 2: return 0x55555555 & mask;
                  case 3: return 0xAAAAAAAA & mask;
                }
             }
    }
    return 0;
}

uint32_t su738::gen_tp(uint32_t last, uint16_t nbits, uint16_t mode)
{
    uint32_t mask;

    if (nbits>32) nbits=32;
    if (nbits<1 ) nbits= 1;
    if (nbits==32)
        mask=0xFFFFFFFF;
    else
        mask = (1 << (nbits-1)) - 1;

    switch (mode)
    {
    case 0 : {last = (last + 1) & mask; break;}                           // count
    case 1 : {last = (last << 1) & mask; if (last == 0) last = 1; break;} // shift
    case 2 : {last = (~last) & mask; break;}
    }
    return last;
}


uint32_t su738::clear_tx()
{
    return m_pCBus->write32(base_addr + TX_BASE_A + 4, 0);
}

uint32_t su738::clear_rx()
{
    return m_pCBus->write32(base_addr + RX_BASE_A+4, 0);

}


uint32_t su738::test_opt_io(uint8_t mode, uint32_t runs, uint8_t show_mess)
{
    uint32_t i, j, nwords, last, got, err, expected;

    nwords = 1024;
    nwords = 16;
    err = 0;
    // clear counters

    clear_tx();
    clear_rx();

    for (i=0; i<runs; i++)
    {
        last = gen_first(16, mode, i);
        for (j=0; j<nwords; j++)
        {
            m_pCBus->write32(base_addr + TX_BASE_A, last);

            m_pCBus->write32(base_addr + TX_BASE_A, ~last);

            m_pCBus->read32(base_addr + RX_BASE_A, &got);

            expected = last | ((~last & 0xFFFF) << 16);
            if (expected != got)
            {
                err++;
                if (show_mess)
                    printf("Error %6d, wrote 0x%04x, got 0x%08x\n",err, last, got);
            }
            last = gen_tp(last, 16, mode);

        }
    }
    if ((err == 0) && (show_mess) ) printf("Done, no errors!\n");
    if ((show_mess) && (opt_io_print_status() != (2*runs*nwords)) )
    {
        printf("Wrong number of words transmitted, expected %10d = 0x%08x\n", runs*nwords, runs*nwords);
    }
    return err;
}

uint32_t su738::test_opt_io_burst(uint8_t mode, uint32_t runs, uint8_t show_mess, uint8_t read_and_check)
{
    uint32_t i, j, nwords, last, got, err, expected, wp, rp;
    nwords = 1024;

    uint32_t words2write = nwords*runs*2; // 16-bit
    uint32_t words2read  = nwords*runs;   // 32-bit
    uint32_t* wBuffer = NULL;
    uint32_t* rBuffer = NULL;

    wBuffer = new uint32_t[words2write];
    if (wBuffer == NULL)
    {
       printf("Error allocating memory\n");
       return -1;
    }
    rBuffer = new uint32_t[words2read];
    if (rBuffer == NULL)
    {
       printf("Error allocating memory\n");
       delete [] wBuffer;
       return -1;
    }

    if (nwords*runs*2 > 0xFFFF)
    {
      delete [] wBuffer;
      delete [] rBuffer;
      return -1;
    }
    err = 0;
    wp  = 0;

    if (show_mess)
        printf("number of 16-bit words to write 0x%04x, expected 32-bit words to read 0x%04x\n",
              words2write, words2read);

    for (i=0; i<runs; i++)
    {
        last = gen_first(16, mode, i);
        for (j=0; j<nwords; j++)
        {
            wBuffer[wp++] =  last;
            wBuffer[wp++] = (~last) & 0xFFFF;
            last = gen_tp(last, 16, mode);
        }
    }
    // write all words
    if (show_mess)
    {
        printf("status before burst\n");
        opt_io_print_status();
    }
    if (read_and_check & 3)
        m_pCBus->writeBurst(base_addr + TX_BASE_A, wBuffer, words2write, 2, 0);

    if (show_mess)
    {
        printf("status after burst\n");
        opt_io_print_status();
    }
//    return 0;
    wp = 0;
    rp = 0;

    if (read_and_check & 1)
    {

        // burst read
        if (show_mess)
            printf("Start reading the data in fifo...\n");
        m_pCBus->readBurst(base_addr + RX_BASE_A, rBuffer, words2read, 4, 0);
        if (show_mess)
            printf("Start checking the read data...\n");


        // check
        for (i=0; i<runs; i++)
        {
    //        last = gen_first(16, mode, i);
            for (j=0; j<nwords; j++)
            {
                got = rBuffer[rp++];
                expected = wBuffer[wp] | (wBuffer[wp+1] << 16);
                if (expected != got)
                {
                    err++;
                    if (show_mess)
                        printf("rp=%5d  Error %6d   wrote 0x%08x   got 0x%08x\n",
                            rp, err, expected, got);
                }
    //            last = gen_tp(last, 16, mode);
                wp += 2;
            }
        }
        if ((err == 0) && (show_mess) ) printf("Done, no errors!\n");
        if ((show_mess) && (opt_io_print_status() != words2write) )
        {
            printf("Wrong number of words transmitted, expected %10d = 0x%08x\n", runs*nwords, runs*nwords);
        }

    }

    delete [] wBuffer;
    delete [] rBuffer;

    return err;
}


unsigned int su738::psrandom(int cpu, int initvalue)
{
  static int DATAi[4];
  int stuffbit, DATAo;
  if ( initvalue>=0 )
    {
    DATAi[cpu]=initvalue;
    return 0;
    }
  else
    {
    DATAo = DATAi[cpu];
    stuffbit = (DATAi[cpu] >> 15) ^ (DATAi[cpu] >> 14) ^ (DATAi[cpu] >> 12) ^ (DATAi[cpu] >>  3) ^ 0x0;
    DATAi[cpu]= ( (DATAi[cpu]<<1) | (stuffbit & 0x1) ) & 0xFFFF;
    }

//  return ( (DATAo >> 8) | (DATAo << 8) ) & 0xFFFF;
  return DATAo & 0xFFFF;
}

int su738::ref_get(int cpu0s, int cpu1s, int cpu2s, int cpu3s)
{
//  int i, q, event;
  static int sample;

  if (cpu0s >= 0) psrandom(0,cpu0s); // CPU 0
  if (cpu1s >= 0) psrandom(1,cpu1s); // CPU 1
  if (cpu2s >= 0) psrandom(2,cpu2s); // CPU 2
  if (cpu3s >= 0) psrandom(3,cpu3s); // CPU 3
  if ( (cpu0s>=0) | (cpu1s>=0) | (cpu2s>=0) | (cpu3s>=0) )
  {
    sample=0;
    return 0;
  }
  if ( (sample >= 0) & (sample <= 3) )
  {
    sample++;
    return endmarkert;
  }
  if ( (sample >= 4) & (sample <= (4+1*Nw-1 ) ) )
  {
    sample++;
    return psrandom(0,-1);
  }

  if ( (sample >= 4+1*Nw) & (sample <= (4+2*Nw-1 ) ) )
  {
    sample++;
    return psrandom(1,-1);
  }

  if ( (sample >= 4+2*Nw) & (sample <= (4+3*Nw-1 ) ) )
  {
    sample++;
    return psrandom(2,-1);
  }

  if ( (sample >= 4+3*Nw) & (sample <= (4+4*Nw-1 ) ) )
  {
    sample++;
    return psrandom(3,-1);
  }

  if ( (sample >= 4+4*Nw) & (sample <= (4+4*Nw+3 ) ) )
  {
    sample++;
    if (sample==(4+4*Nw+4 )) sample=0;
  }
    return endmarkerr;

}

int su738::patt_check(int check)
{

        uint32_t cnt_rx_fifo;
        int show_bit_rep;

        int i, j, c, qsoll, q16;

        int BitErrors[16];
        uint32_t mybuffer[RX_FIFO_SIZE];

        ref_get(cpu0s, cpu1s, cpu2s, cpu3s);
                show_bit_rep = 0;

        for(j=0; j<16; j++)
            BitErrors[j] = 0;

        cnt_rx_fifo = get_fifo_cnt();
        cnt_rx_fifo &= 0xFFFF;
        if (cnt_rx_fifo > RX_FIFO_SIZE) cnt_rx_fifo = RX_FIFO_SIZE;
        m_pCBus->readBurst(base_addr + RX_BASE_A, mybuffer, cnt_rx_fifo, 4, 0);


        for (i=0; i<cnt_rx_fifo; i++)
        {
            // Check lower 16 bit.
            //
            qsoll=ref_get(-1, -1, -1, -1);
            q16 = mybuffer[i] & 0xFFFF;

            if (q16 != qsoll)
            {
                    printf("*** 0x%06x: 0x%04x 0x%04x 0x%04x\n", 2*i, q16, qsoll, (q16 ^ qsoll) );

                    // Scan for bit errors.
                    c = 1;
                    for(j=0; j<16; j++)
                    {
                            if (( (q16 ^ qsoll) & c) > 0)
                            {
                                    BitErrors[j]++;
                                    show_bit_rep = 1;
                            }
                            c = c << 1;
                    }
            }

            //
            // Check upper 16 bit.
            //
            qsoll=ref_get(-1, -1, -1, -1);
            q16 = (mybuffer[i] >> 16) & 0xFFFF;

            if (q16 != qsoll)
            {
                    printf("*** 0x%06x: 0x%04x 0x%04x 0x%04x\n", 2*i+1, q16, qsoll, (q16 ^ qsoll) );

                    // Scan for bit errors.
                    c = 1;
                    for(j=0; j<16; j++)
                    {
                            if (( (q16 ^ qsoll) & c) > 0)
                            {
                                    BitErrors[j]++;
                                    show_bit_rep = 1;
                            }
                            c = c << 1;
                    }
            }
        }

        if (show_bit_rep)  // check && (i > 0)
        {
                printf(" (Bit errors: MSB ");
                for(j=15; j>=0; j--)
                {
                        if (j==15)
                        {
                                printf("%d", BitErrors[j]);
                        }
                        else
                        {
                                printf(",%d", BitErrors[j]);
                        }
                }
                printf(" LSB)\n");
        }

        printf("\ndone. 0x%04x 32 bit words read.\n",i );
        return 0;
}


int su738::mem_dump(FILE *f)
{

    uint32_t cnt_rx_fifo;

    int i;

    uint32_t mybuffer[RX_FIFO_SIZE];

    cnt_rx_fifo = get_fifo_cnt();
    cnt_rx_fifo &= 0xFFFF;

    if (cnt_rx_fifo > RX_FIFO_SIZE) cnt_rx_fifo = RX_FIFO_SIZE;

    m_pCBus->readBurst(base_addr + RX_BASE_A, mybuffer, cnt_rx_fifo, 4, 0);

    for (i=0; i<cnt_rx_fifo; i++)
    {
        fprintf(f, "0x%08x\n",mybuffer[i]);
    }
    return cnt_rx_fifo;
}

uint32_t su738::mem_dump(void* buf, size_t size)
{

  uint32_t* buffer = reinterpret_cast<uint32_t*>(buf);
  uint32_t cnt_rx_fifo, max;
  int i = 0;
  uint32_t mybuffer[RX_FIFO_SIZE];

  cnt_rx_fifo = get_fifo_cnt();
  cnt_rx_fifo &= 0xFFFF;

  if (size > RX_FIFO_SIZE) max = RX_FIFO_SIZE;
  else max = size;

  if (cnt_rx_fifo > max) cnt_rx_fifo = max;

  m_pCBus->readBurst(base_addr + RX_BASE_A, buffer, cnt_rx_fifo, 4, 0);

  // while (i<cnt_rx_fifo && buffer[i] != 0)
  // {
  //   std::cout << std::setw(8) << std::setfill('0') << hex << buffer[i] << std::endl;
  //   i++;
  // }

  return cnt_rx_fifo;
}

uint32_t su738::mem_dump(zmq_buffer_t& buf)
{
    uint32_t nread = mem_dump(buf.get_write_ptr(), buf.get_write_max_size());

    if (nread) {
        // logs.notice() << "read " << nread << " dwords from SFP" << endl;

        // zmqbuffer.set_timestamp();
        // zmqbuffer.set_equipment(0x10+s);
        buf.add_payload_size(nread*sizeof(uint32_t));

    }

    // let others know if we read anything
    return nread;
}



// uint32_t su738::mem_dump(uint32_t* buffer, uint32_t size)
// {
//
//     uint32_t cnt_rx_fifo, max;
//     cnt_rx_fifo = get_fifo_cnt();
//     cnt_rx_fifo &= 0xFFFF;
//
//     if (RX_FIFO_SIZE>size) max = size;
//     else max = RX_FIFO_SIZE;
//
//     if (cnt_rx_fifo > max) cnt_rx_fifo = max;
//
//     std::cout << cnt_rx_fifo << std::endl;
//
//     m_pCBus->readBurst(base_addr + RX_BASE_A, buffer, cnt_rx_fifo, 4, 0);
//
//     std::cout << "hello" << std::endl;
//
//     if (cnt_rx_fifo == max)
//     {
//       std::cerr << "Buffer full" << endl;
//       return 0;
//     }
//
//     return cnt_rx_fifo;
// }


int su738::spi_rw_lb(uint8_t addr, uint8_t wr, uint32_t wdata, uint32_t * rdata)
{
    uint32_t L, R;
    addr &= 0x7F;
    wdata &= 0xFFFFFF;
    wr &= 1;
    if ((addr == SPI_A_CNF) && (wr == 1))
    {
        if (logger.debug()) {
            logger.debug("Write using SPI to config reg:");
            print_tlk(wdata);
        }
    }
    L = (addr << 1) | wr;
    L <<= 24;
    L |= (wdata & 0xFFFFFF);


    //usb_box->USB_WR_A4_D4(SPI_LB, L);
    m_pCBus->write32(base_addr + SPI_LB_BASE, L);

    usleep(10000);
    //  usb_box->USB_RD_A4_D4(SPI_LB+1, &R);
    //  printf("Status is 0x%08x\n",R);
    //  usb_box->USB_RD_A4_D4(SPI_LB+2, &R);
    //  printf("Out reg is 0x%08x\n",R);

    //usb_box->USB_RD_A4_D4(SPI_LB, &R);
    m_pCBus->read32(base_addr + SPI_LB_BASE, &R);

    if (addr != ((R >> 25) & 0x7F) ) {
        logs.critical() << "wrong spi address received!, "
                        << hex << setfill('0')
                        << "Sent 0x" << setw(2) << addr
                        << ", Recv 0x0x" << setw(2) << ((R>>25) & 0x7F)
                        << dec << setfill(' ') << endl;
    }
    if (addr == SPI_A_ID)
    {
        printf("Compile timestamp: ");
        print_id24b(R);
    }
    if (addr == SPI_A_SVN)
    {
        printf("Subversion id: ");
        print_svn24b(R);
    }
    if ((addr == SPI_A_CNF) && (wr == 0))
    {
        printf("Read using SPI from config reg:\n");
        print_tlk(R);
    }
    *rdata =R & 0xFFFFFF;
    return 0;
}



int su738::step_dcm(int16_t up1rst0dwn_1, uint8_t rx0tx1, uint8_t lb0su1)
// return the dcm status read
{
    uint32_t addr, din, L;

    rx0tx1 &= 1;
    lb0su1 &= 1;

    if (lb0su1==0)
    {
    if (rx0tx1) addr = base_addr + FREQ_BASE_A+4;
    else        addr = base_addr + FREQ_BASE_A+6;
    }
    else
    {
    if (rx0tx1) addr = SPI_A_DCM_TX; // SPI
    else        addr = SPI_A_DCM_RX;
    }

    // reset
    if (up1rst0dwn_1==0)
    {
        if (lb0su1==0)
            m_pCBus->write32(addr, 1);

        else
        {
            spi_rw_lb(addr, 1, 1, &L);
            spi_rw_lb(addr, 1, 1, &L);
        }
        return 0;
    }

    if (up1rst0dwn_1>0) din = 0x82; // count up
    else                din = 0x80; // count down

    if (lb0su1==0)
    {
        //usb_box->USB_WR_A4_D4(addr, din); // step
        m_pCBus->write32(addr, din);

        //usb_box->USB_RD_A4_D4(addr, &L);
        m_pCBus->read32(addr, &L);

    }
    else
    {
        spi_rw_lb(addr, 1, din, &L);
    //    Sleep(1);
        spi_rw_lb(addr, 0, 0, &L); // read
    }
    //Sleep(1);
    if (L != 0x18)
        printf("DCM status 0x%02x\n", L);
    if (L & 1)
    {
        printf("The limit reached\n");
    }

    return (L & 0x1F);
}


int su738::set_dcm(int16_t delay, uint32_t rx0tx1, uint32_t lb0su1)
{
    uint32_t i, dcm_status;
    int32_t minp, maxp;

    minp = -255;
    maxp =  255;

    // clip
    if (delay > maxp) delay = maxp;
    if (delay < minp) delay = minp;

    // reset
    step_dcm(0, rx0tx1, lb0su1);
    // if delay is 0, do nothing
    if (delay==0) return 0;

    for (i=0; i<abs(delay); i++)
    {
        dcm_status = step_dcm(delay, rx0tx1, lb0su1);

        if (dcm_status != 0x18)
            printf("step %3d  status 0x%02x\n",i, dcm_status);
        if (dcm_status & 1)
        {
            printf("The limit reached, exiting\n");
            if (delay>0) return i;
            else         return -i;
        }

    }
    return delay;
}


uint32_t su738::test_rxtx_ph(uint8_t rx0tx1, uint8_t lb0su1)
{
    int i, err[512], ph, actual_left, actual_right, dcm_status;
    FILE *f;
    int32_t minp, maxp;

    lb0su1 &= 1;

    minp = -256;
    maxp =  256;

    actual_left = minp;
    actual_right= maxp;

    // loop
    for (i=minp; i<=maxp; i++)
    {
        if (i==minp)
        {
            ph = set_dcm(i, rx0tx1, lb0su1);
            actual_left = ph;
            i = ph;
            printf("Left limit %d reached!\n",ph);
        }
        else
        {
            dcm_status = step_dcm(1, rx0tx1, lb0su1); // step up
            ph++;
            if (dcm_status & 1) // end
            {
                actual_right = ph;
                i = maxp; // exit
                printf("Right limit %d reached!\n",ph);
            }
        }

        err[ph+255]=test_opt_io(1,1,0);
        if (err[ph+255]) printf("Phase %6d, errors %6d\n", ph, err[ph+255]);
    }
    // filename
    if (rx0tx1)
        if (lb0su1)
            f = fopen("phase_tx_su.dat", "w");
        else
            f = fopen("phase_tx_lb.dat", "w");
    else
        if (lb0su1)
            f = fopen("phase_rx_su.dat", "w");
        else
            f = fopen("phase_rx_lb.dat", "w");
    fprintf(f,"# phase  errors\n");
    for (i = actual_left; i <= actual_right; i++)
    {
        fprintf(f, "%6d %6d\n", i, err[i+255]);
    }
    fclose(f);
    return 0;
}


uint32_t su738::get_freq_serdes(uint8_t ch)
{
    uint32_t freq;

    ch &= 1;

    m_pCBus->read32(base_addr + FREQ_BASE_A+2*ch, &freq);


    if ((freq > (FREQ_EXP_SERDES+freq_tol)) ||
        (freq < (FREQ_EXP_SERDES-freq_tol)) )
        {
            printf("Frequency %0.6f MHz is out of spec!!!\n",1.0*freq/1e6);
        }
    return freq;
}

string su738::fmt_tlk(uint32_t d)
{
  ostringstream os;
  os << "PRBSEN="   << (d&1)
     << " LCKREFN=" << ((d>>1)&1)
     << " ENABLE="  << ((d>>2)&1)
     << " LOOPEN="  << ((d>>3)&1)
     << " bridge="  << ((d>>4)&1)
     << " LED="     << ((d>>8)&3)
     << " USE_LED=" << ((d>>16)&3);

  return os.str();
}



int su738::print_tlk(uint32_t d)
{
    Poco::Logger& logger = Poco::Logger::get("LogicBox");

    if (logger.debug()) {
        d &= 0xFFFFFF;
        logger.debug(Poco::format("PRBSEN =%d\n",d & 1));
        d >>= 1;
        logger.debug(Poco::format("LCKREFN=%d\n",d & 1));
        d >>= 1;
        logger.debug(Poco::format("ENABLE =%d\n",d & 1));
        d >>= 1;
        logger.debug(Poco::format("LOOPEN =%d\n",d & 1));
        d >>= 1;
        logger.debug(Poco::format("bridge =%d\n",d & 1));
        d >>= 4;
        logger.debug(Poco::format("LED    =%d\n",d & 3));
        d >>= 8;
        logger.debug(Poco::format("USE_LED=%d\n",d & 3));
        return 0;
    }
}

string su738::fmt_id24b(uint32_t did)
{
  ostringstream os;
  os << ( ((did >> 18) & 0x3F) + 2000) << "-"  // year
     << (  (did >> 14) & 0x0F ) << "-"         // month
     << (  (did >>  9) & 0x1F ) << " "         // day
     << (  (did >>  4) & 0x1F ) << " "         // hour
     << (  (did & 0xF) * 5 );                  // minute

  return os.str();

}


int su738::print_id24b(uint32_t did)
{
    uint32_t year, month, day, hour, min;

    min = (did & 0xF)*5;
    did >>= 4;
    hour = did & 0x1F;
    did >>= 5;
    day = did & 0x1F;
    did >>= 5;
    month = did & 0xF;
    did >>= 4;
    year = (did & 0x3F) + 2000;
    printf("%d-%02d-%02d %02d:%02d\n",year, month, day, hour, min);
    return 0;
}

string su738::fmt_svn24b(uint32_t did)
{
  ostringstream os;
  os << ( ((did >> 18) & 0x3F) + 2000) << "-"   // year
     << (  (did >> 14) & 0x0F ) << "-"          // month
     << (  (did >>  9) & 0x1F )                 // day
     << "  SVN release " << (did & 0x1FF);      // SVN rel

  return os.str();
}

int su738::print_svn24b(uint32_t svn)
{
    uint32_t year, month, day, svn_nr;

    svn_nr = svn & 0x1FF;
    svn >>= 9;

    day = svn & 0x1F;
    svn >>= 5;
    month = svn & 0xF;
    svn >>= 4;
    year = (svn & 0x3F)+2000;
    printf("%d-%02d-%02d SVN NR %d\n",year, month, day, svn_nr);
    return 0;
}

// I2C Functions


int32_t su738::I2C_status()
{
    uint32_t cbus_dat;

        m_pCBus->read32(base_addr + BASE_ADDR_I2C, &cbus_dat);
    if (i2c_debug)
    {
        if ((cbus_dat >> 25) & 1) printf("I2C Timeout!\n");
        if ((cbus_dat >> 24) & 1) printf("I2C ready!\n");
                            else  printf("I2C not ready!\n");
        if (((cbus_dat >> 16) & 0xFF) != 0) printf("State machines not in idle! %02x \n" ,(cbus_dat >> 16) & 0xFF);
    }
    return cbus_dat;
}


int32_t su738::I2C_ByteWrite(uint16_t reg_addr, uint16_t dat)
{
    uint32_t i2c_cmd, i2c_dat, i2c_addr, i2c_long, cbus_dat, err;

    i2c_cmd = 7;
    i2c_dat = ((dat & 0xFF) << 8) | (reg_addr & 0xFF);
    i2c_long = 1;
    i2c_addr = SLAVE_ADDR;
    cbus_dat = (i2c_cmd << 24) | (i2c_long << 27) | (i2c_addr << 16) | i2c_dat;

    m_pCBus->write32(base_addr + BASE_ADDR_I2C, cbus_dat);

    err = 0;
    while ( ( ((I2C_status() >> 24) & 1) == 0) && (err < 100))
    {
        err++;
        if (i2c_debug) printf("i2c_write_wait %2d\n",err);
    }
    return err;
}



uint8_t su738::I2C_ByteRead(uint16_t reg_addr, uint8_t sfp_i2c_addr = 0x51)
{
    uint32_t i2c_cmd, i2c_dat, i2c_addr, i2c_long, cbus_dat, err;

    i2c_cmd = 7;
    i2c_dat = reg_addr & 0xFF;
    i2c_long = 0;
    i2c_addr = sfp_i2c_addr & 0xFF;
    cbus_dat = (i2c_cmd << 24) | (i2c_long << 27) | (i2c_addr << 16) | i2c_dat;

    err = m_pCBus->write32(base_addr + BASE_ADDR_I2C, cbus_dat);
    err = 0;
    while ( ( ((I2C_status() >> 24) & 1) == 0) && (err < 100))
    {
        err++;
        if (i2c_debug) printf("i2c_write_addr_wait %2d\n",err);
    }

    i2c_cmd = 0;
    i2c_dat = reg_addr & 0xFF;
    i2c_long = 0;
    i2c_addr = sfp_i2c_addr & 0xFF;
    cbus_dat = (i2c_cmd << 24) | (i2c_long << 27) | (i2c_addr << 16) | i2c_dat;

    m_pCBus->write32(base_addr + BASE_ADDR_I2C, cbus_dat);

    cbus_dat = I2C_status();
    while ( ( ((cbus_dat >> 24) & 1) == 0) && (err < 100))
    {
        err++;
        cbus_dat = I2C_status();
        if (i2c_debug) printf("i2c_read_wait %2d\n",err);
    }
    return (cbus_dat & 0xFF);
}


uint16_t su738::I2C_2_ByteRead(uint16_t reg_addr, uint8_t sfp_i2c_addr = 0x51)
{
    uint32_t i2c_cmd, i2c_dat, i2c_addr, i2c_long, cbus_dat, err;

        // additional debugging reset of the  statemachine with bit 31
/*      cbus_dat = (1 << 31);
        err = m_pCBus->write32(base_addr, cbus_dat);
        printf("DEBUGGING RESET");
        cbus_dat=0;*/



    i2c_cmd = 7;
    i2c_dat = reg_addr & 0xFF;
    i2c_long = 0;
    i2c_addr = sfp_i2c_addr & 0xFF;
    cbus_dat = (i2c_cmd << 24) | (i2c_long << 27) | (i2c_addr << 16) | i2c_dat;

    err = m_pCBus->write32(base_addr + BASE_ADDR_I2C, cbus_dat);
    err = 0;
    while ( ( ((I2C_status() >> 24) & 1) == 0) && (err < 100))
    {
        err++;
        if (i2c_debug) printf("i2c_write_addr_wait %2d\n",err);
    }


        // additional debugging reset of the  statemachine with bit 31
/*      cbus_dat = (1 << 31);
        err = m_pCBus->write32(base_addr, cbus_dat);
        printf("DEBUGGING RESET");
        cbus_dat=0;*/



    i2c_cmd = 0;
    i2c_dat = reg_addr & 0xFF;
    i2c_long = 1;
    i2c_addr = sfp_i2c_addr & 0xFF;
    cbus_dat = (i2c_cmd << 24) | (i2c_long << 27) | (i2c_addr << 16) | i2c_dat;

    m_pCBus->write32(base_addr + BASE_ADDR_I2C, cbus_dat);

    cbus_dat = I2C_status();
    while ( ( ((cbus_dat >> 24) & 1) == 0) && (err < 100))
    {
        err++;
        cbus_dat = I2C_status();
        if (i2c_debug) printf("i2c_read_wait %2d\n",err);
    }

    //swap the bytes for correct MSB, LSB because I2C gives higher address byte first
    uint8_t hibyte = (cbus_dat & 0xff00) >> 8;
    uint8_t lobyte = (cbus_dat & 0xff);

    return (lobyte << 8 | hibyte);
}


int su738::program_FPGA(std::string filename)
{
  uint32_t lSize;
  uint8_t * buffer;
  size_t result;
  int16_t  rx_phase, tx_phase;
  // char  in_file[512];
  // strcpy(in_file,":");

  FILE* f_in;

  // //Poco::Logger& logger = Poco::Logger::get("TRDbox");
  // Poco::LogStream logs(Poco::Logger::get("TRDbox"));

  // strcpy(in_file, file"/usr/share/trdbox/sfpprog.bin");
  // if (strcmp(in_file, ":") != 0)
  // {

  // get all content of the inputfile into a uint8_t*
  logs.debug() << "Reading from input file " << filename << endl;
  f_in = fopen(filename.c_str(), "rb");
  if (f_in == NULL)
  logs.error() << "failure to open " << filename << endl;

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
  int a = program_FPGA(buffer, lSize);

  if (a == 0) {
    logs.information() << "SFP programming completed" << endl;
  }
  fclose(f_in);
  free(buffer);
  // strcpy(in_file, ":");

  return a;
}


int su738::program_FPGA(uint8_t* bitfile,uint32_t NoCharacters)
{
        // FPGA CBUS Addresses involved:
        //    + 0xC1 FPGA config port (rw)
        //          RW Pins
        //           bit 31 : LB_M1        enable/disable config port
        //           bit 30 : LB_PROG_B    for starting programming sequence
        //           bit 29 : LB_INIT_B    response
        //           bit 28 : LB_DONE_n    end signal
        //          Command for writing bytes for FPGA config stream
        //           bit 9  : ld
        //           bit 8  : switch       =0 LSB send first, =1 MSB send first
        //           bit 7-0: data         MSB to LSB
        //    + 0xC3 FPGA burst write port
        //                       bit 7-0: data         MSB to LSB, others are statically set in hardware

        // See XILINX Documents XAPP502 and UG380 for more information about the programming procedure

        uint32_t cbus_dat, cbus_temp, err, ret;
        err = 0;
        cbus_dat = 0;
        cbus_temp = 0;

// set PROGRAM_B to low to start sequence
        // set LB_M1=0 (bit31) for M1=1 (slave serial) and LB_Prog_B=1 (bit 30) for PROG_B=0
        cbus_dat = (0x40 << 24) | (0x00 << 8) | (0x00);
        err = m_pCBus->write32(base_addr + FPGA_CNF_A, cbus_dat);
        if (debug_conf_FPGA)
                printf("Prog_B set low 0x%08x\n", cbus_dat);
        //control read
        if (debug_conf_FPGA) {
                cbus_temp = 0;
                m_pCBus->read32(base_addr + FPGA_CNF_A, &cbus_temp);
                printf("status is  0x%08x\n", cbus_temp);
        }

// wait for INIT_B to go low, which is LB_INIT_B (bit29) high
        while ((((cbus_dat >> 29) & 1) == 0) && (err < 100)) {
                err++;
                m_pCBus->read32(base_addr + FPGA_CNF_A, &cbus_dat); // read INIT_B (bit 29)
                if (debug_conf_FPGA)
                        printf("wait for INIT_B %2d\n", err);
        }

// release PROGRAM_B
        if (((cbus_dat >> 29) & 1) == 1) {
                cbus_dat = (0x00 << 24) | (0x00 << 8) | (0x00); // set LB_M1 (bit31) and release LB_Prog_B (bit 30)
                err = m_pCBus->write32(base_addr + FPGA_CNF_A, cbus_dat);
                if (debug_conf_FPGA)
                        printf("INIT_B signal found, Prog_B released 0x%08x\n", cbus_dat);
                //control read
                if (debug_conf_FPGA) {
                        cbus_temp = 0;
                        m_pCBus->read32(base_addr + FPGA_CNF_A, &cbus_temp);
                        printf("status is  0x%08x\n", cbus_temp);
                }
        }


        uint32_t addr = base_addr + FPGA_CNF_A + 0x2; // burst write addr 0x4C3
        // new burst write (needs less than 0.5s)
        //printf("\nStart writing bitfile\n");
	//ret =  m_pCBus->writeBlock(addr, bitfile, NoCharacters, 0);



    // old (slow) writing (needs more than 2 min)

        // writing bytes of the bitfile -> set switch (bit 8 should be set for 'byte.bin' to send MSB first)
        printf("\n\nStart writing bitfile\n\n");
        for (int i = 0; i <= NoCharacters; i++) // until NoCharacters is reached
                        {
                cbus_dat = (0x00 << 24) | (0x03 << 8) | (bitfile[i]);
                err = m_pCBus->write32(base_addr + FPGA_CNF_A, cbus_dat);
//              if (debug_conf_FPGA) printf("byte %6d: 0x%08x\n",i, cbus_dat);
                //control read
                if (debug_conf_FPGA) {
                        cbus_temp = 0;
                        m_pCBus->read32(base_addr + FPGA_CNF_A, &cbus_temp);
//              if (debug_conf_FPGA) printf("status is  0x%08x\n", cbus_temp);
                        if (!((cbus_dat & 0x000000FF) == (cbus_temp & 0x000000FF)))
                                printf("\n WRONG DATA WRITTEN \n");
                        if (!((cbus_dat & 0xC0000000) == (0x00000000)))
                                printf("\n M1 = 0, outputs disabled \n");
//              if (debug_conf_FPGA) printf("error %d\n",err);
//              if (debug_conf_FPGA) printf("byte %d:  %x\n",i,bitfile[i]);
                }
        }



    cbus_temp = 0;
        m_pCBus->read32(base_addr + FPGA_CNF_A, &cbus_temp);

        int cnt = 0;
// fill with 0xFF until DONE is high or here: LB_DONE = 0
        while ((((cbus_temp >> 28) & 1) == 1) && (cnt < 100)) {
                cnt++;
                m_pCBus->read32(base_addr + FPGA_CNF_A, &cbus_temp);
                cbus_dat = (0x00 << 24) | (0x03 << 8) | (0xFF); // set LB_M1 (bit31) and write 0xFF
                err = m_pCBus->write32(base_addr + FPGA_CNF_A, cbus_dat);
                if (debug_conf_FPGA)
                        printf("wait for DONE %d\n", cnt);
                if (cnt == 100) printf("No DONE signal found! EXIT Configuration\n");
        }

// send one more 0xFF
        cbus_dat = (0x00 << 24) | (0x03 << 8) | (0xFF); // set LB_M1 (bit31) and write 0xFF
        err = m_pCBus->write32(base_addr + FPGA_CNF_A, cbus_dat);
        if (cnt != 100) printf("\nConfiguration successful\n\n");

        return 0;
}

};

// Local Variables:
//   mode: c++
//     c-basic-offset: 4
//     indent-tabs-mode: nil
// End:

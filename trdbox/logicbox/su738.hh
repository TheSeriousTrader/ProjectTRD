#ifndef LOGICBOX_CSU738_H
#define LOGICBOX_CSU738_H

// $Id: Csu738.h 344 2015-03-04 15:55:14Z angelov $:


#include "data_types.h"
#include "CLogicBox.h"
#include "CCBUS.h"
#ifdef _WIN32
    #include <windows.h> // for the Sleep function
    #define sleep Sleep
#else
    #include <unistd.h>
    #define Sleep(A) usleep(A*1000)
#endif

#include "trdbox_zmq.hh"

#include <stdlib.h>
#include <string>

#define TX_BASE_A       0x000
#define RX_FIFO_SIZE    32768
#define RX_BASE_A       0x008
#define FREQ_BASE_A     0x040
#define FREQ_EXP_SERDES 125000000
#define freq_tol            10000

#define SPI_LB_BASE     0x0A0
#define SPI_A_ID        0x00
#define SPI_A_SVN       0x01
#define SPI_A_CNF       0x20
#define SPI_A_DCM_TX    0x40
#define SPI_A_DCM_RX    0x60

//#define FPGA_CNF_BASE   0x400
#define FPGA_CNF_A      0xC1

#define DCM_LB_RX          0
#define DCM_LB_TX          0

#define DCM_SU_RX          0
#define DCM_SU_TX          0


#define  endmarkert         0xAAAA
#define  endmarkerr         0x0000
#define  Nw                 126

// debug messages on/off
#define i2c_debug 0
#define debug_conf_FPGA 0

// Addresses in the FPGA design in LogicBox
#define BASE_ADDR_I2C 0x080                     // see top_core.vhd documentation

// slave address of the SFP module
 #define SLAVE_ADDR 0x50                            // equal to 0xA0 = 1010000X see Avago Application Note 5016


//#define usleep(x) usleep_busy_wait(x)

namespace trdbox {

class su738
{
    int  cpu0s;
    int  cpu1s;
    int  cpu2s;
    int  cpu3s;
    // int  debug;
    uint32_t base_addr;

public:
    // su738(int new_debug, CCBUS *pCBus, uint32_t new_base_addr);
    su738(CCBUS *pCBus, uint32_t new_base_addr);
    ~su738();

    // main transfer method
    //
    uint32_t    opt_io_print_status(void);
    uint32_t    gen_first(uint16_t nbits, uint16_t mode, uint16_t run);
    uint32_t    gen_tp(uint32_t last, uint16_t nbits, uint16_t mode);
    uint32_t    test_opt_io(uint8_t mode, uint32_t runs, uint8_t show_mess);
    uint32_t    test_opt_io_burst(uint8_t mode, uint32_t runs, uint8_t show_mess, uint8_t read_and_check);
    uint32_t    clear_tx();
    uint32_t    clear_rx();
    uint32_t    get_fifo_cnt(void);

    void        RX_phase();
    int         opticalTest(int loopback);

    unsigned int psrandom(int cpu, int initvalue);
    int         ref_get(int cpu0s, int cpu1s, int cpu2s, int cpu3s);
    int         mem_dump(FILE *f);
    uint32_t    mem_dump(void* buffer, size_t size);
    uint32_t    mem_dump(zmq_buffer_t& buffer);
    int         patt_check(int check);
    int         spi_rw_lb(uint8_t addr, uint8_t wr, uint32_t wdata, uint32_t * rdata);
    int         step_dcm(int16_t up1rst0dwn_1, uint8_t rx0tx1, uint8_t lb0su1);
    int         set_dcm(int16_t delay, uint32_t rx0tx1, uint32_t lb0su1);
    uint32_t    test_rxtx_ph(uint8_t rx0tx1, uint8_t lb0su1);
    uint32_t    get_freq_serdes(uint8_t ch);

    std::string fmt_tlk(uint32_t d);
    std::string fmt_id24b(uint32_t did);
    std::string fmt_svn24b(uint32_t svn);

    int         print_tlk(uint32_t d);
    int         print_id24b(uint32_t did);
    int         print_svn24b(uint32_t svn);


    // I2C Commands to SFP
    int32_t     I2C_status();
    int32_t     I2C_ByteWrite(uint16_t reg_addr, uint16_t dat);
    uint8_t     I2C_ByteRead(uint16_t reg_addr, uint8_t sfp_i2c_addr);
    uint16_t    I2C_2_ByteRead(uint16_t reg_addr, uint8_t sfp_i2c_addr);

    // FPGA programming method
    int program_FPGA(std::string filename="/usr/share/trdbox/sfpprog.bin");
    int program_FPGA(uint8_t* bitfile, uint32_t NoCharacters);

private:
    CCBUS *m_pCBus;

};

};

#endif

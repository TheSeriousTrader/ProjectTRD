#ifndef TRDBOX_ZMQ_HH
#define TRDBOX_ZMQ_HH

#include <zmq.hpp>
#include <stdexcept>
#include "logging.hh"

// std::vector<uint8_t> zmq_filter(uint8_t eqclass=0xFF, uint8_t eqid=0xFF)
// {
//
//     std::vector<uint8_t> flt = {0xDA,0x7A,0xFE,0xED,eqclass,eqid};
//     if (eqid == 0xff) {
//         if (eqclass == 0xff) {
//             flt.resize(4);
//         } else {
//             flt.resize(5);
//         }
//     }
//
//     return flt;
// }

struct header_t {

    header_t(uint8_t eqclass=0xFF, uint8_t eqid=0xFF)
    :
    magic(0xDA7AFEED),
    equipment_class(eqclass), equipment_id(eqid), status(0), version(1),
    reserved2(0), header_size(20), payload_size(0)
    {}

    const uint8_t* get_payload_ptr() const {
        return reinterpret_cast<const uint8_t*>(this) + header_size;
    }

    // DWORD 0
    uint32_t magic;           /// always 0xDA7AFEED

    // DWORD 1
    uint8_t equipment_class;  /// type of data
    uint8_t equipment_id;   /// ID of data source
    uint8_t status;           /// status/error flags - 0=ok
    uint8_t version;          /// version number - 1

    // DWORD 2
    uint8_t  reserved2;
    uint8_t  header_size;     /// header size in bytes (v1 -> 20=0x14)
    uint16_t payload_size;    /// payload size (excl. header) in bytes

    // DWORD 3
    uint32_t timestamp_sec;   /// seconds since 1 Jan 1970 UTC

    // DWORD 4
    uint32_t timestamp_ns;    /// nanoseconds part of timestamp
};



/// Data buffer for ZeroMQ messages in TRDbox eco-system
class zmq_buffer_t {

public:

    zmq_buffer_t(uint8_t eqclass=0xFF, uint8_t eqid=0xFF)
    : header(eqclass, eqid)
    {}

    header_t& get_header() { return header; }

    // void set_equipment(uint8_t eq)    { header.equipment = eq; }
    void set_payload_size(size_t sz)  { header.payload_size = sz; }
    void add_payload_size(size_t sz)  { header.payload_size += sz; }
    size_t get_payload_size() const   { return header.payload_size; }
    const uint8_t* get_payload_ptr() const  { return payload; }
    uint8_t* get_payload_ptr()        { return payload; }
    size_t get_payload_max_size()     { return max_payload_size; }

    // functions to get a buffer to write to
    uint8_t* get_write_ptr() {return payload + header.payload_size;}
    size_t get_write_max_size() {return max_payload_size-header.payload_size;}

    void append(void* ptr, size_t size) {
        if (size > get_write_max_size()) {
            throw std::length_error("zmq buffer size exceeded");
        }
        memcpy(get_write_ptr(), ptr, size);
        header.payload_size += size;
    }

    // append the contents of other to this buf
    // if offset is non-zero, the copy will start at offset
    // if size is non-zero, only the first sz bytes will be appended
    void append(zmq_buffer_t& other, size_t offset=0, size_t size=0) {
        if (size == 0) {
            size =  other.get_payload_size() - offset;
        }
        append(get_write_ptr()+offset, size);
    }

    void append(zmq::message_t& other, size_t offset=0, size_t size=0) {
        if (size == 0) {
            size =  other.size() - offset;
        }
        append(other.data<char>()+offset, size);
    }



    /// set time stamp to current time
    void set_timestamp() {
        const auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();

        header.timestamp_sec = ns / 1000000000;
        header.timestamp_ns = ns % 1000000000;
    }

    // cast operator: use this buffer like a zmq message
    // This function creates a copy of the buffer to be procesed by zmq
    operator zmq::message_t() const {
        return zmq::message_t(this, sizeof(header) + header.payload_size);
    }

    // // cast operator: use this buffer like a zmq const_buffer
    // operator zmq::const_buffer() const {
    //     return zmq::const_buffer(this, sizeof(header) + header.payload_size);
    // }


private:

    static const size_t max_payload_size = 16*1024*sizeof(uint32_t);

    // actual data buffer: header + payload
    struct header_t header;
    uint8_t payload[max_payload_size];

};

#endif

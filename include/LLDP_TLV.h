#ifndef LLDPTLV_H
#define LLDPTLV_H

#include <iostream>

using std::endl;
using std::cout;
using std::cerr;

enum LLDP_TLV_TYPE {
    END,
    CHASSIS_ID,
    PORT_ID,
    TTL,
    PORT_DESC,
    SYSTEM_NAME,
    SYSTEM_DESC,
    SYSTEM_CAP,
    MGMT_ADDR,
    CUSTOM = 127
};


class LLDP_TLV {
    private:
        uint16_t _type = 0; // Using uint16_t because cerr doesn't play nice w/ uint8_t
        uint16_t _length = 0;
        uint8_t* _value = nullptr;
        LLDP_TLV* _next = nullptr;

    public:
        LLDP_TLV();

        LLDP_TLV(uint16_t type, uint16_t length, uint8_t* data = nullptr);

        /* LLDP TLV Format:
         * ---------------------------------------------
         * | 7 bits type | 9 bits length | n bits data |
         * ---------------------------------------------
         */
        LLDP_TLV(uint8_t* buffer);

        ~LLDP_TLV();

        uint16_t type();

        uint16_t length();

        /* Returns pointer (of type VAL_TYPE) to the value
         *  e.g. int *a = someLLDPTLV.pValue<int>();
         */
        template <typename VAL_TYPE>
        VAL_TYPE* pValue() {
            if (std::is_pointer<VAL_TYPE>::value) {
                cerr << "ERROR: Function already returns type pointer" << endl;
                return nullptr;
            }
            else
                return (VAL_TYPE*)_value;
        };

        LLDP_TLV* next();

        LLDP_TLV& rnext();

        /* Updates the current TLV linked list with the one pointed to
         * by 'lldp_tlv'. Does not create new nodes, so user must be careful
         * not to prematurely de-allocate 'lldp_tlv'.
         */
        void append(LLDP_TLV* lldp_tlv);

        /* Allocates and fills a buffer with the contents of the TLVs starting
         * from this TLV frame. User provides the address of a pointer variable
         * (i.e. pointer to a pointer).
         *
         * NOTE: User must take care to delete the buffer afterwards.
         *
         * Returns the length of the buffer in bytes.
         */
        uint16_t createBuffer(uint8_t** ppBuffer);
};

#endif

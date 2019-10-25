
#include <iostream>
#include "string.h"
#include <arpa/inet.h>

#include "LLDP_TLV.h"

using std::cerr;
using std::endl;

LLDP_TLV::LLDP_TLV() {};

/* LLDP TLV Format:
 * ---------------------------------------------
 * | 7 bits type | 9 bits length | n bits data |
 * ---------------------------------------------
 */
LLDP_TLV::LLDP_TLV(uint16_t type, uint16_t length, uint8_t* data) {
    _type = type;
    _length = length;

    if (_type == END) {
        _length = 0; // Ignore 'length' parameter passed in
        return;
    }

    if (_length && data) {
        if (type == TTL) {
            _value = (uint8_t*) new uint16_t;
            *(uint16_t*)_value = htons(*(uint16_t*)data);
        }
        else if (type == SYSTEM_CAP) {
            _value = (uint8_t*) new uint16_t[2];
            *(uint16_t*)_value = htons(*(uint16_t*)data);
            *(uint16_t*)(_value + 2) = htons(*(uint16_t*)(data + 2));
        }
        else {
            // For TLVs that store string info
            _value = new uint8_t[_length + 1];
            _value[_length] = '\0';
            memcpy(_value, data, _length);
        }
    }
    else if (_length && !data) {
        throw std::runtime_error("ERROR: Cannot construct TLV with non-zero length and no data");
    }

    _next = new LLDP_TLV(0, 0, nullptr);
};

LLDP_TLV::LLDP_TLV(uint8_t* buffer) {
    _type = buffer[0] >> 1;
    _length = ((uint16_t)(buffer[0] & 0x1) << 8) + buffer[1];
    _value = new uint8_t[_length + 1]; // In case this is a string, +1 for NULL char
    _value[_length] = '\0';
    if (_length)
        memcpy(_value, buffer + 2, _length);

    if (_type == 0 || _length == 0)
        // TODO; What if malformed LLDP only has type or length 0, but not both?
        return;
    else
        _next = new LLDP_TLV(buffer + 2 + _length);
};

LLDP_TLV::~LLDP_TLV() {
    if (_value)
        delete []_value;

    if (_next)
        delete _next;
};

uint16_t LLDP_TLV::type() {
    return _type;
};

uint16_t LLDP_TLV::length() {
    return _length;
};

/* Returns pointer (of type VAL_TYPE) to the value
 *  e.g. int *a = someLLDPTLV.pValue<int>();
 */
// VAL_TYPE* LLDP_TLV::pValue() definition in header file

LLDP_TLV* LLDP_TLV::next() {
    return _next;
};

LLDP_TLV& LLDP_TLV::rnext() {
    if (_next)
        return *_next;
    else
        throw std::runtime_error("ERROR: No next TLV in LLDP");
};

/* Updates the current TLV linked list with the one pointed to
* by 'lldp_tlv'. Does not create new nodes, so user must be careful
* not to prematurely de-allocate 'lldp_tlv'.
*/
void LLDP_TLV::append(LLDP_TLV* lldp_tlv) {
    if (_type == END) {
        throw std::runtime_error("ERROR: Can only call this method"
                                    "on a non-End of LLDPPDU TLV");
    }

    // Find the second-last node of the TLV list and append
    if (_next->type() == END) {
        delete _next; // Delete the End of LLDPDU TLV
        _next = lldp_tlv;
    }
    else {
        LLDP_TLV* targetTLV = _next;
        while (targetTLV->next()->type() != END) {
            targetTLV = targetTLV->next();
        }
        targetTLV->append(lldp_tlv);
    }

    return;
};

/* Allocates and fills a buffer with the contents of the TLVs starting
* from this TLV frame. User provides the address of a pointer variable
* (i.e. pointer to a pointer).
*
* NOTE: User must take care to delete the buffer afterwards.
*
* Returns the length of the buffer in bytes.
*
* TODO: Think about corner cases and error handling
*/
uint16_t LLDP_TLV::createBuffer(uint8_t** ppBuffer) {
    if (_type == END) {
        throw std::runtime_error("ERROR: Can only call this method"
                                    "on a non-End of LLDPPDU TLV");
    }

    // Calculate necessary buffer length
    const uint8_t TYPE_LEN_FIELDS = 2; // Bytes
    uint16_t bufLength = TYPE_LEN_FIELDS + _length;
    for (LLDP_TLV* tlv = _next; tlv->next() != nullptr; tlv = tlv->next()) {
        bufLength += (TYPE_LEN_FIELDS + tlv->length());
    }
    bufLength += TYPE_LEN_FIELDS; // For End of LLPDU TLV

    // Allocate buffer and save it to *ppBuffer
    uint8_t* pCurrTLV = new uint8_t[bufLength];
    memset(pCurrTLV, 0x0, bufLength);
    *ppBuffer = pCurrTLV;

    // Write into buffer
    uint16_t *pTypeAndLen = (uint16_t*)pCurrTLV;
    for (LLDP_TLV* tlv = this; tlv->next() != nullptr; tlv = tlv->next()) {
        *pTypeAndLen = htons((tlv->type() << 9) + (tlv->length() & 0x1FF));
        memcpy(pCurrTLV + TYPE_LEN_FIELDS, tlv->pValue<uint8_t>(), tlv->length());

        // Move pointers forward
        pCurrTLV += (TYPE_LEN_FIELDS + tlv->length());
        pTypeAndLen = (uint16_t*)pCurrTLV;
    }

    return bufLength;
}

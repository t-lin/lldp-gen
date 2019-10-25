#include <iostream>
#include <string>
#include <arpa/inet.h>

#include <LLDP_TLV.h>
#include <tins/tins.h>

#define ETHTYPE_LLDP 0x88cc

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using namespace Tins;

const unsigned char LLDP_MAC_NEAREST_BRIDGE[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e};

int main() {
    string ifaceName = "eth0";
    NetworkInterface iface(ifaceName);
    cout << ifaceName << " MAC is: " << iface.info().hw_addr << endl;

    EthernetII::address_type dst_addr = LLDP_MAC_NEAREST_BRIDGE;
    EthernetII::address_type src_addr = iface.info().hw_addr;

    EthernetII ethFrame(dst_addr, src_addr);
    ethFrame.payload_type(ETHTYPE_LLDP);

    //RawPDU* lldpPDU = ethFrame.find_pdu<RawPDU>(); // libtins lacks an LLDP PDU

    uint8_t data[100] = {0}; // Re-usable buffer to fill individual TLVs

    // Create Chassis ID TLV
    data[0] = 7; // Chassis ID subtype 'Locally Assigned'
    string chassisID = "tlin-test-chassis-id";
    memcpy(data + 1, chassisID.c_str(), chassisID.length());
    LLDP_TLV firstTLV = LLDP_TLV(CHASSIS_ID, chassisID.length() + 1, data);

    // Create Port ID TLV
    data[0] = 2; // Port ID subtype 'Port Component'
    string portID = "tlin-port-id";
    memcpy(data + 1, portID.c_str(), portID.length());
    firstTLV.append(new LLDP_TLV(PORT_ID, portID.length() + 1, data));

    // Create TTL TLV
    uint16_t ttl = 69;
    firstTLV.append(new LLDP_TLV(TTL, 2, (uint8_t*)&ttl));

    /*
    for (LLDP_TLV *tlv = &firstTLV; tlv->next() != nullptr; tlv = tlv->next()) {
        cout << "TLV type is: " << tlv->type() << endl;
        cout << "TLV length is: " << tlv->length() << endl;
        if (tlv->type() == TTL) {
            cout << "TLV data is: " << ntohs(*(tlv->pValue<uint16_t>())) << endl;
        }
        else {
            cout << "TLV data is: " << tlv->pValue<uint8_t>() << endl;
        }
    }

    return 0;
    */

    uint8_t *allTLVs = nullptr;
    uint16_t bufferLength = firstTLV.createBuffer(&allTLVs);
    if (allTLVs == nullptr || bufferLength == 0) {
        cerr << "Uh oh, createBuffer failed" << endl;
        return 1;
    }

    RawPDU lldpTLVs(allTLVs, (uint32_t)bufferLength);
    ethFrame = ethFrame / lldpTLVs;

    // Send packet
    PacketSender sender;
    sender.send(ethFrame, iface);

    return 0;
}

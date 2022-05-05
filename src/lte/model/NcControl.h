#ifndef NCCONTROL_H
#define NCCONTROL_H
#include "ns3/object.h"
#include "ns3/lte-pdcp-sap.h"
#include <vector>
#include <map>
#include "ns3/simulator.h"
#include "ns3/lte-rlc-header.h"
#include "ns3/lte-pdcp-tag.h"
#include "ns3/seq-ts-header.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
//#include "ns3/epc-gtpu-header.h"
#include "ns3/lte-pdcp-header.h"
#include "ns3/lte-nc-header.h"



namespace ns3 {

class NcControl:public Object{

public:

  struct ncPara
  {
	  Ptr<Packet> p;
	  std::vector<double> m_coff;
	  uint8_t israwpacket = 0;
	  uint8_t SN = 255;
	  uint32_t seq;
	  Time ts;
	  Time txTime;
	  Time rxTime;
	  Time linkDelay;
	  Time networkDelay;
    };

  struct NcEncodingBuffer{
    std::vector < ncPara > m_ncVector; //ncBuffer
    uint8_t NACK_num=0;
    uint8_t m_rank = 0;
    Time startTime;
  };

  uint32_t m_rlcFlag=0;
  uint8_t m_originalBlockSize;
  uint8_t m_encodingBlockSize;
  std::map<uint64_t,NcEncodingBuffer> m_ncEncodingBufferList;
  uint64_t m_groupnum = 0;
  std::vector < ncPara > m_ncVector; //ncBuffer



  NcControl();
  void HelloWorld();
  Ptr<Packet> SendSaveAndSetTime(Ptr<Packet> p);
  Ptr<Packet> MakeRedundancePacket();
  uint32_t GetNcedSize();
  void RecvAndSave();





};


}

#endif

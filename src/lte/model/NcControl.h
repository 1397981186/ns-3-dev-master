#ifndef NCCONTROL_H
#define NCCONTROL_H
#include "ns3/object.h"
#include "ns3/lte-pdcp-sap.h"
#include <vector>
#include <map>
#include <algorithm>
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

//send use
  uint32_t m_rlcFlag=0;
  uint8_t m_originalBlockSize;
  uint8_t m_encodingBlockSize;
  std::map<uint64_t,NcEncodingBuffer> m_ncEncodingBufferList;
  uint64_t m_groupnum = 0;
  std::vector < ncPara > m_ncVector; //ncBuffer

//recv use
  uint64_t m_rxEncodingPacketNum = 0;
  class NcDecodingBuffer
  {
  public:
    std::vector < ncPara > m_ncVector; //ncBuffer
    //uint8_t Polling_num = 0;
    uint8_t m_rank = 0;
    bool m_statusReportSent = false;
    bool m_ncComplete = false;
    uint8_t num_statusReport = 0;
    EventId m_statusReportTimer;
    std::vector<uint8_t> deliverdSN;
    Time startTime;
    std::vector <uint32_t> seqVector;
    std::vector <Time> tsVector;
    std::vector <Time> txTimeVector;
    std::vector <Time> linkDelayVector;
    std::vector <Time> networkDelayVector;
  };
  std::map<uint64_t,NcDecodingBuffer> m_ncDecodingBufferList;



//send use
  NcControl();
  void HelloWorld();
  Ptr<Packet> SendSaveAndSetTime(Ptr<Packet> p);
  Ptr<Packet> MakeRedundancePacket();
  uint32_t GetNcedSize();


//recv use
  Ptr<Packet> RecvAndSave(Ptr<Packet> p);
  bool IfTransmitSdu();
  bool IfDeocde();
  std::vector <Ptr<Packet>> NcDecode();
  bool IfNcSendArq();
  uint8_t CalulateDecodingRank(uint64_t groupnum);
  Ptr<Packet> MakeStatusReport(uint64_t groupnum);
  std::vector <Ptr<Packet>> NcSendArq();
  void ExpireStatusReportTimer( uint64_t groupnum);
  bool m_IfTransmitSduFlag=false;
  uint64_t m_rxOriginalPacketNum = 0;
  uint64_t m_packetStatistic[5] = {0,0,0,0,0};
  uint64_t m_totalRxSize = 0;
  uint64_t m_okGroupNum = 0;
  uint64_t m_statusReportStatistic[5] = {0,0,0,0,0};
  bool m_IfSendArq;
  bool m_IfRecvArq;
  uint64_t m_ncVrMs;   //尚未完整接受的最小组号
  uint64_t m_failedGroupNum = 0;
  Time m_statusReportTimerValue = MilliSeconds(40.0);
  bool IfRecvArq();







};


}

#endif

#include "ns3/log.h"
#include "CopyControl.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CopyControl");
NS_OBJECT_ENSURE_REGISTERED (CopyControl);


CopyControl::CopyControl():
  m_originalBlockSize (1),
  m_encodingBlockSize (2),
  m_ncVrMs(0){
  NS_LOG_LOGIC (this);
}

void
CopyControl::HelloWorld ()
{
  NS_LOG_DEBUG ("Copy ----------------");
}

Ptr<Packet>
CopyControl::SendSaveAndSetTime (Ptr<Packet> p)
{
/**
 *seqTs udp ipv4 nc pdcp
 *
 */
//  NS_LOG_DEBUG ("packet  "<<p->ToString());

  //get header information
//  LtePdcpHeader pdcpheader;
//  p->RemoveHeader(pdcpheader);
  Ipv4Header ipv4header;
  p->RemoveHeader(ipv4header);
  UdpHeader udpheader;
  p->RemoveHeader(udpheader);
  SeqTsHeader seqtsheader;
  p->RemoveHeader(seqtsheader);
  uint32_t seq = seqtsheader.GetSeq();
  Time ts = seqtsheader.GetTs();
  p->AddHeader(seqtsheader);
  p->AddHeader(udpheader);
  p->AddHeader(ipv4header);
//  p->AddHeader(pdcpheader);


  if(m_ncEncodingBufferList.find(m_groupnum)==m_ncEncodingBufferList.end())
  {
    NcEncodingBuffer ncbuffer;
    m_ncEncodingBufferList.insert({m_groupnum,ncbuffer});
  }
  auto it = m_ncEncodingBufferList.find(m_groupnum);

//  uint64_t Coeff = 1;
//  Coeff = Coeff << (64-1-it->second.m_ncVector.size());

  ncPara para;
  para.p = p;
  para.SN = it->second.m_ncVector.size();
  para.seq = seq;
  para.ts = ts;
  para.txTime = Simulator::Now ();

  para.israwpacket = 1;
//  for (int k=63;k>=0;k--)
//  {
//    para.m_coff.push_back(static_cast<double>((Coeff>>k) & 1));
//  }

  Ptr<Packet> packet = p->Copy();

  NcHeader ncheader;
  ncheader.SetIsRawPacket(1);
  ncheader.SetGroupnum(m_groupnum);
  ncheader.SetSN(it->second.m_ncVector.size());
//  ncheader.SetCoeff(Coeff);

  packet->AddHeader(ncheader);

  it->second.m_ncVector.push_back(para);
  return packet;
}

Ptr<Packet>
CopyControl::MakeRedundancePacket ()
{
  NS_LOG_DEBUG ("---making copyPacket");
  auto it = m_ncEncodingBufferList.find(m_groupnum);
  Ptr<Packet> redundantPacket=Create<Packet> ();;
  *redundantPacket= *it->second.m_ncVector[m_originalBlockSize-1].p;

  NcHeader ncheader;
  ncheader.SetGroupnum(m_groupnum);

//  uint64_t Coeff = 0;
//  for (int i=0; i<m_originalBlockSize; i++)
//  {
//    uint64_t bit = rand()%2;
//    Coeff = Coeff | (bit << (63-i));
//  }
//  ncheader.SetCoeff(Coeff);
//  if(it->second.m_ncVector.size()+1==m_encodingBlockSize){
//    ncheader.SetPolling(1);
//  }

  ncPara para;
//  for (int k=63;k>=0;k--)
//  {
//    para.m_coff.push_back(static_cast<double>((Coeff>>k) & 1));
//  }

  it->second.m_ncVector.push_back(para);
  ncheader.SetSN(it->second.m_ncVector.size());
  redundantPacket->AddHeader(ncheader);

  NS_LOG_DEBUG ("Copy  "<<it->second.m_ncVector.size()<<" groupNum "<<m_groupnum);
  return redundantPacket;

//  if
}

Ptr<Packet>
CopyControl::RecvAndSave (Ptr<Packet> p)
{
  NcHeader ncheader;
  if(p->RemoveHeader(ncheader)==0){
     m_drop=true;
     return p;
  }
  m_drop=false;
  if(ncheader.GetDorC()==1){
    p->AddHeader(ncheader);
    m_IfTransmitSduFlag=false;
    m_IfRecvArq=true;
    return p;
  }else{
    m_IfRecvArq=false;
  }

  m_groupnum=ncheader.GetGroupnum();
//  if(ncheader.GetPolling()==1){
//    NS_LOG_DEBUG ("---recv polling packet");
//    m_IfSendArq=true;
//  }else{
//    m_IfSendArq=false;
//  }


  m_MaxRecvGroupnum=std::max(m_MaxRecvGroupnum,m_groupnum);

  m_rxEncodingPacketNum++;

  ncPara para;
  para.p=p;
//  para.israwpacket = ncheader.IsRawPacket();
  para.SN = ncheader.GetSN();
//  uint64_t ncCoeff = ncheader.GetCoeff();
//  for (int k=63;k>=0;k--)
//  {
//    para.m_coff.push_back(static_cast<double>((ncCoeff>>k) & 1));
//  }


  auto it1=m_ncDecodingBufferList.find(m_groupnum);
  NS_LOG_DEBUG ("---recv group num---  "<<ncheader.GetGroupnum());
  if(it1==m_ncDecodingBufferList.end())
  {
    NcDecodingBuffer newBuffer;
    m_ncDecodingBufferList.insert({ncheader.GetGroupnum(),newBuffer});
    it1=m_ncDecodingBufferList.find(ncheader.GetGroupnum());
    m_IfTransmitSduFlag=true;
    it1->second.deliverdSN.push_back(para.SN);
    it1->second.m_ncComplete=true;
    NS_LOG_DEBUG ("---add true group "<<m_groupnum);
    //it1->second.startTime = rxnctag.GetSenderTimestamp();
    m_pollingInterval=10;
    if(m_groupnum%m_pollingInterval==0){
      m_IfSendArq=true;
    }else{
      m_IfSendArq=false;
    }
  }else if(it1->second.m_noReceive){
    m_IfTransmitSduFlag=true;
    m_pollingInterval=10;
    if(m_groupnum%m_pollingInterval==0){
      m_IfSendArq=true;
    }else{
      m_IfSendArq=false;
    }
    it1->second.m_ncComplete=true;
    it1->second.m_noReceive=false;
  }else{
    m_IfTransmitSduFlag=false;
    m_IfSendArq=false;
  }


//  if (!it1->second.m_ncComplete){
//    it1->second.m_ncVector.push_back(para);
//    NS_LOG_DEBUG ("---para sn ---  "<<unsigned(para.SN));
//    if (para.SN < m_originalBlockSize){
//      m_rxOriginalPacketNum ++;
//      it1->second.deliverdSN.push_back(para.SN);
////      m_packetStatistic[0]++;
//      m_IfTransmitSduFlag=true;
//    }else{
//      m_IfTransmitSduFlag=false;
//    }
//  }else{
//    m_IfTransmitSduFlag=false;
//  }

  return p;

}

bool
CopyControl::IfTransmitSdu ()
{
  return m_IfTransmitSduFlag;
}

bool
CopyControl::IfCopySendArq ()
{
  return m_IfSendArq;
}

void
CopyControl::MakeStatusReport (uint64_t groupnum,std::vector<Ptr<Packet> > &ArqPackets)
{
//  auto it=m_ncDecodingBufferList.find(groupnum);
  Ptr<Packet> StatusReport = Create<Packet> (1);
  NcHeader Statusncheader;
  Statusncheader.SetGroupnum(groupnum);
  Statusncheader.SetDorC(1);
//  Statusncheader.SetRank(it->second.m_rank);
  StatusReport->AddHeader(Statusncheader);
  ArqPackets.push_back(StatusReport);
//  it->second.num_statusReport ++;
//  return StatusReport;
}

void
CopyControl::CopySendArqReq (std::vector<uint64_t>&ArqGroupNums, std::vector<Ptr<Packet> > &ArqPackets)
{
  NS_LOG_DEBUG ("---send copy ArqReq");
  //对m_ncVrMs<=i<=ncheader.GetGroupnum()中的每个组号i依次进行处理
//  std::vector<Ptr<Packet> > ArqPackets;
  uint8_t cnt=0;
  NS_LOG_DEBUG ("---it at i "<< m_ncVrMs);
  if(m_groupnum<50){return ;}
  for (uint64_t i=m_ncVrMs; i<=m_groupnum-50; i++)
//  for (uint64_t i=m_ncVrMs; i<=m_MaxRecvGroupnum; i++)
  {

    //将it3指向第i组的解码buffer
    auto it3=m_ncDecodingBufferList.find(i);
    if (it3==m_ncDecodingBufferList.end())
    {
      NcDecodingBuffer newBuffer;
      m_ncDecodingBufferList.insert({i,newBuffer});
      it3 = m_ncDecodingBufferList.find(i);
      it3->second.m_noReceive=true;
      NS_LOG_DEBUG ("---add no recv "<< i);

    }
    if (!it3->second.m_ncComplete && !it3->second.m_statusReportTimer.IsRunning())
//    if (!it3->second.m_ncComplete )
    {
      NS_LOG_DEBUG ("---it at i "<< i);
      if (it3->second.num_statusReport<3)
      {
      //	CalulateDecodingRank(i);
      //	ArqPackets.push_back(MakeSendPackets(i));
        ArqGroupNums.push_back(i);
        MakeStatusReport(i,ArqPackets);
        cnt++;
        NS_LOG_DEBUG ("---add arq req at i "<< i);
      }
      else
      {
        m_statusReportStatistic[it3->second.num_statusReport+1] ++ ;
        m_failedGroupNum ++;
        it3->second.m_ncComplete = true;
        NS_LOG_DEBUG ("num_statusReport is bigger than 3");
      }
    }
    if(cnt==15){break;}
  }
  // 将m_ncVrMs置为ncCompelte=1的连续的最小组号+1
  auto it1 = m_ncDecodingBufferList.find(m_ncVrMs);
  while (it1!=m_ncDecodingBufferList.end() && it1->second.m_ncComplete)
  {
    m_ncVrMs++;
    it1 = m_ncDecodingBufferList.find(m_ncVrMs);
  }
  NS_LOG_DEBUG ("m_ncVrMs now is "<<m_ncVrMs<<" ArqReqPackets nums is "<<ArqPackets.size());

}

void
CopyControl::ExpireStatusReportTimer (uint64_t groupnum,std::vector<Ptr<Packet> > &ArqPackets)
{
  auto it = m_ncDecodingBufferList.find(groupnum);

  if (it->second.num_statusReport<3)
  {
      NS_LOG_DEBUG(" group num is "<<groupnum<<" arq num is "<<(unsigned)it->second.num_statusReport);
      MakeStatusReport(groupnum,ArqPackets);
  }
}

bool
CopyControl::IfRecvArq ()
{
  return m_IfRecvArq;
}

std::vector<Ptr<Packet> >
CopyControl::MakeCopyArqSendPacket (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("made copy arq packets to send ");
  NcHeader ncheader;
  p->RemoveHeader(ncheader);
  NS_LOG_DEBUG (" arq group num is "<<ncheader.GetGroupnum());

  std::vector<Ptr<Packet> > arqPackets;
  auto it = m_ncEncodingBufferList.find(ncheader.GetGroupnum());
  if(it!=m_ncEncodingBufferList.end())
  {
    it->second.NACK_num ++;
    NcHeader reTxheader;
    reTxheader.SetGroupnum(ncheader.GetGroupnum());
    Ptr<Packet> reTxpacket = it->second.m_ncVector[m_originalBlockSize-1].p->Copy();
    if(!reTxpacket->AddHeader(reTxheader)){
        NS_LOG_DEBUG ("---head add wrong");
    }else{
        arqPackets.push_back(reTxpacket);
    }
//    reTxpacket->AddHeader(reTxheader);
//    arqPackets.push_back(reTxpacket);
   }
  NS_LOG_DEBUG ("made copy arq packets to send , num of packets is "<<arqPackets.size());
  return arqPackets;

}


void
CopyControl::stopArqTimer ()
{
  auto it1=m_ncDecodingBufferList.find(m_groupnum);
  if (it1->second.m_statusReportTimer.IsRunning())
  {
	  it1->second.m_statusReportTimer.Cancel();
  }
}

};


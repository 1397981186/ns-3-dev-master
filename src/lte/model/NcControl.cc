#include "ns3/log.h"
#include "NcControl.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NcControl");
NS_OBJECT_ENSURE_REGISTERED (NcControl);

NcControl::NcControl():
  m_originalBlockSize (10),
  m_encodingBlockSize (12){
  NS_LOG_LOGIC (this);
}

void
NcControl::HelloWorld ()
{
  NS_LOG_DEBUG ("Nc ----------------");
}

Ptr<Packet>
NcControl::SendSaveAndSetTime (Ptr<Packet> p)
{
/**
 *seqTs udp ipv4 nc pdcp
 *
 */
  NS_LOG_DEBUG ("packet  "<<p->ToString());

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
  uint64_t Coeff = 1;
  Coeff = Coeff << (64-1-it->second.m_ncVector.size());

  ncPara para;
  para.p = p;
  para.SN = it->second.m_ncVector.size();
  para.seq = seq;
  para.ts = ts;
  para.txTime = Simulator::Now ();

  para.israwpacket = 1;
  for (int k=63;k>=0;k--)
  {
    para.m_coff.push_back(static_cast<double>((Coeff>>k) & 1));
  }

  Ptr<Packet> packet = p->Copy();

  NcHeader ncheader;
  ncheader.SetIsRawPacket(1);
  ncheader.SetGroupnum(m_groupnum);
  ncheader.SetSN(it->second.m_ncVector.size());
  ncheader.SetCoeff(Coeff);

  packet->AddHeader(ncheader);

  it->second.m_ncVector.push_back(para);
  return packet;
}

Ptr<Packet>
NcControl::MakeRedundancePacket ()
{
  NS_LOG_DEBUG ("---making RedundancePacket");
  auto it = m_ncEncodingBufferList.find(m_groupnum);
  Ptr<Packet> redundantPacket=Create<Packet> ();;
  *redundantPacket= *it->second.m_ncVector[m_originalBlockSize-1].p;

  NcHeader ncheader;
  ncheader.SetGroupnum(m_groupnum);

  uint64_t Coeff = 0;
  for (int i=0; i<m_originalBlockSize; i++)
  {
    uint64_t bit = rand()%2;
    Coeff = Coeff | (bit << (63-i));
  }
  ncheader.SetCoeff(Coeff);
  if(it->second.m_ncVector.size()==m_encodingBlockSize){
    ncheader.SetPolling(1);
  }

  ncPara para;
  for (int k=63;k>=0;k--)
  {
    para.m_coff.push_back(static_cast<double>((Coeff>>k) & 1));
  }

  ncheader.SetSN(it->second.m_ncVector.size());
  redundantPacket->AddHeader(ncheader);

  it->second.m_ncVector.push_back(para);
  NS_LOG_DEBUG ("Ncde size  "<<it->second.m_ncVector.size()<<" groupNum "<<m_groupnum);
  return redundantPacket;

//  if
}

uint32_t
NcControl::GetNcedSize ()
{
  auto it = m_ncEncodingBufferList.find(m_groupnum);
  NS_LOG_DEBUG ("Ncde size  "<<it->second.m_ncVector.size());
  return it->second.m_ncVector.size();
}

Ptr<Packet>
NcControl::RecvAndSave (Ptr<Packet> p)
{
  NcHeader ncheader;
  p->RemoveHeader(ncheader);
  m_rxEncodingPacketNum++;
  ncPara para;
  para.p=p;
  para.israwpacket = ncheader.IsRawPacket();
  para.SN = ncheader.GetSN();
  uint64_t ncCoeff = ncheader.GetCoeff();
  for (int k=63;k>=0;k--)
  {
    para.m_coff.push_back(static_cast<double>((ncCoeff>>k) & 1));
  }
  m_groupnum=ncheader.GetGroupnum();
  auto it1=m_ncDecodingBufferList.find(m_groupnum);
  NS_LOG_DEBUG ("---recv group num---  "<<ncheader.GetGroupnum());
  if(it1==m_ncDecodingBufferList.end())
  {
    NcDecodingBuffer newBuffer;
    m_ncDecodingBufferList.insert({ncheader.GetGroupnum(),newBuffer});
    it1=m_ncDecodingBufferList.find(ncheader.GetGroupnum());
    //it1->second.startTime = rxnctag.GetSenderTimestamp();
  }

  if (!it1->second.m_ncComplete){
    it1->second.m_ncVector.push_back(para);
    NS_LOG_DEBUG ("---para sn ---  "<<unsigned(para.SN));
    if (para.SN < m_originalBlockSize){
      m_rxOriginalPacketNum ++;
      it1->second.deliverdSN.push_back(para.SN);
      m_packetStatistic[0]++;
      m_IfTransmitSduFlag=true;
    }else{
      m_IfTransmitSduFlag=false;
    }


  }
  return p;

}

bool
NcControl::IfTransmitSdu ()
{
  return m_IfTransmitSduFlag;
}

bool
NcControl::IfDeocde ()
{
  auto it1=m_ncDecodingBufferList.find(m_groupnum);
  if(it1->second.m_ncVector.size()>=m_originalBlockSize){
      return true;
  }else{
      return false;
  }
}

std::vector <Ptr<Packet>>
NcControl::NcDecode ()
{

}

bool
NcControl::IfNcArq ()
{
  return true;
}

};




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
NcControl::SaveAndSetTime (Ptr<Packet> p)
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


  return packet;
}

Ptr<Packet>
NcControl::RedundancePacket ()
{
  NS_LOG_DEBUG ("---making RedundancePacket");
//  if
}

uint32_t
NcControl::GetNcedSize ()
{
  auto it = m_ncEncodingBufferList.find(m_groupnum);
  NS_LOG_DEBUG ("Ncde size  "<<it->second.m_ncVector.size());
  return it->second.m_ncVector.size();
}

};




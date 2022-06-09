#include "ns3/log.h"
#include "NcControl.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NcControl");
NS_OBJECT_ENSURE_REGISTERED (NcControl);

NcControl::NcControl():
  m_originalBlockSize (10),
  m_encodingBlockSize (12),
  m_ncVrMs(0){
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
  if(it->second.m_ncVector.size()+1==m_encodingBlockSize){
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

  if(ncheader.GetDorC()==1){
    p->AddHeader(ncheader);
    m_IfRecvArq=true;
    return p;
  }else{
    m_IfRecvArq=false;
  }
  m_groupnum=ncheader.GetGroupnum();
  if(ncheader.GetPolling()==1){
    NS_LOG_DEBUG ("---recv polling packet");
    m_IfSendArq=true;
    m_MaxRecvGroupnum=std::max(m_MaxRecvGroupnum,m_groupnum);
  }else{
    m_IfSendArq=false;
  }

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
  }else{
    m_IfTransmitSduFlag=false;
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
  if (!it1->second.m_ncComplete){
	NS_LOG_DEBUG ("---m_ncVector.size() is "<<it1->second.m_ncVector.size()<<" m_originalBlockSize "<<unsigned(m_originalBlockSize));
    if(it1->second.m_ncVector.size()>=m_originalBlockSize){
	return true;
    }else{
	return false;
    }
  }else{
      return false;
  }
}

uint8_t
NcControl::CalulateDecodingRank( uint64_t groupnum)
{

  auto it=m_ncDecodingBufferList.find(groupnum);
  int decodingBlockSize = it->second.m_ncVector.size();

  uint8_t rank = decodingBlockSize<m_originalBlockSize ? decodingBlockSize:m_originalBlockSize;
  it->second.m_rank = rank;
  NS_LOG_DEBUG ("---CalulateDecodingRank is "<<unsigned(rank));
  return rank;
  /*
  Eigen::MatrixXf a(decodingBlockSize,64);
  if (decodingBlockSize!=0)
  {
	  for(int k=0;k<decodingBlockSize;k++)
	  {
		  for (int l=0; l<64; l++)
		  {
			  a(k,l) = it->second.m_ncVector[k].m_coff[l];
		  }
	  }
	  Eigen::JacobiSVD<Eigen::MatrixXf> svd(a);
	  it->second.m_rank = svd.rank();
	  return svd.rank();
  }
  else
  {
	  return 0;
  }
  */
}

std::vector <Ptr<Packet>>
NcControl::NcDecode ()
{
  auto it1=m_ncDecodingBufferList.find(m_groupnum);
  std::vector <Ptr<Packet>> packets;
  if(!it1->second.m_ncComplete){
    NS_LOG_DEBUG ("---Ncdecode");
    CalulateDecodingRank(m_groupnum);
    if(it1->second.m_rank >= m_originalBlockSize)
    {
      uint8_t num_nonDeliveredPackets = 0;
      for(uint8_t j=0; j<m_originalBlockSize ;j++)
      {
	if (std::find(it1->second.deliverdSN.begin(),it1->second.deliverdSN.end(),j)==it1->second.deliverdSN.end())
	{//if not find ,means miss ,need to decode
	  NS_LOG_DEBUG ("---not find j "<<unsigned(j));
	  num_nonDeliveredPackets ++;
	  m_rxOriginalPacketNum ++;
	  m_totalRxSize += it1->second.m_ncVector[j].p->GetSize();


	  Ptr <Packet> retrievalPacket = it1->second.m_ncVector[j].p->Copy();
	  Ipv4Header ipv4header;
	  retrievalPacket->RemoveHeader(ipv4header);
	  UdpHeader udpheader;
	  retrievalPacket->RemoveHeader(udpheader);
	  SeqTsHeader seqtsheader;
	  retrievalPacket->RemoveHeader(seqtsheader);
//	  NS_LOG_DEBUG ("---here");
//	  seqtsheader.SetSeq(it1->second.seqVector[j]);
//	  seqtsheader.SetTs(it1->second.tsVector[j]);

  //	m_rlcSapUser->ReceivePdcpPdu(retrievalPacket);
	  retrievalPacket->AddHeader(seqtsheader);
	  retrievalPacket->AddHeader(udpheader);
	  retrievalPacket->AddHeader(ipv4header);

	  packets.push_back(retrievalPacket);
	  it1->second.deliverdSN.push_back(j);

	}
      }

      m_okGroupNum ++;
      m_packetStatistic[it1->second.num_statusReport] += num_nonDeliveredPackets;
      m_statusReportStatistic[it1->second.num_statusReport] ++ ;
      //if(m_cellId==1 && it1->second.num_statusReport!=0)
      /*
      if(m_cellId==1)
      {
	      std::cout<<"SRnum_"<<ncheader.GetGroupnum()<<" = "<<+it1->second.num_statusReport
			      <<", SRstatistic = {"
			      <<statusReportStatistic[0]<<", "
			      <<statusReportStatistic[1]<<", "
			      <<statusReportStatistic[2]<<", "
			      <<statusReportStatistic[3]<<", "
			      <<statusReportStatistic[4]<<"}"
			      <<", PacketStatistic = {"
			      <<packetStatistic[0]<<", "
			      <<packetStatistic[1]<<", "
			      <<packetStatistic[2]<<", "
			      <<packetStatistic[3]<<", "
			      <<packetStatistic[4]<<"}"
			      <<", rxPacketNum = "<<rxOriginalPacketNum
			      <<std::endl;
      }
      */
      it1->second.m_ncComplete = true;
    }
  }else{
    NS_LOG_DEBUG ("---complete don't need decode");
  }
  NS_LOG_DEBUG ("---Ncdecode packet nums is "<<packets.size());
  return packets;
}


bool
NcControl::IfNcSendArq ()
{
  return m_IfSendArq;
}

void
NcControl::MakeStatusReport (uint64_t groupnum,std::vector<Ptr<Packet> > &ArqPackets)
{
  auto it=m_ncDecodingBufferList.find(groupnum);
  Ptr<Packet> StatusReport = Create<Packet> (1);
  NcHeader Statusncheader;
  Statusncheader.SetGroupnum(groupnum);
  Statusncheader.SetDorC(1);
  Statusncheader.SetRank(it->second.m_rank);
  StatusReport->AddHeader(Statusncheader);
  ArqPackets.push_back(StatusReport);
//  it->second.num_statusReport ++;

//  it->second.m_statusReportTimer = Simulator::Schedule (m_statusReportTimerValue,
//								    &NcControl::ExpireStatusReportTimer, this, groupnum,ArqPackets);

//  return StatusReport;
}


//void
//NcControl::MakeStatusReport (uint64_t groupnum)
//{
//  auto it=m_ncDecodingBufferList.find(groupnum);
//  it->second.m_statusReportTimer = Simulator::Schedule (m_statusReportTimerValue,
//								  &NcControl::ExpireStatusReportTimer, this, groupnum);
//}
//
//Ptr<Packet>
//NcControl::MakeSendPackets (uint64_t groupnum)
//{
//    auto it=m_ncDecodingBufferList.find(groupnum);
//    Ptr<Packet> StatusReport = Create<Packet> (101);
//    NcHeader Statusncheader;
//    Statusncheader.SetGroupnum(groupnum);
//    Statusncheader.SetDorC(1);
//    Statusncheader.SetRank(it->second.m_rank);
//    StatusReport->AddHeader(Statusncheader)	;
//    it->second.num_statusReport ++;
//    return StatusReport;
//}

void
NcControl::NcSendArqReq (std::vector <uint64_t> &ArqGroupNums,std::vector <Ptr<Packet>> &ArqPackets)
{
  NS_LOG_DEBUG ("---send ArqReq");
  //对m_ncVrMs<=i<=ncheader.GetGroupnum()中的每个组号i依次进行处理
//  std::vector<Ptr<Packet> > ArqPackets;
  uint8_t cnt=0;
  NS_LOG_DEBUG ("---it at i "<< m_ncVrMs);
  if(m_groupnum<4){return ;}
  for (uint64_t i=m_ncVrMs; i<=m_groupnum-3; i++)
//  for (uint64_t i=m_ncVrMs; i<=m_MaxRecvGroupnum; i++)
  {

    //将it3指向第i组的解码buffer
    auto it3=m_ncDecodingBufferList.find(i);
    if (it3==m_ncDecodingBufferList.end())
    {
      NcDecodingBuffer newBuffer;
      m_ncDecodingBufferList.insert({i,newBuffer});
      it3 = m_ncDecodingBufferList.find(i);
    }
    if (!it3->second.m_ncComplete && !it3->second.m_statusReportTimer.IsRunning())
//    if (!it3->second.m_ncComplete )
    {
      NS_LOG_DEBUG ("---it at i "<< i);
      if (it3->second.num_statusReport<3)
      {
	CalulateDecodingRank(i);
	if(it3->second.m_rank<6){
	    it3->second.m_ncComplete=true;
	    NS_LOG_DEBUG ("---rank too small ,give up. rank is "<<unsigned( it3->second.m_rank));
	}else{
	    MakeStatusReport(i,ArqPackets);
	    ArqGroupNums.push_back(i);
	    cnt++;
	    NS_LOG_DEBUG ("---add arq req at i "<< i);
	}
//	ArqPackets.push_back(MakeSendPackets(i));

	/*
	if (m_cellId==1)
	{
		std::cout<<"reTxgroup = "<<i
				<<",  SRnum ="<<+it3->second.num_statusReport
				<<",  rank = "<<+it3->second.m_rank
				<<",  vectorsize = "<<+it3->second.m_ncVector.size()
				<<std::endl;
	}
	*/
      }
      else
      {
	/*
	if (m_cellId==1)
	{
		std::cout<<"failedgroup = "<< i
				<<",  vectorsize = "<<it3->second.m_ncVector.size()
				<<",  rank = "<<+it3->second.m_rank
				<<",  statusReportNum = "<<+it3->second.num_statusReport
				<<std::endl;
	}
	*/
	m_statusReportStatistic[it3->second.num_statusReport+1] ++ ;
	m_failedGroupNum ++;
	it3->second.m_ncComplete = true;
	NS_LOG_DEBUG ("num_statusReport is bigger than 3");
      }
    }
    if(cnt==5){break;}
  }
  // 将m_ncVrMs置为ncCompelte=1的连续的最小组号+1
  auto it1 = m_ncDecodingBufferList.find(m_ncVrMs);
  while (it1!=m_ncDecodingBufferList.end() && it1->second.m_ncComplete)
  {
    m_ncVrMs++;
    it1 = m_ncDecodingBufferList.find(m_ncVrMs);
  }
  NS_LOG_DEBUG ("m_ncVrMs now is "<<m_ncVrMs<<" ArqReqPackets nums is "<<ArqPackets.size());
//  return ArqPackets;

}

void
NcControl::ExpireStatusReportTimer (uint64_t groupnum,std::vector<Ptr<Packet> > &ArqPackets)
{
  auto it = m_ncDecodingBufferList.find(groupnum);

  if (it->second.num_statusReport<3)
  {
      MakeStatusReport(groupnum,ArqPackets);
  }
}

bool
NcControl::IfRecvArq ()
{
  return m_IfRecvArq;
}

std::vector<Ptr<Packet> >
NcControl::MakeNcArqSendPacket (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("made arq packets to send ");
  NcHeader ncheader;
  p->RemoveHeader(ncheader);
  NS_LOG_DEBUG (" arq group num is "<<ncheader.GetGroupnum()<<" arq rank is "<<unsigned(ncheader.GetRank()));

  std::vector<Ptr<Packet> > arqPackets;
  auto it = m_ncEncodingBufferList.find(ncheader.GetGroupnum());
  if(it!=m_ncEncodingBufferList.end())
  {
    it->second.NACK_num ++;
    for(int i=0;i<(m_originalBlockSize-ncheader.GetRank());i++ )
    {
      NcHeader reTxheader;
      if(i==(m_originalBlockSize-1-ncheader.GetRank()))
      {
	reTxheader.SetPolling(1);
      }
      reTxheader.SetGroupnum(ncheader.GetGroupnum());
      uint64_t ncCoeff = 0;
      for (int i=0; i<m_originalBlockSize; i++)
      {
	      uint64_t bit = rand()%2;
	      ncCoeff = ncCoeff | (bit << (63-i));
      }
      reTxheader.SetCoeff(ncCoeff);
      Ptr<Packet> reTxpacket = it->second.m_ncVector[m_originalBlockSize-1].p->Copy();
      if(!reTxpacket->AddHeader(reTxheader)){
          NS_LOG_DEBUG ("---head add wrong");
      }else{
          arqPackets.push_back(reTxpacket);
      }
//      txEncodingPacketNum ++;
//      NcTag nctag(Simulator::Now ());
//      for (int i=0; i<originalBlockSize; i++)
//      {
//	  nctag.seqVector.push_back(it->second.m_ncVector[i].seq);
//	  nctag.tsVector.push_back(it->second.m_ncVector[i].ts);
//	  nctag.txTimeVector.push_back(it->second.m_ncVector[i].txTime);
//      }
//      nctag.vectorSize = originalBlockSize;
//      reTxpacket->AddByteTag(nctag);
    }
  }
  NS_LOG_DEBUG ("made arq packets to send , num of packets is "<<arqPackets.size());
  return arqPackets;
}


void
NcControl::stopArqTimer ()
{
  auto it1=m_ncDecodingBufferList.find(m_groupnum);
  if (it1->second.m_statusReportTimer.IsRunning())
  {
	  it1->second.m_statusReportTimer.Cancel();
  }
}

};


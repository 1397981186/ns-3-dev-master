/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 */


#include "epc-enb-application.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/uinteger.h"

#include "epc-gtpu-header.h"
#include "eps-bearer-tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcEnbApplication");

EpcEnbApplication::EpsFlowId_t::EpsFlowId_t ()
{
}

EpcEnbApplication::EpsFlowId_t::EpsFlowId_t (const uint16_t a, const uint8_t b)
  : m_rnti (a),
    m_bid (b)
{
}

bool
operator == (const EpcEnbApplication::EpsFlowId_t &a, const EpcEnbApplication::EpsFlowId_t &b)
{
  return ( (a.m_rnti == b.m_rnti) && (a.m_bid == b.m_bid) );
}

bool
operator < (const EpcEnbApplication::EpsFlowId_t& a, const EpcEnbApplication::EpsFlowId_t& b)
{
  return ( (a.m_rnti < b.m_rnti) || ( (a.m_rnti == b.m_rnti) && (a.m_bid < b.m_bid) ) );
}


TypeId
EpcEnbApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcEnbApplication")
    .SetParent<Object> ()
    .SetGroupName("Lte")
    .AddTraceSource ("RxFromEnb",
                     "Receive data packets from LTE Enb Net Device",
                     MakeTraceSourceAccessor (&EpcEnbApplication::m_rxLteSocketPktTrace),
                     "ns3::EpcEnbApplication::RxTracedCallback")
    .AddTraceSource ("RxFromS1u",
                     "Receive data packets from S1-U Net Device",
                     MakeTraceSourceAccessor (&EpcEnbApplication::m_rxS1uSocketPktTrace),
                     "ns3::EpcEnbApplication::RxTracedCallback")
    ;
  return tid;
}

void
EpcEnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_lteSocket = 0;
  m_lteSocket6 = 0;
  m_s1uSocket = 0;
  delete m_s1SapProvider;
  delete m_s1apSapEnb;
}

EpcEnbApplication::EpcEnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6, uint16_t cellId)
  : m_lteSocket (lteSocket),
    m_lteSocket6 (lteSocket6),
    m_gtpuUdpPort (2152), // fixed by the standard
    m_s1SapUser (0),
    m_s1apSapMme (0),
    m_cellId (cellId)
{
  NS_LOG_FUNCTION (this << lteSocket << lteSocket6 << cellId);

  m_lteSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromLteSocket, this));
  m_lteSocket6->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromLteSocket, this));
  m_s1SapProvider = new MemberEpcEnbS1SapProvider<EpcEnbApplication> (this);
  m_s1apSapEnb = new MemberEpcS1apSapEnb<EpcEnbApplication> (this);
}


//znr_note: example使用PointToPointHelper创建epchelper，然后使用AddEnb函数将调用AddS1Interface函数
void
EpcEnbApplication::AddS1Interface (Ptr<Socket> s1uSocket, Ipv4Address enbAddress, Ipv4Address sgwAddress)
{
  NS_LOG_FUNCTION (this << s1uSocket << enbAddress << sgwAddress);

  m_s1uSocket = s1uSocket;
  m_s1uSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromS1uSocket, this));
  m_enbS1uAddress = enbAddress;
  m_sgwS1uAddress = sgwAddress;
}


EpcEnbApplication::~EpcEnbApplication (void)
{
  NS_LOG_FUNCTION (this);
}


void 
EpcEnbApplication::SetS1SapUser (EpcEnbS1SapUser * s)
{
  m_s1SapUser = s;
}

  
EpcEnbS1SapProvider* 
EpcEnbApplication::GetS1SapProvider ()
{
  return m_s1SapProvider;
}

void 
EpcEnbApplication::SetS1apSapMme (EpcS1apSapMme * s)
{
  m_s1apSapMme = s;
}

  
EpcS1apSapEnb* 
EpcEnbApplication::GetS1apSapEnb ()
{
  return m_s1apSapEnb;
}

void 
EpcEnbApplication::DoInitialUeMessage (uint64_t imsi, uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  // side effect: create entry if not exist
  m_imsiRntiMap[imsi] = rnti;
  m_s1apSapMme->InitialUeMessage (imsi, rnti, imsi, m_cellId);
}

void 
EpcEnbApplication::DoPathSwitchRequest (EpcEnbS1SapProvider::PathSwitchRequestParameters params)
{
  NS_LOG_FUNCTION (this);
  uint16_t enbUeS1Id = params.rnti;  
  uint64_t mmeUeS1Id = params.mmeUeS1Id;
  uint64_t imsi = mmeUeS1Id;
  // side effect: create entry if not exist
  m_imsiRntiMap[imsi] = params.rnti;

  uint16_t gci = params.cellId;
  std::list<EpcS1apSapMme::ErabSwitchedInDownlinkItem> erabToBeSwitchedInDownlinkList;
  for (std::list<EpcEnbS1SapProvider::BearerToBeSwitched>::iterator bit = params.bearersToBeSwitched.begin ();
       bit != params.bearersToBeSwitched.end ();
       ++bit)
    {
      EpsFlowId_t flowId;
      flowId.m_rnti = params.rnti;
      flowId.m_bid = bit->epsBearerId;
      uint32_t teid = bit->teid;
      
      EpsFlowId_t rbid (params.rnti, bit->epsBearerId);
      // side effect: create entries if not exist
      m_rbidTeidMap[params.rnti][bit->epsBearerId] = teid;
      m_teidRbidMap[teid] = rbid;

      EpcS1apSapMme::ErabSwitchedInDownlinkItem erab;
      erab.erabId = bit->epsBearerId;
      erab.enbTransportLayerAddress = m_enbS1uAddress;
      erab.enbTeid = bit->teid;

      erabToBeSwitchedInDownlinkList.push_back (erab);
    }
  m_s1apSapMme->PathSwitchRequest (enbUeS1Id, mmeUeS1Id, gci, erabToBeSwitchedInDownlinkList);
}

void 
EpcEnbApplication::DoUeContextRelease (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  std::map<uint16_t, std::map<uint8_t, uint32_t> >::iterator rntiIt = m_rbidTeidMap.find (rnti);
  if (rntiIt != m_rbidTeidMap.end ())
    {
      for (std::map<uint8_t, uint32_t>::iterator bidIt = rntiIt->second.begin ();
           bidIt != rntiIt->second.end ();
           ++bidIt)
        {
          uint32_t teid = bidIt->second;
          m_teidRbidMap.erase (teid);
          NS_LOG_INFO ("TEID: " << teid << " erased");
        }
      m_rbidTeidMap.erase (rntiIt);
      NS_LOG_INFO ("RNTI: " << rntiIt->first << " erased");
    }
}

void 
EpcEnbApplication::DoInitialContextSetupRequest (uint64_t mmeUeS1Id, uint16_t enbUeS1Id, std::list<EpcS1apSapEnb::ErabToBeSetupItem> erabToBeSetupList)
{
  NS_LOG_FUNCTION (this);

  uint64_t imsi = mmeUeS1Id;
  std::map<uint64_t, uint16_t>::iterator imsiIt = m_imsiRntiMap.find (imsi);
  NS_ASSERT_MSG (imsiIt != m_imsiRntiMap.end (), "unknown IMSI");
  uint16_t rnti = imsiIt->second;

  for (std::list<EpcS1apSapEnb::ErabToBeSetupItem>::iterator erabIt = erabToBeSetupList.begin ();
       erabIt != erabToBeSetupList.end ();//znr_note: erab Evolved Radio Access Bearer
       ++erabIt)
    {
      // request the RRC to setup a radio bearer
      struct EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params;
      params.rnti = rnti;
      params.bearer = erabIt->erabLevelQosParameters;//znr_note: 此参数不含bear isGbr()

      /*
      //znr_note: 方案2，pdcp复制修改后，适用cttc-nr-demo.cc（20220430编译版本）
      //params.bearerId = erabIt->erabId;//znr_com: 问题见lte-enb-rrc.cc, line=423 "bearerId == 0 || bid == bearerId"
      if( erabIt->erabId == 2 )//znr_add: 关键代码，适应nr双数据承载情况下实现pdcp dup；是否需要其他适应性修改？？？
        params.bearerId = erabIt->erabId+1;//znr_add: 如此bearerId只有奇数（且与bid不同）他处可能通过需要-1匹配使用？？？
      else//znr_add
        params.bearerId = erabIt->erabId;//znr_add
      //znr_note: 以上为方案2
      */
      
      //znr_note: 方案1，cttc原始状态，pdcp复制修改后，适用cttc-nr-cc-bwp-demo.cc，也可适用cttc-nr-demo.cc（20220603编译版本）
      params.bearerId = erabIt->erabId;
      //znr_note: 以上为方案1

      params.gtpTeid = erabIt->sgwTeid;
      NS_LOG_INFO (this << " EpcEnbApplication m_s1SapUser->DataRadioBearerSetupRequest");//znr_add
      m_s1SapUser->DataRadioBearerSetupRequest (params);//znr_note: 此处需要做适应性修改 

      EpsFlowId_t rbid (rnti, erabIt->erabId);
      // side effect: create entries if not exist
      //znr_note: 隧道协议双向交互地图
      m_rbidTeidMap[rnti][erabIt->erabId] = params.gtpTeid;
      m_teidRbidMap[params.gtpTeid] = rbid;
    }

  // Send Initial Context Setup Request to RRC
  struct EpcEnbS1SapUser::InitialContextSetupRequestParameters params;
  params.rnti = rnti;
  m_s1SapUser->InitialContextSetupRequest (params);
}

void 
EpcEnbApplication::DoPathSwitchRequestAcknowledge (uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t gci, std::list<EpcS1apSapEnb::ErabSwitchedInUplinkItem> erabToBeSwitchedInUplinkList)
{
  NS_LOG_FUNCTION (this);

  uint64_t imsi = mmeUeS1Id;
  std::map<uint64_t, uint16_t>::iterator imsiIt = m_imsiRntiMap.find (imsi);
  NS_ASSERT_MSG (imsiIt != m_imsiRntiMap.end (), "unknown IMSI");
  uint16_t rnti = imsiIt->second;
  EpcEnbS1SapUser::PathSwitchRequestAcknowledgeParameters params;
  params.rnti = rnti;
  m_s1SapUser->PathSwitchRequestAcknowledge (params);
}

void 
EpcEnbApplication::RecvFromLteSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);  
  if(m_lteSocket6)
    {
      NS_ASSERT (socket == m_lteSocket || socket == m_lteSocket6);
    }
  else
    {
      NS_ASSERT (socket == m_lteSocket);
    }
  Ptr<Packet> packet = socket->Recv ();

  EpsBearerTag tag;
  bool found = packet->RemovePacketTag (tag);
  NS_ASSERT (found);
  uint16_t rnti = tag.GetRnti ();
  uint8_t bid = tag.GetBid ();
  NS_LOG_LOGIC ("received packet with RNTI=" << (uint32_t) rnti << ", BID=" << (uint32_t)  bid);
  std::map<uint16_t, std::map<uint8_t, uint32_t> >::iterator rntiIt = m_rbidTeidMap.find (rnti);
  if (rntiIt == m_rbidTeidMap.end ())
    {
      NS_LOG_WARN ("UE context not found, discarding packet");
    }
  else
    {
      std::map<uint8_t, uint32_t>::iterator bidIt = rntiIt->second.find (bid);
      NS_ASSERT (bidIt != rntiIt->second.end ());
      uint32_t teid = bidIt->second;
      m_rxLteSocketPktTrace (packet->Copy ());
      SendToS1uSocket (packet, teid);
    }
}

void 
EpcEnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);  
  NS_ASSERT (socket == m_s1uSocket);
  Ptr<Packet> packet = socket->Recv ();
  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);
  uint32_t teid = gtpu.GetTeid ();
  std::map<uint32_t, EpsFlowId_t>::iterator it = m_teidRbidMap.find (teid);//znr_note: 关键参数m_teidRbidMap
  if (it == m_teidRbidMap.end ())
    {
      //NS_LOG_WARN ("UE context at cell id " << m_cellId << " not found, discarding packet");//znr_com
      NS_LOG_INFO ("UE context at cell id " << m_cellId << " not found, discarding packet");//znr_add
    }
  else
    {
      m_rxS1uSocketPktTrace (packet->Copy ());
      Ptr<Packet> packet_leg = packet->Copy ();//znr_add
      SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);//znr_note: 运行cttc-nr-demo.cc，此处bid不是1，而是2，与之前调用UeManager::SetupDataRadioBearer有所不同，当时第1路是1，第2路是2；但因为相同rnti的lcid 3/4对应pdcp是1个，因此能匹配
      
      //znr-note: cttc-nr-demo.cc(20220603版本)属于单数据承载，因此屏蔽以及packet_leg
      //SendToLteSocket (packet_leg, it->second.m_rnti, it->second.m_bid+2);//znr_add:关键代码，适用cttc-nr-demo.cc(20220430版本)，解决nr gnb多数据承载问题，但是影响LTE，需要增加一个判断，或者修改m_teidRbidMap
    }
}

void 
EpcEnbApplication::SendToLteSocket (Ptr<Packet> packet, uint16_t rnti, uint8_t bid)
{
  NS_LOG_INFO (this << " EpcEnbApplication::SendToLteSocket");//znr_add
  NS_LOG_INFO ("               rnti = " << (uint16_t) rnti << " bid = "<< (uint16_t) bid);//znr_add
  
  NS_LOG_FUNCTION (this << packet << rnti << (uint16_t) bid << packet->GetSize ());  
  EpsBearerTag tag (rnti, bid);
  packet->AddPacketTag (tag);
  uint8_t ipType;

  packet->CopyData (&ipType, 1);
  ipType = (ipType>>4) & 0x0f;

  int sentBytes;
  if (ipType == 0x04)
    {
      sentBytes = m_lteSocket->Send (packet);
    }
  else if (ipType == 0x06)
    {
      sentBytes = m_lteSocket6->Send (packet);
    }
  else
    {
      NS_ABORT_MSG ("EpcEnbApplication::SendToLteSocket - Unknown IP type...");
    }

  NS_ASSERT (sentBytes > 0);
}


void 
EpcEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());  
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  // From 3GPP TS 29.281 v10.0.0 Section 5.1
  // Length of the payload + the non obligatory GTP-U header
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);  
  packet->AddHeader (gtpu);
  uint32_t flags = 0;
  m_s1uSocket->SendTo (packet, flags, InetSocketAddress (m_sgwS1uAddress, m_gtpuUdpPort));
}

void
EpcEnbApplication::DoReleaseIndication (uint64_t imsi, uint16_t rnti, uint8_t bearerId)
{
  NS_LOG_FUNCTION (this << bearerId );
  std::list<EpcS1apSapMme::ErabToBeReleasedIndication> erabToBeReleaseIndication;
  EpcS1apSapMme::ErabToBeReleasedIndication erab;
  erab.erabId = bearerId;
  erabToBeReleaseIndication.push_back (erab);
  //From 3GPP TS 23401-950 Section 5.4.4.2, enB sends EPS bearer Identity in Bearer Release Indication message to MME
  m_s1apSapMme->ErabReleaseIndication (imsi, rnti, erabToBeReleaseIndication);
}

}  // namespace ns3

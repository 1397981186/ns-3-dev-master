/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/lte-pdcp.h"
#include "ns3/lte-pdcp-header.h"
#include "ns3/lte-pdcp-sap.h"
#include "ns3/lte-pdcp-tag.h"
#include "ns3/lte-rlc-sdu-status-tag.h"//zjh_add
#include "ns3/packet.h"//zjh_add

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LtePdcp");

/// LtePdcpSpecificLteRlcSapUser class
class LtePdcpSpecificLteRlcSapUser : public LteRlcSapUser
{
public:
  /**
   * Constructor
   *
   * \param pdcp PDCP
   */
  LtePdcpSpecificLteRlcSapUser (LtePdcp* pdcp);

  // Interface provided to lower RLC entity (implemented from LteRlcSapUser)
  virtual void ReceivePdcpPdu (Ptr<Packet> p);

private:
  LtePdcpSpecificLteRlcSapUser ();
  LtePdcp* m_pdcp; ///< the PDCP
};

LtePdcpSpecificLteRlcSapUser::LtePdcpSpecificLteRlcSapUser (LtePdcp* pdcp)
  : m_pdcp (pdcp)
{
}

LtePdcpSpecificLteRlcSapUser::LtePdcpSpecificLteRlcSapUser ()
{
}

void
LtePdcpSpecificLteRlcSapUser::ReceivePdcpPdu (Ptr<Packet> p)
{
  m_pdcp->DoReceivePdu (p);
}

///////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED (LtePdcp);

LtePdcp::LtePdcp ()
  : m_pdcpSapUser (0),
    m_rlcSapProvider (0),
    m_rlcSapProvider_leg (0),//zjh_add
    m_rnti (0),
    m_lcid (0),
    m_lcid_leg (10),//zjh_add: because ue' lcid_leg can't to set now, so maybe to set 4 for default
    m_txSequenceNumber (0),
    m_rxSequenceNumber (0),
    m_pdcpDuplication (true),//zjh_add
    m_NcEnable(false),
    m_CopyEnable(false)
{
  NS_LOG_FUNCTION (this);
  m_pdcpSapProvider = new LtePdcpSpecificLtePdcpSapProvider<LtePdcp> (this);
  m_rlcSapUser = new LtePdcpSpecificLteRlcSapUser (this);
  m_rlcSapUser_leg = new LtePdcpSpecificLteRlcSapUser (this);//zjh_add
  m_Nc=new NcControl();
  m_Copy=new CopyControl();
}

LtePdcp::~LtePdcp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LtePdcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LtePdcp")
    .SetParent<Object> ()
    .SetGroupName("Lte")
    .AddTraceSource ("TxPDU",
                     "PDU transmission notified to the RLC.",
                     MakeTraceSourceAccessor (&LtePdcp::m_txPdu),
                     "ns3::LtePdcp::PduTxTracedCallback")
    .AddTraceSource ("RxPDU",
                     "PDU received.",
                     MakeTraceSourceAccessor (&LtePdcp::m_rxPdu),
                     "ns3::LtePdcp::PduRxTracedCallback")
    //zjh_add---
    .AddTraceSource ("TxPDU_leg",
                   "PDU_leg transmission notified to the RLC.",
                   MakeTraceSourceAccessor (&LtePdcp::m_txPdu_leg),
                   "ns3::LtePdcp::PduTxTracedCallback")
    .AddTraceSource ("RxPDU_leg",
                   "PDU_leg received.",
                   MakeTraceSourceAccessor (&LtePdcp::m_rxPdu_leg),
                   "ns3::LtePdcp::PduRxTracedCallback")
    //---zjh_add

    .AddAttribute ("NcEnable",
                  "Nc if open",
                  BooleanValue (false),
                  MakeBooleanAccessor (&LtePdcp::m_NcEnable),
                  MakeBooleanChecker ())
    .AddAttribute ("CopyEnable",
                  "Copy if open",
                  BooleanValue (false),
                  MakeBooleanAccessor (&LtePdcp::m_CopyEnable),
                  MakeBooleanChecker ())
    ;
  return tid;
}

void
LtePdcp::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete (m_pdcpSapProvider);
  delete (m_rlcSapUser);
  delete (m_rlcSapUser_leg);//zjh_add
}


void
LtePdcp::SetRnti (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << (uint32_t) rnti);
  m_rnti = rnti;
}

void
LtePdcp::SetLcId (uint8_t lcId)
{
  //NS_LOG_FUNCTION (this << (uint32_t) lcId);//zjh_com

  NS_LOG_INFO (this << " LtePdcp::SetLcId " << (uint32_t) lcId);//zjh_add
  m_lcid = lcId;
}

//zjh_new:
void
LtePdcp::SetLcId_leg (uint8_t lcId)
{
  NS_LOG_INFO (this << " LtePdcp::SetLcId_leg " << (uint32_t) lcId);
  m_lcid_leg = lcId;
}

void
LtePdcp::SetLtePdcpSapUser (LtePdcpSapUser * s)
{
  NS_LOG_FUNCTION (this << s);
  m_pdcpSapUser = s;
}

//zjh_new:
void
LtePdcp::SetLtePdcpSapUser_leg (LtePdcpSapUser * s)
{
  NS_LOG_FUNCTION (this << s);
  m_pdcpSapUser_leg = s;
}

LtePdcpSapProvider*
LtePdcp::GetLtePdcpSapProvider ()
{
  NS_LOG_FUNCTION (this);
  return m_pdcpSapProvider;
}

void
LtePdcp::SetLteRlcSapProvider (LteRlcSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_rlcSapProvider = s;
}

//zjh_new:
void
LtePdcp::SetLteRlcSapProvider_leg (LteRlcSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_rlcSapProvider_leg = s;
}

LteRlcSapUser*
LtePdcp::GetLteRlcSapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_rlcSapUser;
}

//zjh_new:
LteRlcSapUser*
LtePdcp::GetLteRlcSapUser_leg ()
{
  NS_LOG_FUNCTION (this);
  return m_rlcSapUser_leg;
}

LtePdcp::Status 
LtePdcp::GetStatus ()
{
  Status s;
  s.txSn = m_txSequenceNumber;
  s.rxSn = m_rxSequenceNumber;
  return s;
}

void 
LtePdcp::SetStatus (Status s)
{
  m_txSequenceNumber = s.txSn;
  m_rxSequenceNumber = s.rxSn;
}

//zjh_new:
bool
LtePdcp::GetDuplication ()
{
  return m_pdcpDuplication;
}

//zjh_new:
void
LtePdcp::SetDuplication (bool onoff)
{
  m_pdcpDuplication = onoff;
}

////////////////////////////////////////



void
LtePdcp::DoTransmitPdcpSdu (LtePdcpSapProvider::TransmitPdcpSduParameters params)
{
//  NS_LOG_FUNCTION (this << m_rnti << static_cast <uint16_t> (m_lcid) << params.pdcpSdu->GetSize ());
  NS_LOG_DEBUG (this << " LtePdcp::DoTransmitPdcpSdu m_lcid = " << (uint16_t) m_lcid );//zjh_add
  Ptr<Packet> p = params.pdcpSdu;

  // Sender timestamp
  PdcpTag pdcpTag (Simulator::Now ());

  LtePdcpHeader pdcpHeader;
  pdcpHeader.SetSequenceNumber (m_txSequenceNumber);

  m_txSequenceNumber++;
  if (m_txSequenceNumber > m_maxPdcpSn)
    {
      m_txSequenceNumber = 0;
    }

  pdcpHeader.SetDcBit (LtePdcpHeader::DATA_PDU);

  NS_LOG_LOGIC ("PDCP header: " << pdcpHeader);
  p->AddHeader (pdcpHeader);
  p->AddByteTag (pdcpTag, 1, pdcpHeader.GetSerializedSize ());

  m_txPdu (m_rnti, m_lcid, p->GetSize ());//zjh_note:TracedCallback

  LteRlcSapProvider::TransmitPdcpPduParameters txParams;
  txParams.rnti = m_rnti;
  txParams.lcid = m_lcid;
  txParams.pdcpPdu = p;
  
  NS_LOG_INFO (" m_lcid = " << (uint16_t) m_lcid);//znr-add
  m_rlcSapProvider->TransmitPdcpPdu (txParams);

  /**
  //zjh_add:
  if (m_pdcpDuplication)
    {
      //Ptr<Packet> p_leg = p;//不能使用指针赋值，会出现LteRlcSduStatusTag错误
      Ptr<Packet> p_leg = p->Copy();

      LteRlcSduStatusTag tag;
      if (p_leg->RemovePacketTag (tag)==true)
      {
        NS_LOG_INFO (this << " RemovePacketTag right!");
      }
      else
      {
        NS_LOG_INFO (this << " RemovePacketTag wrong!");
      }
      //m_txPdu_leg (m_rnti, m_lcid_leg, p->GetSize ());//zjh_note:TracedCallback

      LteRlcSapProvider::TransmitPdcpPduParameters txParams_leg;
      txParams_leg.rnti = m_rnti;
      txParams_leg.lcid = m_lcid_leg;
      NS_LOG_INFO (" m_lcid_leg = " << (uint16_t) m_lcid_leg);//znr-add
      txParams_leg.pdcpPdu = p_leg;
      m_rlcSapProvider_leg->TransmitPdcpPdu (txParams_leg);
    }
    */
}

void
LtePdcp::DoTransmitPdcpSdu_leg (LtePdcpSapProvider::TransmitPdcpSduParameters params)
{

//  NS_LOG_FUNCTION (this << m_rnti << static_cast <uint16_t> (m_lcid) << params.pdcpSdu->GetSize ());
  NS_LOG_DEBUG (this << " m_lcid_leg = " << (uint16_t) m_lcid_leg);//zjh_add
  Ptr<Packet> p_leg = params.pdcpSdu;

  // Sender timestamp
  PdcpTag pdcpTag (Simulator::Now ());

  LtePdcpHeader pdcpHeader;
  pdcpHeader.SetSequenceNumber (m_txSequenceNumber);

  m_txSequenceNumber++;
  if (m_txSequenceNumber > m_maxPdcpSn)
    {
      m_txSequenceNumber = 0;
    }

  pdcpHeader.SetDcBit (LtePdcpHeader::DATA_PDU);

  NS_LOG_LOGIC ("PDCP header: " << pdcpHeader);
  p_leg->AddHeader (pdcpHeader);
  p_leg->AddByteTag (pdcpTag, 1, pdcpHeader.GetSerializedSize ());

  m_txPdu (m_rnti, m_lcid_leg, p_leg->GetSize ());//zjh_note:TracedCallback

  LteRlcSapProvider::TransmitPdcpPduParameters txParams;
  txParams.rnti = m_rnti;
  txParams.lcid = m_lcid_leg;
  txParams.pdcpPdu = p_leg;

  NS_LOG_INFO (" m_lcid = " << (uint16_t) m_lcid_leg);//znr-add
  m_rlcSapProvider_leg->TransmitPdcpPdu (txParams);
/**
  //zjh_add:
  if (m_pdcpDuplication)
    {
      //Ptr<Packet> p_leg = p;//不能使用指针赋值，会出现LteRlcSduStatusTag错误
      Ptr<Packet> p_leg = p->Copy();

      LteRlcSduStatusTag tag;
      if (p_leg->RemovePacketTag (tag)==true)
      {
        NS_LOG_INFO (this << " RemovePacketTag right!");
      }
      else
      {
        NS_LOG_INFO (this << " RemovePacketTag wrong!");
      }
      //m_txPdu_leg (m_rnti, m_lcid_leg, p->GetSize ());//zjh_note:TracedCallback

      LteRlcSapProvider::TransmitPdcpPduParameters txParams_leg;
      txParams_leg.rnti = m_rnti;
      txParams_leg.lcid = m_lcid_leg;
      NS_LOG_INFO (" m_lcid_leg = " << (uint16_t) m_lcid_leg);//znr-add
      txParams_leg.pdcpPdu = p_leg;
      m_rlcSapProvider_leg->TransmitPdcpPdu (txParams_leg);
    }
    */

}

void
LtePdcp::ToogleSend(LtePdcpSapProvider::TransmitPdcpSduParameters params){
  if(m_NcRlcToSend%2==0){
    DoTransmitPdcpSdu (params);
    m_NcRlcToSend++;
  }else if(m_NcRlcToSend%2==1){
    DoTransmitPdcpSdu_leg (params);
    m_NcRlcToSend++;
  }
}

void
LtePdcp::SingleSend(LtePdcpSapProvider::TransmitPdcpSduParameters params){
  DoTransmitPdcpSdu (params);
}


void
LtePdcp::TriggerDoTransmitPdcpSdu (LtePdcpSapProvider::TransmitPdcpSduParameters params)
{
  if(m_NcEnable){
//    if(params.pdcpSdu->GetSize()<100){
//      DoTransmitPdcpSdu (params);//never at here?
//    }else{
  //start Nc
      m_Nc->HelloWorld();
      params.pdcpSdu=m_Nc->SendSaveAndSetTime(params.pdcpSdu);
      params.NcArqAddTop=0;
      uint32_t Ncedsize=m_Nc->GetNcedSize();
//      m_Nc
      ToogleSend(params);
      if(Ncedsize==m_Nc->m_originalBlockSize){
//  if(((m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize)%2==0&&m_NcRlcToSend%2==1)
//      ||((m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize)%2==1&&m_NcRlcToSend%2==0)){
//    m_NcRlcToSend++;
//  }
        for (int var = 0; var < m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize; var++) {
          LtePdcpSapProvider::TransmitPdcpSduParameters paramsRe;
          paramsRe.lcid=params.lcid;
          paramsRe.rnti=params.rnti;
          paramsRe.pdcpSdu=m_Nc->MakeRedundancePacket();
          paramsRe.NcArqAddTop=0;
          Ncedsize++;
          if(var==m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize-1){
            Simulator::Schedule (MicroSeconds(10000),&LtePdcp::ToogleSend, this, paramsRe);
          }else{
            ToogleSend(paramsRe);
          }
          //Simulator::Schedule (m_statusReportTimerValuePdcpCopy,&LtePdcp::CopyExpireStatusReportTimer, this, ArqGroupNum,p,m_Copy);
        }
        if(Ncedsize==m_Nc->m_encodingBlockSize){
            m_Nc->m_groupnum++;
            m_NcRlcToSend=m_NcRlcToSend%2;
        }
//      }
    }
  }else if(m_CopyEnable){
      m_Copy->HelloWorld();
      params.pdcpSdu=m_Copy->SendSaveAndSetTime(params.pdcpSdu);
      params.NcArqAddTop=0;
      DoTransmitPdcpSdu (params);

      //sht change for single
      LtePdcpSapProvider::TransmitPdcpSduParameters paramsRe;
      paramsRe.lcid=params.lcid;
      paramsRe.rnti=params.rnti;
      paramsRe.pdcpSdu=m_Copy->MakeRedundancePacket();
      paramsRe.NcArqAddTop=0;
      DoTransmitPdcpSdu_leg (paramsRe);

      m_Copy->m_groupnum++;
  }else{
      DoTransmitPdcpSdu (params);
  }
}

void
LtePdcp::CopyArqReqSendOnce(uint64_t ArqGroupNum,Ptr<Packet> p,CopyControl* m_Copy){

  LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
  paramsArq.lcid=m_lcid;
  paramsArq.rnti=m_rnti;
  paramsArq.pdcpSdu=p->Copy();
  paramsArq.NcArqAddTop=0;
  DoTransmitPdcpSdu (paramsArq);
  auto it=m_Copy->m_ncDecodingBufferList.find(ArqGroupNum);
  it->second.num_statusReport ++;
  NS_LOG_DEBUG("CopyArqReqSendOnce group num is "<<ArqGroupNum<<" arq num is "<<(unsigned)it->second.num_statusReport<<" time "<<Simulator::Now ().GetNanoSeconds());
  it->second.m_statusReportTimer = Simulator::Schedule (m_statusReportTimerValuePdcpCopy,
                  &LtePdcp::CopyExpireStatusReportTimer, this, ArqGroupNum,p,m_Copy);
}

void
LtePdcp::CopyExpireStatusReportTimer (uint64_t ArqGroupNum,Ptr<Packet> p,CopyControl* m_Copy)
{
  auto it = m_Copy->m_ncDecodingBufferList.find(ArqGroupNum);

  if (it->second.num_statusReport<3)
  {
      NS_LOG_DEBUG("CopyExpireStatusReportTimer group num is "<<ArqGroupNum<<" arq num is "<<(unsigned)it->second.num_statusReport<<" time "<<Simulator::Now ().GetNanoSeconds());
      CopyArqReqSendOnce(ArqGroupNum,p,m_Copy);
  }
}

void
LtePdcp::NcArqReqSendOnce(uint64_t ArqGroupNum,Ptr<Packet> p,NcControl* m_Nc){

  LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
  paramsArq.lcid=m_lcid;
  paramsArq.rnti=m_rnti;
  paramsArq.pdcpSdu=p->Copy();
  paramsArq.NcArqAddTop=0;
  DoTransmitPdcpSdu (paramsArq);
  auto it=m_Nc->m_ncDecodingBufferList.find(ArqGroupNum);
  it->second.num_statusReport ++;
  NS_LOG_DEBUG("NcArqReqSendOnce group num is "<<ArqGroupNum<<" arq num is "<<(unsigned)it->second.num_statusReport<<" time "<<Simulator::Now ().GetNanoSeconds());
  it->second.m_statusReportTimer = Simulator::Schedule (m_statusReportTimerValuePdcpCopy,
                  &LtePdcp::CopyExpireStatusReportTimer, this, ArqGroupNum,p,m_Copy);
}

void
LtePdcp::NcExpireStatusReportTimer (uint64_t ArqGroupNum,Ptr<Packet> p,NcControl* m_Nc)
{
  auto it = m_Nc->m_ncDecodingBufferList.find(ArqGroupNum);

  if (it->second.num_statusReport<3)
  {
      NS_LOG_DEBUG("NcExpireStatusReportTimer group num is "<<ArqGroupNum<<" arq num is "<<(unsigned)it->second.num_statusReport<<" time "<<Simulator::Now ().GetNanoSeconds());
      NcArqReqSendOnce(ArqGroupNum,p,m_Nc);
  }
}

int
LtePdcp::TriggerRecvPdcpSdu(Ptr<Packet> p){
  /**
   *
   * Nc
   * Sht add
   *
   *
   */
  if(m_NcEnable){
    Ptr<Packet> Sdu=Create<Packet> ();
    Sdu=m_Nc->RecvAndSave(p);
    if(m_Nc->IfRecvArq()){
      std::vector<Ptr<Packet> > arqPackets;
      arqPackets=m_Nc->MakeNcArqSendPacket(Sdu);
      for(auto it=arqPackets.begin();it != arqPackets.end();it++){
        LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
        paramsArq.lcid=m_lcid;
        paramsArq.rnti=m_rnti;
        paramsArq.pdcpSdu=* it;
        paramsArq.NcArqAddTop=1;
        NS_LOG_DEBUG("---set NcArqAddTop = = 1");
        ToogleSend(paramsArq);
      //  DoTransmitPdcpSdu(paramsArq);
      }
    }else{
      if(m_Nc->IfTransmitSdu()){
        LtePdcpSapUser::ReceivePdcpSduParameters params;
        params.pdcpSdu = Sdu;
        params.rnti = m_rnti;
        params.lcid = m_lcid;
        m_pdcpSapUser->ReceivePdcpSdu (params);
            }
      if(m_Nc->IfDeocde()){
        std::vector <Ptr<Packet>> decodePackets;
        decodePackets=m_Nc->NcDecode();
        for(auto it =decodePackets.begin();it != decodePackets.end();it ++){
            Ptr<Packet> deocdeOne=*it;
            LtePdcpSapUser::ReceivePdcpSduParameters params;
            params.pdcpSdu = deocdeOne;
            params.rnti = m_rnti;
            params.lcid = m_lcid;
            m_pdcpSapUser->ReceivePdcpSdu (params);
        }
      }
      m_Nc->stopArqTimer();
      if(m_Nc->IfNcSendArq()){
        std::vector <Ptr<Packet>> ArqPackets;
        std::vector <uint64_t> ArqGroupNums;

        m_Nc->NcSendArqReq(ArqGroupNums,ArqPackets);

//    for(auto it=ArqPackets.begin();it != ArqPackets.end();it++){
//      LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
//      paramsArq.lcid=m_lcid;
//      paramsArq.rnti=m_rnti;
//      paramsArq.pdcpSdu=* it;
//      paramsArq.NcArqAddTop=0;
//      DoTransmitPdcpSdu (paramsArq);
//    }
        if(ArqPackets.size()!=0){
          for(uint32_t i=0;i<=ArqPackets.size()-1;i++){
              NcArqReqSendOnce(ArqGroupNums[i],ArqPackets[i],m_Nc);
          }
        }

      }
    }

   /**
    *
    * Copy
    *
    *
    */
  }else if(m_CopyEnable){
    Ptr<Packet> Sdu=Create<Packet> ();
    Sdu=m_Copy->RecvAndSave(p);
    if(m_Copy->m_drop){
        NS_LOG_DEBUG("remove header wrong ,drop ");
        return 0 ;}
    if(m_Copy->IfRecvArq()){
      std::vector<Ptr<Packet> > arqPackets;
      arqPackets=m_Copy->MakeCopyArqSendPacket(Sdu);

      if(arqPackets.size()!=0){

          for(auto it=arqPackets.begin();it != arqPackets.end();it++){
            LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
            paramsArq.lcid=m_lcid;
            paramsArq.rnti=m_rnti;
            paramsArq.pdcpSdu=* it;
            paramsArq.NcArqAddTop=1;
            NS_LOG_DEBUG("---set NcArqAddTop = = 1");


            LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq2;
            paramsArq2.lcid=m_lcid;
            paramsArq2.rnti=m_rnti;
            paramsArq2.pdcpSdu=paramsArq.pdcpSdu->Copy();
            paramsArq2.NcArqAddTop=1;
            NS_LOG_DEBUG("---set NcArqAddTop = = 1");

            DoTransmitPdcpSdu(paramsArq);
            DoTransmitPdcpSdu_leg(paramsArq2);
          }
      }
    }
//    NS_LOG_DEBUG ("here0");
    if(m_Copy->IfTransmitSdu()){
//        NS_LOG_DEBUG ("here1");
      LtePdcpSapUser::ReceivePdcpSduParameters params;
//      NS_LOG_DEBUG ("here2");
      params.pdcpSdu = Sdu;
//      NS_LOG_DEBUG ("here3");
      params.rnti = m_rnti;
      params.lcid = m_lcid;
//      NS_LOG_DEBUG ("here4");
      m_pdcpSapUser->ReceivePdcpSdu (params);
//      NS_LOG_DEBUG ("here5");
    }

    m_Copy->stopArqTimer();

    if(m_Copy->IfCopySendArq()){
      std::vector <Ptr<Packet>> ArqPackets;
      std::vector <uint64_t> ArqGroupNums;

      m_Copy->CopySendArqReq(ArqGroupNums,ArqPackets);

//    for(auto it=ArqPackets.begin();it != ArqPackets.end();it++){
//      LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
//      paramsArq.lcid=m_lcid;
//      paramsArq.rnti=m_rnti;
//      paramsArq.pdcpSdu=* it;
//      paramsArq.NcArqAddTop=0;
//      DoTransmitPdcpSdu (paramsArq);
//    }
      if(ArqPackets.size()!=0){
        for(uint32_t i=0;i<=ArqPackets.size()-1;i++){
            CopyArqReqSendOnce(ArqGroupNums[i],ArqPackets[i],m_Copy);
        }
    }
//      NS_LOG_DEBUG ("here3");
    }

  /**
   *
   * normal
   *
   */
  }else{
    LtePdcpSapUser::ReceivePdcpSduParameters params;
    params.pdcpSdu = p;
    params.rnti = m_rnti;
    params.lcid = m_lcid;
    m_pdcpSapUser->ReceivePdcpSdu (params);
  }
  return 1;

}

void
LtePdcp::DoReceivePdu (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("LtePdcp::DoReceivePdu "<<this
                << " m_rnti "<<m_rnti
                << " m_lcid "<<(uint32_t) m_lcid
                << " size "<<p->GetSize ()
                <<" time "<<Simulator::Now ().GetNanoSeconds());
//  if(p->GetSize ()!=852){
//  if(p->GetSize ()!=873&&p->GetSize ()!=24){
//      NS_LOG_DEBUG("not 542/563/24, drop");
//
//  }else{
      // Receiver timestamp
    PdcpTag pdcpTag;
    Time delay;
    p->FindFirstMatchingByteTag (pdcpTag);
    delay = Simulator::Now() - pdcpTag.GetSenderTimestamp ();
    m_rxPdu(m_rnti, m_lcid, p->GetSize (), delay.GetNanoSeconds ());

    uint32_t size;
    LtePdcpHeader pdcpHeader;
    size=p->RemoveHeader (pdcpHeader);//sht add ,but dont know this happen
    if(size!=0){

      NS_LOG_LOGIC ("PDCP header: " << pdcpHeader);

      m_rxSequenceNumber = pdcpHeader.GetSequenceNumber () + 1;
      if (m_rxSequenceNumber > m_maxPdcpSn)
      {
        m_rxSequenceNumber = 0;
      }

      TriggerRecvPdcpSdu(p);
    }else{
        NS_LOG_DEBUG("size0, drop");
      }
//}
}
/**
void
LtePdcp::DoReceivePdu (Ptr<Packet> p)
{
  NS_LOG_INFO (this << " LtePdcp::DoReceivePdu m_lcid = " << (uint16_t) m_lcid << " m_lcid_leg = " << (uint16_t) m_lcid_leg);//zjh_add
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ());
  
  // Receiver timestamp
  PdcpTag pdcpTag;
  Time delay;
  p->FindFirstMatchingByteTag (pdcpTag);
  delay = Simulator::Now() - pdcpTag.GetSenderTimestamp ();
  m_rxPdu(m_rnti, m_lcid, p->GetSize (), delay.GetNanoSeconds ());//zjh_com
  
  LtePdcpHeader pdcpHeader;
  p->RemoveHeader (pdcpHeader);
  NS_LOG_LOGIC ("PDCP header: " << pdcpHeader);
  //NS_LOG_INFO (" m_lcid/m_lcid_leg = " << (uint16_t) m_lcid_leg);//znr-add: 并不能真实反映接收数据的逻辑信道

  m_rxSequenceNumber = pdcpHeader.GetSequenceNumber () + 1;
  if (m_rxSequenceNumber > m_maxPdcpSn)
    {
      m_rxSequenceNumber = 0;
      NS_LOG_INFO ("PDCP PDU size: " << p->ToString ());
    }

  LtePdcpSapUser::ReceivePdcpSduParameters params;
  params.pdcpSdu = p;
  params.rnti = m_rnti;
  params.lcid = m_lcid;
  m_pdcpSapUser->ReceivePdcpSdu (params);
}
*/

} // namespace ns3

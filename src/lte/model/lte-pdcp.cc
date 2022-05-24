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
    m_rlcSapProvider2 (0),
    m_rnti (0),
    m_lcid (0),
    m_txSequenceNumber (0),
    m_rxSequenceNumber (0),
    m_NcEnable(false),
    m_CopyEnable(false)
{
  NS_LOG_FUNCTION (this);
  m_pdcpSapProvider = new LtePdcpSpecificLtePdcpSapProvider<LtePdcp> (this);
  m_rlcSapUser = new LtePdcpSpecificLteRlcSapUser (this);
//  m_rlcSapUser2 = new LtePdcpSpecificLteRlcSapUser (this);
  m_Nc=new NcControl();
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
  NS_LOG_FUNCTION (this << (uint32_t) lcId);
  m_lcid = lcId;
}

void
LtePdcp::SetLtePdcpSapUser (LtePdcpSapUser * s)
{
  NS_LOG_FUNCTION (this << s);
  m_pdcpSapUser = s;
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

//sht
void
LtePdcp::SetLteRlcSapProvider2 (LteRlcSapProvider * s)
{
  NS_LOG_FUNCTION (this << s);
  m_rlcSapProvider2 = s;
}

LteRlcSapUser*
LtePdcp::GetLteRlcSapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_rlcSapUser;
}

//LteRlcSapUser*
//LtePdcp::GetLteRlcSapUser2 ()
//{
//  NS_LOG_FUNCTION (this);
//  return m_rlcSapUser2;
//}

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

////////////////////////////////////////

void
LtePdcp::DoTransmitPdcpSdu (LtePdcpSapProvider::TransmitPdcpSduParameters params)
{
  NS_LOG_FUNCTION (this << m_rnti << static_cast <uint16_t> (m_lcid) << params.pdcpSdu->GetSize ());
  Ptr<Packet> p = params.pdcpSdu;
  NS_LOG_FUNCTION(this<<"p address"<<&p);
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

  m_txPdu (m_rnti, m_lcid, p->GetSize ());
//  m_txPdu (m_rnti, m_lcid+99, p->GetSize ());

  LteRlcSapProvider::TransmitPdcpPduParameters txParams;
  txParams.rnti = m_rnti;
  txParams.lcid = m_lcid;
  txParams.pdcpPdu = p;
  txParams.NcArqAddTop=params.NcArqAddTop;

  m_rlcSapProvider->TransmitPdcpPdu (txParams);

}

//sht
void
LtePdcp::DoTransmitPdcpSdu2 (LtePdcpSapProvider::TransmitPdcpSduParameters params)
{
  NS_LOG_FUNCTION (this << m_rnti << static_cast <uint16_t> (m_lcid) << params.pdcpSdu->GetSize ());
  Ptr<Packet> p = params.pdcpSdu;
  NS_LOG_FUNCTION(this<<"p address"<<&p);
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

  m_txPdu (m_rnti, m_lcid, p->GetSize ());

  LteRlcSapProvider::TransmitPdcpPduParameters txParams;
  txParams.rnti = m_rnti;
  txParams.lcid = m_lcid;
  txParams.pdcpPdu = p;
  txParams.NcArqAddTop=params.NcArqAddTop;

  m_rlcSapProvider2->TransmitPdcpPdu2 (txParams);
}

//Nc main process
/**
 * m_Nc->HelloWorld();
 * m_Nc->SaveAndSetTime(params.pdcpSdu);
 * if > 10
 *
 *
 *
 *
 *
 */
void
LtePdcp::ToogleSend(LtePdcpSapProvider::TransmitPdcpSduParameters params){
  if(m_NcRlcToSend%2==0){
	DoTransmitPdcpSdu (params);
	m_NcRlcToSend++;
  }else if(m_NcRlcToSend%2==1){
	DoTransmitPdcpSdu2 (params);
	m_NcRlcToSend++;
  }
}

void
LtePdcp::TriggerDoTransmitPdcpSdu (LtePdcpSapProvider::TransmitPdcpSduParameters params)
{
  if(m_NcEnable){
    if(params.pdcpSdu->GetSize()<100){
      DoTransmitPdcpSdu (params);//never at here?
    }else{
	//start Nc
      m_Nc->HelloWorld();
      params.pdcpSdu=m_Nc->SendSaveAndSetTime(params.pdcpSdu);
      params.NcArqAddTop=0;
      uint32_t Ncedsize=m_Nc->GetNcedSize();
//      m_Nc
      ToogleSend(params);
      if(Ncedsize==m_Nc->m_originalBlockSize){
//	if(((m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize)%2==0&&m_NcRlcToSend%2==1)
//	    ||((m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize)%2==1&&m_NcRlcToSend%2==0)){
//	  m_NcRlcToSend++;
//	}
	for (int var = 0; var < m_Nc->m_encodingBlockSize-m_Nc->m_originalBlockSize; var++) {
	  LtePdcpSapProvider::TransmitPdcpSduParameters paramsRe;
	  paramsRe.lcid=params.lcid;
	  paramsRe.rnti=params.rnti;
	  paramsRe.pdcpSdu=m_Nc->MakeRedundancePacket();
	  paramsRe.NcArqAddTop=0;
	  Ncedsize++;
	  ToogleSend(paramsRe);
	}
	if(Ncedsize==m_Nc->m_encodingBlockSize){
	    m_Nc->m_groupnum++;
	    m_NcRlcToSend=m_NcRlcToSend%2;
	}
      }
    }
  }else if(m_CopyEnable){
    LtePdcpSapProvider::TransmitPdcpSduParameters params2;

    Ptr<Packet> pdcpSdu=Create<Packet> ();
    *pdcpSdu=*params.pdcpSdu;

    params2.lcid=params.lcid;
    params2.rnti=params.rnti;
    params2.pdcpSdu=pdcpSdu;


    DoTransmitPdcpSdu (params);
    DoTransmitPdcpSdu2 (params2);
  }else{
    DoTransmitPdcpSdu (params);
  }
}

void
LtePdcp::TriggerRecvPdcpSdu(Ptr<Packet> p){
  if(m_NcEnable&&p->GetSize()>100){
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
//	DoTransmitPdcpSdu(paramsArq);
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
	  ArqPackets=m_Nc->NcSendArqReq();
	  for(auto it=ArqPackets.begin();it != ArqPackets.end();it++){
	    LtePdcpSapProvider::TransmitPdcpSduParameters paramsArq;
	    paramsArq.lcid=m_lcid;
	    paramsArq.rnti=m_rnti;
	    paramsArq.pdcpSdu=* it;
	    paramsArq.NcArqAddTop=0;
	    DoTransmitPdcpSdu (paramsArq);

	  }
      }
    }
  }else if(m_CopyEnable){


  }else{
    LtePdcpSapUser::ReceivePdcpSduParameters params;
    params.pdcpSdu = p;
    params.rnti = m_rnti;
    params.lcid = m_lcid;
    m_pdcpSapUser->ReceivePdcpSdu (params);
  }

}

void
LtePdcp::DoReceivePdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ());
  if(p->GetSize ()!=542&&p->GetSize ()!=563&&p->GetSize ()!=125){
      NS_LOG_DEBUG("not 542/563/125, drop");

  }else{
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
    }
}



void
LtePdcp::NcPdcpARQ (Ptr<Packet> ARQp){
  NS_LOG_FUNCTION ("NcPdcpARQ");
  Ptr<Packet> p = ARQp->Copy();
  NS_LOG_FUNCTION(this<<"p address"<<&p);
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

  m_txPdu (m_rnti, m_lcid, p->GetSize ());

  LteRlcSapProvider::TransmitPdcpPduParameters txParams;
  txParams.rnti = m_rnti;
  txParams.lcid = m_lcid;
  txParams.pdcpPdu = p;

  m_rlcSapProvider->TransmitPdcpPdu (txParams);
}


} // namespace ns3

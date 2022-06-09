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
    m_pdcpDuplication (true)//zjh_add
{
  NS_LOG_FUNCTION (this);
  m_pdcpSapProvider = new LtePdcpSpecificLtePdcpSapProvider<LtePdcp> (this);
  m_rlcSapUser = new LtePdcpSpecificLteRlcSapUser (this);
  m_rlcSapUser_leg = new LtePdcpSpecificLteRlcSapUser (this);//zjh_add
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
  NS_LOG_FUNCTION (this << m_rnti << static_cast <uint16_t> (m_lcid) << params.pdcpSdu->GetSize ());
  NS_LOG_INFO (this << " LtePdcp::DoTransmitPdcpSdu m_lcid = " << (uint16_t) m_lcid << " m_lcid_leg = " << (uint16_t) m_lcid_leg);//zjh_add
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
}

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


} // namespace ns3

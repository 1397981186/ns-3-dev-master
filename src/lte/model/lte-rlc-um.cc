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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/simulator.h"
#include "ns3/log.h"

#include "ns3/lte-rlc-header.h"
#include "ns3/lte-rlc-um.h"
#include "ns3/lte-rlc-sdu-status-tag.h"
#include "ns3/lte-rlc-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteRlcUm");

NS_OBJECT_ENSURE_REGISTERED (LteRlcUm);

LteRlcUm::LteRlcUm ()
  : m_maxTxBufferSize (10 * 1024*2),
    m_txBufferSize (0),
    m_sequenceNumber (0),
    m_vrUr (0),
    m_vrUx (0),
    m_vrUh (0),
    m_windowSize (512),
    m_expectedSeqNumber (0)
{
  NS_LOG_FUNCTION (this);
  m_reassemblingState = WAITING_S0_FULL;
}

LteRlcUm::~LteRlcUm ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LteRlcUm::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteRlcUm")
    .SetParent<LteRlc> ()
    .SetGroupName("Lte")
    .AddConstructor<LteRlcUm> ()
    .AddAttribute ("MaxTxBufferSize",
                   "Maximum Size of the Transmission Buffer (in Bytes)",
                   UintegerValue (10 * 1024*2),
                   MakeUintegerAccessor (&LteRlcUm::m_maxTxBufferSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ReorderingTimer",
                   "Value of the t-Reordering timer (See section 7.3 of 3GPP TS 36.322)",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&LteRlcUm::m_reorderingTimerValue),
                   MakeTimeChecker ())
    ;
  return tid;
}

void
LteRlcUm::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_reorderingTimer.Cancel ();
  m_rbsTimer.Cancel ();

  LteRlc::DoDispose ();
}

/**
 * RLC SAP
 */

void
LteRlcUm::DoTransmitPdcpPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ()<<" time "<<Simulator::Now ().GetNanoSeconds());

  if (m_txBufferSize + p->GetSize () <= m_maxTxBufferSize)
    {
      /** Store PDCP PDU */
      LteRlcSduStatusTag tag;
      tag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
      p->AddPacketTag (tag);

      NS_LOG_LOGIC ("Tx Buffer: New packet added");
      if(m_NcArqAddTop==1){
	auto it =m_txBuffer.begin();
	if(m_txBufferSize!=0){it++;}
	m_txBuffer.insert(it,TxPdu (p, Simulator::Now ()));
	NS_LOG_DEBUG("ADD to queue head");
      }else{
	m_txBuffer.push_back (TxPdu (p, Simulator::Now ()));
      }
      m_txBufferSize += p->GetSize ();
      NS_LOG_LOGIC ("NumOfBuffers = " << m_txBuffer.size() );
      NS_LOG_LOGIC ("txBufferSize = " << m_txBufferSize);
    }
  else
    {
      // Discard full RLC SDU
      NS_LOG_LOGIC ("TxBuffer is full. RLC SDU discarded");
      NS_LOG_LOGIC ("MaxTxBufferSize = " << m_maxTxBufferSize);
      NS_LOG_LOGIC ("txBufferSize    = " << m_txBufferSize);
      NS_LOG_LOGIC ("packet size     = " << p->GetSize ());
      m_txDropTrace (p);
    }

  /** Report Buffer Status */
  DoReportBufferStatus ();
  m_rbsTimer.Cancel ();
}

void
LteRlcUm::DoTransmitPdcpPdu2 (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ()<<" time "<<Simulator::Now ().GetNanoSeconds());

  if (m_txBufferSize + p->GetSize () <= m_maxTxBufferSize)
    {
      /** Store PDCP PDU */
      LteRlcSduStatusTag tag;
      tag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
      p->AddPacketTag (tag);

      NS_LOG_LOGIC ("Tx Buffer: New packet added");
      if(m_NcArqAddTop==1){
	auto it =m_txBuffer.begin();
	if(m_txBufferSize!=0){it++;}
	m_txBuffer.insert(it,TxPdu (p, Simulator::Now ()));
	NS_LOG_DEBUG("ADD to queue head");
      }else{
	m_txBuffer.push_back (TxPdu (p, Simulator::Now ()));
      }
      m_txBufferSize += p->GetSize ();
      NS_LOG_LOGIC ("NumOfBuffers = " << m_txBuffer.size() );
      NS_LOG_LOGIC ("txBufferSize = " << m_txBufferSize);
    }
  else
    {
      // Discard full RLC SDU
      NS_LOG_LOGIC ("TxBuffer is full. RLC SDU discarded");
      NS_LOG_LOGIC ("MaxTxBufferSize = " << m_maxTxBufferSize);
      NS_LOG_LOGIC ("txBufferSize    = " << m_txBufferSize);
      NS_LOG_LOGIC ("packet size     = " << p->GetSize ());
      m_txDropTrace (p);
    }

  /** Report Buffer Status */
  DoReportBufferStatus ();
  m_rbsTimer.Cancel ();
}


/**
 * MAC SAP
 */

void
LteRlcUm::DoNotifyTxOpportunity (LteMacSapUser::TxOpportunityParameters txOpParams)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << txOpParams.bytes);
  NS_LOG_DEBUG ("---UeRLC DoNotifyTxOpportunity "<<this <<" txOpParams.bytes "<< txOpParams.bytes<<" m_txBuffer size "<<m_txBuffer.size ()<<" time "<<Simulator::Now ().GetNanoSeconds());

  if (txOpParams.bytes <= 2)
    {
      // Stingy MAC: Header fix part is 2 bytes, we need more bytes for the data
      NS_LOG_LOGIC ("TX opportunity too small = " << txOpParams.bytes);
      return;
    }

  Ptr<Packet> packet = Create<Packet> ();
  LteRlcHeader rlcHeader;

  // Build Data field
  uint32_t nextSegmentSize = txOpParams.bytes - 2;
  uint32_t nextSegmentId = 1;
  uint32_t dataFieldTotalSize = 0;
  uint32_t dataFieldAddedSize = 0;
  std::vector < Ptr<Packet> > dataField;

  // Remove the first packet from the transmission buffer.
  // If only a segment of the packet is taken, then the remaining is given back later
  if ( m_txBuffer.size () == 0 )
    {
      NS_LOG_LOGIC ("No data pending");
      return;
    }

  Ptr<Packet> firstSegment = m_txBuffer.begin ()->m_pdu->Copy ();
  Time firstSegmentTime = m_txBuffer.begin ()->m_waitingSince;

  NS_LOG_LOGIC ("SDUs in TxBuffer  = " << m_txBuffer.size ());
  NS_LOG_LOGIC ("First SDU buffer  = " << firstSegment);
  NS_LOG_LOGIC ("First SDU size    = " << firstSegment->GetSize ());
  NS_LOG_LOGIC ("Next segment size = " << nextSegmentSize);
  NS_LOG_LOGIC ("Remove SDU from TxBuffer");
  m_txBufferSize -= firstSegment->GetSize ();
  NS_LOG_LOGIC ("txBufferSize      = " << m_txBufferSize );
  m_txBuffer.erase (m_txBuffer.begin ());
//  m_allSendPduNums++;

  while ( firstSegment && (firstSegment->GetSize () > 0) && (nextSegmentSize > 0) )
    {
      NS_LOG_LOGIC ("WHILE ( firstSegment && firstSegment->GetSize > 0 && nextSegmentSize > 0 )");
      NS_LOG_LOGIC ("    firstSegment size = " << firstSegment->GetSize ());
      NS_LOG_LOGIC ("    nextSegmentSize   = " << nextSegmentSize);
      if ( (firstSegment->GetSize () > nextSegmentSize) ||
           // Segment larger than 2047 octets can only be mapped to the end of the Data field
           (firstSegment->GetSize () > 2047)
         )
        {
          // Take the minimum size, due to the 2047-bytes 3GPP exception
          // This exception is due to the length of the LI field (just 11 bits)
          uint32_t currSegmentSize = std::min (firstSegment->GetSize (), nextSegmentSize);

          NS_LOG_LOGIC ("    IF ( firstSegment > nextSegmentSize ||");
          NS_LOG_LOGIC ("         firstSegment > 2047 )");

          // Segment txBuffer.FirstBuffer and
          // Give back the remaining segment to the transmission buffer
          Ptr<Packet> newSegment = firstSegment->CreateFragment (0, currSegmentSize);
          NS_LOG_LOGIC ("    newSegment size   = " << newSegment->GetSize ());

          // Status tag of the new and remaining segments
          // Note: This is the only place where a PDU is segmented and
          // therefore its status can change
          LteRlcSduStatusTag oldTag, newTag;
          firstSegment->RemovePacketTag (oldTag);
          newSegment->RemovePacketTag (newTag);
          if (oldTag.GetStatus () == LteRlcSduStatusTag::FULL_SDU)
            {
              newTag.SetStatus (LteRlcSduStatusTag::FIRST_SEGMENT);
              oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }
          else if (oldTag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT)
            {
              newTag.SetStatus (LteRlcSduStatusTag::MIDDLE_SEGMENT);
              //oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }

          // Give back the remaining segment to the transmission buffer
          firstSegment->RemoveAtStart (currSegmentSize);
          NS_LOG_LOGIC ("    firstSegment size (after RemoveAtStart) = " << firstSegment->GetSize ());
          if (firstSegment->GetSize () > 0)
            {
              firstSegment->AddPacketTag (oldTag);

              m_txBuffer.insert (m_txBuffer.begin (), TxPdu (firstSegment, firstSegmentTime));
//              m_allSendPduNums--;
              m_txBufferSize += m_txBuffer.begin()->m_pdu->GetSize ();

              NS_LOG_LOGIC ("    TX buffer: Give back the remaining segment");
              NS_LOG_LOGIC ("    TX buffers = " << m_txBuffer.size ());
              NS_LOG_LOGIC ("    Front buffer size = " << m_txBuffer.begin()->m_pdu->GetSize ());
              NS_LOG_LOGIC ("    txBufferSize = " << m_txBufferSize );
            }
          else
            {
              // Whole segment was taken, so adjust tag
              if (newTag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
                }
              else if (newTag.GetStatus () == LteRlcSduStatusTag::MIDDLE_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
                }
            }
          // Segment is completely taken or
          // the remaining segment is given back to the transmission buffer
          firstSegment = 0;

          // Put status tag once it has been adjusted
          newSegment->AddPacketTag (newTag);

          // Add Segment to Data field
          dataFieldAddedSize = newSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (newSegment);
          newSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          // nextSegmentSize MUST be zero (only if segment is smaller or equal to 2047)

          // (NO more segments) → exit
          // break;
        }
      else if ( (nextSegmentSize - firstSegment->GetSize () <= 2) || (m_txBuffer.size () == 0) )
        {
          NS_LOG_LOGIC ("    IF nextSegmentSize - firstSegment->GetSize () <= 2 || txBuffer.size == 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);
          firstSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
              NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);

          // nextSegmentSize <= 2 (only if txBuffer is not empty)

          // (NO more segments) → exit
          // break;
        }
      else // (firstSegment->GetSize () < m_nextSegmentSize) && (m_txBuffer.size () > 0)
        {
          NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);

          // ExtensionBit (Next_Segment - 1) = 1
          rlcHeader.PushExtensionBit (LteRlcHeader::E_LI_FIELDS_FOLLOWS);

          // LengthIndicator (Next_Segment)  = txBuffer.FirstBuffer.length()
          rlcHeader.PushLengthIndicator (firstSegment->GetSize ());

          nextSegmentSize -= ((nextSegmentId % 2) ? (2) : (1)) + dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
              NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
          NS_LOG_LOGIC ("        Remove SDU from TxBuffer");

          // (more segments)
          firstSegment = m_txBuffer.begin ()->m_pdu->Copy ();
          firstSegmentTime = m_txBuffer.begin ()->m_waitingSince;
          m_txBufferSize -= firstSegment->GetSize ();
          m_txBuffer.erase (m_txBuffer.begin ());
          NS_LOG_LOGIC ("        txBufferSize = " << m_txBufferSize );
        }

    }

  // Build RLC header
  rlcHeader.SetSequenceNumber (m_sequenceNumber++);

  // Build RLC PDU with DataField and Header
  std::vector< Ptr<Packet> >::iterator it;
  it = dataField.begin ();

  uint8_t framingInfo = 0;

  // FIRST SEGMENT
  LteRlcSduStatusTag tag;
  NS_ASSERT_MSG ((*it)->PeekPacketTag (tag), "LteRlcSduStatusTag is missing");
  (*it)->PeekPacketTag (tag);
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT) )
    {
      framingInfo |= LteRlcHeader::FIRST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcHeader::NO_FIRST_BYTE;
    }

  while (it < dataField.end ())
    {
      NS_LOG_LOGIC ("Adding SDU/segment to packet, length = " << (*it)->GetSize ());

      NS_ASSERT_MSG ((*it)->PeekPacketTag (tag), "LteRlcSduStatusTag is missing");
      (*it)->RemovePacketTag (tag);
      if (packet->GetSize () > 0)
        {
          packet->AddAtEnd (*it);
        }
      else
        {
          packet = (*it);
        }
      it++;
    }

  // LAST SEGMENT (Note: There could be only one and be the first one)
  it--;
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT) )
    {
      framingInfo |= LteRlcHeader::LAST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcHeader::NO_LAST_BYTE;
    }

  rlcHeader.SetFramingInfo (framingInfo);

  NS_LOG_LOGIC ("RLC header: " << rlcHeader);
  packet->AddHeader (rlcHeader);

  // Sender timestamp
  RlcTag rlcTag (Simulator::Now ());
  packet->AddByteTag (rlcTag, 1, rlcHeader.GetSerializedSize ());
  m_txPdu (m_rnti, m_lcid, packet->GetSize ());
//  m_txPdu (m_rnti, m_lcid+99, packet->GetSize ());
  // Send RLC PDU to MAC layer
  LteMacSapProvider::TransmitPduParameters params;
  params.pdu = packet;
  params.rnti = m_rnti;
  params.lcid = m_lcid;
  params.layer = txOpParams.layer;
  params.harqProcessId = txOpParams.harqId;
  params.componentCarrierId = txOpParams.componentCarrierId;
  params.m_signOfRlc=txOpParams.m_signOfRlc;

  m_macSapProvider->TransmitPdu (params);

  if (! m_txBuffer.empty ())
    {
      m_rbsTimer.Cancel ();
      m_rbsTimer = Simulator::Schedule (MilliSeconds (10), &LteRlcUm::ExpireRbsTimer, this);
    }
}
//sht
/**
uint32_t
LteRlcUm::DoNotifyTxOpportunity (LteMacSapUser::TxOpportunityParameters txOpParams,uint32_t flag)
{
  NS_LOG_DEBUG ("--RLC DoNotifyTxOpportunity "<<this <<" m_rnti "<<m_rnti<<" m_lcid " << (unsigned int) m_lcid <<" txOpParams.bytes "<< txOpParams.bytes<<" m_txBuffer size "<<m_txBuffer.size ()<<" time "<<Simulator::Now ().GetNanoSeconds());
  uint32_t remain=0;
  m_toogleFlagRlc=false;
  m_noDataFlagRlc=false;

  if (txOpParams.bytes <= 4)
    {
      // Stingy MAC: Header fix part is 2 bytes, we need more bytes for the data
      NS_LOG_DEBUG ("TX opportunity too small = " << txOpParams.bytes);
      if(flag==0){
	remain=0;
	m_toogleFlagRlc=false;//sht add for rlc2 source use
	return remain;
      }else{
	remain=2;
	m_toogleFlagRlc=false;//sht add for rlc2 source use
	return remain;//do nothing remain state
      }
    }

  Ptr<Packet> packet = Create<Packet> ();
  LteRlcHeader rlcHeader;

  // Build Data field
  uint32_t nextSegmentSize = txOpParams.bytes - 2;
  uint32_t nextSegmentId = 1;
  uint32_t dataFieldTotalSize = 0;
  uint32_t dataFieldAddedSize = 0;
  std::vector < Ptr<Packet> > dataField;

  // Remove the first packet from the transmission buffer.
  // If only a segment of the packet is taken, then the remaining is given back later
  if ( m_txBuffer.size () == 0 )
    {
      if(flag==0){
	remain=txOpParams.bytes+4;//gives to rlc2
	m_toogleFlagRlc=true;//sht add for rlc2 source use
	m_noDataFlagRlc=true;
	NS_LOG_DEBUG ("No data pending,give to rlc2 ,remain is "<<remain<<" Nums Of transed Pdu "<<m_allSendPduNums);
	return remain;
      }else{
	remain=txOpParams.bytes+4;
	m_toogleFlagRlc=true;//sht add for rlc2 source use
	m_noDataFlagRlc=true;
	NS_LOG_DEBUG ("No data pending,give to rlc1 , remain is "<<remain<<" Nums Of transed Pdu "<<m_allSendPduNums);
	return remain;
      }
    }

  Ptr<Packet> firstSegment = m_txBuffer.begin ()->m_pdu->Copy ();
  Time firstSegmentTime = m_txBuffer.begin ()->m_waitingSince;

  NS_LOG_LOGIC ("SDUs in TxBuffer  = " << m_txBuffer.size ());
  NS_LOG_LOGIC ("First SDU buffer  = " << firstSegment);
  NS_LOG_LOGIC ("First SDU size    = " << firstSegment->GetSize ());
  NS_LOG_LOGIC ("Next segment size = " << nextSegmentSize);
  NS_LOG_LOGIC ("Remove SDU from TxBuffer");
  m_txBufferSize -= firstSegment->GetSize ();
  NS_LOG_LOGIC ("txBufferSize      = " << m_txBufferSize );
  m_txBuffer.erase (m_txBuffer.begin ());
  m_allSendPduNums++;

  while ( firstSegment && (firstSegment->GetSize () > 0) && (nextSegmentSize > 0) )//resource is not enough ,remain=0
    {
      NS_LOG_LOGIC ("WHILE ( firstSegment && firstSegment->GetSize > 0 && nextSegmentSize > 0 )");
      NS_LOG_LOGIC ("    firstSegment size = " << firstSegment->GetSize ());
      NS_LOG_LOGIC ("    nextSegmentSize   = " << nextSegmentSize);
      if ( (firstSegment->GetSize () > nextSegmentSize) ||
           // Segment larger than 2047 octets can only be mapped to the end of the Data field
           (firstSegment->GetSize () > 2047)
         )
        {
          // Take the minimum size, due to the 2047-bytes 3GPP exception
          // This exception is due to the length of the LI field (just 11 bits)
          uint32_t currSegmentSize = std::min (firstSegment->GetSize (), nextSegmentSize);

          NS_LOG_LOGIC ("    IF ( firstSegment > nextSegmentSize ||");
          NS_LOG_LOGIC ("         firstSegment > 2047 )");

          // Segment txBuffer.FirstBuffer and
          // Give back the remaining segment to the transmission buffer
          Ptr<Packet> newSegment = firstSegment->CreateFragment (0, currSegmentSize);
          NS_LOG_LOGIC ("    newSegment size   = " << newSegment->GetSize ());

          // Status tag of the new and remaining segments
          // Note: This is the only place where a PDU is segmented and
          // therefore its status can change
          LteRlcSduStatusTag oldTag, newTag;
          firstSegment->RemovePacketTag (oldTag);
          newSegment->RemovePacketTag (newTag);
          if (oldTag.GetStatus () == LteRlcSduStatusTag::FULL_SDU)
            {
              newTag.SetStatus (LteRlcSduStatusTag::FIRST_SEGMENT);
              oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }
          else if (oldTag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT)
            {
              newTag.SetStatus (LteRlcSduStatusTag::MIDDLE_SEGMENT);
              //oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }

          // Give back the remaining segment to the transmission buffer
          firstSegment->RemoveAtStart (currSegmentSize);
          NS_LOG_LOGIC ("    firstSegment size (after RemoveAtStart) = " << firstSegment->GetSize ());
          if (firstSegment->GetSize () > 0)
            {
              firstSegment->AddPacketTag (oldTag);

              m_txBuffer.insert (m_txBuffer.begin (), TxPdu (firstSegment, firstSegmentTime));
              m_allSendPduNums--;
              m_txBufferSize += m_txBuffer.begin()->m_pdu->GetSize ();

              NS_LOG_LOGIC ("    TX buffer: Give back the remaining segment");
              NS_LOG_LOGIC ("    TX buffers = " << m_txBuffer.size ());
              NS_LOG_LOGIC ("    Front buffer size = " << m_txBuffer.begin()->m_pdu->GetSize ());
              NS_LOG_LOGIC ("    txBufferSize = " << m_txBufferSize );
            }
          else
            {
              // Whole segment was taken, so adjust tag
              if (newTag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
                }
              else if (newTag.GetStatus () == LteRlcSduStatusTag::MIDDLE_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
                }
            }
          // Segment is completely taken or
          // the remaining segment is given back to the transmission buffer
          firstSegment = 0;

          // Put status tag once it has been adjusted
          newSegment->AddPacketTag (newTag);

          // Add Segment to Data field
          dataFieldAddedSize = newSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (newSegment);
          newSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          // nextSegmentSize MUST be zero (only if segment is smaller or equal to 2047)

          // (NO more segments) → exit
          // break;
          if(flag==2){
            remain=2;//give a not 0 number
            m_toogleFlagRlc=false;//sht add for rlc2 source use
            NS_LOG_DEBUG ("---still not enough for RLC2"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
          }else{
	    remain=0;
	    m_toogleFlagRlc=false;//sht add for rlc2 source use
	    NS_LOG_DEBUG ("---not enough for RLC1"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
          }
        }
      else if ( (nextSegmentSize - firstSegment->GetSize () <= 2) || (m_txBuffer.size () == 0) )//451-1022 1037-530
        {
          NS_LOG_LOGIC ("    IF nextSegmentSize - firstSegment->GetSize () <= 2 || txBuffer.size == 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);
          firstSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
              NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);

          // nextSegmentSize <= 2 (only if txBuffer is not empty)

          // (NO more segments) → exit
          // break;
          if(flag==0){
	    remain=nextSegmentSize;
	    if(remain<=4){
	      m_toogleFlagRlc=false;//sht add for rlc2 source use
	      remain=0;
	      NS_LOG_DEBUG ("---enough for RLC1 gives to rlc2 but too small(<4) to give"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
	    }else{
	      m_toogleFlagRlc=true;//sht add for rlc2 source use
	      NS_LOG_DEBUG ("---enough for RLC1 gives to rlc2"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
	    }
//	    m_toogleFlagRlc=true;//sht add for rlc2 source use
//	    NS_LOG_DEBUG ("---enough for RLC1 gives to rlc2"<<"    remain = " << remain );
          }else{
            remain=nextSegmentSize;
	    if(remain<=4){
	      m_toogleFlagRlc=false;//sht add for rlc2 source use
	      remain=0;
	      NS_LOG_DEBUG ("---enough for RLC2 gives to rlc1 but too small(4) to give"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
	    }else{
	      m_toogleFlagRlc=true;//sht add for rlc2 source use
	      NS_LOG_DEBUG ("---enough for RLC2 gives to rlc1 "<<"    remain = " << remain<<" Nums Of transed Pdu "<<m_allSendPduNums );
	    }
          }
        }
      else // (firstSegment->GetSize () < m_nextSegmentSize) && (m_txBuffer.size () > 0)
        {
          NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");//have source not use
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);
          firstSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
              NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
          if(flag==0){
	    remain=nextSegmentSize;
	    if(remain<=4){
	      m_toogleFlagRlc=false;//sht add for rlc2 source use
	      remain=0;
	      NS_LOG_DEBUG ("---source remain ,rlc1buffer have data but gives to rlc2, but too small(4) to give"<<"    remain = " << remain<<" Nums Of transed Pdu "<<m_allSendPduNums );
	    }else{
	      m_toogleFlagRlc=true;//sht add for rlc2 source use
	      NS_LOG_DEBUG ("---source remain ,rlc1buffer have data but gives to rlc2"<<"    remain = " << remain<<" Nums Of transed Pdu "<<m_allSendPduNums );
	    }
          }else{
  	    remain=nextSegmentSize;
	    if(remain<=4){
	      m_toogleFlagRlc=false;//sht add for rlc2 source use
	      remain=0;
	      NS_LOG_DEBUG ("---source remain ,rlc2buffer have data but gives to rlc1, but too small(4) to give"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
	    }else{
	      m_toogleFlagRlc=true;//sht add for rlc2 source use
	      NS_LOG_DEBUG ("---source remain ,rlc2buffer have data but gives to rlc1"<<"    remain = " << remain<<" Nums Of transed Pdu "<<m_allSendPduNums );
	    }
          }
	  break;
//          NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");//have source not use
//          // Add txBuffer.FirstBuffer to DataField
//          dataFieldAddedSize = firstSegment->GetSize ();
//          dataFieldTotalSize += dataFieldAddedSize;
//          dataField.push_back (firstSegment);
//
//          // ExtensionBit (Next_Segment - 1) = 1
//          rlcHeader.PushExtensionBit (LteRlcHeader::E_LI_FIELDS_FOLLOWS);
//
//          // LengthIndicator (Next_Segment)  = txBuffer.FirstBuffer.length()
//          rlcHeader.PushLengthIndicator (firstSegment->GetSize ());
//
//          nextSegmentSize -= ((nextSegmentId % 2) ? (2) : (1)) + dataFieldAddedSize;
//          nextSegmentId++;
//
//          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
//          if (m_txBuffer.size () > 0)
//            {
//              NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
//              NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
//            }
//          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
//          NS_LOG_LOGIC ("        Remove SDU from TxBuffer");
//
//          // (more segments)
//          firstSegment = m_txBuffer.begin ()->m_pdu->Copy ();
//          firstSegmentTime = m_txBuffer.begin ()->m_waitingSince;
//          m_txBufferSize -= firstSegment->GetSize ();
//          m_txBuffer.erase (m_txBuffer.begin ());
//          NS_LOG_LOGIC ("        txBufferSize = " << m_txBufferSize );
//          remain=nextSegmentSize;
//          NS_LOG_DEBUG ("---don't want this happen"<<"    remain = " << remain );
        }
//
//
    }

  // Build RLC header
  rlcHeader.SetSequenceNumber (m_sequenceNumber++);

  // Build RLC PDU with DataField and Header
  std::vector< Ptr<Packet> >::iterator it;
  it = dataField.begin ();

  uint8_t framingInfo = 0;

  // FIRST SEGMENT
  LteRlcSduStatusTag tag;
  NS_ASSERT_MSG ((*it)->PeekPacketTag (tag), "LteRlcSduStatusTag is missing");
  (*it)->PeekPacketTag (tag);
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT) )
    {
      framingInfo |= LteRlcHeader::FIRST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcHeader::NO_FIRST_BYTE;
    }

  while (it < dataField.end ())
    {
      NS_LOG_LOGIC ("Adding SDU/segment to packet, length = " << (*it)->GetSize ());

      NS_ASSERT_MSG ((*it)->PeekPacketTag (tag), "LteRlcSduStatusTag is missing");
      (*it)->RemovePacketTag (tag);
      if (packet->GetSize () > 0)
        {
          packet->AddAtEnd (*it);
        }
      else
        {
          packet = (*it);
        }
      it++;
    }

  // LAST SEGMENT (Note: There could be only one and be the first one)
  it--;
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT) )
    {
      framingInfo |= LteRlcHeader::LAST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcHeader::NO_LAST_BYTE;
    }

  rlcHeader.SetFramingInfo (framingInfo);

  NS_LOG_LOGIC ("RLC header: " << rlcHeader);
  packet->AddHeader (rlcHeader);

  // Sender timestamp
  RlcTag rlcTag (Simulator::Now ());
  packet->AddByteTag (rlcTag, 1, rlcHeader.GetSerializedSize ());
  m_txPdu (m_rnti, m_lcid, packet->GetSize ());
//  m_txPdu (m_rnti, m_lcid+99, packet->GetSize ());
  // Send RLC PDU to MAC layer
  LteMacSapProvider::TransmitPduParameters params;
  params.pdu = packet;
  params.rnti = m_rnti;
  params.lcid = m_lcid;
  params.layer = txOpParams.layer;
  params.harqProcessId = txOpParams.harqId;
  params.componentCarrierId = txOpParams.componentCarrierId;
  params.m_signOfRlc=txOpParams.m_signOfRlc;

  m_macSapProvider->TransmitPdu (params);

  if (! m_txBuffer.empty ())
    {
      m_rbsTimer.Cancel ();
      m_rbsTimer = Simulator::Schedule (MilliSeconds (10), &LteRlcUm::ExpireRbsTimer, this);
    }
  return remain;
}
**/

//sht--2
uint32_t
LteRlcUm::DoNotifyTxOpportunity (LteMacSapUser::TxOpportunityParameters txOpParams,uint32_t flag)
{
  NS_LOG_DEBUG ("--RLC DoNotifyTxOpportunity "<<this <<" m_rnti "<<m_rnti<<" m_lcid " << (unsigned int) m_lcid <<" txOpParams.bytes "<< txOpParams.bytes<<" m_txBuffer size "<<m_txBuffer.size ()<<" time "<<Simulator::Now ().GetNanoSeconds());
  uint32_t remain=0;
  m_toogleFlagRlc=false;
  m_noDataFlagRlc=false;

  if (txOpParams.bytes <= 4)
    {
      // Stingy MAC: Header fix part is 2 bytes, we need more bytes for the data
      NS_LOG_DEBUG ("TX opportunity too small = " << txOpParams.bytes<<" Nums Of transed Pdu "<<m_allSendPduNums);
      if(flag==0||flag==2){
	remain=0;
//	m_toogleFlagRlc=false;//sht add for rlc2 source use
	return remain;
      }else{
	remain=1;
//	m_toogleFlagRlc=false;//sht add for rlc2 source use
	return remain;//do nothing remain state
      }
    }

  Ptr<Packet> packet = Create<Packet> ();
  LteRlcHeader rlcHeader;

  // Build Data field
  uint32_t nextSegmentSize = txOpParams.bytes - 2;
  uint32_t nextSegmentId = 1;
  uint32_t dataFieldTotalSize = 0;
  uint32_t dataFieldAddedSize = 0;
  std::vector < Ptr<Packet> > dataField;

  // Remove the first packet from the transmission buffer.
  // If only a segment of the packet is taken, then the remaining is given back later
  if ( m_txBuffer.size () == 0 )
    {
      if(flag==0){
	remain=txOpParams.bytes+4;//gives to rlc2
//	m_toogleFlagRlc=true;//sht add for rlc2 source use
//	m_noDataFlagRlc=true;
	NS_LOG_DEBUG ("flag is 0 ,No data pending,give to rlc2 ,remain is "<<remain<<" Nums Of transed Pdu "<<m_allSendPduNums);
	return remain;
      }else if (flag==1){
	remain=txOpParams.bytes+4;
//	m_toogleFlagRlc=true;//sht add for rlc2 source use
//	m_noDataFlagRlc=true;
	NS_LOG_DEBUG ("flag is 1 ,No data pending,give to rlc1 , remain is "<<remain<<" Nums Of transed Pdu "<<m_allSendPduNums);
	return remain;
      }else if(flag==2){
	remain=txOpParams.bytes+4;
//	m_toogleFlagRlc=true;
	NS_LOG_DEBUG ("flag is 2 ,No data pending in rlc1   ,flag and all source gives to rlc2 and , remain is "<<remain<<" Nums Of transed Pdu "<<m_allSendPduNums);
	return remain;
      }else{
	remain=txOpParams.bytes+4;
//	m_toogleFlagRlc=true;
        NS_LOG_DEBUG ("flag is 3 ,No data pending in rlc2 ,flag and all source gives to rlc1 , remain is "<<remain<<" Nums Of transed Pdu "<<m_allSendPduNums);
        return remain;
      }
    }

  Ptr<Packet> firstSegment = m_txBuffer.begin ()->m_pdu->Copy ();
  Time firstSegmentTime = m_txBuffer.begin ()->m_waitingSince;

  NS_LOG_LOGIC ("SDUs in TxBuffer  = " << m_txBuffer.size ());
  NS_LOG_LOGIC ("First SDU buffer  = " << firstSegment);
  NS_LOG_LOGIC ("First SDU size    = " << firstSegment->GetSize ());
  NS_LOG_LOGIC ("Next segment size = " << nextSegmentSize);
  NS_LOG_LOGIC ("Remove SDU from TxBuffer");
  m_txBufferSize -= firstSegment->GetSize ();
  NS_LOG_LOGIC ("txBufferSize      = " << m_txBufferSize );
  m_txBuffer.erase (m_txBuffer.begin ());
  m_allSendPduNums++;

  while ( firstSegment && (firstSegment->GetSize () > 0) && (nextSegmentSize > 0) )//resource is not enough ,remain=0
    {
      NS_LOG_LOGIC ("WHILE ( firstSegment && firstSegment->GetSize > 0 && nextSegmentSize > 0 )");
      NS_LOG_LOGIC ("    firstSegment size = " << firstSegment->GetSize ());
      NS_LOG_LOGIC ("    nextSegmentSize   = " << nextSegmentSize);
      if ( (firstSegment->GetSize () > nextSegmentSize) ||
           // Segment larger than 2047 octets can only be mapped to the end of the Data field
           (firstSegment->GetSize () > 2047)
         )
        {
          // Take the minimum size, due to the 2047-bytes 3GPP exception
          // This exception is due to the length of the LI field (just 11 bits)
          uint32_t currSegmentSize = std::min (firstSegment->GetSize (), nextSegmentSize);

          NS_LOG_LOGIC ("    IF ( firstSegment > nextSegmentSize ||");
          NS_LOG_LOGIC ("         firstSegment > 2047 )");

          // Segment txBuffer.FirstBuffer and
          // Give back the remaining segment to the transmission buffer
          Ptr<Packet> newSegment = firstSegment->CreateFragment (0, currSegmentSize);
          NS_LOG_LOGIC ("    newSegment size   = " << newSegment->GetSize ());

          // Status tag of the new and remaining segments
          // Note: This is the only place where a PDU is segmented and
          // therefore its status can change
          LteRlcSduStatusTag oldTag, newTag;
          firstSegment->RemovePacketTag (oldTag);
          newSegment->RemovePacketTag (newTag);
          if (oldTag.GetStatus () == LteRlcSduStatusTag::FULL_SDU)
            {
              newTag.SetStatus (LteRlcSduStatusTag::FIRST_SEGMENT);
              oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }
          else if (oldTag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT)
            {
              newTag.SetStatus (LteRlcSduStatusTag::MIDDLE_SEGMENT);
              //oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }

          // Give back the remaining segment to the transmission buffer
          firstSegment->RemoveAtStart (currSegmentSize);
          NS_LOG_LOGIC ("    firstSegment size (after RemoveAtStart) = " << firstSegment->GetSize ());
          if (firstSegment->GetSize () > 0)
            {
              firstSegment->AddPacketTag (oldTag);

              m_txBuffer.insert (m_txBuffer.begin (), TxPdu (firstSegment, firstSegmentTime));
              m_allSendPduNums--;
              m_txBufferSize += m_txBuffer.begin()->m_pdu->GetSize ();

              NS_LOG_LOGIC ("    TX buffer: Give back the remaining segment");
              NS_LOG_LOGIC ("    TX buffers = " << m_txBuffer.size ());
              NS_LOG_LOGIC ("    Front buffer size = " << m_txBuffer.begin()->m_pdu->GetSize ());
              NS_LOG_LOGIC ("    txBufferSize = " << m_txBufferSize );
            }
          else
            {
              // Whole segment was taken, so adjust tag
              if (newTag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
                }
              else if (newTag.GetStatus () == LteRlcSduStatusTag::MIDDLE_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
                }
            }
          // Segment is completely taken or
          // the remaining segment is given back to the transmission buffer
          firstSegment = 0;

          // Put status tag once it has been adjusted
          newSegment->AddPacketTag (newTag);

          // Add Segment to Data field
          dataFieldAddedSize = newSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (newSegment);
          newSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          // nextSegmentSize MUST be zero (only if segment is smaller or equal to 2047)

          // (NO more segments) → exit
          // break;
//          m_toogleFlagRlc=false;
          if(flag==0||flag==2){
	    remain=0;
	    NS_LOG_DEBUG ("---flag is 0,2 ,still not enough for RLC1"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
          }else if(flag==1||flag==3){
            remain=1;
	    NS_LOG_DEBUG ("---flag is 1,3 ,still not enough for RLC2"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
          }



        }
      else if ( (nextSegmentSize - firstSegment->GetSize () <= 2) || (m_txBuffer.size () == 0) )//451-1022 1037-530
//      only nextSegmentSize - firstSegment->GetSize () <= 0  || (m_txBuffer.size () == 0)
        {
          NS_LOG_LOGIC ("    IF nextSegmentSize - firstSegment->GetSize () <= 2 || txBuffer.size == 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);
          firstSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
          if (m_txBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
              NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);

          // nextSegmentSize <= 2 (only if txBuffer is not empty)

          // (NO more segments) → exit
          // break;
          remain=nextSegmentSize;

//          m_toogleFlagRlc=false;
          if(flag==0){
              if(remain<=4){
        	  remain=1;//once rlc0 then next is rlc2
        	  NS_LOG_DEBUG ("---flag is 0 ,enough for RLC1 gives to rlc2 but too small(<4) to give"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
              }else{
        	  NS_LOG_DEBUG ("---flag is 0 ,enough for RLC1 gives to rlc2"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
              }
          }else if(flag==1){
              if(remain<=4){
        	  remain=0;//once rlc2 then next is rlc0
        	  NS_LOG_DEBUG ("---flag is 1 ,enough for RLC2 gives to rlc1 but too small(<4) to give"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
              }else{
        	  NS_LOG_DEBUG ("---flag is 1 ,enough for RLC2 gives to rlc1"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
              }
          }else if(flag==2){
              remain=1;
              NS_LOG_DEBUG ("---flag is 2, enough for RLC1 gives to rlc0"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
          }else{
              remain=0;
              NS_LOG_DEBUG ("---flag is"<<flag <<" , enough for RLC2 gives to rlc0"<<"    remain = " << remain <<" Nums Of transed Pdu "<<m_allSendPduNums);
          }


        }
      else // (firstSegment->GetSize () < m_nextSegmentSize) && (m_txBuffer.size () > 0)
        {
          if(flag==0||flag==1){
              NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");//have source not use
              // Add txBuffer.FirstBuffer to DataField
              dataFieldAddedSize = firstSegment->GetSize ();
              dataFieldTotalSize += dataFieldAddedSize;
              dataField.push_back (firstSegment);
              firstSegment = 0;

              // ExtensionBit (Next_Segment - 1) = 0
              rlcHeader.PushExtensionBit (LteRlcHeader::DATA_FIELD_FOLLOWS);

              // no LengthIndicator for the last one

              nextSegmentSize -= dataFieldAddedSize;
              nextSegmentId++;

              NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
              if (m_txBuffer.size () > 0)
                {
                  NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
                  NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
                }
              NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
              remain=nextSegmentSize;
              NS_LOG_DEBUG ("---not finish in this "<<"    remain = " << remain );
              break;
          }else{
              NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");//have source not use
              // Add txBuffer.FirstBuffer to DataField
              dataFieldAddedSize = firstSegment->GetSize ();
              dataFieldTotalSize += dataFieldAddedSize;
              dataField.push_back (firstSegment);

              // ExtensionBit (Next_Segment - 1) = 1
              rlcHeader.PushExtensionBit (LteRlcHeader::E_LI_FIELDS_FOLLOWS);

              // LengthIndicator (Next_Segment)  = txBuffer.FirstBuffer.length()
              rlcHeader.PushLengthIndicator (firstSegment->GetSize ());

              nextSegmentSize -= ((nextSegmentId % 2) ? (2) : (1)) + dataFieldAddedSize;
              nextSegmentId++;

              NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txBuffer.size ());
              if (m_txBuffer.size () > 0)
                {
                  NS_LOG_LOGIC ("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
                  NS_LOG_LOGIC ("        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize ());
                }
              NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
              NS_LOG_LOGIC ("        Remove SDU from TxBuffer");

              // (more segments)
              firstSegment = m_txBuffer.begin ()->m_pdu->Copy ();
              firstSegmentTime = m_txBuffer.begin ()->m_waitingSince;
              m_txBufferSize -= firstSegment->GetSize ();
              m_txBuffer.erase (m_txBuffer.begin ());
              m_allSendPduNums++;
              NS_LOG_LOGIC ("        txBufferSize = " << m_txBufferSize );
              remain=nextSegmentSize;
              NS_LOG_DEBUG ("---not finish in this "<<"    remain = " << remain<<" Nums Of transed Pdu "<<m_allSendPduNums );
          }


        }
//
//
    }

  // Build RLC header
  rlcHeader.SetSequenceNumber (m_sequenceNumber++);

  // Build RLC PDU with DataField and Header
  std::vector< Ptr<Packet> >::iterator it;
  it = dataField.begin ();

  uint8_t framingInfo = 0;

  // FIRST SEGMENT
  LteRlcSduStatusTag tag;
  NS_ASSERT_MSG ((*it)->PeekPacketTag (tag), "LteRlcSduStatusTag is missing");
  (*it)->PeekPacketTag (tag);
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT) )
    {
      framingInfo |= LteRlcHeader::FIRST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcHeader::NO_FIRST_BYTE;
    }

  while (it < dataField.end ())
    {
      NS_LOG_LOGIC ("Adding SDU/segment to packet, length = " << (*it)->GetSize ());

      NS_ASSERT_MSG ((*it)->PeekPacketTag (tag), "LteRlcSduStatusTag is missing");
      (*it)->RemovePacketTag (tag);
      if (packet->GetSize () > 0)
        {
          packet->AddAtEnd (*it);
        }
      else
        {
          packet = (*it);
        }
      it++;
    }

  // LAST SEGMENT (Note: There could be only one and be the first one)
  it--;
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT) )
    {
      framingInfo |= LteRlcHeader::LAST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcHeader::NO_LAST_BYTE;
    }

  rlcHeader.SetFramingInfo (framingInfo);

  NS_LOG_LOGIC ("RLC header: " << rlcHeader);
  packet->AddHeader (rlcHeader);

  // Sender timestamp
  RlcTag rlcTag (Simulator::Now ());
  packet->AddByteTag (rlcTag, 1, rlcHeader.GetSerializedSize ());
  m_txPdu (m_rnti, m_lcid, packet->GetSize ());
//  m_txPdu (m_rnti, m_lcid+99, packet->GetSize ());
  // Send RLC PDU to MAC layer
  LteMacSapProvider::TransmitPduParameters params;
  params.pdu = packet;
  params.rnti = m_rnti;
  params.lcid = m_lcid;
  params.layer = txOpParams.layer;
  params.harqProcessId = txOpParams.harqId;
  params.componentCarrierId = txOpParams.componentCarrierId;
  params.m_signOfRlc=txOpParams.m_signOfRlc;

  m_macSapProvider->TransmitPdu (params);

  if (! m_txBuffer.empty ())
    {
      m_rbsTimer.Cancel ();
      m_rbsTimer = Simulator::Schedule (MilliSeconds (10), &LteRlcUm::ExpireRbsTimer, this);
    }
  return remain;
}



void
LteRlcUm::DoNotifyHarqDeliveryFailure ()
{
  NS_LOG_FUNCTION (this);
}

void
LteRlcUm::DoReceivePdu (LteMacSapUser::ReceivePduParameters rxPduParams)
{
  NS_LOG_DEBUG ("RLC DoReceivePdu "<<this << " m_rnti "<<m_rnti << " m_lcid "<<(uint32_t) m_lcid <<" rxPduParams.p->GetSize () " <<rxPduParams.p->GetSize ()<<" rxPduParams.lcid "<<rxPduParams.lcid<<" time "<<Simulator::Now ().GetNanoSeconds());
  if(rxPduParams.lcid>=99){
    rxPduParams.lcid=rxPduParams.lcid-99;
  }
  // Receiver timestamp
  RlcTag rlcTag;
  Time delay;

  bool ret = rxPduParams.p->FindFirstMatchingByteTag (rlcTag);
  NS_ASSERT_MSG (ret, "RlcTag is missing");

  delay = Simulator::Now() - rlcTag.GetSenderTimestamp ();
  m_rxPdu (m_rnti, m_lcid, rxPduParams.p->GetSize (), delay.GetNanoSeconds ());
//  m_rxPdu (m_rnti, m_lcid+99, rxPduParams.p->GetSize (), delay.GetNanoSeconds ());
  // 5.1.2.2 Receive operations

  // Get RLC header parameters
  LteRlcHeader rlcHeader;
  rxPduParams.p->PeekHeader (rlcHeader);
  NS_LOG_LOGIC ("RLC header: " << rlcHeader);
  SequenceNumber10 seqNumber = rlcHeader.GetSequenceNumber ();

  // 5.1.2.2.1 General
  // The receiving UM RLC entity shall maintain a reordering window according to state variable VR(UH) as follows:
  // - a SN falls within the reordering window if (VR(UH) - UM_Window_Size) <= SN < VR(UH);
  // - a SN falls outside of the reordering window otherwise.
  // When receiving an UMD PDU from lower layer, the receiving UM RLC entity shall:
  // - either discard the received UMD PDU or place it in the reception buffer (see sub clause 5.1.2.2.2);
  // - if the received UMD PDU was placed in the reception buffer:
  // - update state variables, reassemble and deliver RLC SDUs to upper layer and start/stop t-Reordering as needed (see sub clause 5.1.2.2.3);
  // When t-Reordering expires, the receiving UM RLC entity shall:
  // - update state variables, reassemble and deliver RLC SDUs to upper layer and start t-Reordering as needed (see sub clause 5.1.2.2.4).

  // 5.1.2.2.2 Actions when an UMD PDU is received from lower layer
  // When an UMD PDU with SN = x is received from lower layer, the receiving UM RLC entity shall:
  // - if VR(UR) < x < VR(UH) and the UMD PDU with SN = x has been received before; or
  // - if (VR(UH) - UM_Window_Size) <= x < VR(UR):
  //    - discard the received UMD PDU;
  // - else:
  //    - place the received UMD PDU in the reception buffer.

  NS_LOG_LOGIC ("VR(UR) = " << m_vrUr);//1
  NS_LOG_LOGIC ("VR(UX) = " << m_vrUx);//0
  NS_LOG_LOGIC ("VR(UH) = " << m_vrUh);//1
  NS_LOG_LOGIC ("SN = " << seqNumber);//0

  m_vrUr.SetModulusBase (m_vrUh - m_windowSize);
  m_vrUh.SetModulusBase (m_vrUh - m_windowSize);
  seqNumber.SetModulusBase (m_vrUh - m_windowSize);

  if ( ( (m_vrUr < seqNumber) && (seqNumber < m_vrUh) && (m_rxBuffer.count (seqNumber.GetValue ()) > 0) ) ||
       ( ((m_vrUh - m_windowSize) <= seqNumber) && (seqNumber < m_vrUr) )
     )
    {
      NS_LOG_LOGIC ("PDU discarded");
      rxPduParams.p = 0;
      return;
    }
  else
    {
      NS_LOG_LOGIC ("Place PDU in the reception buffer");
      m_rxBuffer[seqNumber.GetValue ()] = rxPduParams.p;
    }


  // 5.1.2.2.3 Actions when an UMD PDU is placed in the reception buffer
  // When an UMD PDU with SN = x is placed in the reception buffer, the receiving UM RLC entity shall:

  // - if x falls outside of the reordering window:
  //    - update VR(UH) to x + 1;
  //    - reassemble RLC SDUs from any UMD PDUs with SN that falls outside of the reordering window, remove
  //      RLC headers when doing so and deliver the reassembled RLC SDUs to upper layer in ascending order of the
  //      RLC SN if not delivered before;
  //    - if VR(UR) falls outside of the reordering window:
  //        - set VR(UR) to (VR(UH) - UM_Window_Size);

  if ( ! IsInsideReorderingWindow (seqNumber))
    {
      NS_LOG_LOGIC ("SN is outside the reordering window");

      m_vrUh = seqNumber + 1;
      NS_LOG_LOGIC ("New VR(UH) = " << m_vrUh);

      ReassembleOutsideWindow ();

      if ( ! IsInsideReorderingWindow (m_vrUr) )
        {
          m_vrUr = m_vrUh - m_windowSize;
          NS_LOG_LOGIC ("VR(UR) is outside the reordering window");
          NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);
        }
    }

  // - if the reception buffer contains an UMD PDU with SN = VR(UR):
  //    - update VR(UR) to the SN of the first UMD PDU with SN > current VR(UR) that has not been received;
  //    - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when doing
  //      so and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not delivered
  //      before;

  if ( m_rxBuffer.count (m_vrUr.GetValue ()) > 0 )
    {
      NS_LOG_LOGIC ("Reception buffer contains SN = " << m_vrUr);

      std::map <uint16_t, Ptr<Packet> >::iterator it;
      uint16_t newVrUr;
      SequenceNumber10 oldVrUr = m_vrUr;

      it = m_rxBuffer.find (m_vrUr.GetValue ());
      newVrUr = (it->first) + 1;
      while ( m_rxBuffer.count (newVrUr) > 0 )
        {
          newVrUr++;
        }
      m_vrUr = newVrUr;
      NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);

      ReassembleSnInterval (oldVrUr, m_vrUr);//from here gives to RLC
    }

  // m_vrUh can change previously, set new modulus base
  // for the t-Reordering timer-related comparisons
  m_vrUr.SetModulusBase (m_vrUh - m_windowSize);
  m_vrUx.SetModulusBase (m_vrUh - m_windowSize);
  m_vrUh.SetModulusBase (m_vrUh - m_windowSize);

  // - if t-Reordering is running:
  //    - if VR(UX) <= VR(UR); or
  //    - if VR(UX) falls outside of the reordering window and VR(UX) is not equal to VR(UH)::
  //        - stop and reset t-Reordering;
  if ( m_reorderingTimer.IsRunning () )
    {
      NS_LOG_LOGIC ("Reordering timer is running");

      if ( (m_vrUx <= m_vrUr) ||
           ((! IsInsideReorderingWindow (m_vrUx)) && (m_vrUx != m_vrUh)) )
        {
          NS_LOG_LOGIC ("Stop reordering timer");
          m_reorderingTimer.Cancel ();
        }
    }

  // - if t-Reordering is not running (includes the case when t-Reordering is stopped due to actions above):
  //    - if VR(UH) > VR(UR):
  //        - start t-Reordering;
  //        - set VR(UX) to VR(UH).
  if ( ! m_reorderingTimer.IsRunning () )
    {
      NS_LOG_LOGIC ("Reordering timer is not running");

      if ( m_vrUh > m_vrUr )
        {
          NS_LOG_LOGIC ("VR(UH) > VR(UR)");
          NS_LOG_LOGIC ("Start reordering timer");
          m_reorderingTimer = Simulator::Schedule (m_reorderingTimerValue,
                                                   &LteRlcUm::ExpireReorderingTimer ,this);
          m_vrUx = m_vrUh;
          NS_LOG_LOGIC ("New VR(UX) = " << m_vrUx);
        }
    }

}


bool
LteRlcUm::IsInsideReorderingWindow (SequenceNumber10 seqNumber)
{
  NS_LOG_FUNCTION (this << seqNumber);
  NS_LOG_LOGIC ("Reordering Window: " <<
                m_vrUh << " - " << m_windowSize << " <= " << seqNumber << " < " << m_vrUh);

  m_vrUh.SetModulusBase (m_vrUh - m_windowSize);
  seqNumber.SetModulusBase (m_vrUh - m_windowSize);

  if ( ((m_vrUh - m_windowSize) <= seqNumber) && (seqNumber < m_vrUh))
    {
      NS_LOG_LOGIC (seqNumber << " is INSIDE the reordering window");
      return true;
    }
  else
    {
      NS_LOG_LOGIC (seqNumber << " is OUTSIDE the reordering window");
      return false;
    }
}


void
LteRlcUm::ReassembleAndDeliver (Ptr<Packet> packet)
{
  LteRlcHeader rlcHeader;
  packet->RemoveHeader (rlcHeader);
  uint8_t framingInfo = rlcHeader.GetFramingInfo ();
  SequenceNumber10 currSeqNumber = rlcHeader.GetSequenceNumber ();
  bool expectedSnLost;

  if ( currSeqNumber != m_expectedSeqNumber )
    {
      expectedSnLost = true;
      NS_LOG_LOGIC ("There are losses. Expected SN = " << m_expectedSeqNumber << ". Current SN = " << currSeqNumber);
      m_expectedSeqNumber = currSeqNumber + 1;
    }
  else
    {
      expectedSnLost = false;
      NS_LOG_LOGIC ("No losses. Expected SN = " << m_expectedSeqNumber << ". Current SN = " << currSeqNumber);
      m_expectedSeqNumber++;
    }

  // Build list of SDUs
  uint8_t extensionBit;
  uint16_t lengthIndicator;
  do
    {
      extensionBit = rlcHeader.PopExtensionBit ();
      NS_LOG_LOGIC ("E = " << (uint16_t)extensionBit);

      if ( extensionBit == 0 )
        {
          m_sdusBuffer.push_back (packet);
        }
      else // extensionBit == 1
        {
          lengthIndicator = rlcHeader.PopLengthIndicator ();
          NS_LOG_LOGIC ("LI = " << lengthIndicator);

          // Check if there is enough data in the packet
          if ( lengthIndicator >= packet->GetSize () )
            {
              NS_LOG_LOGIC ("INTERNAL ERROR: Not enough data in the packet (" << packet->GetSize () << "). Needed LI=" << lengthIndicator);
            }

          // Split packet in two fragments
          Ptr<Packet> data_field = packet->CreateFragment (0, lengthIndicator);
          packet->RemoveAtStart (lengthIndicator);

          m_sdusBuffer.push_back (data_field);
        }
    }
  while ( extensionBit == 1 );

  std::list < Ptr<Packet> >::iterator it;

  // Current reassembling state
  if (m_reassemblingState == WAITING_S0_FULL)       NS_LOG_LOGIC ("Reassembling State = 'WAITING_S0_FULL'");
  else if (m_reassemblingState == WAITING_SI_SF)    NS_LOG_LOGIC ("Reassembling State = 'WAITING_SI_SF'");
  else                                              NS_LOG_LOGIC ("Reassembling State = Unknown state");

  // Received framing Info
  NS_LOG_LOGIC ("Framing Info = " << (uint16_t)framingInfo);

  // Reassemble the list of SDUs (when there is no losses)
  if (!expectedSnLost)
    {
      switch (m_reassemblingState)
        {
          case WAITING_S0_FULL:
                  switch (framingInfo)
                    {
                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                              * Deliver one or multiple PDUs
                              */
                              for ( it = m_sdusBuffer.begin () ; it != m_sdusBuffer.end () ; it++ )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (*it);
                                }
                              m_sdusBuffer.clear ();
                      break;

                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                              * Deliver full PDUs
                              */
                              while ( m_sdusBuffer.size () > 1 )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                              * Keep S0
                              */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();
                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                   * Deliver zero, one or multiple PDUs
                                   */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                   * Keep S0
                                   */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                              * ERROR: Transition not possible
                              */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                              * Deliver (Kept)S0 + SN
                              */
                              m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                              m_sdusBuffer.pop_front ();
                              m_rlcSapUser->ReceivePdcpPdu (m_keepS0);

                              /**
                                * Deliver zero, one or multiple PDUs
                                */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                              * Keep SI
                              */
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                              else // m_sdusBuffer.size () > 1
                                {
                                  /**
                                  * Deliver (Kept)S0 + SN
                                  */
                                  m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                  m_rlcSapUser->ReceivePdcpPdu (m_keepS0);

                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_reassemblingState);
          break;
        }
    }
  else // Reassemble the list of SDUs (when there are losses, i.e. the received SN is not the expected one)
    {
      switch (m_reassemblingState)
        {
          case WAITING_S0_FULL:
                  switch (framingInfo)
                    {
                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Deliver one or multiple PDUs
                               */
                              for ( it = m_sdusBuffer.begin () ; it != m_sdusBuffer.end () ; it++ )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (*it);
                                }
                              m_sdusBuffer.clear ();
                      break;

                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                               * Deliver full PDUs
                               */
                              while ( m_sdusBuffer.size () > 1 )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();
                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                               * ERROR: Transition not possible
                               */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( m_sdusBuffer.size () > 1 )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();

                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                   * Deliver zero, one or multiple PDUs
                                   */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                   * Keep S0
                                   */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_reassemblingState);
          break;
        }
    }

}


void
LteRlcUm::ReassembleOutsideWindow (void)
{
  NS_LOG_LOGIC ("Reassemble Outside Window");

  std::map <uint16_t, Ptr<Packet> >::iterator it;
  it = m_rxBuffer.begin ();

  while ( (it != m_rxBuffer.end ()) && ! IsInsideReorderingWindow (SequenceNumber10 (it->first)) )
    {
      NS_LOG_LOGIC ("SN = " << it->first);

      // Reassemble RLC SDUs and deliver the PDCP PDU to upper layer
      ReassembleAndDeliver (it->second);

      std::map <uint16_t, Ptr<Packet> >::iterator it_tmp = it;
      ++it;
      m_rxBuffer.erase (it_tmp);
    }

  if (it != m_rxBuffer.end ())
    {
      NS_LOG_LOGIC ("(SN = " << it->first << ") is inside the reordering window");
    }
}

void
LteRlcUm::ReassembleSnInterval (SequenceNumber10 lowSeqNumber, SequenceNumber10 highSeqNumber)
{
  NS_LOG_LOGIC ("Reassemble SN between " << lowSeqNumber << " and " << highSeqNumber);

  std::map <uint16_t, Ptr<Packet> >::iterator it;

  SequenceNumber10 reassembleSn = lowSeqNumber;
  NS_LOG_LOGIC ("reassembleSN = " << reassembleSn);
  NS_LOG_LOGIC ("highSeqNumber = " << highSeqNumber);
  while (reassembleSn < highSeqNumber)
    {
      NS_LOG_LOGIC ("reassembleSn < highSeqNumber");
      it = m_rxBuffer.find (reassembleSn.GetValue ());
      NS_LOG_LOGIC ("it->first  = " << it->first);
      NS_LOG_LOGIC ("it->second = " << it->second);
      if (it != m_rxBuffer.end () )
        {
          NS_LOG_LOGIC ("SN = " << it->first);

          // Reassemble RLC SDUs and deliver the PDCP PDU to upper layer
          ReassembleAndDeliver (it->second);

          m_rxBuffer.erase (it);
        }
        
      reassembleSn++;
    }
}


void
LteRlcUm::DoReportBufferStatus (void)
{
  Time holDelay (0);
  uint32_t queueSize = 0;

  if (! m_txBuffer.empty ())
    {
      holDelay = Simulator::Now () - m_txBuffer.front().m_waitingSince;

      queueSize = m_txBufferSize + 2 * m_txBuffer.size (); // Data in tx queue + estimated headers size
    }

  LteMacSapProvider::ReportBufferStatusParameters r;
  r.rnti = m_rnti;
  r.lcid = m_lcid;
//  r.txQueueSize = queueSize;
  r.txQueueSize = queueSize;
//  r.txQueueHolDelay = holDelay.GetMilliSeconds () ;
  r.txQueueHolDelay = holDelay.GetMilliSeconds () *2;
  r.retxQueueSize = 0;
  r.retxQueueHolDelay = 0;
  r.statusPduSize = 0;

  NS_LOG_LOGIC ("Send ReportBufferStatus = " << r.txQueueSize << ", " << r.txQueueHolDelay );
  m_macSapProvider->ReportBufferStatus (r);
}


void
LteRlcUm::ExpireReorderingTimer (void)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid);
  NS_LOG_LOGIC ("Reordering timer has expired");

  // 5.1.2.2.4 Actions when t-Reordering expires
  // When t-Reordering expires, the receiving UM RLC entity shall:
  // - update VR(UR) to the SN of the first UMD PDU with SN >= VR(UX) that has not been received;
  // - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when doing so
  //   and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not delivered before;
  // - if VR(UH) > VR(UR):
  //    - start t-Reordering;
  //    - set VR(UX) to VR(UH).

  std::map <uint16_t, Ptr<Packet> >::iterator it;
  SequenceNumber10 newVrUr = m_vrUx;

  while ( (it = m_rxBuffer.find (newVrUr.GetValue ())) != m_rxBuffer.end () )
    {
      newVrUr++;
    }
  SequenceNumber10 oldVrUr = m_vrUr;
  m_vrUr = newVrUr;
  NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);

  ReassembleSnInterval (oldVrUr, m_vrUr);

  if ( m_vrUh > m_vrUr)
    {
      NS_LOG_LOGIC ("Start reordering timer");
      m_reorderingTimer = Simulator::Schedule (m_reorderingTimerValue,
                                               &LteRlcUm::ExpireReorderingTimer, this);
      m_vrUx = m_vrUh;
      NS_LOG_LOGIC ("New VR(UX) = " << m_vrUx);
    }
}


void
LteRlcUm::ExpireRbsTimer (void)
{
  NS_LOG_LOGIC ("RBS Timer expires");

  if (! m_txBuffer.empty ())
    {
      DoReportBufferStatus ();
      m_rbsTimer = Simulator::Schedule (MilliSeconds (10), &LteRlcUm::ExpireRbsTimer, this);
    }
}

} // namespace ns3

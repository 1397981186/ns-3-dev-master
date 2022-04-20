/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "nr-mac-header-vs-dl.h"
#include <ns3/log.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NrMacHeaderVsDl);
NS_LOG_COMPONENT_DEFINE("NrMacHeaderVsDl");

TypeId
NrMacHeaderVsDl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::NrMacHeaderVsDl")
    .SetParent<NrMacHeaderVs> ()
    .AddConstructor <NrMacHeaderVsDl> ()
    ;
  return tid;
}

TypeId NrMacHeaderVsDl::GetInstanceTypeId () const
{
  return GetTypeId ();
}

NrMacHeaderVsDl::NrMacHeaderVsDl()
{
  NS_LOG_FUNCTION (this);
}

NrMacHeaderVsDl::~NrMacHeaderVsDl ()
{
  NS_LOG_FUNCTION (this);
}

void
NrMacHeaderVsDl::SetLcId (uint8_t lcId)
{
  if (lcId <= 32)
    {
      NrMacHeaderVs::SetLcId (lcId);
    }
  else
    {
      m_lcid = lcId;
      NS_ASSERT (IsVariableSizeHeader ());
    }
}

bool
NrMacHeaderVsDl::IsVariableSizeHeader () const
{
  if (m_lcid <= 32) return true;
  if (m_lcid == SP_SRS) return true;
  if (m_lcid == TCI_STATES_PDSCH) return true;
  if (m_lcid == APERIODIC_CSI) return true;
  if (m_lcid == SP_CSI_RS_IM) return true;
  return false;
}
//sht
uint32_t
NrMacHeaderVsDl::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
//  if (m_size > 127)
//    {
//      return 3;
//    }
//  return 2;
  if (m_size > 127)
    {
      return 4;
    }
  return 3;
}

uint32_t
NrMacHeaderVsDl::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this);

  uint32_t readBytes = 1;
  uint8_t firstByte = start.ReadU8 ();

  // 0x3F: 0 0 1 1 1 1 1 1
  m_lcid = firstByte & 0x3F; // Clear the first 2 bits, the rest is the lcId

  // 0xC0: 1 1 0 0 0 0 0 0
  uint8_t lFieldSize = firstByte & 0xC0;

  if (lFieldSize == 0x40)
    {
      // 0x40: 0 1 0 0 0 0 0 0
      // the F bit is set to 1
      m_size = start.ReadNtohU16 ();
      readBytes += 2;
    }
  else if (lFieldSize == 0x00)
    {
      // the F bit is set to 0
      m_size = start.ReadU8 ();
      readBytes += 1;
    }
  else
    {
      NS_FATAL_ERROR ("The author of the code, who lies behind a christmas tree, is guilty");
    }
  m_signOfRlc=start.ReadU8 ();
  readBytes += 1;
//  if (m_size > 127)
//    {
//      NS_ASSERT (readBytes == 3);
//    }
//  else
//    {
//      NS_ASSERT (readBytes == 2);
//    }
  if (m_size > 127)
    {
      NS_ASSERT (readBytes == 4);
    }
  else
    {
      NS_ASSERT (readBytes == 3);
    }
  return readBytes;
}

void
NrMacHeaderVsDl::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this);

  // 0x3F: 0 0 1 1 1 1 1 1
  uint8_t firstByte = m_lcid & 0x3F;  // R, F bit set to 0, the rest equal to lcId

  if (m_size > 127)
    {
      // 0x40: 0 1 0 0 0 0 0 0
      firstByte = firstByte | 0x40; // set the F bit to 1, the rest as before
    }

  start.WriteU8 (firstByte);

  if (m_size > 127)
    {
      start.WriteHtonU16 (m_size);
    }
  else
    {
      start.WriteU8 (static_cast<uint8_t> (m_size));
    }
  start.WriteU8 (static_cast<uint8_t> (m_signOfRlc));
}

void
NrMacHeaderVsDl::SetSignOfRlc(uint16_t signOfRlc)
{
  m_signOfRlc = signOfRlc;
}


uint16_t
NrMacHeaderVsDl::GetSignOfRlc() const
{
  return m_signOfRlc;
}

} // namespace ns3


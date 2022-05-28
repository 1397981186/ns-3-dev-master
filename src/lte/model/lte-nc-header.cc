/*
 * lte-nc-header.cc
 *
 *  Created on: 2020年7月19日
 *      Author: rec
 */

#include "ns3/lte-nc-header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <iostream>

namespace ns3{
    NS_LOG_COMPONENT_DEFINE ("NcHeader");

    NS_OBJECT_ENSURE_REGISTERED (NcHeader);

    NcHeader::NcHeader()
    :m_DorC(0),
    m_polling(0),
	m_rank(0),
    m_groupnum(0),
    m_SN(255),
    m_israwpacket(0)
    {
    }
    NcHeader::~NcHeader()
    {
        m_polling=0;
        m_groupnum=0;
        m_SN=255;
        m_israwpacket=0;
    }
    void NcHeader::SetDorC(uint8_t DorC)
        {
        	m_DorC=DorC;
        }
    void NcHeader::SetRank(uint8_t rank)
        {
        	m_rank=rank;
        }
    void NcHeader::SetPolling(uint8_t polling)
    {
    	m_polling=polling;
    }

    void NcHeader::SetGroupnum(uint64_t groupnum)
    {
        m_groupnum=groupnum;
    }


    void NcHeader::SetSN(uint8_t SN)
    {
    	m_SN = SN;
    }
    uint8_t NcHeader::GetSN()
    {
    	return m_SN;
    }

    void NcHeader::SetIsRawPacket(uint8_t israwpacket)
    {
        m_israwpacket=israwpacket;
    }
    uint8_t NcHeader::IsRawPacket()
    {
    	return m_israwpacket;
    }
    uint8_t NcHeader::GetDorC() const
     {
         return m_DorC;
     }
    uint8_t NcHeader::GetPolling() const
         {
             return m_polling;
         }
    uint8_t NcHeader::GetRank() const
         {
             return m_rank;
         }

    uint64_t NcHeader::GetGroupnum() const
    {
        return m_groupnum;
    }

    void NcHeader::SetCoeff(uint64_t coeff)
    {
    	m_codingcoefficient = coeff;
    }

    uint64_t NcHeader::GetCoeff()
    {
    	return m_codingcoefficient;
    }

   TypeId
   NcHeader::GetTypeId(void)
   {
       static TypeId tid = TypeId ("ns3::NcHeader")
               .SetParent<Header> ()
               .SetGroupName("Lte")
               .AddConstructor<NcHeader> ();
       return tid;
   }

    TypeId
    NcHeader::GetInstanceTypeId(void) const
    {
        return GetTypeId ();
    }

    void
    NcHeader::Print(std::ostream &os) const
    {
        os<<"D/C"<<m_DorC<<",  "
		 << "polling=" <<m_polling<<",  "
		 <<"rank = "<<m_rank<<",  "
         << "groupnum="<<m_groupnum<<",  "
		 << "SN = "<<m_SN<<",  "
         << "israwpacket="<<m_israwpacket<<",  "
		 << "coeff = "<<m_codingcoefficient<<'\n';
    }

    uint32_t
    NcHeader::GetSerializedSize (void) const
    {
        return 21;
    }

    void
    NcHeader::Serialize(Buffer::Iterator start) const
    {
        Buffer::Iterator i=start;
        i.WriteU8(m_DorC);
        i.WriteU8(m_polling);
        i.WriteU8(m_rank);
        i.WriteU64(m_groupnum);
        i.WriteU8(m_SN);
        i.WriteU8(m_israwpacket);
        i.WriteU64(m_codingcoefficient);
    }

    uint32_t
    NcHeader::Deserialize(Buffer ::Iterator start)
    {
        Buffer::Iterator i=start;
        m_DorC=i.ReadU8();
        m_polling=i.ReadU8();
        m_rank=i.ReadU8();
        m_groupnum=i.ReadU64();
        m_SN=i.ReadU8();
        m_israwpacket=i.ReadU8();
        m_codingcoefficient = i.ReadU64();
        return GetSerializedSize();
    }

}






/*
 * lte-nc-header.h
 *
 *  Created on: 2020年7月19日
 *      Author: rec
 */

#ifndef SRC_LTE_MODEL_LTE_NC_HEADER_H_
#define SRC_LTE_MODEL_LTE_NC_HEADER_H_

#include "ns3/header.h"
#include <list>

namespace ns3{

    class NcHeader : public Header
    {
    public:
           NcHeader();
           ~NcHeader();
           static TypeId GetTypeId (void);
           virtual TypeId GetInstanceTypeId (void) const;
           void SetPolling(uint8_t polling);
           uint8_t GetPolling() const;
           void SetGroupnum(uint64_t groupnum);
           uint64_t GetGroupnum() const;
           void SetSN(uint8_t SN);
           uint8_t GetSN();
           void SetIsRawPacket(uint8_t israwpacket);
           uint8_t IsRawPacket();
           void SetRank(uint8_t rank);
           uint8_t GetRank() const;
           void SetDorC(uint8_t DorC);
           uint8_t GetDorC() const;
           void SetCoeff(uint64_t coeff);
           uint64_t GetCoeff();
           virtual void Print (std::ostream &os) const;
           virtual uint32_t GetSerializedSize (void) const;
           virtual void Serialize (Buffer::Iterator start) const;
           virtual uint32_t Deserialize (Buffer::Iterator start);
       private:
           uint8_t m_DorC;//0为数据包，1为控制包
           uint8_t m_polling;
           uint8_t m_rank;
           uint64_t m_groupnum;
           uint8_t m_SN;
           uint8_t m_israwpacket;
           uint64_t m_codingcoefficient;//8bytes


    };
}




#endif /* SRC_LTE_MODEL_LTE_NC_HEADER_H_ */

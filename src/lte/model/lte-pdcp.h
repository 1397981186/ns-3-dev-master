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

#ifndef LTE_PDCP_H
#define LTE_PDCP_H

#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"

#include "ns3/object.h"

#include "ns3/lte-pdcp-sap.h"
#include "ns3/lte-rlc-sap.h"
#include "NcControl.h"
#include "CopyControl.h"
#include "ns3/lte-nc-header.h"

namespace ns3 {

/**
 * LTE PDCP entity, see 3GPP TS 36.323
 */
class LtePdcp : public Object // SimpleRefCount<LtePdcp>
{
  /// allow LtePdcpSpecificLteRlcSapUser class friend access
  friend class LtePdcpSpecificLteRlcSapUser;
  /// allow LtePdcpSpecificLtePdcpSapProvider<LtePdcp> class friend access
  friend class LtePdcpSpecificLtePdcpSapProvider<LtePdcp>;
public:
  LtePdcp ();
  virtual ~LtePdcp ();
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   *
   *
   * \param rnti
   */
  void SetRnti (uint16_t rnti);

  /**
   *
   *
   * \param lcId
   */
  void SetLcId (uint8_t lcId);

  /**
   *
   *
   * \param s the PDCP SAP user to be used by this LTE_PDCP
   */
   
  void SetLcId_leg (uint8_t lcId);//zjh_add
   
  void SetLtePdcpSapUser (LtePdcpSapUser * s);
  void SetLtePdcpSapUser_leg (LtePdcpSapUser * s);//zjh_add
  /**
   *
   *
   * \return the PDCP SAP Provider interface offered to the RRC by this LTE_PDCP
   */
  LtePdcpSapProvider* GetLtePdcpSapProvider ();

  /**
   *
   *
   * \param s the RLC SAP Provider to be used by this LTE_PDCP
   */
  void SetLteRlcSapProvider (LteRlcSapProvider * s);


 /**
   * zjh_new:
   *
   * \param s the second_leg RLC SAP Provider to be used by this LTE_PDCP
   */
  void SetLteRlcSapProvider_leg (LteRlcSapProvider * s);
  
  /**
   *
   *
   * \return the RLC SAP User interface offered to the RLC by this LTE_PDCP
   */
  LteRlcSapUser* GetLteRlcSapUser ();

  /**
   * zjh_new: 
   *
   * \return the second RLC SAP User interface offered to the RLC by this LTE_PDCP
   */
  LteRlcSapUser* GetLteRlcSapUser_leg ();

  /// maximum PDCP SN
  static const uint16_t MAX_PDCP_SN = 4096;

  /**
   * Status variables of the PDCP
   */
  struct Status
  {
    uint16_t txSn; ///< TX sequence number
    uint16_t rxSn; ///< RX sequence number
  };

  /** 
   * 
   * \return the current status of the PDCP
   */
  Status GetStatus ();

  /**
   * Set the status of the PDCP
   * 
   * \param s 
   */
  void SetStatus (Status s);
 
  /** 
   * zjh_new: 
   * \return the current duplication status of the PDCP
   */
  bool GetDuplication ();
   
  /**
   * zjh_new: Set the duplication of the PDCP
   * 
   * \param onoff 
   */
  void SetDuplication (bool onoff);
  
  /**
   * TracedCallback for PDU transmission event.
   *
   * \param [in] rnti The C-RNTI identifying the UE.
   * \param [in] lcid The logical channel id corresponding to
   *             the sending RLC instance.
   * \param [in] size Packet size.
   */
  typedef void (* PduTxTracedCallback)
    (uint16_t rnti, uint8_t lcid, uint32_t size);

  /**
   * TracedCallback signature for PDU receive event.
   *
   * \param [in] rnti The C-RNTI identifying the UE.
   * \param [in] lcid The logical channel id corresponding to
   *             the sending RLC instance.
   * \param [in] size Packet size.
   * \param [in] delay Delay since packet sent, in ns..
   */
  typedef void (* PduRxTracedCallback)
    (const uint16_t rnti, const uint8_t lcid,
     const uint32_t size, const uint64_t delay);

protected:
  /**
   * Interface provided to upper RRC entity
   *
   * \param params the TransmitPdcpSduParameters
   */
  virtual void DoTransmitPdcpSdu (LtePdcpSapProvider::TransmitPdcpSduParameters params);
  virtual void DoTransmitPdcpSdu_leg (LtePdcpSapProvider::TransmitPdcpSduParameters params);
  virtual void TriggerDoTransmitPdcpSdu(LtePdcpSapProvider::TransmitPdcpSduParameters params);
  void TriggerRecvPdcpSdu(Ptr<Packet> p);
  void ToogleSend(LtePdcpSapProvider::TransmitPdcpSduParameters params);
  void SingleSend(LtePdcpSapProvider::TransmitPdcpSduParameters params);
  void CopyArqReqSendOnce(uint64_t ArqGroupNum,Ptr<Packet> p,CopyControl* m_Copy);
  void NcArqReqSendOnce(uint64_t ArqGroupNum,Ptr<Packet> p,NcControl* m_Nc);
  void CopyExpireStatusReportTimer (uint64_t ArqGroupNum,Ptr<Packet> p,CopyControl* m_Copy);
  void NcExpireStatusReportTimer (uint64_t ArqGroupNum,Ptr<Packet> p,NcControl* m_Nc);
  Time m_statusReportTimerValuePdcpCopy=MicroSeconds(65000.0);
  Time m_statusReportTimerValuePdcpNc=MicroSeconds(65000.0);

  NcControl* m_Nc;
  CopyControl* m_Copy;

  LtePdcpSapUser* m_pdcpSapUser; ///< PDCP SAP user
  LtePdcpSapUser* m_pdcpSapUser_leg; //zjh_add
  LtePdcpSapProvider* m_pdcpSapProvider; ///< PDCP SAP provider

  /**
   * Interface provided to lower RLC entity
   *
   * \param p packet
   */
  virtual void DoReceivePdu (Ptr<Packet> p);

  LteRlcSapUser* m_rlcSapUser; ///< RLC SAP user 
  LteRlcSapUser* m_rlcSapUser_leg; ///< zjh_new: the second leg LC SAP user 
  LteRlcSapProvider* m_rlcSapProvider; ///< RLC SAP provider
  LteRlcSapProvider* m_rlcSapProvider_leg; ///< zjh_new: the second leg RLC SAP provider

  uint16_t m_rnti; ///< RNTI
  uint8_t m_lcid; ///< LCID
  
  //uint16_t m_rnti_leg; //zjh_add，暂且使用用一个rnti
  uint8_t m_lcid_leg; ///zjh_add

  /**
   * Used to inform of a PDU delivery to the RLC SAP provider.
   * The parameters are RNTI, LCID and bytes delivered
   */
  TracedCallback<uint16_t, uint8_t, uint32_t> m_txPdu;
  TracedCallback<uint16_t, uint8_t, uint32_t> m_txPdu_leg;//zjh_add
  /**
   * Used to inform of a PDU reception from the RLC SAP user.
   * The parameters are RNTI, LCID, bytes delivered and delivery delay in nanoseconds. 
   */
  TracedCallback<uint16_t, uint8_t, uint32_t, uint64_t> m_rxPdu;
  TracedCallback<uint16_t, uint8_t, uint32_t, uint64_t> m_rxPdu_leg;//zjh_add

private:
  /**
   * State variables. See section 7.1 in TS 36.323
   */
  uint16_t m_txSequenceNumber;
  /**
   * State variables. See section 7.1 in TS 36.323
   */
  uint16_t m_rxSequenceNumber;

  /**
   * Constants. See section 7.2 in TS 36.323
   */
  static const uint16_t m_maxPdcpSn = 4095;
  
  /**
   * zjh_new: onoff pdcp duplication, See section 7.1 in TS 38.323
   */
  bool m_pdcpDuplication;
  bool m_NcEnable;
  bool m_CopyEnable;
  uint32_t m_NcRlcToSend=0;
};


} // namespace ns3

#endif // LTE_PDCP_H

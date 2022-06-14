/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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

/**
 * \ingroup examples
 * \file cttc-nr-demo.cc
 * \brief A cozy, simple, NR demo (in a tutorial style)
 *
 * This example describes how to setup a simulation using the 3GPP channel model
 * from TR 38.900. This example consists of a simple grid topology, in which you
 * can choose the number of gNbs and UEs. Have a look at the possible parameters
 * to know what you can configure through the command line.
 *
 * With the default configuration, the example will create two flows that will
 * go through two different subband numerologies (or bandwidth parts). For that,
 * specifically, two bands are created, each with a single CC, and each CC containing
 * one bandwidth part.
 *
 * The example will print on-screen the end-to-end result of one (or two) flows,
 * as well as writing them on a file.
 *
 * \code{.unparsed}
$ ./waf --run "cttc-nr-demo --Help"
    \endcode
 *
 */

/*
 * Include part. Often, you will have to include the headers for an entire module;
 * do that by including the name of the module you need with the suffix "-module.h".
 */

#include "ns3/core-module.h"
#include "ns3/config-store-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/buildings-module.h"
#include "ns3/nr-module.h"
#include "ns3/antenna-module.h"

/*
 * Use, always, the namespace ns3. All the NR classes are inside such namespace.
 */
using namespace ns3;

/*
 * With this line, we will be able to see the logs of the file by enabling the
 * component "CttcNrDemo"
 */
NS_LOG_COMPONENT_DEFINE ("CttcNrDemo");

int
main (int argc, char *argv[])
{
  /*
   * Variables that represent the parameters we will accept as input by the
   * command line. Each of them is initialized with a default value, and
   * possibly overridden below when command-line arguments are parsed.
   */
  // Scenario parameters (that we will use inside this script):
  uint16_t gNbNum = 1;//znr_com
  //uint16_t gNbNum = 2;//znr_add: 测试是否可避免：原1个基站2个终端（this example），外加pdcp dup两路数据造成的仿真混乱
  //修改效果：仿真耗费时长大幅增加，仿真数据量大幅增加（文件4与5对比），原因待分析？？？
  
  //uint16_t ueNumPergNb = 2;//znr_note: 关键参数
  uint16_t ueNumPergNb = 1;//znr_add: 只使用一个ue以减少仿真复杂度;但不满足ueLowLat、ueVoice两种应用同时使用？？？//znr_com: （单用户测试）
  //znr_add: 测试无效果，lcid5、6仍然无数据传输
  
  
  bool logging = true;
  bool doubleOperationalBand = true;//znr_note: 关键参数，原值doubleOperationalBand = true; 改为false将报错
  //znr_add: 如果为false，将使得allBwps=1，导致InstallGnbDevice调用InstallSingleGnbDevice中ccMap长度为1，即1个cc
  //znr_add: Lte设置2个cc，通过LteHelper在InstallSingleEnbDevice中实现，与此方式不同！！！
  //znr_add: nr pdcp dup第一方案：沿用lte pdcp dup使用2个cc思路。设置true，this example含2个cc，1个cc对应1个bwp，allBwps=2（见第135行说明）。
  //znr_add: nr pdcp dup第二方案：考虑1个cc对应多个Bwps（暂不实现）

  // Traffic parameters (that we will use inside this script):
  uint32_t udpPacketSizeULL = 100;//znr_note: rlc之前的数据包大小，影响LteRlcUm::DoNotifyTxOpportunity调用次数，但不是phy资源块大小的决定因素
  uint32_t udpPacketSizeBe = 1252;
  uint32_t lambdaULL = 10000;
  uint32_t lambdaBe = 10000;//znr_change: 原值lambdaBe = 10000；关键代码，通过调低流量大小，可避免出现4倍echo流量带来拥堵错误

  // Simulation parameters. Please don't use double to indicate seconds; use
  // ns-3 Time values which use integers to avoid portability issues.
  Time simTime = MilliSeconds (300);//znr-change: 原值MilliSeconds (1000)
  Time udpAppStartTime = MilliSeconds (200);//znr-change: 原值MilliSeconds (400)

  // NR parameters. We will take the input from the command line, and then we
  // will pass them inside the NR module.
  uint16_t numerologyBwp1 = 4;
  double centralFrequencyBand1 = 28e9;
  double bandwidthBand1 = 100e6;
  uint16_t numerologyBwp2 = 2;
  double centralFrequencyBand2 = 28.2e9;
  double bandwidthBand2 = 100e6;
  double totalTxPower = 4;

  // Where we will store the output files.
  std::string simTag = "default";
  std::string outputDir = "./";

  /*
   * From here, we instruct the ns3::CommandLine class of all the input parameters
   * that we may accept as input, as well as their description, and the storage
   * variable.
   */
  CommandLine cmd;

  cmd.AddValue ("gNbNum",
                "The number of gNbs in multiple-ue topology",
                gNbNum);
  cmd.AddValue ("ueNumPergNb",
                "The number of UE per gNb in multiple-ue topology",
                ueNumPergNb);
  cmd.AddValue ("logging",
                "Enable logging",
                logging);
  cmd.AddValue ("doubleOperationalBand",
                "If true, simulate two operational bands with one CC for each band,"
                "and each CC will have 1 BWP that spans the entire CC.",
                doubleOperationalBand);
  cmd.AddValue ("packetSizeUll",
                "packet size in bytes to be used by ultra low latency traffic",
                udpPacketSizeULL);
  cmd.AddValue ("packetSizeBe",
                "packet size in bytes to be used by best effort traffic",
                udpPacketSizeBe);
  cmd.AddValue ("lambdaUll",
                "Number of UDP packets in one second for ultra low latency traffic",
                lambdaULL);
  cmd.AddValue ("lambdaBe",
                "Number of UDP packets in one second for best effor traffic",
                lambdaBe);
  cmd.AddValue ("simTime",
                "Simulation time",
                simTime);
  cmd.AddValue ("numerologyBwp1",
                "The numerology to be used in bandwidth part 1",
                numerologyBwp1);
  cmd.AddValue ("centralFrequencyBand1",
                "The system frequency to be used in band 1",
                centralFrequencyBand1);
  cmd.AddValue ("bandwidthBand1",
                "The system bandwidth to be used in band 1",
                bandwidthBand1);
  cmd.AddValue ("numerologyBwp2",
                "The numerology to be used in bandwidth part 2",
                numerologyBwp2);
  cmd.AddValue ("centralFrequencyBand2",
                "The system frequency to be used in band 2",
                centralFrequencyBand2);
  cmd.AddValue ("bandwidthBand2",
                "The system bandwidth to be used in band 2",
                bandwidthBand2);
  cmd.AddValue ("totalTxPower",
                "total tx power that will be proportionally assigned to"
                " bands, CCs and bandwidth parts depending on each BWP bandwidth ",
                totalTxPower);
  cmd.AddValue ("simTag",
                "tag to be appended to output filenames to distinguish simulation campaigns",
                simTag);
  cmd.AddValue ("outputDir",
                "directory where to store simulation results",
                outputDir);


  // Parse the command line
  cmd.Parse (argc, argv);

  /*
   * Check if the frequency is in the allowed range.
   * If you need to add other checks, here is the best position to put them.
   */
  NS_ABORT_IF (centralFrequencyBand1 > 100e9);
  NS_ABORT_IF (centralFrequencyBand2 > 100e9);

  /*
   * If the logging variable is set to true, enable the log of some components
   * through the code. The same effect can be obtained through the use
   * of the NS_LOG environment variable:
   *
   * export NS_LOG="UdpClient=level_info|prefix_time|prefix_func|prefix_node:UdpServer=..."
   *
   * Usually, the environment variable way is preferred, as it is more customizable,
   * and more expressive.
   */
  if (logging)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);//znr-add
 
      //LogComponentEnable ("UdpClient", logLevel);//znr-com
      //LogComponentEnable ("UdpServer", logLevel);//znr-com
          
      LogComponentEnable ("NrHelper", logLevel);//znr-add
      LogComponentEnable ("CcBwpHelper", logLevel);//znr-add

      //LogComponentEnable ("SpectrumChannel", logLevel);//znr-add
      //LogComponentEnable ("NetDevice", logLevel);//znr-add
      LogComponentEnable ("NrNetDevice", logLevel);//znr-add: 注意该类函数内部没有log语句，例如this指针，默认情况waf看不到执行过程
      LogComponentEnable ("NrGnbNetDevice", logLevel);//znr-add
      //LogComponentEnable ("NrUeNetDevice", logLevel);//znr-add
      //LogComponentEnable ("EpcUeNas", logLevel);//znr-add

      LogComponentEnable ("LteEnbRrc", logLevel);//znr-add
      //LogComponentEnable ("LteUeRrc", logLevel);//znr-add
      //LogComponentEnable ("UeManager", logLevel);//znr-add: 不在日志空间注册，UeManager日志依然是LteEnbRrc，例如：
                                                 //UeManager::GetDataRadioBearerInfo 在日志中是 LteEnbRrc:GetDataRadioBearerInfo
      //LogComponentEnable ("LtePdcp", logLevel);
      //LogComponentEnable ("LteRlcUm", logLevel);//znr-add

      //LogComponentEnable ("nrControlMessage", logLevel);//znr-add
      //LogComponentEnable ("BwpManagerGnb", logLevel);//znr-add: class BwpManagerGnb : public RrComponentCarrierManager
                                                                //class RrComponentCarrierManager : public NoOpComponentCarrierManager
                                                                //class NoOpComponentCarrierManager : public LteEnbComponentCarrierManager
                                                                //class LteEnbComponentCarrierManager : public Object
      //LogComponentEnable ("BwpManagerUe", logLevel);//znr-add:  class BwpManagerUe : public SimpleUeComponentCarrierManager
                                                                //class SimpleUeComponentCarrierManager : public LteUeComponentCarrierManager
                                                                //class LteUeComponentCarrierManager : public Object
    
      //LogComponentEnable ("BandwidthPartGnb", logLevel);//znr-add:class BandwidthPartGnb : public ComponentCarrierBaseStation
                                                                  //class ComponentCarrierBaseStation: pubilic ComponentCarrier
      //LogComponentEnable ("BandwidthPartUe", logLevel);//znr-add: class BandwidthPartUe : public ComponentCarrier
                                                                  //class ComponentCarrier : public Object
      //LogComponentEnable ("ComponentCarrier", logLevel);//znr-add
      
      
      //LogComponentEnable ("NrMacSchedulerNs3", logLevel);//znr-add: class NrMacSchedulerNs3 : public NrMacScheduler
                                                                    //class NrMacScheduler : public Object
      //LogComponentEnable ("NrMacScheduler", logLevel);//znr-add
      
      //LogComponentEnable ("NrGnbMac", logLevel);//znr-add: 包含class NrMacEnbMemberPhySapUser : public NrGnbPhySapUser 
      //LogComponentEnable ("NrUeMac", logLevel);//znr-add
      
      //LogComponentEnable ("NrPhy", logLevel);//znr-add
      //LogComponentEnable ("NrGnbPhy", logLevel);//znr-add
      //LogComponentEnable ("NrUePhy", logLevel);//znr-add
      //LogComponentEnable ("NrSpectrumPhy", logLevel);//znr-add

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

      LogComponentEnable ("UdpClient", LOG_LEVEL_FUNCTION);//zjh_com
      //LogComponentEnable ("UdpServer", LOG_LEVEL_FUNCTION);//zjh_com
      LogComponentEnable ("LtePdcp", LOG_LEVEL_FUNCTION);//znr-add: 观察程序运行过程，采用LOG_LEVEL_LOGIC
   
      //LogComponentEnable ("NrHelper", LOG_LEVEL_INFO);//znr_add
      //LogComponentEnable ("NrHelper", LOG_LEVEL_FUNCTION);//znr_add
      //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_INFO);//znr_add
      //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_FUNCTION);//znr_add
      //LogComponentEnable ("LteUeRrc", LOG_LEVEL_INFO);//znr_add
      LogComponentEnable ("LteRlcUm", LOG_LEVEL_FUNCTION);//znr_add
      //LogComponentEnable ("PacketMetadata", LOG_LEVEL_INFO);//znr_add：不需要修改此函数
      //LogComponentEnable ("LteRlc", LOG_LEVEL_FUNCTION);//znr_add
      //LogComponentEnable ("BwpManagerGnb", LOG_LEVEL_FUNCTION);//znr_add
      //LogComponentEnable ("NrGnbMac", LOG_LEVEL_DEBUG);//znr_add: 使用logLevel或者LOG_LEVEL_LOGIC，在仿真统计结束后有报错？？？（暂且搁置）
      //LogComponentEnable ("NrUeMac", LOG_LEVEL_INFO);//znr_add  
      //LogComponentEnable ("NrGnbPhy", LOG_LEVEL_LOGIC);//znr_add
      //LogComponentEnable ("NrUePhy", LOG_LEVEL_FUNCTION);//znr_add
      //LogComponentEnable ("NrPhy", LOG_LEVEL_DEBUG);//znr_add
      //LogComponentEnable ("NrSpectrumPhy", LOG_LEVEL_DEBUG);//znr_add
      //LogComponentEnable ("MultiModelSpectrumChannel", LOG_LEVEL_INFO);//znr_add 
      //LogComponentEnable ("CttcNrDemo", LOG_LEVEL_INFO);//znr_add
      //LogComponentEnable ("EpcEnbApplication", LOG_LEVEL_INFO);//znr_add
      //LogComponentEnable ("NoOpComponentCarrierManager", LOG_LEVEL_DEBUG);//znr_add
      //LogComponentEnable ("SimpleUeComponentCarrierManager", LOG_LEVEL_DEBUG);//znr_add
      //LogComponentEnable ("nrRrcProtocolIdeal", LOG_LEVEL_INFO);//znr_add
      //LogComponentEnable ("LteHelper", LOG_LEVEL_INFO);//znr_add: NR与LteHelper无关
    }

  /*
   * Default values for the simulation. We are progressively removing all
   * the instances of SetDefault, but we need it for legacy code (LTE)
   */
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (999999999));

  /*
   * Create the scenario. In our examples, we heavily use helpers that setup
   * the gnbs and ue following a pre-defined pattern. Please have a look at the
   * GridScenarioHelper documentation to see how the nodes will be distributed.
   */
  int64_t randomStream = 1;//znr_note: 随机数据流序号
  GridScenarioHelper gridScenario;
  gridScenario.SetRows (1);
  gridScenario.SetColumns (gNbNum);
  gridScenario.SetHorizontalBsDistance (5.0);
  gridScenario.SetVerticalBsDistance (5.0);
  gridScenario.SetBsHeight (1.5);
  gridScenario.SetUtHeight (1.5);
  // must be set before BS number
  gridScenario.SetSectorization (GridScenarioHelper::SINGLE);
  gridScenario.SetBsNumber (gNbNum);
  gridScenario.SetUtNumber (ueNumPergNb * gNbNum);
  gridScenario.SetScenarioHeight (3); // Create a 3x3 scenario where the UE will
  gridScenario.SetScenarioLength (3); // be distribuited.
  randomStream += gridScenario.AssignStreams (randomStream);////znr_note: 网格场景随机流x方向设为1,y方向设为2，randomStream增加2变为3
  gridScenario.CreateScenario ();

  /*
   * Create two different NodeContainer for the different traffic type.
   * In ueLowLat we will put the UEs that will receive low-latency traffic,
   * while in ueVoice we will put the UEs that will receive the voice traffic.
   */
  NodeContainer ueLowLatContainer, ueVoiceContainer;
  
  //NS_LOG_INFO("numUT = " << (uint16_t)gridScenario.GetUserTerminals ().GetN ());//znr_add
  for (uint32_t j = 0; j < gridScenario.GetUserTerminals ().GetN (); ++j)
    {
      Ptr<Node> ue = gridScenario.GetUserTerminals ().Get (j);
      if (j % 2 == 0)
        {
          ueLowLatContainer.Add (ue);
        }
      else
        {
          ueVoiceContainer.Add (ue);
        }
    }

  /*
   * TODO: Add a print, or a plot, that shows the scenario.
   */

  /*
   * Setup the NR module. We create the various helpers needed for the
   * NR simulation:
   * - EpcHelper, which will setup the core network
   * - IdealBeamformingHelper, which takes care of the beamforming part
   * - NrHelper, which takes care of creating and connecting the various
   * part of the NR stack
   */
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  // Put the pointers inside nrHelper
  nrHelper->SetBeamformingHelper (idealBeamformingHelper);
  nrHelper->SetEpcHelper (epcHelper);
  
  /*
   * Spectrum division. We create two operational bands, each of them containing
   * one component carrier, and each CC containing a single bandwidth part
   * centered at the frequency specified by the input parameters.
   * Each spectrum part length is, as well, specified by the input parameters.
   * Both operational bands will use the StreetCanyon channel modeling.
   */
  BandwidthPartInfoPtrVector allBwps;
  CcBwpCreator ccBwpCreator;
  //const uint8_t numCcPerBand = 1;  // in this example, both bands have a single CC
  const uint8_t numCcPerBand = 2;//znr-change: 20220603

  // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
  // a single BWP per CC
  CcBwpCreator::SimpleOperationBandConf bandConf1 (centralFrequencyBand1, bandwidthBand1, numCcPerBand, BandwidthPartInfo::UMi_StreetCanyon);
  CcBwpCreator::SimpleOperationBandConf bandConf2 (centralFrequencyBand2, bandwidthBand2, numCcPerBand, BandwidthPartInfo::UMi_StreetCanyon);

  // By using the configuration created, it is time to make the operation bands
  //znr_note: 此处创建2个cc
  OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc (bandConf1);//znr_note: 创建cc
  OperationBandInfo band2 = ccBwpCreator.CreateOperationBandContiguousCc (bandConf2);//znr_note: 创建cc

  /*
   * The configured spectrum division is:
   * ------------Band1--------------|--------------Band2-----------------
   * ------------CC1----------------|--------------CC2-------------------
   * ------------BWP1---------------|--------------BWP2------------------
   */

  /*
   * Attributes of ThreeGppChannelModel still cannot be set in our way.
   * TODO: Coordinate with Tommaso
   */
  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds (0)));
  nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

  /*
   * Initialize channel and pathloss, plus other things inside band1. If needed,
   * the band configuration can be done manually, but we leave it for more
   * sophisticated examples. For the moment, this method will take care
   * of all the spectrum initialization needs.
   */
  nrHelper->InitializeOperationBand (&band1);

  /*
   * Start to account for the bandwidth used by the example, as well as
   * the total power that has to be divided among the BWPs.
   */
  double x = pow (10, totalTxPower / 10);
  double totalBandwidth = bandwidthBand1;

  /*
   * if not single band simulation, initialize and setup power in the second band
   */
  if (doubleOperationalBand)
    {
      // Initialize channel and pathloss, plus other things inside band2
      nrHelper->InitializeOperationBand (&band2);
      totalBandwidth += bandwidthBand2;
      allBwps = CcBwpCreator::GetAllBwps ({band1, band2});//znr_note: allBwps长度等于2
    }
  else
    {
      allBwps = CcBwpCreator::GetAllBwps ({band1});//znr_note: allBwps长度等于1
    }

  /*
   * allBwps contains all the spectrum configuration needed for the nrHelper.
   *
   * Now, we can setup the attributes. We can have three kind of attributes:
   * (i) parameters that are valid for all the bandwidth parts and applies to
   * all nodes, (ii) parameters that are valid for all the bandwidth parts
   * and applies to some node only, and (iii) parameters that are different for
   * every bandwidth parts. The approach is:
   *
   * - for (i): Configure the attribute through the helper, and then install;//znr_note:this example采用第一种模式  
   * - for (ii): Configure the attribute through the helper, and then install
   * for the first set of nodes. Then, change the attribute through the helper,
   * and install again;
   * - for (iii): Install, and then configure the attributes by retrieving
   * the pointer needed, and calling "SetAttribute" on top of such pointer.
   *
   */

  Packet::EnableChecking ();
  Packet::EnablePrinting ();

  /*
   *  Case (i): Attributes valid for all the nodes
   */
  // Beamforming method
  idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));

  // Core latency
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (0)));

  // Antennas for all the UEs
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  // Antennas for all the gNbs
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  uint32_t bwpIdForLowLat = 0;
  //uint32_t bwpIdForVoice = 0;//znr_note: 如果不设置双cc，那么两个应用可能都对应ComponentCarrierId=0？//znr_com: （单用户测试）
  if (doubleOperationalBand)
    {
      //bwpIdForVoice = 1;//znr_note:语音通道，对应ComponentCarrierId？//znr_com: （单用户测试）
      bwpIdForLowLat = 0;//znr_note:低时延通道，对应ComponentCarrierId？
    }

  // gNb routing between Bearer and bandwidh part
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("NGBR_LOW_LAT_EMBB", UintegerValue (bwpIdForLowLat));
  //nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (bwpIdForVoice));//znr_com: （单用户测试）

  // Ue routing between Bearer and bandwidth part
  nrHelper->SetUeBwpManagerAlgorithmAttribute ("NGBR_LOW_LAT_EMBB", UintegerValue (bwpIdForLowLat));
  //nrHelper->SetUeBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (bwpIdForVoice));//znr_com: （单用户测试）


  /*
   * We miss many other parameters. By default, not configuring them is equivalent
   * to use the default values. Please, have a look at the documentation to see
   * what are the default values for all the attributes you are not seeing here.
   */

  /*
   * Case (ii): Attributes valid for a subset of the nodes
   */

  // NOT PRESENT IN THIS SIMPLE EXAMPLE

  /*
   * We have configured the attributes we needed. Now, install and get the pointers
   * to the NetDevices, which contains all the NR stack:
   */

  //znr_note: nrHelper->InstallGnbDevice与lte不同，需要设置allBwps长度为2
  //znr_note: 第255行，gridScenario配置了ueLowLat、ueVoice
  NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice (gridScenario.GetBaseStations (), allBwps);
  NetDeviceContainer ueLowLatNetDev = nrHelper->InstallUeDevice (ueLowLatContainer, allBwps); 
  //NetDeviceContainer ueVoiceNetDev = nrHelper->InstallUeDevice (ueVoiceContainer, allBwps);//znr_com: （单用户测试）

  NS_LOG_INFO( "randomStream = " << (uint16_t) randomStream);//znr_add: randomStream=3
  randomStream += nrHelper->AssignStreams (enbNetDev, randomStream);//znr_note: randomStream=17，增加14个流序号
  NS_LOG_INFO( "randomStream = " << (uint16_t) randomStream);//znr_add
  
  randomStream += nrHelper->AssignStreams (ueLowLatNetDev, randomStream);//znr_note: randomStream=27，增加10个流序号
  NS_LOG_INFO( "randomStream = " << (uint16_t) randomStream);//znr_add
  
  //randomStream += nrHelper->AssignStreams (ueVoiceNetDev, randomStream);//znr_note: randomStream=37，增加10个流序号//znr_com: （单用户测试）
  //NS_LOG_INFO( "randomStream = " << (uint16_t) randomStream);//znr_add
  
  /*
   * Case (iii): Go node for node and change the attributes we have to setup
   * per-node.
   */

  // Get the first netdevice (enbNetDev.Get (0)) and the first bandwidth part (0)
  // and set the attribute.
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("Numerology", UintegerValue (numerologyBwp1));
  nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("TxPower", DoubleValue (10 * log10 ((bandwidthBand1 / totalBandwidth) * x)));

  if (doubleOperationalBand)
    {
      //znr_note: 使用enbNetDev.Get(0)只支持1个enb，如果上文设置gNbNum = 2，则需要修改代码
      // Get the first netdevice (enbNetDev.Get (0)) and the second bandwidth part (1)
      // and set the attribute.
      nrHelper->GetGnbPhy (enbNetDev.Get (0), 1)->SetAttribute ("Numerology", UintegerValue (numerologyBwp2));
      nrHelper->GetGnbPhy (enbNetDev.Get (0), 1)->SetTxPower (10 * log10 ((bandwidthBand2 / totalBandwidth) * x));
    }

  // When all the configuration is done, explicitly call UpdateConfig ()

  for (auto it = enbNetDev.Begin (); it != enbNetDev.End (); ++it)
    {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();//znr_note: 将调用LteEnbRrc::ConfigureCell
    }

  for (auto it = ueLowLatNetDev.Begin (); it != ueLowLatNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  //znr_com: （单用户测试）
  /*
  for (auto it = ueVoiceNetDev.Begin (); it != ueVoiceNetDev.End (); ++it)
    {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }
  */

  // From here, it is standard NS3. In the future, we will create helpers
  // for this part as well.

  // create the internet and install the IP stack on the UEs
  // get SGW/PGW and create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.000)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  internet.Install (gridScenario.GetUserTerminals ());


  Ipv4InterfaceContainer ueLowLatIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLowLatNetDev));
  //Ipv4InterfaceContainer ueVoiceIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueVoiceNetDev));//znr_com: （单用户测试）

  // Set the default gateway for the UEs
  for (uint32_t j = 0; j < gridScenario.GetUserTerminals ().GetN (); ++j)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (gridScenario.GetUserTerminals ().Get (j)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (ueLowLatNetDev, enbNetDev);
  //nrHelper->AttachToClosestEnb (ueVoiceNetDev, enbNetDev);//znr_com: （单用户测试）

  /*
   * Traffic part. Install two kind of traffic: low-latency and voice, each
   * identified by a particular source port.
   */
  uint16_t dlPortLowLat = 1234;
  //uint16_t dlPortVoice = 1235;//znr_com: （单用户测试）

  ApplicationContainer serverApps;

  // The sink will always listen to the specified ports
  UdpServerHelper dlPacketSinkLowLat (dlPortLowLat);//znr_com
  //UdpEchoServerHelper dlPacketSinkLowLat (dlPortLowLat);//znr_add
  //UdpServerHelper dlPacketSinkVoice (dlPortVoice);//znr_com: 为排查问题，不加载voice应用（测试发现输入输出未减少？？？已取消）//znr_com: （单用户测试）
  //UdpEchoServerHelper dlPacketSinkVoice (dlPortVoice);//znr_add: 测试Ue到gNb通信，调低lambdaBe可以避免数据拥堵错误//znr_com: （单用户测试）
  //znr_note: 添加Echo应用不能只修改服务器端，还需修改客户端（lambdaBe属于客户端）？已修改，见第585行

  // The server, that is the application which is listening, is installed in the UE
  serverApps.Add (dlPacketSinkLowLat.Install (ueLowLatContainer));
  //serverApps.Add (dlPacketSinkVoice.Install (ueVoiceContainer));//znr_com: 为排查问题，不加载voice应用（测试发现输入输出未减少？？？已取消）//znr_com: （单用户测试）

  /*
   * Configure attributes for the different generators, using user-provided
   * parameters for generating a CBR traffic
   *
   * Low-Latency configuration and object creation:
   */
  UdpClientHelper dlClientLowLat;
  dlClientLowLat.SetAttribute ("RemotePort", UintegerValue (dlPortLowLat));
  dlClientLowLat.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));
  dlClientLowLat.SetAttribute ("PacketSize", UintegerValue (udpPacketSizeULL));
  dlClientLowLat.SetAttribute ("Interval", TimeValue (Seconds (1.0 / lambdaULL)));

  // The bearer that will carry low latency traffic
  EpsBearer lowLatBearer (EpsBearer::NGBR_LOW_LAT_EMBB);

  // The filter for the low-latency traffic
  Ptr<EpcTft> lowLatTft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpfLowLat;
  dlpfLowLat.localPortStart = dlPortLowLat;
  dlpfLowLat.localPortEnd = dlPortLowLat;
  lowLatTft->Add (dlpfLowLat);


  /*
  //znr_com: （单用户测试）
  // Voice configuration and object creation:
  //UdpClientHelper dlClientVoice;//znr_com
  UdpEchoClientHelper dlClientVoice(ueVoiceIpIface.GetAddress (0), dlPortVoice);//znr_add
  
  dlClientVoice.SetAttribute ("RemotePort", UintegerValue (dlPortVoice));
  dlClientVoice.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));
  dlClientVoice.SetAttribute ("PacketSize", UintegerValue (udpPacketSizeBe));
  //dlClientVoice.SetAttribute ("Interval", TimeValue (Seconds (1.0 / lambdaBe)));//znr_com: echoclient不采用高速率
  dlClientVoice.SetAttribute ("Interval", TimeValue (Seconds (0.0005)));//znr_add

  // The bearer that will carry voice traffic
  EpsBearer voiceBearer (EpsBearer::GBR_CONV_VOICE);

  // The filter for the voice traffic
  Ptr<EpcTft> voiceTft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpfVoice;
  dlpfVoice.localPortStart = dlPortVoice;
  dlpfVoice.localPortEnd = dlPortVoice;
  voiceTft->Add (dlpfVoice);
  */

  /*
   * Let's install the applications!
   */
  ApplicationContainer clientApps;

  for (uint32_t i = 0; i < ueLowLatContainer.GetN (); ++i)
    {
      Ptr<Node> ue = ueLowLatContainer.Get (i);
      Ptr<NetDevice> ueDevice = ueLowLatNetDev.Get (i);
      Address ueAddress = ueLowLatIpIface.GetAddress (i);

      // The client, who is transmitting, is installed in the remote host,
      // with destination address set to the address of the UE
      dlClientLowLat.SetAttribute ("RemoteAddress", AddressValue (ueAddress));
      clientApps.Add (dlClientLowLat.Install (remoteHost));

      // Activate a dedicated bearer for the traffic type
      nrHelper->ActivateDedicatedEpsBearer (ueDevice, lowLatBearer, lowLatTft);
    }

  //znr_com: （单用户测试）
  //znr_com---: 为排查问题，暂不加载voice应用（测试发现输入输出未减少？？？已取消）
  /***
  for (uint32_t i = 0; i < ueVoiceContainer.GetN (); ++i)
    {
      Ptr<Node> ue = ueVoiceContainer.Get (i);
      Ptr<NetDevice> ueDevice = ueVoiceNetDev.Get (i);
      Address ueAddress = ueVoiceIpIface.GetAddress (i);

      // The client, who is transmitting, is installed in the remote host,
      // with destination address set to the address of the UE
      dlClientVoice.SetAttribute ("RemoteAddress", AddressValue (ueAddress));
      clientApps.Add (dlClientVoice.Install (remoteHost));

      // Activate a dedicated bearer for the traffic type
      nrHelper->ActivateDedicatedEpsBearer (ueDevice, voiceBearer, voiceTft);
    }
    ***/
    //---znr_com

  // start UDP server and client apps
  serverApps.Start (udpAppStartTime);
  clientApps.Start (udpAppStartTime);
  serverApps.Stop (simTime);
  clientApps.Stop (simTime);

  // enable the traces provided by the nr module
  //nrHelper->EnableTraces();


  FlowMonitorHelper flowmonHelper;
  NodeContainer endpointNodes;
  endpointNodes.Add (remoteHost);
  endpointNodes.Add (gridScenario.GetUserTerminals ());

  Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install (endpointNodes);
  monitor->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
  monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));

  Simulator::Stop (simTime);
  Simulator::Run ();
  //NS_LOG_INFO ("CttcNrDemo go to here!");//zjh_add: 用于确定程序报错位置
  /*
   * To check what was installed in the memory, i.e., BWPs of eNb Device, and its configuration.
   * Example is: Node 1 -> Device 0 -> BandwidthPartMap -> {0,1} BWPs -> NrGnbPhy -> Numerology,
  GtkConfigStore config;
  config.ConfigureAttributes ();
  */

  // Print per-flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

  double averageFlowThroughput = 0.0;
  double averageFlowDelay = 0.0;

  std::ofstream outFile;
  std::string filename = outputDir + "/" + simTag;
  outFile.open (filename.c_str (), std::ofstream::out | std::ofstream::trunc);
  if (!outFile.is_open ())
    {
      std::cerr << "Can't open file " << filename << std::endl;
      return 1;
    }

  outFile.setf (std::ios_base::fixed);

  double flowDuration = (simTime - udpAppStartTime).GetSeconds ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::stringstream protoStream;
      protoStream << (uint16_t) t.protocol;
      if (t.protocol == 6)
        {
          protoStream.str ("TCP");
        }
      if (t.protocol == 17)
        {
          protoStream.str ("UDP");
        }
      outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":" << t.destinationPort << ") proto " << protoStream.str () << "\n";
      outFile << "  Tx Packets: " << i->second.txPackets << "\n";
      outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
      outFile << "  TxOffered:  " << i->second.txBytes * 8.0 / flowDuration / 1000.0 / 1000.0  << " Mbps\n";
      outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      if (i->second.rxPackets > 0)
        {
          // Measure the duration of the flow from receiver's perspective
          averageFlowThroughput += i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000;
          averageFlowDelay += 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;

          outFile << "  Throughput: " << i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000  << " Mbps\n";
          outFile << "  Mean delay:  " << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << " ms\n";
          //outFile << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << " Mbps \n";
          outFile << "  Mean jitter:  " << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets  << " ms\n";
        }
      else
        {
          outFile << "  Throughput:  0 Mbps\n";
          outFile << "  Mean delay:  0 ms\n";
          outFile << "  Mean jitter: 0 ms\n";
        }
      outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
    }

  outFile << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size () << "\n";
  outFile << "  Mean flow delay: " << averageFlowDelay / stats.size () << "\n";

  outFile.close ();

  std::ifstream f (filename.c_str ());

  if (f.is_open ())
    {
      std::cout << f.rdbuf ();
    }

  Simulator::Destroy ();
  return 0;
}

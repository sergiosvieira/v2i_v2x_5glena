/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2024 Federal Institute of Education, Science and Technology of Ceará
// Author: Sérgio Vieira - sergio.vieira@ifce.edu.br
// SPDX-License-Identifier: GPL-2.0-only

#include "mob-utils.h"

#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ideal-beamforming-algorithm.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/lte-sl-tft.h"
#include "ns3/buildings-helper.h"
#include "ns3/stats-module.h"
#include "ns3/udp-header.h"

#include <filesystem>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("5GV2XExample01");

std::vector<node> map_to_vector(const NodeMap& map) {
    std::vector<node> r; r.reserve(map.size());
    for (const auto [key, value]: map) {
        r.push_back(value);
    }
    return r;
}

void sort_node_vector(std::vector<node>& nodes) {
    std::sort(nodes.begin(), nodes.end(), [](const node& a, const node& b){
        return a.id < b.id;
    });
}

ns3::NodeContainer
create_gnb_nodes(const NodeMap& map)
{
    ns3::NodeContainer gnbNodes;
    std::vector<node> nodes = map_to_vector(map);
    sort_node_vector(nodes);
    gnbNodes.Create(map.size());
    MobilityHelper gnbs_mobility;
    gnbs_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> gnbs_pos_allocator = CreateObject<ListPositionAllocator>();
    for (const node& n: nodes) {
        gnbs_pos_allocator->Add(Vector(n.x, n.y, n.z));
    }
    gnbs_mobility.SetPositionAllocator(gnbs_pos_allocator);
    gnbs_mobility.Install(gnbNodes);
    return gnbNodes;
}

ns3::NodeContainer
create_ue_nodes(mob_info& info, std::string& full_filename)
{
    ns3::NodeContainer ueNodes;
    ueNodes.Create(info.nodes);
    Ns2MobilityHelper ns2 = Ns2MobilityHelper(full_filename);
    ns2.Install(ueNodes.Begin(), ueNodes.End());
    return ueNodes;
}

void
logging(bool logging)
{
    if (logging)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        // LogComponentEnable("DistanceBasedThreeGppSpectrumPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppAntennaModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppChannelModel", LOG_LEVEL_INFO);
        // LogComponentEnable("ThreeGppSpectrumPropagationLossModel", LOG_LEVEL_INFO);
        // LogComponentEnable("ThreeGppPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppV2vChannelConditionModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppV2vPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable("NrSpectrumPhy", LOG_LEVEL_ALL);
    }
}

void
parse(ns3::CommandLine& cmd,
      bool& logging,
      std::string& mobilityFile,
      std::string& gnbFile,
      std::string& outputDir,
      uint32_t& seed,
      int argc,
      char* argv[])
{
    cmd.AddValue("logging", "Enable logging", logging);
    cmd.AddValue("mobilityFile", "Mobility file", mobilityFile);
    cmd.AddValue("GNbPositions", "GNb's positions file", gnbFile);
    cmd.AddValue("outputDir", "Directory where to store simulation results", outputDir);
    cmd.AddValue("seed", "Seed value", seed);
    cmd.Parse(argc, argv);
}

void global_config() {
    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(500)));
    // Config::SetDefault("ns3::ThreeGppPropagationLossModel::BuildingPenetrationLossesEnabled ", BooleanValue(false));
}

Ptr<NrPointToPointEpcHelper>
create_EPC_helper() {
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    // Core latency
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));
    return epcHelper;
}

Ptr<NrHelper>
create_5GNR_helper(Ptr<NrPointToPointEpcHelper> epcHelper,
    double tx_power, uint8_t bwpIdForGbrMcptt = 0) {     
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(epcHelper);
    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(500)));
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(true));
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));
    // UE Antenna Attributes
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(tx_power));
    // Sidelink Attributes
    nrHelper->SetUeMacTypeId(NrSlUeMac::GetTypeId());
    nrHelper->SetUeMacAttribute("EnableSensing", BooleanValue(false));
    nrHelper->SetUeMacAttribute("T1", UintegerValue(2));
    nrHelper->SetUeMacAttribute("T2", UintegerValue(33));
    nrHelper->SetUeMacAttribute("ActivePoolId", UintegerValue(0));
    // nrHelper->SetUeMacAttribute("NumSidelinkProcess", UintegerValue(4));
    // nrHelper->SetUeMacAttribute("EnableBlindReTx", BooleanValue(true));
    nrHelper->SetUeMacAttribute("SlThresPsschRsrp", IntegerValue(-128));
    nrHelper->SetBwpManagerTypeId(TypeId::LookupByName("ns3::NrSlBwpManagerUe"));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_MC_PUSH_TO_TALK", 
        UintegerValue(bwpIdForGbrMcptt));    
    return nrHelper;
}

void
show_nodes_info(NodeContainer& gnbNodes, NodeContainer& ueNodes) {
    for(NodeContainer::Iterator n = gnbNodes.Begin(); n != gnbNodes.End(); n++) {
        Ptr<Node> object = *n;
        uint32_t id = object->GetId();
        std::cout << "Node id:"<< id 
                  << " - devices:" << object->GetNDevices()
                  << std::endl;
    } 

    for(NodeContainer::Iterator n = ueNodes.Begin(); n != ueNodes.End(); n++) {
        Ptr<Node> object = *n;
        uint32_t id = object->GetId();
        std::cout << "Node id:"<< id 
                  << " - devices:" << object->GetNDevices()
                  << std::endl;
    } 
}

CcBwpCreator::SimpleOperationBandConf 
create_operation_band() {
    //      Cent.F, ChaBw, #CC, Scenario
    return {5.89e9, 400e6, 1, BandwidthPartInfo::V2V_Urban};    
}

OperationBandInfo 
create_band(CcBwpCreator& creator,
    CcBwpCreator::SimpleOperationBandConf& conf) {
    return creator.CreateOperationBandContiguousCc(conf);
}

Ptr<NrSlHelper>
create_nr_sidelink_helper(Ptr<NrPointToPointEpcHelper> epcHelper) {
    uint8_t mcs = 14;
    Ptr<NrSlHelper> result = CreateObject<NrSlHelper>();
    result->SetEpcHelper(epcHelper);
    /*
     * Set the SL error model and AMC
     * Error model type: ns3::NrEesmCcT1, ns3::NrEesmCcT2, ns3::NrEesmIrT1,
     *                   ns3::NrEesmIrT2, ns3::NrLteMiErrorModel
     * AMC type: NrAmc::ShannonModel or NrAmc::ErrorModel
     */    
    result->SetSlErrorModel("ns3::NrEesmIrT1");
    result->SetUeSlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));
    // result->SetNrSlSchedulerTypeId(NrSlUeMacSchedulerSimple::GetTypeId());
    result->SetNrSlSchedulerTypeId(NrSlUeMacSchedulerFixedMcs::GetTypeId());
    result->SetUeSlSchedulerAttribute("Mcs", UintegerValue(mcs));
    // nrHelper->SetUeMacAttribute("ReservationPeriod", TimeValue(MilliSeconds(100)));
    // result->SetAttribute("ReservationPeriod", TimeValue(MilliSeconds(100)));

    // result->SetUeSlSchedulerAttribute("FixNrSlMcs", BooleanValue(true));
    // result->SetUeSlSchedulerAttribute("InitialNrSlMcs", UintegerValue(14));
    return result;
}

LteRrcSap::SlResourcePoolIdNr
create_pool_id(uint16_t poolId) {
    LteRrcSap::SlResourcePoolIdNr result;
    result.id = poolId;
    return result;
}

LteRrcSap::SlResourcePoolConfigNr
configure_sidelink_pool(LteRrcSap::SlResourcePoolIdNr& slResourcePoolIdNr,
    LteRrcSap::SlResourcePoolNr& slResourcePoolNr) {
    // Pool id, ranges from 0 to 15
    LteRrcSap::SlResourcePoolConfigNr result;
    result.haveSlResourcePoolConfigNr = true;
    result.slResourcePoolId = slResourcePoolIdNr;
    result.slResourcePool = slResourcePoolNr;
    // LteRrcSap::SlBwpPoolConfigCommonNr array_of_sidelink_pools;
    // array_of_sidelink_pools.slTxPoolSelectedNormal[slResourcePoolIdNr.id] = result;    
    return result;
}


Ptr<NrSlCommResourcePoolFactory>
create_preconfigured_sidelink_resource_pool_factory() {
    // Usar factory completamente padrão
    return Create<NrSlCommResourcePoolFactory>();
}

// Ptr<NrSlCommResourcePoolFactory>
// create_preconfigured_sidelink_resource_pool_factory() {
//     auto result = Create<NrSlCommResourcePoolFactory>();
    
//     // Use configuração simplificada baseada nos exemplos oficiais
//     // Sem definir bitmap explicitamente - deixar o sistema calcular
//     result->SetSlSensingWindow(1000);          // 1 segundo
//     result->SetSlSelectionWindow(5);           // 5ms 
//     result->SetSlSubchannelSize(10);           // 10 PRBs
//     result->SetSlMaxNumPerReserve(2);          // Máximo 2 reservas
//     result->SetSlFreqResourcePscch(10);        // PSCCH RBs
    
//     return result;
// }

LteRrcSap::SlBwpPoolConfigCommonNr 
create_array_of_sidelink_pool() {
    LteRrcSap::SlBwpPoolConfigCommonNr result;
    return result;
}

void 
insert_pool_in_array(LteRrcSap::SlBwpPoolConfigCommonNr& array,
    uint16_t id, LteRrcSap::SlResourcePoolConfigNr& slResourcePoolConfigNr) {
    array.slTxPoolSelectedNormal[id] = slResourcePoolConfigNr;
}

LteRrcSap::Bwp 
create_bwp_information_element(uint16_t numerology = 0,
    uint16_t symbolsPerSlots = 14,
    uint8_t rbPerRbg = 1,
    uint32_t bandwidth = 400) {
    LteRrcSap::Bwp result;
    result.numerology = numerology;
    result.symbolsPerSlots = symbolsPerSlots;
    result.rbPerRbg = rbPerRbg;
    result.bandwidth = bandwidth;
    return result;
};


LteRrcSap::SlBwpGeneric 
create_bwp_generic(LteRrcSap::Bwp& bwp,
    uint16_t symbolsPerSlots = 14) {
    LteRrcSap::SlBwpGeneric result;
    result.bwp = bwp;
    // result.slLengthSymbols = LteRrcSap::GetSlLengthSymbolsEnum(symbolsPerSlots);
    // result.slStartSymbol = LteRrcSap::GetSlStartSymbolEnum(0);
    result.slLengthSymbols = LteRrcSap::GetSlLengthSymbolsEnum(symbolsPerSlots);
    result.slStartSymbol = LteRrcSap::GetSlStartSymbolEnum(0);
    return result;
}

LteRrcSap::SlBwpConfigCommonNr 
create_bwp_config_common(LteRrcSap::SlBwpGeneric& slBwpGeneric,
    LteRrcSap::SlBwpPoolConfigCommonNr&  array_of_pools) {
    LteRrcSap::SlBwpConfigCommonNr result;
    result.haveSlBwpGeneric = true;
    result.slBwpGeneric = slBwpGeneric;
    result.haveSlBwpPoolConfigCommonNr = true;
    result.slBwpPoolConfigCommonNr = array_of_pools;    
    return result;
}

LteRrcSap::SlFreqConfigCommonNr 
create_sidelink_frequency_config(std::set<uint8_t>& bwpIdContainer,
    LteRrcSap::SlBwpConfigCommonNr& slBwpConfigCommonNr) {
    LteRrcSap::SlFreqConfigCommonNr result;
    for (const auto& it : bwpIdContainer) {
        result.slBwpList[it] = slBwpConfigCommonNr;
    }
    return result;
}

LteRrcSap::TddUlDlConfigCommon
create_tdd_uplink_downlink_config(const std::string& pattern = "DL|DL|DL|F|UL|UL|UL|UL|UL|UL|") {
    LteRrcSap::TddUlDlConfigCommon result;
    result.tddPattern = pattern;
    return result;
}

LteRrcSap::SlPreconfigGeneralNr 
create_sidelink_general_config(LteRrcSap::TddUlDlConfigCommon& tddUlDlConfigCommon) {
    LteRrcSap::SlPreconfigGeneralNr result;
    result.slTddConfig = tddUlDlConfigCommon;
    return result;
}

LteRrcSap::SlPsschTxParameters
create_physical_sidelink_shared_channel_parameters(uint8_t slMaxTxTransNumPssch = 5) {
    // Indicates the maximum transmission number (including new transmission and
    // retransmission) for PSSCH.                                           
    LteRrcSap::SlPsschTxParameters result;
    result.slMaxTxTransNumPssch = slMaxTxTransNumPssch;
    return result;
}

LteRrcSap::SlPsschTxConfigList
create_physical_sidelink_shared_tx_config_list(uint16_t index,
     LteRrcSap::SlPsschTxParameters& psschParams) {
    LteRrcSap::SlPsschTxConfigList result;
    result.slPsschTxParameters[index] = psschParams;
    return result;
}

LteRrcSap::SlUeSelectedConfig
create_sidelink_ue_selected_config(double probability,
    LteRrcSap::SlPsschTxConfigList& pscchTxConfigList) {
    /* Indicates the probability with which the UE keeps the current resource when
       the resource reselection counter reaches zero for sensing based UE 
       autonomous resource selection (see TS 38.321). Standard values for this 
       parameter are 0, 0.2, 0.4, 0.6, 0.8, however, in the simulator we are not 
       restricting it to evaluate other values. */
    LteRrcSap::SlUeSelectedConfig result;
    result.slProbResourceKeep = probability;
    result.slPsschTxConfigList = pscchTxConfigList;
    return result;
}

LteRrcSap::SidelinkPreconfigNr
create_sidelink_preconfig(LteRrcSap::SlPreconfigGeneralNr& slPreconfigGeneralNr,
    LteRrcSap::SlUeSelectedConfig& slUeSelectedPreConfig,
    uint16_t index,
    LteRrcSap::SlFreqConfigCommonNr& slFreConfigCommonNr) {
    LteRrcSap::SidelinkPreconfigNr result;
    result.slPreconfigGeneral = slPreconfigGeneralNr;
    result.slUeSelectedPreConfig = slUeSelectedPreConfig;
    result.slPreconfigFreqInfoList[index] = slFreConfigCommonNr;
    return result;
}

void 
set_default_gatway(Ptr<NrPointToPointEpcHelper> epc_helper,
    NodeContainer& ue_nodes) {
    // set the default gateway for the UE
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    for (uint32_t u = 0; u < ue_nodes.GetN(); ++u) {
        Ptr<Node> ueNode = ue_nodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epc_helper->GetUeDefaultGatewayAddress(), 1);
    }
}

void
TxCallback(Ptr<CounterCalculator<uint32_t>> datac, 
std::string path, 
Ptr<const Packet> packet)
{
    NS_LOG_INFO("Sent frame counted in " << datac->GetKey());
    datac->Update();
}

uint32_t rxByteCounter = 0; //!< Global variable to count RX bytes
uint32_t txByteCounter = 0; //!< Global variable to count TX bytes
uint32_t rxPktCounter = 0;  //!< Global variable to count RX packets
uint32_t txPktCounter = 0;  //!< Global variable to count TX packets

void
ReceivePacket(Ptr<const Packet> packet, const Address& addr)
{    
    std::cout << "Received a Packet of size: " << packet->GetSize() 
        << " at time " << Now().GetSeconds()
        << " from " << InetSocketAddress::ConvertFrom(addr).GetIpv4() << '\n' ;
    rxByteCounter += packet->GetSize();
    rxPktCounter++;
}

void
TransmitPacket(Ptr<const Packet> packet)
{
    std::cout << "Sent Packet of size " << packet->GetSize() << '\n';
    txByteCounter += packet->GetSize();
    txPktCounter++;
}

int 
main(int argc, char* argv[]) 
{
    bool log = true; logging(log);
    std::filesystem::path home = "./scratch";
    std::filesystem::path mobility_path = home / "mob";
    std::string outputDir = "./";
    std::string mobilityFile = "urban-low.tcl";
    std::string gnbPositionFile = "001-gnb.tcl";
    uint32_t seed = 1;
    double tx_power = 23; // dBm

    /* Parsing */
    CommandLine cmd(__FILE__);
    parse(cmd, log, 
        mobilityFile, 
        gnbPositionFile, 
        outputDir, seed, argc, argv);

    /* Global Configurations */
    global_config();

    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(1);

    /* Mobility and Positioning */
    std::string full_filename = mobility_path / mobilityFile;
    std::cout << "Loading node's mobility: " << full_filename << '\n';
    mob_info info = get_mob_info(full_filename);
    std::cout << get_mob_info_str(info) << '\n';
    std::string full_gnb_filename = mobility_path / gnbPositionFile;
    std::cout << "Loading GNb's positions: " << full_gnb_filename << '\n';
    NodeMap node_map = make_nodes_from_file(full_gnb_filename);
    NodeContainer gnb_nodes = create_gnb_nodes(node_map);
    NodeContainer ue_nodes = create_ue_nodes(info, full_filename);
    BuildingsHelper::Install(ue_nodes);
    Ptr<NrPointToPointEpcHelper> epc_helper = create_EPC_helper();
    uint8_t bwp_id_for_gbr_mcptt = 0;
    Ptr<NrHelper> nr_helper = create_5GNR_helper(epc_helper, tx_power, bwp_id_for_gbr_mcptt);
    // show_nodes_info(gnbNodes, ueNodes);

    /* Simulation Time */
    Time simulation_time = Seconds(info.end_time - info.start_time);
    Time sidelink_bearers_activation_time = Seconds(info.start_time);
    Time final_sidelink_bearers_activation_time = sidelink_bearers_activation_time + Seconds(0.01);
    // Time final_simulation_time = simulation_time + final_sidelink_bearers_activation_time;
    Time final_simulation_time = simulation_time + Seconds(1);

    /* Configure Bands */
    CcBwpCreator bwp_creator;
    OperationBandInfo band_01 = bwp_creator.CreateOperationBandContiguousCc(
        create_operation_band()
    );
    nr_helper->InitializeOperationBand(&band_01);
    BandwidthPartInfoPtrVector all_bwps = CcBwpCreator::GetAllBwps({band_01});

    NetDeviceContainer gnbNetDevices = nr_helper->InstallGnbDevice(gnb_nodes, all_bwps);
    for (auto it = gnbNetDevices.Begin(); it != gnbNetDevices.End(); ++it) {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }


    /* Configure and update UEs */
    NetDeviceContainer ue_devices = nr_helper->InstallUeDevice(ue_nodes, all_bwps);
    for (auto it = ue_devices.Begin(); it != ue_devices.End(); ++it) {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }

    /* -- Sidelink configuration -- */
    Ptr<NrSlHelper> nr_sidelink_helper = create_nr_sidelink_helper(epc_helper);
    std::set<uint8_t> bwp_id_set = {bwp_id_for_gbr_mcptt};
    nr_sidelink_helper->PrepareUeForSidelink(ue_devices, bwp_id_set);
    /*  ---- Sidelink resource pool configuration ---- */
    // auto sidelink_pool_factory = create_preconfigure_sidelink_resource_pool_factory();
    auto sidelink_pool = create_preconfigured_sidelink_resource_pool_factory()->CreatePool();
    auto sidelink_pool_id = create_pool_id(0);
    auto sidelink_pool_config = configure_sidelink_pool(sidelink_pool_id, sidelink_pool);
    LteRrcSap::SlBwpPoolConfigCommonNr array_of_sidelink_pool = create_array_of_sidelink_pool();
    insert_pool_in_array(array_of_sidelink_pool, sidelink_pool_id.id, sidelink_pool_config);
    auto bwp = create_bwp_information_element();
    auto bwp_generic = create_bwp_generic(bwp);
    auto bwp_config = create_bwp_config_common(bwp_generic, array_of_sidelink_pool);
    auto sidelink_frequency_config = create_sidelink_frequency_config(bwp_id_set, bwp_config);
    auto tdd_uplink_downlink_config = create_tdd_uplink_downlink_config();
    auto sidelink_general_config = create_sidelink_general_config(tdd_uplink_downlink_config);
    auto pssch_params =  create_physical_sidelink_shared_channel_parameters();
    auto pssch_tx_config = create_physical_sidelink_shared_tx_config_list(0, pssch_params);
    auto sidelink_ue_selected_config = create_sidelink_ue_selected_config(0.0, pssch_tx_config);
    auto sidelink_preconfig = create_sidelink_preconfig(
        sidelink_general_config,
        sidelink_ue_selected_config,
        0,
        sidelink_frequency_config
    );
    nr_sidelink_helper->InstallNrSlPreConfiguration(ue_devices, sidelink_preconfig);
    /****************************** End SL Configuration ***********************/

    /* Configure the IP stack */    
    InternetStackHelper internet_stack_helper;
    internet_stack_helper.Install(ue_nodes);
    auto ue_ipv4_interfaces = epc_helper->AssignUeIpv4Address(ue_devices);
    set_default_gatway(epc_helper, ue_nodes);

    /* Configure IPV4 Addresses */
    uint16_t port = 1978;
    Ipv4Address multicast_ipv4_addr("225.0.0.0");
    Address remote_addr = InetSocketAddress(multicast_ipv4_addr, port);
    Address local_addr = InetSocketAddress(Ipv4Address::GetAny(), port);

    /* Configure Sidelink Bearers */
    uint32_t dst_layer2_id = 255;
    uint16_t reservationPeriod = 70; // in ms
    bool harqEnabled = true;
    Time delayBudget = Seconds(0); // Use T2 configuration

    SidelinkInfo slInfo;
    slInfo.m_castType = SidelinkInfo::CastType::Groupcast;
    slInfo.m_dstL2Id = dst_layer2_id;
    slInfo.m_rri = MilliSeconds(reservationPeriod);
    slInfo.m_dynamic = false;
    slInfo.m_pdb = delayBudget;
    slInfo.m_harqEnabled = harqEnabled;
    auto lte_sidelink_traffic_flow_template = Create<LteSlTft>(
        LteSlTft::Direction::BIDIRECTIONAL, 
        multicast_ipv4_addr,
        slInfo);
    nr_sidelink_helper->ActivateNrSlBearer(final_sidelink_bearers_activation_time, 
        ue_devices, 
        lte_sidelink_traffic_flow_template);

    /* Configure client application */
    uint32_t udp_packet_size = 200;
    double data_rate = 16; // 16 kilobits per second
    OnOffHelper sidelink_client("ns3::UdpSocketFactory", remote_addr);
    sidelink_client.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string data_rate_str = std::to_string(data_rate) + "kb/s";
    std::cout << "Data rate " << DataRate(data_rate_str) << '\n';
    sidelink_client.SetConstantRate(DataRate(data_rate_str), udp_packet_size);
    ApplicationContainer client_apps = sidelink_client.Install(ue_nodes.Get(0));
    client_apps.Start(final_sidelink_bearers_activation_time);
    client_apps.Stop(final_simulation_time);

    /* Output app start, stop and duration */
    double realAppStart = final_sidelink_bearers_activation_time.GetSeconds() +
        ((double)udp_packet_size * 8.0 / (DataRate(data_rate_str).GetBitRate()));
    double appStopTime = (final_simulation_time).GetSeconds();
    std::cout << "App start time at " << realAppStart << " sec" << std::endl;
    std::cout << "App stop time at " << appStopTime << " sec" << std::endl;

    /* Configure server application */
    PacketSinkHelper sidelink_sink("ns3::UdpSocketFactory", local_addr);
    sidelink_sink.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    ApplicationContainer server_apps = sidelink_sink.Install(ue_nodes.Get(ue_nodes.GetN() - 1));
    server_apps.Start(sidelink_bearers_activation_time);

    /* Statistics */
    std::ostringstream path;
    path << "/NodeList/" << ue_nodes.Get(ue_nodes.GetN() - 1)->GetId()
         << "/ApplicationList/0/$ns3::PacketSink/Rx";
    Config::ConnectWithoutContext(path.str(), MakeCallback(&ReceivePacket));
    path.str("");

    path << "/NodeList/" << ue_nodes.Get(0)->GetId()
         << "/ApplicationList/0/$ns3::OnOffApplication/Tx";
    Config::ConnectWithoutContext(path.str(), MakeCallback(&TransmitPacket));
    path.str("");


    /* Start Simulation */
    Simulator::Stop(final_simulation_time);
    ShowProgress progress (Seconds (50), std::cerr);
    Simulator::Run();

    std::cout << "Total Tx bits = " << txByteCounter * 8 << std::endl;
    std::cout << "Total Tx packets = " << txPktCounter << std::endl;

    std::cout << "Total Rx bits = " << rxByteCounter * 8 << std::endl;
    std::cout << "Total Rx packets = " << rxPktCounter << std::endl;

    std::cout << "Avrg thput = "
              << (rxByteCounter * 8) / (final_simulation_time - Seconds(realAppStart)).GetSeconds() / 1000.0
              << " kbps" << std::endl;



    /* End Simulation */
    Simulator::Destroy();

    return 0;
}

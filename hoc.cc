#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdHocNetwork");

int main(int argc, char *argv[]) {

    // 设置时间分辨率
    Time::SetResolution(Time::NS);

    // 激活日志组件
    LogComponentEnable("AdHocNetwork", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_ALL);

    // ad hoc 设备数量
    uint32_t nAdHoc = 6;

    // 命令行参数
    CommandLine cmd;
    cmd.AddValue("nAdHoc", "Number of WiFi AD devices", nAdHoc);
    cmd.Parse(argc, argv);

    // 节点
    NodeContainer adHocNodes;
    adHocNodes.Create(nAdHoc);

    // 创建信道和物理信息
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    // 创建 WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode",
        StringValue("OfdmRate6Mbps")
    );

    // 设置 mac
    WifiMacHelper mac;
    mac.SetType(
        "ns3::AdhocWifiMac",
        "Slot",
        StringValue("0.1s")
    );

    // 创建设备
    NetDeviceContainer adHocDevices;
    adHocDevices = wifi.Install(phy, mac, adHocNodes);

    // 位置信息
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(5.0),
        "GridWidth", UintegerValue(10),
        "LayoutType", StringValue("RowFirst")
    );
    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",
        RectangleValue(
            Rectangle(-500, 500, -500, 500)
        )
    );
    mobility.Install(adHocNodes);

    // 安装 internet 协议栈
    InternetStackHelper internet;
    internet.Install(adHocNodes);

    // 设置 ip 地址
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interface;
    interface = address.Assign(adHocDevices);

    // 应用层
    NS_LOG_INFO("Create Applications");
    uint16_t port = 9999;
    OnOffHelper onOff1(
        "ns3::TcpSocketFactory",
        Address(InetSocketAddress(interface.GetAddress(0), port))
    );
    onOff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer apps1 = onOff1.Install(adHocNodes);
    apps1.Start(Seconds(1.0));
    apps1.Stop(Seconds(15.0));

    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        Address(InetSocketAddress(Ipv4Address::GetAny(), port))
    );
    ApplicationContainer apps2 = sinkHelper.Install(adHocNodes.Get(0));

    apps2.Start(Seconds(1.0));
    apps2.Stop(Seconds(15.0));

    // 全局路由表
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // trace
    AsciiTraceHelper ascii;
    phy.EnableAsciiAll(ascii.CreateFileStream("hoc.tr"));

    // 先停止模拟器
    Simulator::Stop(Seconds(15.0));

    // pcap
    phy.EnablePcap("hoc", adHocDevices.Get(0));

    // NetAnim
    AnimationInterface anim("hoc.xml");

    // 启动模拟器
    Simulator::Run();
    Simulator::Destroy();

    return 0;

}

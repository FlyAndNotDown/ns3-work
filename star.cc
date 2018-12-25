#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

#include <iostream>

using namespace ns3;
using namespace std;

// 定义组件名
NS_LOG_COMPONENT_DEFINE("ApStarNetwork");

// main函数
int main(int argc, char *argv[]) {

    // sta设备数量
    uint32_t nSta = 6;

    // 定义 CommandLine 对象，以供外部传入参数
    CommandLine cmd;
    cmd.AddValue("nSta", "Number of Wifi STA devices", nSta);
    cmd.Parse(argc, argv);

    // 参数校验
    if (nSta < 5 || nSta > 20) {
        cout << "Number of STA devices must between 5 and 20" << endl;
        return 1;
    }

    // 启动 udpEchoService 日志
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);

    // 创建多个 STA 节点和一个 AP 节点
    NodeContainer staNodes;
    staNodes.Create(nSta);
    NodeContainer apNodes;
    apNodes.Create(1);

    // 设置通信信道和物理层信息
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    // 配置 wifi 使用 AARF 速率控制算法
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // 配置 mac 和 ssid 并且安装 STA 和 AP 设备
    WifiMacHelper mac;
    Ssid ssid = Ssid("ns3-ssid");
    mac.SetType(
        "ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(false)
    );
    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, staNodes);
    mac.SetType(
        "ns3::ApWifiMac",
        "Ssid", SsidValue(ssid)
    );
    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, apNodes);

    // 设置 wifi 位置信息
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst")
    );
    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50))
    );
    mobility.Install(staNodes);
    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel"
    );
    mobility.Install(apNodes);

    // 安装 internet 协议栈
    InternetStackHelper stack;
    stack.Install(staNodes);
    stack.Install(apNodes);

    // 设置 IP 地址
    Ipv4AddressHelper address;
    address.SetBase(
        "10.1.1.0",
        "255.255.255.0"
    );
    Ipv4InterfaceContainer interface;
    interface = address.Assign(staDevices);
    address.Assign(apDevices);

    // 安装 echo 服务
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(staNodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));
    UdpEchoClientHelper echoClient(interface.GetAddress(0), 0);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5000));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(staNodes.Get(nSta - 1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // 设置路由表
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 先停止模拟器
    Simulator::Stop(Seconds(10.0));

    // 开启 pcap 记录
    phy.EnablePcap("star", apDevices.Get(0));

    // NetAnim
    AnimationInterface anim("star-anim.xml");

    // trace
    AsciiTraceHelper asciiTraceHelper;
    phy.EnableAsciiAll(asciiTraceHelper.CreateFileStream("star.tr"));

    // 启动模拟器
    Simulator::Run();
    Simulator::Destroy();

    return 0;

}

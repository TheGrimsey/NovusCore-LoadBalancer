#pragma once
#include <memory>

class NetPacketHandler;
class NetClient;
struct NetPacket;
namespace InternalSocket
{
    class GeneralHandlers
    {
    public:
        static void Setup(NetPacketHandler*);
        static bool HandleConnected(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleRequestAddress(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleFullServerInfoUpdate(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleServerInfoAdd(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleServerInfoRemove(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
    };
}
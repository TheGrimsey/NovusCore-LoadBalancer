#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>

struct NetPacket;
class NetClient;

struct ConnectionSingleton
{
    ConnectionSingleton() : packetQueue(256) { }

    std::shared_ptr<NetClient> netClient;
    bool didHandleDisconnect = false;

    moodycamel::ConcurrentQueue<std::shared_ptr<NetPacket>> packetQueue;
};
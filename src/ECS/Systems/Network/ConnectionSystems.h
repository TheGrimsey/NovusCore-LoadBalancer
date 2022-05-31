#pragma once
#include <entity/fwd.hpp>

class NetClient;

class ConnectionUpdateSystem
{
public:
    static void Update(entt::registry& registry);

    // Handlers for Network Client
    static void HandleRead(std::shared_ptr<NetClient> netClient);
    static void HandleConnect(std::shared_ptr<NetClient> netClient, bool connected);
    static void HandleDisconnect(std::shared_ptr<NetClient> netClient);
};
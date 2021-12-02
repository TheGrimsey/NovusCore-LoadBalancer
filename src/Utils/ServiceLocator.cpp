#include "ServiceLocator.h"
#include <Networking/NetPacketHandler.h>

entt::registry* ServiceLocator::_gameRegistry = nullptr;
NetPacketHandler* ServiceLocator::_netPacketHandler = nullptr;

void ServiceLocator::SetRegistry(entt::registry* registry)
{
    assert(_gameRegistry == nullptr);
    _gameRegistry = registry;
}
void ServiceLocator::SetNetPacketHandler(NetPacketHandler* netPacketHandler)
{
    assert(_netPacketHandler == nullptr);
    _netPacketHandler = netPacketHandler;
}
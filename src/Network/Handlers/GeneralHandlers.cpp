#include "GeneralHandlers.h"
#include <Networking/NetStructures.h>
#include <Networking/NetPacket.h>
#include <Networking/NetClient.h>
#include <Networking/NetPacketHandler.h>
#include <Networking/PacketUtils.h>
#include "../../Utils/ServiceLocator.h"
#include "../../ECS/Components/Network/LoadBalanceSingleton.h"

namespace InternalSocket
{
    void GeneralHandlers::Setup(NetPacketHandler* netPacketHandler)
    {
        netPacketHandler->SetMessageHandler(Opcode::SMSG_CONNECTED, { ConnectionStatus::AUTH_SUCCESS, 0, GeneralHandlers::HandleConnected });
        netPacketHandler->SetMessageHandler(Opcode::MSG_REQUEST_ADDRESS, { ConnectionStatus::CONNECTED, sizeof(AddressType), 128, GeneralHandlers::HandleRequestAddress });
        netPacketHandler->SetMessageHandler(Opcode::SMSG_SEND_FULL_INTERNAL_SERVER_INFO, { ConnectionStatus::CONNECTED, sizeof(ServerInformation), 8192, GeneralHandlers::HandleFullServerInfoUpdate });
        netPacketHandler->SetMessageHandler(Opcode::SMSG_SEND_ADD_INTERNAL_SERVER_INFO, { ConnectionStatus::CONNECTED, sizeof(ServerInformation), GeneralHandlers::HandleServerInfoAdd });
        netPacketHandler->SetMessageHandler(Opcode::SMSG_SEND_REMOVE_INTERNAL_SERVER_INFO, { ConnectionStatus::CONNECTED, sizeof(entt::entity) + sizeof(AddressType) + sizeof(u8), GeneralHandlers::HandleServerInfoRemove});
    }

    bool GeneralHandlers::HandleConnected(std::shared_ptr<NetClient> netClient, std::shared_ptr<NetPacket> packet)
    {
        netClient->SetConnectionStatus(ConnectionStatus::CONNECTED);

        return true;
    }
    bool GeneralHandlers::HandleRequestAddress(std::shared_ptr<NetClient> netClient, std::shared_ptr<NetPacket> packet)
    {
        // Validate that we did get an AddressType and that it is valid
        AddressType requestType;
        if (!packet->payload->Get(requestType) ||
            (requestType < AddressType::AUTH || requestType >= AddressType::COUNT))
        {
            return false;
        }

        entt::registry* registry = ServiceLocator::GetRegistry();
        auto& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();

        ServerInformation serverInformation;
        if (requestType == AddressType::AUTH)
        {
            loadBalanceSingleton.Get<AddressType::AUTH>(serverInformation);
        }
        else if (requestType == AddressType::REALM)
        {
            loadBalanceSingleton.Get<AddressType::REALM>(serverInformation);
        }
        else if (requestType == AddressType::WORLD)
        {
            loadBalanceSingleton.Get<AddressType::WORLD>(serverInformation);
        }
        else if (requestType == AddressType::INSTANCE)
        {
            loadBalanceSingleton.Get<AddressType::INSTANCE>(serverInformation);
        }
        else if (requestType == AddressType::CHAT)
        {
            loadBalanceSingleton.Get<AddressType::CHAT>(serverInformation);
        }
        else if (requestType == AddressType::LOADBALANCE)
        {
            loadBalanceSingleton.Get<AddressType::LOADBALANCE>(serverInformation);
        }
        else if (requestType == AddressType::REGION)
        {
            loadBalanceSingleton.Get<AddressType::REGION>(serverInformation);
        }

        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
        u8 status = 1;

        // If the load balancer couldn't find a valid server, we send status 0 back
        if (serverInformation.type == AddressType::INVALID)
        {
            status = 0;
        }

        if (!PacketUtils::Write_SMSG_SEND_ADDRESS(buffer, status, serverInformation.address, serverInformation.port, packet->payload->GetReadPointer(), packet->payload->GetReadSpace()))
            return false;

        netClient->Send(buffer);
        return true;
    }
    bool GeneralHandlers::HandleFullServerInfoUpdate(std::shared_ptr<NetClient> netClient, std::shared_ptr<NetPacket> packet)
    {
        entt::registry* registry = ServiceLocator::GetRegistry();
        LoadBalanceSingleton& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();

        // Clear Cache
        loadBalanceSingleton.Clear();

        ServerInformation serverInformation;
        while (packet->payload->GetReadSpace())
        {
            if (!packet->payload->Get(serverInformation.entity))
                return false;

            if (!packet->payload->Get(serverInformation.type) ||
                (serverInformation.type < AddressType::AUTH || serverInformation.type >= AddressType::COUNT))
            {
                return false;
            }

            if (!packet->payload->GetU8(serverInformation.realmId))
                return false;

            if (!packet->payload->GetU32(serverInformation.address))
                return false;

            if (!packet->payload->GetU16(serverInformation.port))
                return false;

            if (serverInformation.type == AddressType::AUTH)
            {
                loadBalanceSingleton.Add<AddressType::AUTH>(serverInformation);
            }
            else if (serverInformation.type == AddressType::REALM)
            {
                loadBalanceSingleton.Add<AddressType::REALM>(serverInformation);
            }
            else if (serverInformation.type == AddressType::WORLD)
            {
                loadBalanceSingleton.Add<AddressType::WORLD>(serverInformation);
            }
            else if (serverInformation.type == AddressType::INSTANCE)
            {
                loadBalanceSingleton.Add<AddressType::INSTANCE>(serverInformation);
            }
            else if (serverInformation.type == AddressType::CHAT)
            {
                loadBalanceSingleton.Add<AddressType::CHAT>(serverInformation);
            }
            else if (serverInformation.type == AddressType::LOADBALANCE)
            {
                loadBalanceSingleton.Add<AddressType::LOADBALANCE>(serverInformation);
            }
            else if (serverInformation.type == AddressType::REGION)
            {
                loadBalanceSingleton.Add<AddressType::REGION>(serverInformation);
            }
        }

        return true;
    }
    bool GeneralHandlers::HandleServerInfoAdd(std::shared_ptr<NetClient> netClient, std::shared_ptr<NetPacket> packet)
    {
        entt::registry* registry = ServiceLocator::GetRegistry();
        LoadBalanceSingleton& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();

        ServerInformation serverInformation;

        if (!packet->payload->Get(serverInformation.entity))
            return false;

        if (!packet->payload->Get(serverInformation.type) ||
            (serverInformation.type < AddressType::AUTH || serverInformation.type >= AddressType::COUNT))
        {
            return false;
        }

        if (!packet->payload->GetU8(serverInformation.realmId))
            return false;

        if (!packet->payload->GetU32(serverInformation.address))
            return false;

        if (!packet->payload->GetU16(serverInformation.port))
            return false;

        if (serverInformation.type == AddressType::AUTH)
        {
            loadBalanceSingleton.Add<AddressType::AUTH>(serverInformation);
        }
        else if (serverInformation.type == AddressType::REALM)
        {
            loadBalanceSingleton.Add<AddressType::REALM>(serverInformation);
        }
        else if (serverInformation.type == AddressType::WORLD)
        {
            loadBalanceSingleton.Add<AddressType::WORLD>(serverInformation);
        }
        else if (serverInformation.type == AddressType::INSTANCE)
        {
            loadBalanceSingleton.Add<AddressType::INSTANCE>(serverInformation);
        }
        else if (serverInformation.type == AddressType::CHAT)
        {
            loadBalanceSingleton.Add<AddressType::CHAT>(serverInformation);
        }
        else if (serverInformation.type == AddressType::LOADBALANCE)
        {
            loadBalanceSingleton.Add<AddressType::LOADBALANCE>(serverInformation);
        }
        else if (serverInformation.type == AddressType::REGION)
        {
            loadBalanceSingleton.Add<AddressType::REGION>(serverInformation);
        }

        return true;
    }
    bool GeneralHandlers::HandleServerInfoRemove(std::shared_ptr<NetClient> netClient, std::shared_ptr<NetPacket> packet)
    {
        entt::registry* registry = ServiceLocator::GetRegistry();
        LoadBalanceSingleton& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();

        entt::entity entity = entt::null;
        AddressType type = AddressType::INVALID;
        u8 realmId = 0;

        if (!packet->payload->Get(entity))
            return false;

        if (!packet->payload->Get(type) ||
            (type < AddressType::AUTH || type >= AddressType::COUNT))
        {
            return false;
        }

        if (!packet->payload->GetU8(realmId))
            return false;

        loadBalanceSingleton.Remove(type, entity, realmId);

        return true;
    }
}
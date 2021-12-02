#include "ConnectionSystems.h"
#include <entt.hpp>
#include <Utils/DebugHandler.h>
#include <Networking/Netpacket.h>
#include <Networking/NetClient.h>
#include <Networking/NetPacketHandler.h>
#include "../../Components/Network/ConnectionSingleton.h"
#include "../../Components/Network/AuthenticationSingleton.h"
#include "../../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void ConnectionUpdateSystem::Update(entt::registry& registry)
{
    ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue)
    ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();

    if (connectionSingleton.netClient)
    {
        if (connectionSingleton.netClient->Read())
        {
            HandleRead(connectionSingleton.netClient);
        }

        if (!connectionSingleton.netClient->IsConnected())
        {
            if (!connectionSingleton.didHandleDisconnect)
            {
                connectionSingleton.didHandleDisconnect = true;

                HandleDisconnect(connectionSingleton.netClient);
            }

            return;
        }

        std::shared_ptr<NetPacket> packet = nullptr;

        NetPacketHandler* netPacketHandler = ServiceLocator::GetNetPacketHandler();
        while (connectionSingleton.packetQueue.try_dequeue(packet))
        {
#ifdef NC_Debug
            DebugHandler::PrintSuccess("[Network/Socket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

            if (!netPacketHandler->CallHandler(connectionSingleton.netClient, packet))
            {
                connectionSingleton.netClient->Close();
                break;
            }
        }
    }
}

void ConnectionUpdateSystem::HandleConnect(std::shared_ptr<NetClient> netClient, bool connected)
{
    if (connected)
    {
#ifdef NC_Debug
        const NetSocket::ConnectionInfo& connectionInfo = netClient->GetSocket()->GetConnectionInfo();
        DebugHandler::PrintSuccess("[Network/Socket]: Successfully connected to (%s, %u)", connectionInfo.ipAddrStr.c_str(), connectionInfo.port);
#endif // NC_Debug

        entt::registry* registry = ServiceLocator::GetRegistry();
        AuthenticationSingleton& authentication = registry->ctx<AuthenticationSingleton>();
        ConnectionSingleton& connectionSingleton = registry->ctx<ConnectionSingleton>();
        
        /* Send Initial Packet */
        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<512>();

        authentication.srp.username = "loadbalancer";
        authentication.srp.password = "password";
        connectionSingleton.didHandleDisconnect = false;

        // If StartAuthentication fails, it means A failed to generate and thus we cannot connect
        if (!authentication.srp.StartAuthentication())
            return;

        buffer->Put(Opcode::CMSG_LOGON_CHALLENGE);
        buffer->SkipWrite(sizeof(u16));

        u16 size = static_cast<u16>(buffer->writtenData);
        buffer->PutString(authentication.srp.username);
        buffer->PutBytes(authentication.srp.aBuffer->GetDataPointer(), authentication.srp.aBuffer->size);

        u16 writtenData = static_cast<u16>(buffer->writtenData) - size;

        buffer->Put<u16>(writtenData, 2);
        netClient->Send(buffer);

        netClient->SetConnectionStatus(ConnectionStatus::AUTH_CHALLENGE);
    }
    else
    {
#ifdef NC_Debug
        const NetSocket::ConnectionInfo& connectionInfo = netClient->GetSocket()->GetConnectionInfo();
        DebugHandler::PrintWarning("[Network/Socket]: Failed to connect to (%s, %u)", connectionInfo.ipAddrStr.c_str(), connectionInfo.port);
#endif // NC_Debug
    }
}
void ConnectionUpdateSystem::HandleRead(std::shared_ptr<NetClient> netClient)
{
    entt::registry* registry = ServiceLocator::GetRegistry();
    ConnectionSingleton& connectionSingleton = registry->ctx<ConnectionSingleton>();

    std::shared_ptr<Bytebuffer> buffer = netClient->GetReadBuffer();

    while (size_t activeSize = buffer->GetActiveSize())
    {
        // We have received a partial header and need to read more
        if (activeSize < sizeof(PacketHeader))
        {
            buffer->Normalize();
            break;
        }

        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer->GetReadPointer());

        if (header->opcode == Opcode::INVALID || header->opcode > Opcode::MAX_COUNT)
        {
#ifdef NC_Debug
            DebugHandler::PrintError("Received Invalid Opcode (%u) from network stream", static_cast<u16>(header->opcode));
#endif // NC_Debug
            break;
        }

        if (header->size > 8192)
        {
#ifdef NC_Debug
            DebugHandler::PrintError("Received Invalid Opcode Size (%u) from network stream", header->size);
#endif // NC_Debug
            break;
        }

        size_t sizeWithoutHeader = activeSize - sizeof(PacketHeader);

        // We have received a valid header, but we have yet to receive the entire payload
        if (sizeWithoutHeader < header->size)
        {
            buffer->Normalize();
            break;
        }

        // Skip Header
        buffer->SkipRead(sizeof(PacketHeader));

        std::shared_ptr<NetPacket> packet = NetPacket::Borrow();
        {
            // Header
            {
                packet->header = *header;
            }

            // Payload
            {
                if (packet->header.size)
                {
                    packet->payload = Bytebuffer::Borrow<8192/*NETWORK_BUFFER_SIZE*/ >();
                    packet->payload->size = packet->header.size;
                    packet->payload->writtenData = packet->header.size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), packet->header.size);

                    // Skip Payload
                    buffer->SkipRead(header->size);
                }
            }

            connectionSingleton.packetQueue.enqueue(packet);
        }
    }

    // Only reset if we read everything that was written
    if (buffer->GetActiveSize() == 0)
    {
        buffer->Reset();
    }
}
void ConnectionUpdateSystem::HandleDisconnect(std::shared_ptr<NetClient> netClient)
{
#ifdef NC_Debug
    const NetSocket::ConnectionInfo& connectionInfo = netClient->GetSocket()->GetConnectionInfo();
    DebugHandler::PrintWarning("[Network/Socket]: Disconnected from (%s, %u)", connectionInfo.ipAddrStr.c_str(), connectionInfo.port);
#endif // NC_Debug
}
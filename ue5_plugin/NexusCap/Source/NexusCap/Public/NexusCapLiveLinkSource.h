// Copyright NexusCap Contributors. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Sockets.h"
#include "NexusCapTypes.h"

class ILiveLinkClient;
struct FLiveLinkSkeletonStaticData;

/**
 * FNexusCapLiveLinkSource
 *
 * ILiveLinkSource implementation that opens a UDP socket on a configurable
 * port, parses NexusCap wire-protocol datagrams received from the hub node
 * (or nexuscap_hub.py), and pushes quaternion + position data into the
 * Live Link subsystem as a single animated subject.
 *
 * The subject exposes one skeleton named "NexusCap_Body" whose bones map
 * directly to the sensor IDs defined in NexusCapTypes.h.
 */
class NEXUSCAP_API FNexusCapLiveLinkSource
    : public ILiveLinkSource
    , public FRunnable
{
public:
    explicit FNexusCapLiveLinkSource(int32 InListenPort);
    virtual ~FNexusCapLiveLinkSource();

    // -----------------------------------------------------------------------
    // ILiveLinkSource interface
    // -----------------------------------------------------------------------
    virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
    virtual void InitializeSettings(ULiveLinkSourceSettings* Settings) override {}
    virtual bool IsSourceStillValid() const override;
    virtual bool RequestSourceShutdown() override;
    virtual FText GetSourceType() const override;
    virtual FText GetSourceMachineName() const override;
    virtual FText GetSourceStatus() const override;

    // -----------------------------------------------------------------------
    // FRunnable interface  (runs on a dedicated receiver thread)
    // -----------------------------------------------------------------------
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------
    int32 GetListenPort() const { return ListenPort; }

private:
    /** Open the UDP socket and start the receiver thread. */
    bool OpenSocket();

    /** Process a single raw datagram. */
    void HandlePacket(const uint8* Data, int32 Len);

    /** Send the static skeleton description to Live Link (called once). */
    void SendStaticData();

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------
    int32              ListenPort;
    ILiveLinkClient*   Client       = nullptr;
    FGuid              SourceGuid;

    FSocket*           Socket       = nullptr;
    FRunnableThread*   ReceiverThread = nullptr;
    FThreadSafeBool    bRunning;
    FThreadSafeBool    bStaticDataSent;

    /** Subject name registered with Live Link. */
    FName              SubjectName;

    /** Tracks the number of valid packets received (for status text). */
    std::atomic<int64> PacketCount  { 0 };

    /** Millisecond timestamp of the last received packet. */
    std::atomic<uint32> LastPacketMs { 0 };
};

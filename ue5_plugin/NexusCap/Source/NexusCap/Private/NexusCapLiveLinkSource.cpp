// Copyright NexusCap Contributors. All Rights Reserved.

#include "NexusCapLiveLinkSource.h"

#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include "Common/UdpSocketBuilder.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

#include "Misc/ScopeLock.h"
#include "HAL/RunnableThread.h"

// --------------------------------------------------------------------------
// Subject name used in the Live Link panel
// --------------------------------------------------------------------------
static const FName NexusCapSubjectName(TEXT("NexusCap_Body"));

// --------------------------------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------------------------------

FNexusCapLiveLinkSource::FNexusCapLiveLinkSource(int32 InListenPort)
    : ListenPort(InListenPort)
    , SubjectName(NexusCapSubjectName)
{
}

FNexusCapLiveLinkSource::~FNexusCapLiveLinkSource()
{
    RequestSourceShutdown();
}

// --------------------------------------------------------------------------
// ILiveLinkSource
// --------------------------------------------------------------------------

void FNexusCapLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient,
                                             FGuid             InSourceGuid)
{
    Client     = InClient;
    SourceGuid = InSourceGuid;

    bRunning         = true;
    bStaticDataSent  = false;

    if (!OpenSocket())
    {
        UE_LOG(LogTemp, Error,
               TEXT("[NexusCap] Failed to open UDP socket on port %d"), ListenPort);
        bRunning = false;
        return;
    }

    ReceiverThread = FRunnableThread::Create(
        this,
        TEXT("NexusCap_ReceiverThread"),
        0,
        TPri_AboveNormal);
}

bool FNexusCapLiveLinkSource::IsSourceStillValid() const
{
    return bRunning && Socket != nullptr;
}

bool FNexusCapLiveLinkSource::RequestSourceShutdown()
{
    Stop();

    if (ReceiverThread)
    {
        ReceiverThread->Kill(true);
        delete ReceiverThread;
        ReceiverThread = nullptr;
    }

    if (Socket)
    {
        ISocketSubsystem* SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        Socket->Close();
        SocketSub->DestroySocket(Socket);
        Socket = nullptr;
    }

    if (Client)
    {
        Client->RemoveSubject_AnyThread(FLiveLinkSubjectKey(SourceGuid, SubjectName));
        Client = nullptr;
    }

    return true;
}

FText FNexusCapLiveLinkSource::GetSourceType() const
{
    return FText::FromString(TEXT("NexusCap IMU"));
}

FText FNexusCapLiveLinkSource::GetSourceMachineName() const
{
    return FText::Format(
        NSLOCTEXT("NexusCap", "MachineName", "UDP :{0}"),
        FText::AsNumber(ListenPort));
}

FText FNexusCapLiveLinkSource::GetSourceStatus() const
{
    if (!bRunning || Socket == nullptr)
    {
        return NSLOCTEXT("NexusCap", "StatusStopped", "Stopped");
    }

    if (PacketCount.load() == 0)
    {
        return NSLOCTEXT("NexusCap", "StatusWaiting", "Waiting for data ...");
    }

    return FText::Format(
        NSLOCTEXT("NexusCap", "StatusRunning", "Running  ({0} packets)"),
        FText::AsNumber(PacketCount.load()));
}

// --------------------------------------------------------------------------
// FRunnable
// --------------------------------------------------------------------------

bool FNexusCapLiveLinkSource::Init()
{
    return true;
}

uint32 FNexusCapLiveLinkSource::Run()
{
    // Buffer large enough for one wire packet
    TArray<uint8> RecvBuffer;
    RecvBuffer.SetNumUninitialized(NEXUSCAP_PACKET_SIZE);

    while (bRunning)
    {
        if (!Socket)
        {
            FPlatformProcess::Sleep(0.01f);
            continue;
        }

        uint32 PendingSize = 0;
        if (!Socket->HasPendingData(PendingSize))
        {
            FPlatformProcess::Sleep(0.0005f);  // ~0.5 ms poll
            continue;
        }

        int32 BytesRead = 0;
        Socket->Recv(RecvBuffer.GetData(), RecvBuffer.Num(), BytesRead);

        if (BytesRead == NEXUSCAP_PACKET_SIZE)
        {
            HandlePacket(RecvBuffer.GetData(), BytesRead);
        }
    }

    return 0;
}

void FNexusCapLiveLinkSource::Stop()
{
    bRunning = false;
}

// --------------------------------------------------------------------------
// Internal helpers
// --------------------------------------------------------------------------

bool FNexusCapLiveLinkSource::OpenSocket()
{
    ISocketSubsystem* SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSub) return false;

    Socket = FUdpSocketBuilder(TEXT("NexusCap_UDP"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToPort(ListenPort)
        .WithReceiveBufferSize(512 * 1024)
        .Build();

    if (!Socket)
    {
        UE_LOG(LogTemp, Error,
               TEXT("[NexusCap] Could not create UDP socket on port %d"), ListenPort);
        return false;
    }

    UE_LOG(LogTemp, Log,
           TEXT("[NexusCap] Listening on UDP port %d"), ListenPort);
    return true;
}

void FNexusCapLiveLinkSource::SendStaticData()
{
    if (!Client) return;

    FLiveLinkStaticDataStruct StaticDataStruct =
        FLiveLinkStaticDataStruct(FLiveLinkSkeletonStaticData::StaticStruct());

    FLiveLinkSkeletonStaticData& Skeleton =
        *StaticDataStruct.Cast<FLiveLinkSkeletonStaticData>();

    Skeleton.BoneNames   = GNexusCapBoneNames;
    Skeleton.BoneParents = GNexusCapBoneParents;

    Client->PushSubjectStaticData_AnyThread(
        FLiveLinkSubjectKey(SourceGuid, SubjectName),
        ULiveLinkAnimationRole::StaticClass(),
        MoveTemp(StaticDataStruct));

    bStaticDataSent = true;
}

void FNexusCapLiveLinkSource::HandlePacket(const uint8* Data, int32 Len)
{
    if (Len != NEXUSCAP_PACKET_SIZE) return;

    // Validate magic
    if (Data[0] != NEXUSCAP_MAGIC[0] || Data[1] != NEXUSCAP_MAGIC[1] ||
        Data[2] != NEXUSCAP_MAGIC[2] || Data[3] != NEXUSCAP_MAGIC[3])
    {
        return;
    }

    // Validate version
    if (Data[4] != NEXUSCAP_PROTOCOL_VERSION) return;

    // Parse packet fields (little-endian floats)
    const FNexusCapWirePacket* Wire =
        reinterpret_cast<const FNexusCapWirePacket*>(Data);

    const int32 SensorId = static_cast<int32>(Wire->SensorId);
    if (SensorId < 0 || SensorId >= GNexusCapBoneNames.Num()) return;

    // Send static skeleton the first time we have valid data
    if (!bStaticDataSent)
    {
        SendStaticData();
    }

    if (!Client) return;

    // Build frame data with one bone rotation per packet
    // The skeleton has NEXUSCAP_MAX_SENSORS bones; fill the rest with identity
    const int32 NumBones = GNexusCapBoneNames.Num();

    FLiveLinkFrameDataStruct FrameDataStruct =
        FLiveLinkFrameDataStruct(FLiveLinkAnimationFrameData::StaticStruct());

    FLiveLinkAnimationFrameData& FrameData =
        *FrameDataStruct.Cast<FLiveLinkAnimationFrameData>();

    FrameData.Transforms.SetNum(NumBones);
    for (int32 i = 0; i < NumBones; ++i)
    {
        FrameData.Transforms[i] = FTransform::Identity;
    }

    // Apply this sensor's orientation
    // NexusCap coordinate system: Y-up quaternion from MPU6050 DMP
    // UE5 coordinate system:      Z-up, left-handed
    // Remap:  x_UE = x_IMU,  y_UE = -z_IMU,  z_UE = y_IMU
    // Quaternion remap equivalent:
    const FQuat IMUQuat(Wire->Qx, Wire->Qy, Wire->Qz, Wire->Qw);
    const FQuat UE5Quat(IMUQuat.X, -IMUQuat.Z, IMUQuat.Y, IMUQuat.W);

    FrameData.Transforms[SensorId].SetRotation(UE5Quat);

    // World time for Live Link
    FrameData.WorldTime = FLiveLinkWorldTime(
        static_cast<double>(Wire->Timestamp) * 0.001);

    Client->PushSubjectFrameData_AnyThread(
        FLiveLinkSubjectKey(SourceGuid, SubjectName),
        MoveTemp(FrameDataStruct));

    ++PacketCount;
    LastPacketMs.store(Wire->Timestamp);
}

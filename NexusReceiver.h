#include "Networking.h"
#include "HAL/Runnable.h"

class FNexusReceiverWorker : public FRunnable {
    FSocket* ListenSocket;
    FIPv4Endpoint Endpoint;
    bool bRunThread = true;

    virtual uint32 Run() override {
        while (bRunThread) {
            uint32 Size;
            if (ListenSocket->HasPendingData(Size)) {
                TArray<uint8> ReceivedData;
                ReceivedData.SetNumUninitialized(Size);
                int32 Read = 0;
                ListenSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
                
                // Parse ReceivedData and Broadcast to an Event/Delegate
                // This is where you'd map the bytes back to your BoneData struct
            }
        }
        return 0;
    }
};

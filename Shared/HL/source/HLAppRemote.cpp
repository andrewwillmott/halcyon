//
//  File:       HLAppRemote.cpp
//
//  Function:   Provides remote UI event support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLAppRemote.h>

#include <IHLApp.h>

#include <CLLog.h>

#include <dispatch/dispatch.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

using namespace nHL;

namespace
{
    const int kAppRemotePort = 22888;

}

bool cEventBroadcaster::Init()
{
    // create socket
    mUDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mUDPSocket < 0)
        return false;

    // set broadcast mode
    int broadcast = 1;
    setsockopt(mUDPSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int));

    memset(mTargetAddr, 0, sizeof(mTargetAddr));

	struct sockaddr_in& targetAddress = *(struct sockaddr_in*) mTargetAddr;

    targetAddress.sin_family = AF_INET;
    targetAddress.sin_addr.s_addr = htonl(0xFFFFFFFF);
    targetAddress.sin_port = htons(kAppRemotePort);
    targetAddress.sin_len = sizeof(targetAddress);

    return true;
}

bool cEventBroadcaster::Shutdown()
{
    if (mUDPSocket >= 0)
    {
        close(mUDPSocket);
        mUDPSocket = -1;
    }

    return true;
}


namespace
{
    enum tRemoteType : uint8_t
    {
        kPointerDown,
        kPointerUp,
        kPointerMove,
        kPointersCancel,
        kKeyDown,
        kKeyUp,
        kKeyModifiers,
        kAcceleration,
        kTimeStamp,
        kMaxRemoteTypes
    };

    struct cRemoteMessage
    {
        uint8_t mChannel;
        uint8_t mType;
    };

    struct cRemotePointerMessage
    {
        uint8_t  mChannel;
        uint8_t  mType;
        uint16_t mIndex;
        float    mX;
        float    mY;
    };

    struct cRemotePointerButtonMessage
    {
        uint8_t  mChannel;
        uint8_t  mType;
        uint16_t mIndex;
        float    mX;
        float    mY;
        uint16_t mButton;
    };

    struct cRemoteKeyMessage
    {
        uint8_t  mChannel;
        uint8_t  mType;
        uint16_t mIndex;
        uint16_t mKey;
    };

    struct cRemoteKeyModMessage
    {
        uint8_t  mChannel;
        uint8_t  mType;
        uint16_t mIndex;
        uint32_t mModifiers;
    };

    struct cRemoteAccMessage
    {
        uint8_t mChannel;
        uint8_t mType;
        float   mA[3];
    };

    struct cRemoteTimeStampMessage
    {
        uint8_t     mChannel;
        uint8_t     mType;
        uint64_t    mTimeStamp;
    };
}

void cEventBroadcaster::PointerDown(int index, float x, float y, int button)
{
    cRemotePointerButtonMessage message =
    {
        mChannel,
        kPointerDown,
        uint16_t(index),
        x,
        y,
        uint16_t(button)
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::PointerUp(int index, float x, float y, int button)
{
    cRemotePointerButtonMessage message =
    {
        mChannel,
        kPointerUp,
        uint16_t(index),
        x,
        y,
        uint16_t(button)
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::PointerMove(int index, float x, float y)
{
    cRemotePointerMessage message =
    {
        mChannel,
        kPointerMove,
        uint16_t(index),
        x,
        y
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::PointersCancel()
{
    cRemoteMessage message =
    {
        mChannel,
        kPointersCancel,
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::KeyDown(int index, int key)
{
    cRemoteKeyMessage message =
    {
        mChannel,
        kKeyDown,
        uint16_t(index),
        uint16_t(key)
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::KeyUp(int index, int key)
{
    cRemoteKeyMessage message =
    {
        mChannel,
        kKeyUp,
        uint16_t(index),
        uint16_t(key)
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::KeyModifiers(int index, uint32_t mods)
{
    cRemoteKeyModMessage message =
    {
        mChannel,
        kKeyModifiers,
        uint16_t(index),
        mods
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::Acceleration(const float a[3])
{
    cRemoteAccMessage message =
    {
        mChannel,
        kAcceleration,
        a[0],
        a[1],
        a[2]
    };

    SendMessage(sizeof(message), &message);
}

void cEventBroadcaster::TimeStamp(uint64_t timeStampMS)
{
    cRemoteTimeStampMessage message =
    {
        mChannel,
        kTimeStamp,
        timeStampMS
    };

    SendMessage(sizeof(message), &message);
}


// Internal

bool cEventBroadcaster::SendMessage(size_t dataSize, const void* data)
{
	if (mUDPSocket < 0)
        return false;

    int error = sendto(mUDPSocket, data, dataSize, 0, (struct sockaddr*) mTargetAddr, sizeof(mTargetAddr));

    if( error < 0 )
    {
        perror("sendto");
        CL_LOG("AppRemote", "Error sending message\n");
        return false;
    }

    return true;
}

bool cEventReceiver::Init()
{
	mUDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));

	// listen on all interfaces
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_len = sizeof(struct sockaddr_in);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(kAppRemotePort);
	
	if (bind(mUDPSocket, (const struct sockaddr*) &sin, sizeof(sin)) != 0)
    {
        perror("bind");
        CL_LOG("AppRemote", "Issue binding to port %d\n", ntohs(sin.sin_port));
        Shutdown();
        return false;
    }

    dispatch_async
    (
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        ^{
            Listen();
        }
    );

    return true;
}

bool cEventReceiver::Shutdown()
{
    if (mUDPSocket >= 0)
    {
        close(mUDPSocket);
        mUDPSocket = -1;
    }

    return true;
}

void cEventReceiver::SetTarget(cIApp* app)
{
    mApp = app;
}

void cEventReceiver::Listen()
{
	uint8_t buffer[1024];

    dispatch_group_t messageGroup = dispatch_group_create();

	while (true)
    {
		int count = recv(mUDPSocket, buffer, sizeof(buffer), 0);

		if (count > 0)
		{
            // Post back to main thread.
            uint8_t* newBuffer = (uint8_t*) malloc(count);
            memcpy(newBuffer, buffer, count);

            dispatch_group_async(messageGroup, dispatch_get_main_queue(),
                ^{
                    ProcessMessage(newBuffer, count);
                    free(newBuffer);
                }
            );
		}
        else
            break;
    }

    // Don't exit until messages have stopped using the buffer above.
    dispatch_group_wait(messageGroup, DISPATCH_TIME_FOREVER);
    dispatch_release(messageGroup);
}

void cEventReceiver::Update()
{
	uint8_t buffer[32];
    uint8_t* b = buffer;

	while (true)
    {
        struct sockaddr address;
        socklen_t addressLen;

		int count = recvfrom(mUDPSocket, buffer, sizeof(buffer), 0, &address, &addressLen);

		if (count > 0)
            ProcessMessage(b, count);
        else
            break;
    }
}

void cEventReceiver::ProcessMessage(const uint8_t* data, size_t dataSize)
{
    if (!mApp)
        return;

    if (dataSize < sizeof(cRemoteMessage))
    {
        CL_LOG("AppRemote", "Short message\n");
        return;
    }

    const cRemoteMessage* messageBasic = reinterpret_cast<const cRemoteMessage*>(data);

    switch (messageBasic->mType)
    {
    case kPointerDown:
        {
            auto* m = reinterpret_cast<const cRemotePointerButtonMessage*>(data);
            mApp->PointerDown(m->mIndex, m->mX, m->mY, m->mButton);
        }
        break;
    case kPointerUp:
        {
            auto* m = reinterpret_cast<const cRemotePointerButtonMessage*>(data);
            mApp->PointerUp(m->mIndex, m->mX, m->mY, m->mButton);
        }
        break;
    case kPointerMove:
        {
            auto* m = reinterpret_cast<const cRemotePointerMessage*>(data);
            mApp->PointerMove(m->mIndex, m->mX, m->mY);
        }
        break;
    case kPointersCancel:
        {
            mApp->PointersCancel();
        }
        break;
    case kKeyDown:
        {
            auto* m = reinterpret_cast<const cRemoteKeyMessage*>(data);
            mApp->KeyDown(m->mIndex, m->mKey);
        }
        break;
    case kKeyUp:
        {
            auto* m = reinterpret_cast<const cRemoteKeyMessage*>(data);
            mApp->KeyUp(m->mIndex, m->mKey);
        }
        break;
    case kKeyModifiers:
        {
            auto* m = reinterpret_cast<const cRemoteKeyModMessage*>(data);
            mApp->KeyModifiers(m->mIndex, m->mModifiers);
        }
        break;
    case kAcceleration:
        {
            auto* m = reinterpret_cast<const cRemoteAccMessage*>(data);
            mApp->Acceleration(m->mA);
        }
        break;
    case kTimeStamp:
        {
            auto* m = reinterpret_cast<const cRemoteTimeStampMessage*>(data);
            mApp->TimeStamp(m->mTimeStamp);
        }
        break;
    }
}

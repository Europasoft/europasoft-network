// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
#include "Sockets/Sockets.h"
#include "NetThread/NetThreadSync.h"
#include <thread>
#include <chrono>

#ifndef _Acquires_lock_()
#define _Acquires_lock_()
#endif

enum class StreamEncryptionMode
{
	NoEncryption,
	Encrypted
};

namespace Encryption { class TLSContext; }
struct StreamEncryptionState
{
	StreamEncryptionMode mode = StreamEncryptionMode::NoEncryption;
	std::unique_ptr<Encryption::TLSContext> context = nullptr;

	bool enabled() const;
	void init(StreamEncryptionMode encryptMode);
};

class NetAgentSettings;

// TCP send/receive op thread class
class StreamThread
{
public:
    using Lock = Sockets::Lock; // syntactic sugar
    StreamThread(size_t sendBufferSize, size_t receiveBufferSize, StreamEncryptionMode encryptMode);
    ~StreamThread();
	// client
    void start(std::string_view hostname_, std::string_view port_);
	// server
    void start(SOCKET socket_);
	void updateSettings(const std::shared_ptr<NetAgentSettings>& settingsNew);
    
    bool isStreamConnected() const { return streamConnected; };
    bool isFailed() const { return connectionFailure; }

    // thread-safely copies to send buffer, returns false if buffer still has unsent data
    bool queueSend(std::string_view data);

    void getReceiveBuffer(std::string& data);
	size_t getReceiveDataSize() const;

    // forces the stream thread to shut down
    void stop() { forceTerminate = true; }

protected:
    void threadMain();
    std::thread thread{};
    std::atomic<bool> streamConnected = false;
    std::atomic<bool> connectionFailure = false;
    std::atomic<bool> forceTerminate = false;

    Sockets::MutexSocket socket;
	NetBufferAdvanced recvBuffer, sendBuffer;
    std::string hostname, port;
	Timer lastComTimer;
	std::shared_ptr<NetAgentSettings> settings = nullptr;

	StreamEncryptionState encryption{};

	bool threadSendData(Timer& lastComTimer, bool& terminate);
	bool threadSendDataTLS(Timer& lastComTimer, bool& terminate);
	bool threadReceiveData(Timer& lastComTimer, bool& terminate);
	bool threadReceiveDataTLS(Timer& lastComTimer, bool& terminate);
	void updateBuffersTLS(NetBufferAdvanced& recvBuffer, NetBufferAdvanced& sendBuffer, bool& terminate);
};



// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "StreamThread.h"
#include "NetAgent/HttpServerUtils/Logging.h"
#include "NetAgent/HttpServerUtils/BearSSL/inc/TLSInterface.h"
#include "NetAgent/Agent.h"
#include <cassert>
#include <limits.h>
#include <cstring>
#include <array>

#include <iostream>

StreamThread::StreamThread(size_t sendBufferSize, size_t receiveBufferSize, StreamEncryptionMode encryptMode)
    : sendBuffer{ sendBufferSize }, recvBuffer{ receiveBufferSize }
{
	encryption.init(encryptMode);
}

StreamThread::~StreamThread() 
{ 
    stop();
    thread.join();
}

void StreamThread::start(std::string_view hostname_, std::string_view port_)
{
    hostname = hostname_;
    port = port_;
    start(INVALID_SOCKET);
}

void StreamThread::start(SOCKET socket_)
{
    if (streamConnected) { return; }
    
    if (socket_ != INVALID_SOCKET)
    {
        // assumes socket is already connected (server mode)
        socket.set(socket_);
        streamConnected = true;
    }
    // start thread
    thread = std::thread([this] { this->threadMain(); }); 
}

void StreamThread::threadMain()
{
	WIN_SET_THREAD_NAME(L"Stream Thread");
    bool terminate = false;

    // resolve hostname and connect (client mode only)
    if (!streamConnected)
    {
        Timer t;
        t.start();
        SOCKET s;
        while (!t.checkTimeout(settings->clientConnectTimeoutSec))
        {
            if (Sockets::setupStream(hostname, port, s))
            {
                socket.set(s);
                streamConnected = true;
                break;
            }
        }
        if (!streamConnected) 
        { 
            // timeout
            terminate = true;
            connectionFailure = true;
        }
    }

	{
		Lock socketLock;
		if (not Sockets::setReceiveTimeout(socket.get(socketLock), settings->socketMaxReceiveWaitMs))
			terminate = true;
	}
	lastComTimer.start();

    // thread main loop
    while (!terminate)
    {
		bool didSend, didRecv;
        // send
		if (encryption.enabled())
			didSend = threadSendDataTLS(lastComTimer, terminate);
		else
			didSend = threadSendData(lastComTimer, terminate);

        // receive
		if (encryption.enabled())
			didRecv = threadReceiveDataTLS(lastComTimer, terminate);
		else
			didRecv = threadReceiveData(lastComTimer, terminate);

		updateBuffersTLS(recvBuffer, sendBuffer, terminate);

		const double delta = lastComTimer.getElapsed();
		if (delta > settings->communicationGapMaxSec)
		{
			terminate = true;
			ESLog::es_detail("Connection thread terminating: comms delta timeout");
		}
		else if (delta > settings->communicationGapSlowdownDelaySec)
		{
			Sockets::threadSleep(settings->communicationGapSlowdownAmountMs); // idle the thread if no communication has happened in a while
		}
		else if (not (didSend or didRecv))
		{
			Sockets::threadSleep(5); // idle the thread if no communication happened this iteration
		}

        terminate = forceTerminate or terminate;
    }
    streamConnected = false;
}

// when using TLS, data is sent from the encryption library buffer
bool StreamThread::threadSendDataTLS(Timer& lastComTimer, bool& terminate)
{
	assert(encryption.enabled());

	// TODO: small optimization: could avoid copying the data here
	Lock socketLock;
	std::string encrypted;
	if ((not encryption.context->getEncryptedOutgoing(encrypted)) or encrypted.size() == 0)
	{
		return false;
	}

	SOCKET s = socket.get(socketLock); // lock socket mutex, released at end of scope
	// send using encryption library as the data source
	// TODO: all the data might not be sent at once, in that case some will be lost (could be handled better)
	const size_t sizeSent = Sockets::sendData(s, encrypted.c_str(), encrypted.size());
	if (sizeSent != encrypted.size())
	{
		ESLog::es_detail("Connection thread terminating: attempt to send failed");
		terminate = true;
		return false;
	}

	lastComTimer.start();
	return true;
}

// when unencrypted, send data from the send buffer
bool StreamThread::threadSendData(Timer& lastComTimer, bool& terminate)
{
	assert(not encryption.enabled());
	Lock socketLock, bufferLock;
	size_t readable = 0;
	auto* buf = sendBuffer.getBufferForRead(bufferLock, readable);
	if (not (readable and buf))
		return false;

	SOCKET s = socket.get(socketLock); // lock socket mutex, released at end of scope
	// using the send buffer directly
	const size_t sizeSent = Sockets::sendData(s, buf, readable);
	sendBuffer.read(sizeSent);
	if (not sizeSent)
	{
		ESLog::es_detail("Connection thread terminating: attempt to send returned socket error");
		terminate = true;
		return false;
	}

	lastComTimer.start();
	return true;
}

// when using TLS data is received and sent to the encryption library
bool StreamThread::threadReceiveDataTLS(Timer& lastComTimer, bool& terminate)
{
	assert(encryption.enabled());

	Lock socketLock;
	SOCKET s = socket.get(socketLock); // lock socket mutex, released at end of scope

	// get the size available to receive on socket
	auto canRecvSize = Sockets::getReceiveSize(s);
	if (canRecvSize == 0 or not encryption.context->canPushIncoming())
	{
		return false;
	}
	if (canRecvSize > 5e7)
	{
		terminate = true; // end the connection if the network buffer grows too large
		ESLog::es_detail("Connection thread terminating: received data too large");
		return false;
	}

	// determine the size to receive
	const size_t bufferMax = encryption.context->getPushMaxSizeIncoming();
	const size_t sizeToReceive = min(bufferMax, canRecvSize);

	// allocate a temporary buffer to hold encrypted data
	// TODO: small optimization: could avoid copying the data here
	std::unique_ptr<char[]> encrypted = std::unique_ptr<char[]>(new char[sizeToReceive]);
	// receive
	size_t receivedSize = Sockets::receiveData(s, encrypted.get(), sizeToReceive);
	// push received data to encryption library
	encryption.context->pushEncryptedIncoming(encrypted.get(), receivedSize);
	lastComTimer.start();
	return true;
}

// when unencrypted, receive into the receive buffer
bool StreamThread::threadReceiveData(Timer& lastComTimer, bool& terminate)
{
	assert(not encryption.enabled());
	Lock socketLock, bufferLock;
	SOCKET s = socket.get(socketLock); // lock socket mutex, released at end of scope
	// get the size available to receive on socket
	auto sizeToReceive = Sockets::getReceiveSize(s);

	if (sizeToReceive == 0)
		return false;
	if (sizeToReceive > 5e7)
	{
		terminate = true; // end the connection if the network buffer grows too large
		ESLog::es_detail("Connection thread terminating: received data too large");
		return false;
	}

	auto* buf = recvBuffer.getBufferForWrite(bufferLock, sizeToReceive);
	if (not buf)
	{
		terminate = true;
		ESLog::es_error("Connection thread terminating: receive buffer allocation failed");
		return false;
	}

	// write received data directly to the receive buffer
	size_t receivedSize = Sockets::receiveData(s, buf, sizeToReceive);
	recvBuffer.written(receivedSize);
	lastComTimer.start();
}

// when using TLS the encryption buffers must communicate with the regular buffers
void StreamThread::updateBuffersTLS(NetBufferAdvanced& recvBuffer, NetBufferAdvanced& sendBuffer, bool& terminate)
{
	if (not encryption.enabled())
		return;

	if (encryption.context->isClosed())
	{
		ESLog::es_detail("Connection thread terminating: encryption failure");
		terminate = true;
		return;
	}

	// push data to be encrypted, from send buffer
	if (encryption.context->canPushOutgoing())
	{
		const size_t pushSizeMax = encryption.context->getPushMaxSizeOutgoing();
		size_t pushableSize = 0;
		Lock sendBufferLock;
		auto* buf = sendBuffer.getBufferForRead(sendBufferLock, pushableSize);
		if (pushableSize > 0 and pushSizeMax > 0 and buf)
		{
			const size_t sizeToPush = ESMin(pushableSize, pushSizeMax);
			const size_t sizePushed = encryption.context->pushOutgoing(buf, sizeToPush);
			if (sizePushed != sizeToPush)
				ESLog::es_error("Failed to push data to encryption buffer");
			sendBuffer.read(sizePushed);
			//ESLog::es_detail(ESLog::FormatStr() << "To be encrypted: '" << std::string(buf, sizeToPush) << "'");
		}
	}

	// get decrypted data, to receive buffer
	std::string incomingDecrypted;
	if (encryption.context->getDecryptedIncoming(incomingDecrypted))
	{
		Lock recvBufferLock;
		auto* buf = recvBuffer.getBufferForWrite(recvBufferLock, incomingDecrypted.size());
		if (buf and incomingDecrypted.size())
		{
			memcpy(buf, incomingDecrypted.data(), incomingDecrypted.size());
			recvBuffer.written(incomingDecrypted.size());
			//ESLog::es_detail(ESLog::FormatStr() << "Decrypted: '" << incomingDecrypted << "'");
		}
	}
	
}

// public: must be synchronized

bool StreamThread::queueSend(std::string_view data)
{
	if (data.size() == 0)
		return false;

	Lock l;
	auto* buf = sendBuffer.getBufferForWrite(l, data.size());
	if (buf)
	{
		memcpy(buf, data.data(), data.size());
		sendBuffer.written(data.size());
		return true;
	}
	return false;
}

// public: must be synchronized
void StreamThread::getReceiveBuffer(std::string& data) 
{
	Lock l;
	size_t readable = 0;
	auto* buf = recvBuffer.getBufferForRead(l, readable);
	if (buf and readable)
	{
		data = std::string(buf, readable);
		recvBuffer.read(readable);
	}
}

// public
size_t StreamThread::getReceiveDataSize() const
{
	Lock l;
	const size_t available = recvBuffer.peekReadSize(l);
	return available;
}

void StreamEncryptionState::init(StreamEncryptionMode encryptMode)
{
	mode = encryptMode;
	if (not enabled())
		return;
	// TODO: load certificate chain from file and detect cert type
	// initialize encryption library
	context = std::make_unique<Encryption::TLSContext>(Encryption::TLSKeyType::RSA, Encryption::CipherSuiteMode::FULL);
}

bool StreamEncryptionState::enabled() const
{
	return mode != StreamEncryptionMode::NoEncryption;
}

void StreamThread::updateSettings(const std::shared_ptr<NetAgentSettings>& settingsNew)
{
	settings = settingsNew;
}





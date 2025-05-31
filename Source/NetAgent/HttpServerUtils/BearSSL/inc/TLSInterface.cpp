/*
	BearSSL C++ wrapper
	Copyright 2025 Simon Liimatainen and Europa Software Ltd
	Copyright 2016 Thomas Pornin
	BearSSL is provided without warranty by Thomas Pornin under the MIT License.
	The C++ wrapper is provided without warranty by Simon Liimatainen and Europa Software Ltd.
	Different license terms may apply to other Software Components connected with the Software.
	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
	DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "TLSInterface.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

namespace BearSSL
{
	extern "C" 
	{
		#include "bearssl.h"
	}

	namespace RSA_Example
	{
		#include "chain-rsa.h"
		#include "key-rsa.h"
	}
	namespace EC_Example
	{
		#include "chain-ec.h"
		#include "key-ec.h"
	}
	namespace MIXED_Example
	{
		#include "chain-ec+rsa.h"
		#include "key-ec.h"
	}
}

namespace Encryption
{
	// declared here to avoid including BearSSL C headers in other files
	struct BearSSLResources
	{
		// unique_ptr is used for these resources so they stay put if the container moves
		std::unique_ptr<BearSSL::br_ssl_server_context> serverContext = nullptr;
		std::unique_ptr<BearSSL::br_sslio_context> ioContext = nullptr;
		std::unique_ptr<uint8_t[]> inputBuffer = nullptr;
		std::unique_ptr<uint8_t[]> outputBuffer = nullptr;
		size_t inputBufferSize, outputBufferSize;

		BearSSLResources(size_t inputMaxSize, size_t outputMaxSize);
		~BearSSLResources();
		void initIo();
		BearSSL::br_ssl_server_context* getServerContext() const { return serverContext.get(); }
		BearSSL::br_ssl_engine_context* getEngineContext() const { return &getServerContext()->eng; }
		BearSSL::br_sslio_context* getIoContext() const { return ioContext.get(); }
		uint8_t* getInputBuffer() const { return inputBuffer.get(); }
		uint8_t* getOutputBuffer() const { return outputBuffer.get(); }
	};

	BearSSLResources::BearSSLResources(size_t inputMaxSize, size_t outputMaxSize)
	{
		inputBufferSize = inputMaxSize;
		outputBufferSize = outputMaxSize;
		// create the BearSSL context structures
		serverContext = std::make_unique<BearSSL::br_ssl_server_context>();
		ioContext = std::make_unique<BearSSL::br_sslio_context>();
	}

	void BearSSLResources::initIo()
	{
		// initialize io buffers
		inputBuffer = std::unique_ptr<uint8_t[]>(new uint8_t[inputBufferSize]);
		outputBuffer = std::unique_ptr<uint8_t[]>(new uint8_t[outputBufferSize]);
		BearSSL::br_ssl_engine_set_buffers_bidi(getEngineContext(), getInputBuffer(), inputBufferSize, getOutputBuffer(), outputBufferSize);
	}

	BearSSLResources::~BearSSLResources()
	{
		// wipe the encryption io buffers before freeing, just in case
		if (inputBuffer and inputBufferSize > 0)
			memset(inputBuffer.get(), 0x00, inputBufferSize);
		if (outputBuffer and outputBufferSize > 0)
			memset(outputBuffer.get(), 0x00, outputBufferSize);
	}

	TLSContext::TLSContext(TLSKeyType keyType, CipherSuiteMode mode)
	{
		resources = std::make_unique<BearSSLResources>(BR_SSL_BUFSIZE_INPUT, BR_SSL_BUFSIZE_OUTPUT);
		initCipherSuites(keyType, mode);
		resources->initIo();
		resetForHandshake();
	}

	// destructor needs to be defined here, otherwise the BearSSLResources unique_ptr won't work, as it was forward-declared
	TLSContext::~TLSContext() = default;
	
	BearSSLResources& TLSContext::getResources() { return *resources.get(); }

	void TLSContext::resetForHandshake()
	{
		BearSSL::br_ssl_server_reset(getResources().getServerContext());
	}

	unsigned int TLSContext::getState()
	{
		const auto state = BearSSL::br_ssl_engine_current_state(getResources().getEngineContext());
		return state;
	}

	bool TLSContext::isClosed()
	{
		return (getState() & BR_SSL_CLOSED);
	}

	bool TLSContext::canPushIncoming()
	{
		return (getState() & BR_SSL_RECVREC);
	}

	bool TLSContext::canPushOutgoing()
	{
		return (getState() & BR_SSL_SENDAPP);
	}

	size_t TLSContext::pushEncryptedIncoming(const char* dataIn, size_t size)
	{
		using namespace BearSSL;
		if (not (getState() & BR_SSL_RECVREC) or size == 0)
			return 0;
		size_t sizeMax = 0;
		auto* encBuffer = br_ssl_engine_recvrec_buf(getResources().getEngineContext(), &sizeMax);
		const size_t copySize = std::min(size, sizeMax);
		if (copySize < 1 or not encBuffer)
			return 0;
		memcpy(encBuffer, dataIn, copySize);
		br_ssl_engine_recvrec_ack(getResources().getEngineContext(), copySize);
		br_ssl_engine_flush(getResources().getEngineContext(), 0);
		return copySize;
	}

	bool TLSContext::getDecryptedIncoming(std::string& dataOut)
	{
		using namespace BearSSL;
		if (not (getState() & BR_SSL_RECVAPP))
			return 0;
		size_t size = 0;
		const unsigned char* decBuffer = br_ssl_engine_recvapp_buf(getResources().getEngineContext(), &size);
		if (size < 1 or not decBuffer)
			return false;
		dataOut = std::string(reinterpret_cast<const char*>(decBuffer), size);
		br_ssl_engine_recvapp_ack(getResources().getEngineContext(), size);
		return true;
	}

	size_t TLSContext::pushOutgoing(const char* dataIn, size_t size)
	{
		using namespace BearSSL;
		if (not (getState() & BR_SSL_SENDAPP) or size == 0)
			return 0;
		size_t sizeMax = 0;
		auto* encBuffer = br_ssl_engine_sendapp_buf(getResources().getEngineContext(), &sizeMax);
		const size_t copySize = std::min(size, sizeMax);
		if (copySize < 1 or not encBuffer)
			return 0;
		memcpy(encBuffer, dataIn, copySize);
		br_ssl_engine_sendapp_ack(getResources().getEngineContext(), copySize);
		br_ssl_engine_flush(getResources().getEngineContext(), 0);
		return copySize;
	}

	bool TLSContext::getEncryptedOutgoing(std::string& dataOut)
	{
		using namespace BearSSL;
		if (not (getState() & BR_SSL_SENDREC))
			return false;
		size_t size = 0;
		const unsigned char* decBuffer = br_ssl_engine_sendrec_buf(getResources().getEngineContext(), &size);
		if (size < 1 or not decBuffer)
			return false;
		dataOut = std::string(reinterpret_cast<const char*>(decBuffer), size);
		br_ssl_engine_sendrec_ack(getResources().getEngineContext(), size);
		return true;
	}

	size_t TLSContext::getPushMaxSizeIncoming()
	{
		if (not (getState() & BR_SSL_RECVREC))
			return 0;
		size_t sizeMax = 0;
		auto* encBuffer = BearSSL::br_ssl_engine_recvrec_buf(getResources().getEngineContext(), &sizeMax);
		return sizeMax;
	}

	size_t TLSContext::getPushMaxSizeOutgoing()
	{
		if (not (getState() & BR_SSL_SENDAPP))
			return 0;
		size_t sizeMax = 0;
		auto* encBuffer = BearSSL::br_ssl_engine_sendapp_buf(getResources().getEngineContext(), &sizeMax);
		return sizeMax;
	}

	size_t TLSContext::getSizeDecryptedIncoming()
	{
		if (not (getState() & BR_SSL_RECVAPP))
			return 0;
		size_t size = 0;
		BearSSL::br_ssl_engine_recvapp_buf(getResources().getEngineContext(), &size);
		return size;
	}

	void TLSContext::initCipherSuites(TLSKeyType keyType, CipherSuiteMode mode)
	{
		/*
		 * Initialise the context with the cipher suites and algorithms.
		 * This depends on the server key type (and, for EC keys, the signature algorithm used by the CA to sign the server's certificate).
		 * We may select one of the "minimal" profiles. Key exchange algorithm depends on the key type:
		 * RSA key: RSA or ECDHE_RSA
		 * EC key, cert signed with ECDSA: ECDH_ECDSA or ECDHE_ECDSA
		 * EC key, cert signed with RSA: ECDH_RSA or ECDHE_ECDSA
		 */
		using namespace BearSSL;
		auto* serverContext = getResources().getServerContext();
		if (keyType == TLSKeyType::RSA)
		{
			if (mode == CipherSuiteMode::SERVER_MIN_FS_CHACHA20)
			{
				// Only TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256. Server key is RSA, and ECDHE key exchange is used. Provides forward secrecy.
				br_ssl_server_init_mine2c(serverContext, BearSSL::RSA_Example::CHAIN, CHAIN_LEN, &BearSSL::RSA_Example::RSA);
			}
			else if (mode == CipherSuiteMode::SERVER_MIN_FS_GCM)
			{
				// Only TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256. Server key is RSA, and ECDHE key exchange is used. Provides forward secrecy, with a higher CPU expense on the client(compared to "minr2g").
				br_ssl_server_init_mine2g(serverContext, BearSSL::RSA_Example::CHAIN, CHAIN_LEN, &BearSSL::RSA_Example::RSA);
			}
			else if (mode == CipherSuiteMode::SERVER_MIN_NOFS_GCM)
			{
				// Only TLS_RSA_WITH_AES_128_GCM_SHA256. Server key is RSA, and RSA key exchange is used (not forward secure, but cheaper on the client)
				br_ssl_server_init_minr2g(serverContext, BearSSL::RSA_Example::CHAIN, CHAIN_LEN, &BearSSL::RSA_Example::RSA);
			}
			else
			{
				// initializes with all supported algorithms and cipher suites that rely on a RSA key pair
				br_ssl_server_init_full_rsa(serverContext, BearSSL::RSA_Example::CHAIN, CHAIN_LEN, &BearSSL::RSA_Example::RSA);
			}
		}
		else if (keyType == TLSKeyType::EC)
		{
			if (mode == CipherSuiteMode::SERVER_MIN_FS_CHACHA20)
			{
				// Only TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256. Server key is EC, and ECDHE key exchange is used. Provides forward secrecy.
				br_ssl_server_init_minf2c(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, &BearSSL::EC_Example::EC);
			}
			else if (mode == CipherSuiteMode::SERVER_MIN_FS_GCM)
			{
				// Only TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256. Server key is EC, and ECDHE key exchange is used. Provides forward secrecy. Higher CPU expense on the client and server(vs. "minu2g" and "minv2g").
				br_ssl_server_init_minf2g(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, &BearSSL::EC_Example::EC);
			}
			else if (mode == CipherSuiteMode::SERVER_MIN_NOFS_GCM)
			{
				// Only TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256. Server key is EC, and ECDH key exchange is used; the issuing CA used an EC key. "minu2g" and "minv2g" do not provide forward secrecy, but are the lightest on the server
				br_ssl_server_init_minv2g(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, &BearSSL::EC_Example::EC);
			}
			else
			{
				// initializes with all supported algorithms and cipher suites that rely on a EC key pair
				// The key type of the CA that issued the server's certificate must be provided, since it matters for ECDH cipher suites(ECDH_RSA suites require a RSA - powered CA).
				// The key type is either BR_KEYTYPE_RSA or BR_KEYTYPE_EC.
				br_ssl_server_init_full_ec(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, BR_KEYTYPE_EC, &BearSSL::EC_Example::EC);
			}
		}
		else
		{
			/* EC_MIXED */
			if (mode == CipherSuiteMode::SERVER_MIN_FS_CHACHA20)
			{
				// Only TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256. Server key is EC, and ECDHE key exchange is used. Provides forward secrecy.
				br_ssl_server_init_minf2c(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, &BearSSL::EC_Example::EC);
			}
			else if (mode == CipherSuiteMode::SERVER_MIN_FS_GCM)
			{
				// Only TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256. Server key is EC, and ECDHE key exchange is used. Provides forward secrecy. Higher CPU expense on the client and server(vs. "minu2g" and "minv2g").
				br_ssl_server_init_minf2g(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, &BearSSL::EC_Example::EC);
			}
			else if (mode == CipherSuiteMode::SERVER_MIN_NOFS_GCM)
			{
				// Only TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256. Server key is EC, and ECDH key exchange is used; the issuing CA used a RSA key. "minu2g" and "minv2g" do not provide forward secrecy, but are the lightest on the server
				br_ssl_server_init_minu2g(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, &BearSSL::EC_Example::EC);
			}
			else
			{
				// initializes with all supported algorithms and cipher suites that rely on a EC key pair
				// The key type of the CA that issued the server's certificate must be provided, since it matters for ECDH cipher suites(ECDH_RSA suites require a RSA - powered CA).
				// The key type is either BR_KEYTYPE_RSA or BR_KEYTYPE_EC.
				br_ssl_server_init_full_ec(serverContext, BearSSL::EC_Example::CHAIN, CHAIN_LEN, BR_KEYTYPE_RSA, &BearSSL::EC_Example::EC);
			}
		}
	}

}

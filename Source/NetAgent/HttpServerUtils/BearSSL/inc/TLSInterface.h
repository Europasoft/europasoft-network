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

#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace Encryption
{
	enum class TLSKeyType
	{
		RSA, // Rivest–Shamir–Adleman cryptosystem
		EC, // Elliptic Curve cryptography
		EC_MIXED // Server key is EC, but issuer used RSA
	};

	enum class CipherSuiteMode
	{
		// minimal profile with forward secrecy, AES-GCM (Galois/Counter Mode using AES-128)
		SERVER_MIN_FS_GCM,
		// minimal profile with forward secrecy, ChaCha20-Poly1305
		SERVER_MIN_FS_CHACHA20,
		// minimal profile without forward secrecy, AES-GCM (Galois/Counter Mode using AES-128)
		SERVER_MIN_NOFS_GCM,
		// profile with all algorithms available
		FULL
	};

	struct BearSSLResources;

	// contains all the context BearSSL needs for a specific TLS connection (server)
	class TLSContext
	{
	public:
		TLSContext(TLSKeyType keyType, CipherSuiteMode mode);
		// BearSSL requires everything to stay at the same address, so copying is not allowed
		TLSContext(const TLSContext&) = delete;
		~TLSContext();
		TLSContext& operator=(const TLSContext&) = delete;
		TLSContext(TLSContext&&) = default;
		void resetForHandshake();

		// returns true if the TLS session was stopped
		bool isClosed();
		// returns true if the TLS engine can accept data through pushEncryptedIncoming()
		bool canPushIncoming();
		// returns true if the TLS engine can accept data through pushOutgoing()
		bool canPushOutgoing();
		// push encrypted data received from client, to be decrypted (socket receive -> "recvrec")
		size_t pushEncryptedIncoming(const char* dataIn, size_t size);
		// get decrypted data received from client ("recvapp" -> data in)
		bool getDecryptedIncoming(std::string& dataOut);
		// push unencrypted data to be encrypted, to be sent later (response -> "sendapp")
		size_t pushOutgoing(const char* dataIn, size_t size);
		// get encrypted data, to be sent to client ("sendrec" -> socket send)
		bool getEncryptedOutgoing(std::string& dataOut);

		size_t getPushMaxSizeIncoming();
		size_t getPushMaxSizeOutgoing();
		size_t getSizeDecryptedIncoming();

	private:
		// the BearSSL library C types are encapsulated to keep them in the translation unit
		std::unique_ptr<BearSSLResources> resources = nullptr;
		
	protected:
		BearSSLResources& getResources();
		void initCipherSuites(TLSKeyType keyType, CipherSuiteMode mode);
		unsigned int getState();
	};
}

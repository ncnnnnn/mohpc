#pragma once

#include "../Global.h"
#include <string>

namespace MOHPC
{

	class MOHPC_EXPORTS SHA256
	{
	private:
		unsigned char m_data[64];
		unsigned int m_datalen;
		unsigned long long m_bitlen;
		unsigned int m_state[8];

	public:
		void Init();
		void Update(const void* data, size_t len);
		std::string Final();

	private:
		void Transform(const void* data);
	};

	std::string sha256(const char* input, size_t length);
};

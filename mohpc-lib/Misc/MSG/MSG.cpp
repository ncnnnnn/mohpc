#include <Shared.h>
#include <MOHPC/Misc/MSG/MSG.h>
#include <MOHPC/Misc/MSG/Codec.h>
#include <MOHPC/Misc/MSG/Stream.h>
#include <MOHPC/Misc/MSG/Serializable.h>
#include <MOHPC/Misc/MSG/HuffmanTree.h>
#include <MOHPC/Network/InfoTypes.h>
#include "../Endian.h"
#include <string.h>
#include <cassert>
#include <algorithm>

using namespace MOHPC;

const uint8_t charByteMapping[256] =
{
	254, 120, 89, 13, 27, 73, 103, 78, 74, 102, 21, 117, 76, 86, 238, 96, 88, 62, 59, 60,
	40, 84, 52, 119, 251, 51, 75, 121, 192, 85, 44, 54, 114, 87, 25, 53, 35, 224, 67, 31,
	82, 41, 45, 99, 233, 112, 255, 11, 46, 115, 8, 32, 19, 100, 110, 95, 116, 48, 58, 107,
	70, 91, 104, 81, 118, 109, 36, 24, 17, 39, 43, 65, 49, 83, 56, 57, 33, 64, 80, 28,
	184, 160, 18, 105, 42, 20, 194, 38, 29, 26, 61, 50, 9, 90, 37, 128, 79, 2, 108, 34,
	4, 0, 47, 12, 101, 10, 92, 15, 5, 7, 22, 55, 23, 14, 3, 1, 66, 16, 63, 30,
	6, 97, 111, 248, 72, 197, 191, 122, 176, 245, 250, 68, 195, 77, 232, 106, 228, 93, 240, 98,
	208, 69, 164, 144, 186, 222, 94, 246, 148, 170, 244, 190, 205, 234, 252, 202, 230, 239, 174, 225,
	226, 209, 236, 216, 237, 151, 149, 231, 129, 188, 200, 172, 204, 154, 168, 71, 133, 217, 196, 223,
	134, 253, 173, 177, 219, 235, 214, 182, 132, 227, 183, 175, 137, 152, 158, 221, 243, 150, 210, 136,
	167, 211, 179, 193, 218, 124, 140, 178, 213, 249, 185, 113, 127, 220, 180, 145, 138, 198, 123, 162,
	189, 203, 166, 126, 159, 156, 212, 207, 146, 181, 247, 139, 142, 169, 242, 241, 171, 187, 153, 135,
	201, 155, 161, 125, 163, 130, 229, 206, 165, 157, 141, 147, 143, 199, 215, 131
};

const uint8_t byteCharMapping[256] =
{
	101, 115, 97, 114, 100, 108, 120, 109, 50, 92, 105, 47, 103, 3, 113, 107, 117, 68, 82, 52,
	85, 10, 110, 112, 67, 34, 89, 4, 79, 88, 119, 39, 51, 76, 99, 36, 66, 94, 87, 69,
	20, 41, 84, 70, 30, 42, 48, 102, 57, 72, 91, 25, 22, 35, 31, 111, 74, 75, 58, 18,
	19, 90, 17, 118, 77, 71, 116, 38, 131, 141, 60, 175, 124, 5, 8, 26, 12, 133, 7, 96,
	78, 63, 40, 73, 21, 29, 13, 33, 16, 2, 93, 61, 106, 137, 146, 55, 15, 121, 139, 43,
	53, 104, 9, 6, 62, 83, 135, 59, 98, 65, 54, 122, 45, 211, 32, 49, 56, 11, 64, 23,
	1, 27, 127, 218, 205, 243, 223, 212, 95, 168, 245, 255, 188, 176, 180, 239, 199, 192, 216, 231,
	206, 250, 232, 252, 143, 215, 228, 251, 148, 166, 197, 165, 193, 238, 173, 241, 225, 249, 194, 224,
	81, 242, 219, 244, 142, 248, 222, 200, 174, 233, 149, 236, 171, 182, 158, 191, 128, 183, 207, 202,
	214, 229, 187, 190, 80, 210, 144, 237, 169, 220, 151, 126, 28, 203, 86, 132, 178, 125, 217, 253,
	170, 240, 155, 221, 172, 152, 247, 227, 140, 161, 198, 201, 226, 208, 186, 254, 163, 177, 204, 184,
	213, 195, 145, 179, 37, 159, 160, 189, 136, 246, 156, 167, 134, 44, 153, 185, 162, 164, 14, 157,
	138, 235, 234, 196, 150, 129, 147, 230, 123, 209, 130, 24, 154, 181, 0, 46
};

namespace MOHPC
{
	// Unscrambled char mapping
	class charCharMapping_c
	{
	private:
		char mapping[256];

	public:
		constexpr charCharMapping_c()
			: mapping{ 0 }
		{
			for (uint16_t i = 0; i < 256; ++i) {
				mapping[i] = (uint8_t)i;
			}
		}

		constexpr operator const char* () const { return mapping; }
	};
	charCharMapping_c charCharMapping;
}

MOHPC::MSG::MSG(IMessageStream& stream, msgMode_e inMode) noexcept
	: msgStream(stream)
	, msgCodec(&MessageCodecs::Bit)
	, mode(inMode)
	, bit(0)
	, bitData{ 0 }
{
	if(mode == msgMode_e::Reading)
	{
		// Make sure it's readable
		Reset();
	}
}

MOHPC::MSG::~MSG()
{
	Flush();
}

void MOHPC::MSG::Flush()
{
	if (bit && IsWriting())
	{
		const size_t sz = ((bit + 7) & ~7) >> 3;
		stream().Write(bitData, sz);
		bit = 0;
	}
}

void MOHPC::MSG::SetCodec(IMessageCodec& inCodec) noexcept
{
	msgCodec = &inCodec;
}

void MOHPC::MSG::SetMode(msgMode_e inMode)
{
	mode = inMode;
}

void MOHPC::MSG::Reset()
{
	bit = 0;
	memset(bitData, 0, sizeof(bitData));
	stream().Read(bitData, std::min(stream().GetLength() - stream().GetPosition(), sizeof(bitData)));
}

void MOHPC::MSG::SerializeBits(void* value, intptr_t bits)
{
	if (!IsReading()) {
		WriteBits(value, bits);
	}
	else {
		ReadBits(value, bits);
	}
}

MSG& MOHPC::MSG::Serialize(void* data, size_t length)
{
	for (size_t i = 0; i < length; i++) {
		SerializeByte(((uint8_t*)data)[i]);
	}
	return *this;
}

MSG& MOHPC::MSG::SerializeDelta(const void* a, void* b, size_t bits)
{
	bool isSame = !memcmp(a, b, std::max(bits >> 3, size_t(1)));
	SerializeBits(&isSame, 1);

	if (!isSame) {
		SerializeBits(b, bits);
	}
	return *this;
}

MOHPC::MSG& MOHPC::MSG::SerializeDelta(const void* a, void* b, intptr_t key, size_t bits)
{
	if (!IsReading())
	{
		bool isSame = !memcmp(a, b, std::max(bits >> 3, size_t(1)));
		SerializeBits(&isSame, 1);

		if (!isSame)
		{
			const size_t sz = std::max(bits >> 3, size_t(1));
			uint8_t* buffer = new uint8_t[sz];
			std::memcpy(buffer, b, sz);

			for (size_t i = 0; i < sz; ++i) {
				buffer[i] ^= key;
			}

			SerializeBits(b, bits);
			delete[] buffer;
		}
	}
	else
	{
		bool isSame;
		SerializeBits(&isSame, 1);

		if (!isSame)
		{
			SerializeBits(b, bits);

			uint8_t* p2 = (uint8_t*)b;
			const size_t sz = std::max(bits >> 3, size_t(1));
			for (size_t i = 0; i < sz; ++i) {
				p2[i] ^= key;
			}
		}
	}
	return *this;
}

MSG& MOHPC::MSG::SerializeDeltaClass(const ISerializableMessage* a, ISerializableMessage* b)
{
	b->SerializeDelta(*this, a);
	if (!IsReading()) b->SaveDelta(*this, a);
	else b->LoadDelta(*this, a);
	return *this;
}

MSG& MOHPC::MSG::SerializeDeltaClass(const ISerializableMessage* a, ISerializableMessage* b, intptr_t key)
{
	b->SerializeDelta(*this, a, key);
	if (!IsReading()) b->SaveDelta(*this, a,key);
	else b->LoadDelta(*this, a, key);
	return *this;
}

template<>
MSG& MOHPC::MSG::SerializeDeltaType<bool>(const bool& a, bool& b, intptr_t key)
{
	if (!IsReading())
	{
		bool isSame = !((a & 1) ^ (b & 1));
		SerializeBits(&isSame, 1);

		if (!isSame)
		{
			bool val = b ^ bool(key);
			SerializeBits(&val, 1);
		}
	}
	else
	{
		bool isSame;
		SerializeBits(&isSame, 1);

		if (!isSame)
		{
			SerializeBits(&b, 1);
			b ^= bool(key);
		}
	}
	return *this;
}

MSG& MOHPC::MSG::SerializeBool(bool& value)
{
	SerializeBits(&value, 1);
	return *this;
}

MSG& MOHPC::MSG::SerializeByteBool(bool& value)
{
	uint8_t bval;
	SerializeByte(bval);
	value = (bool)bval;
	return *this;
}

MSG& MOHPC::MSG::SerializeChar(char& value)
{
	SerializeBits(&value, 8);
	return *this;
}

MSG& MOHPC::MSG::SerializeByte(unsigned char& value)
{
	SerializeBits(&value, 8);
	return *this;
}

MSG& MOHPC::MSG::SerializeShort(short& value)
{
	SerializeBits(&value, 16);
	return *this;
}

MSG& MOHPC::MSG::SerializeUShort(unsigned short& value)
{
	SerializeBits(&value, 16);
	return *this;
}

MSG& MOHPC::MSG::SerializeInteger(int& value)
{
	SerializeBits(&value, 32);
	return *this;
}

MSG& MOHPC::MSG::SerializeUInteger(unsigned int& value)
{
	SerializeBits(&value, 32);
	return *this;
}

MSG& MOHPC::MSG::SerializeFloat(float& value)
{
	SerializeBits(&value, 32);
	return *this;
}

MSG& MOHPC::MSG::SerializeClass(ISerializableMessage* value)
{
	value->Serialize(*this);
	if (!IsReading()) value->Save(*this);
	else value->Load(*this);
	return *this;
}

void MOHPC::MSG::SerializeString(StringMessage& s)
{
	if (!IsReading()) {
		WriteString(s);
	}
	else {
		s = std::move(ReadString());
	}
}

void MOHPC::MSG::SerializeStringWithMapping(StringMessage& s)
{
	if (!IsReading())
	{
		const size_t len = strlen(s) + 1;
		for (size_t i = 0; i < len; ++i)
		{
			uint8_t byteValue = charByteMapping[s[i]];
			SerializeByte(byteValue);
		}
	}
	else
	{
		s.preAlloc(1024);

		// FIXME: temporary solution
		uint8_t val = 0;
		char c = 0;
		size_t i = 0;
		do
		{
			SerializeByte(val);
			c = byteCharMapping[val];
			s.writeChar(c, i++);
		} while (c > 0);
	}
}

size_t MOHPC::MSG::Size() const
{
	return stream().GetLength();
}

size_t MOHPC::MSG::GetPosition() const
{
	const size_t pos = stream().GetPosition();
	if(pos >= sizeof(bitData)) {
		return stream().GetPosition() - sizeof(bitData) + (bit >> 3);
	}
	else
	{
		// if the stream is in a position below the buffer size, return read bytes read instead
		return (bit >> 3);
	}
}

size_t MOHPC::MSG::GetBitPosition() const
{
	return bit;
}

bool MOHPC::MSG::IsReading() noexcept
{
	return mode == msgMode_e::Reading || mode == msgMode_e::Both;
}

bool MOHPC::MSG::IsWriting() noexcept
{
	return mode == msgMode_e::Writing || mode == msgMode_e::Both;
}

void MOHPC::MSG::ReadData(void* data, size_t length)
{
	for (size_t i = 0; i < length; i++) {
		((uint8_t*)data)[i] = ReadByte();
	}
}

MSG& MOHPC::MSG::SerializeClass(ISerializableMessage& value)
{
	SerializeClass(&value);
	return *this;
}

void MOHPC::MSG::ReadBits(void* value, intptr_t bits)
{
	// Read the first n bytes
	if (!stream().GetPosition()) {
		stream().Read(bitData, std::min(stream().GetLength(), sizeof(bitData)));
	}

	if (bits < 0) bits = -bits;

	codec().Decode(value, bits, bit, stream(), bitData, sizeof(bitData));
}

bool MOHPC::MSG::ReadBool()
{
	assert(IsReading());
	bool val = false;
	ReadBits(&val, 1);
	return val;
}

bool MOHPC::MSG::ReadByteBool()
{
	assert(IsReading());
	uint8_t val = 0;
	ReadBits(&val, 8);
	return (bool)val;
}

char MOHPC::MSG::ReadChar()
{
	assert(IsReading());
	char val = 0;
	ReadBits(&val, 8);
	return val;
}

unsigned char MOHPC::MSG::ReadByte()
{
	assert(IsReading());
	unsigned char val = 0;
	ReadBits(&val, 8);
	return val;
}

short MOHPC::MSG::ReadShort()
{
	assert(IsReading());
	short val = 0;
	ReadBits(&val, 16);
	return val;
}

unsigned short MOHPC::MSG::ReadUShort()
{
	assert(IsReading());
	unsigned short val = 0;
	ReadBits(&val, 16);
	return val;
}

int MOHPC::MSG::ReadInteger()
{
	assert(IsReading());
	int val = 0;
	ReadBits(&val, 32);
	return val;
}

unsigned int MOHPC::MSG::ReadUInteger()
{
	assert(IsReading());
	unsigned int val = 0;
	ReadBits(&val, 32);
	return val;
}

float MOHPC::MSG::ReadFloat()
{
	assert(IsReading());
	float val = 0;
	ReadBits(&val, 32);
	return val;
}

MOHPC::StringMessage MOHPC::MSG::ReadString()
{
	return ReadStringInternal(charCharMapping);
}

MOHPC::StringMessage MOHPC::MSG::ReadScrambledString(const char* byteCharMapping)
{
	return ReadStringInternal(byteCharMapping);
}

MSG& MOHPC::MSG::ReadDeltaClass(const ISerializableMessage* a, ISerializableMessage* b)
{
	assert(IsReading());
	b->SerializeDelta(*this, a);
	b->LoadDelta(*this, a);
	return *this;
}

MSG& MOHPC::MSG::WriteData(const void* data, uintptr_t size)
{
	for (size_t i = 0; i < size; i++) {
		WriteByte(((const uint8_t*)data)[i]);
	}
	return *this;
}

MSG& MOHPC::MSG::ReadDeltaClass(const ISerializableMessage* a, ISerializableMessage* b, intptr_t key)
{
	assert(IsReading());
	b->SerializeDelta(*this, a, key);
	b->LoadDelta(*this, a, key);
	return *this;
}

MSG& MOHPC::MSG::WriteBits(const void* value, intptr_t bits)
{
	assert(IsWriting());

	if (bits < 0) bits = -bits;
	codec().Encode(value, bits, bit, stream(), bitData, sizeof(bitData));

	return *this;
}

MSG& MOHPC::MSG::WriteBool(bool value)
{
	assert(IsWriting());
	WriteBits(&value, 1);
	return *this;
}

MSG& MOHPC::MSG::WriteByteBool(bool value)
{
	assert(IsWriting());
	WriteBits(&value, 8);
	return *this;
}

MSG& MOHPC::MSG::WriteChar(char value)
{
	assert(IsWriting());
	WriteBits(&value, 8);
	return *this;
}

MSG& MOHPC::MSG::WriteByte(unsigned char value)
{
	assert(IsWriting());
	WriteBits(&value, 8);
	return *this;
}

MSG& MOHPC::MSG::WriteShort(short value)
{
	assert(IsWriting());
	WriteBits(&value, 16);
	return *this;
}

MSG& MOHPC::MSG::WriteUShort(unsigned short value)
{
	assert(IsWriting());
	WriteBits(&value, 16);
	return *this;
}

MSG& MOHPC::MSG::WriteInteger(int value)
{
	assert(IsWriting());
	WriteBits(&value, 32);
	return *this;
}

MSG& MOHPC::MSG::WriteUInteger(unsigned int value)
{
	assert(IsWriting());
	WriteBits(&value, 32);
	return *this;
}

MSG& MOHPC::MSG::WriteFloat(float value)
{
	assert(IsWriting());
	WriteBits(&value, 32);
	return *this;
}

MSG& MOHPC::MSG::WriteString(const StringMessage& s)
{
	assert(IsWriting());

	static char emptyString = 0;

	if (!s) {
		WriteData(&emptyString, 1);
	}
	else
	{
		WriteData((const void*)s.getData(), strlen(s) + 1);
	}

	return *this;
}

MSG& MOHPC::MSG::WriteScrambledString(const StringMessage& s, const uint8_t* charByteMapping)
{
	const char emptyString = charByteMapping[0];

	if (s)
	{
		const char* p = s.getData();
		while(*p)
		{
			const uint8_t c = *p++;
			const uint8_t val = charByteMapping[c];
			WriteData(&val, 1);
		}
	}

	WriteData(&emptyString, 1);

	return *this;
}

MSG& MOHPC::MSG::WriteDeltaClass(const ISerializableMessage* a, ISerializableMessage* b)
{
	assert(IsWriting());
	b->SerializeDelta(*this, a);
	b->SaveDelta(*this, a);
	return *this;
}

MSG& MOHPC::MSG::WriteDeltaClass(const ISerializableMessage* a, ISerializableMessage* b, intptr_t key)
{
	assert(IsWriting());
	b->SerializeDelta(*this, a, key);
	b->SaveDelta(*this, a, key);
	return *this;
}

template<>
float MOHPC::MSG::XORType(const float& b, intptr_t key)
{
	int xored = *(int*)&b ^ int(key);
	return *(float*)&xored;
}

MOHPC::StringMessage MOHPC::MSG::ReadStringInternal(const char* byteCharMapping)
{
	size_t startBit = bit;
	size_t startPos = stream().GetPosition();
	uint8_t oldBits[sizeof(bitData)];

	// Copy bits for later restoring
	memcpy(oldBits, bitData, sizeof(bitData));

	// Calculate the length of strings in the message
	size_t len = 0;
	uint8_t val = 0;
	char c = 0;
	do
	{
		val = ReadByte();
		if (val == -1) break;
		c = byteCharMapping[val];
		++len;
	} while (c);

	StringMessage s;

	if (len > 0)
	{
		bit = startBit;
		stream().Seek(startPos);
		memcpy(bitData, oldBits, sizeof(bitData));

		s.preAlloc(len);
		len--; // not including the null-terminating character

		for (size_t i = 0; i < len; ++i)
		{
			val = ReadByte();

			c = byteCharMapping[val];
			assert(c);
			s.writeChar(c, i);
		}

		// null-terminating character
		ReadByte();
	}

	return s;
}

template<>
double MOHPC::MSG::XORType(const double& b, intptr_t key)
{
	if constexpr (sizeof(double) == 4)
	{
		int xored = *(int*)&b ^ int(key);
		return *(double*)&xored;
	}
	else if constexpr (sizeof(double) == 8)
	{
		long long xored = *(long long*)&b ^ key;
		return *(double*)&xored;
	}
	else {
		// don't know what to do in this case
		return b;
	}
}

MOHPC::StringMessage::StringMessage() noexcept
	: strData(NULL)
	, isAlloced(false)
{
}

MOHPC::StringMessage::StringMessage(StringMessage&& str) noexcept
{
	strData = str.strData;
	isAlloced = str.isAlloced;
	str.isAlloced = false;
}

StringMessage& StringMessage::operator=(StringMessage&& str) noexcept
{
	if (isAlloced) delete[] strData;
	strData = str.strData;
	isAlloced = str.isAlloced;
	str.isAlloced = false;

	return *this;
}

MOHPC::StringMessage::StringMessage(const str& str) noexcept
{
	strData = new char[str.length() + 1];
	memcpy(strData, str.c_str(), str.length() + 1);
	isAlloced = true;
}

MOHPC::StringMessage::StringMessage(const char* str) noexcept
	: strData((char*)str)
	, isAlloced(false)
{
}

MOHPC::StringMessage::~StringMessage() noexcept
{
	if (isAlloced) {
		delete[] strData;
	}
}

char* MOHPC::StringMessage::getData() noexcept
{
	return strData;
}

char* MOHPC::StringMessage::getData() const noexcept
{
	return strData;
}

void MOHPC::StringMessage::preAlloc(size_t len) noexcept
{
	if (isAlloced) delete[] strData;
	strData = new char[len];
	strData[len - 1] = 0;
	isAlloced = true;
}

void MOHPC::StringMessage::writeChar(char c, size_t i) noexcept
{
	strData[i] = c;
}

MOHPC::StringMessage::operator const char* () const noexcept
{
	return strData;
}

void MOHPC::CompressedMessage::Compress(size_t offset, size_t len)
{
	const size_t size = std::min(input().GetLength(), len);

	if (size <= 0) {
		return;
	}

	Huff huff;

	uint8_t bitData[8]{ 0 };
	// store the original size
	bitData[0] = (uint8_t)(size >> 8);
	bitData[1] = (uint8_t)size & 0xff;

	// start position
	size_t bloc = 16;

	// Seek at the specified offset
	input().Seek(offset);
	MessageCodecs::FlushBits(bloc, output(), bitData, sizeof(bitData));

	uint8_t buffer[8]{ 0 };

	for (uintptr_t i = 0; i < size; i += sizeof(buffer))
	{
		const size_t bufSize = std::min(sizeof(buffer), size - i);

		input().Read(buffer, bufSize);
		compressBuf(huff, bloc, buffer, bufSize, bitData, sizeof(bitData));
	}

	if (bloc)
	{
		// write remaining bits
		output().Write(bitData, (bloc >> 3) + 1);
	}
}

void MOHPC::CompressedMessage::Decompress(size_t offset, size_t len)
{
	const size_t size = input().GetLength() - offset;

	if (size <= 0) {
		return;
	}

	Huff huff;

	// Seek at the specified offset
	input().Seek(offset);

	uint8_t bitData[8]{ 0 };
	input().Read(bitData, sizeof(bitData));

	size_t cch = bitData[0] * 256 + bitData[1];
	// don't overflow with bad messages
	if (cch > len) {
		cch = len;
	}
	size_t bloc = 16;

	output().Seek(0);

	uint8_t buffer[8];
	size_t pos = 0;

	for (uintptr_t j = 0; j < cch; ++j)
	{
		MessageCodecs::ReadBits(bloc, input(), bitData, sizeof(bitData));

		// don't overflow reading from the messages
		// FIXME: would it be better to have a overflow check in get_bit ?
		if (((size_t)bloc >> 3) > len)
		{
			// write the NUL character
			output().Write("", 1);
			break;
		}

		// Get a character
		uintptr_t ch = huff.receive(bitData, bloc);
		if (ch == Huff::NYT)
		{
			// We got a NYT, get the symbol associated with it
			ch = 0;
			for (uintptr_t i = 0; i < 8; i++) {
				ch = (ch << 1) + Huff::getBit(bitData, bloc);
			}
		}

		// insert the character
		buffer[pos] = (uint8_t)ch;

		// Increment node
		huff.addRef((uint8_t)ch);

		pos++;
		if (pos == sizeof(buffer))
		{
			output().Write(buffer, sizeof(buffer));
			pos = 0;
		}
	}

	if (pos > 0)
	{
		// write remaining bytes
		output().Write(buffer, pos);
	}
}

void CompressedMessage::compressBuf(Huff& huff, size_t& bloc, uint8_t* buffer, size_t bufSize, uint8_t* bitData, size_t bitDataSize)
{
	for (size_t i = 0; i < bufSize; ++i)
	{
		uint8_t ch = buffer[i];
		// Transmit symbol
		huff.transmit(ch, bitData, bloc);
		// Do update
		huff.addRef(ch);

		MessageCodecs::FlushBits(bloc, output(), bitData, sizeof(bitData));
	}
}

float MOHPC::MsgTypesHelper::ReadCoord()
{
	uint32_t read = 0;
	msg.ReadBits(&read, 19);

	float sign = 1.0f;
	if (read & 262144)
	{
		// the 19th bit is the sign
		sign = -1.0f;
	}

	read &= ~262144; //  uint=4294705151
	return sign * (read / 16.f);
}

float MOHPC::MsgTypesHelper::ReadCoordSmall()
{
	uint32_t read = 0;
	msg.ReadBits(&read, 17);

	float sign = 1.0f;
	if (read & 65536)
	{
		// the 17th bit is the sign
		sign = -1.0f;
	}

	read &= ~65536; //  uint=4294705151
	return sign * (read / 8.f);
}

int32_t MOHPC::MsgTypesHelper::ReadDeltaCoord(uint32_t offset)
{
	int32_t result = 0;

	const bool isSmall = msg.ReadBool();
	if (isSmall)
	{
		uint8_t byteValue = 0;
		msg.ReadBits(&byteValue, 8);
		result = (byteValue >> 1) + 1;
		if (byteValue & 1) result = -result;

		result += offset;

	}
	else {
		msg.ReadBits(&result, 16);
	}

	return result;
}

int32_t MOHPC::MsgTypesHelper::ReadDeltaCoordExtra(uint32_t offset)
{
	int32_t result = 0;

	const bool isSmall = msg.ReadBool();
	if (isSmall)
	{
		uint16_t shortValue = 0;
		msg.ReadBits(&shortValue, 10);
		result = (shortValue >> 1) + 1;
		if (shortValue & 1) result = -result;

		result += offset;
	}
	else {
		msg.ReadBits(&result, 18);
	}

	return result;
}

MOHPC::Vector MOHPC::MsgTypesHelper::ReadVectorCoord()
{
	const Vector vec =
	{
		ReadCoord(),
		ReadCoord(),
		ReadCoord()
	};
	return vec;
}

MOHPC::Vector MOHPC::MsgTypesHelper::ReadVectorFloat()
{
	const Vector vec =
	{
		msg.ReadFloat(),
		msg.ReadFloat(),
		msg.ReadFloat()
	};
	return vec;
}

MOHPC::Vector MOHPC::MsgTypesHelper::ReadDir()
{
	Vector dir;

	const uint8_t byteValue = msg.ReadByte();
	ByteToDir(byteValue, dir);

	return dir;
}

uint16_t MOHPC::MsgTypesHelper::ReadEntityNum()
{
	uint16_t entNum = 0;
	msg.ReadBits(&entNum, GENTITYNUM_BITS);
	return entNum & (MAX_GENTITIES - 1);
}

uint16_t MOHPC::MsgTypesHelper::ReadEntityNum2()
{
	uint16_t entNum = 0;
	msg.ReadBits(&entNum, GENTITYNUM_BITS);
	return (entNum - 1) & (MAX_GENTITIES - 1);
}

void MOHPC::MsgTypesHelper::WriteCoord(float& value)
{
	int32_t bits = int32_t(value * 16.0f);
	if (value < 0) {
		bits = ((-bits) & 262143) | 262144;
	}
	else {
		bits = bits & 262143;
	}

	msg.SerializeBits(&bits, 19);
}

void MOHPC::MsgTypesHelper::WriteCoordSmall(float& value)
{
	int32_t bits = (uint32_t)(value * 8.0f);
	if (value < 0) {
		bits = ((-bits) & 65535) | 65536;
	}
	else {
		bits = bits & 65535;
	}

	msg.WriteBits(&bits, 17);
}

void MOHPC::MsgTypesHelper::WriteVectorCoord(Vector& value)
{
	WriteCoord(value.x);
	WriteCoord(value.y);
	WriteCoord(value.z);
}

void MOHPC::MsgTypesHelper::WriteDir(Vector& dir)
{
	uint8_t byteValue = 0;
	ByteToDir(byteValue, dir);
	msg.WriteByte(byteValue);
}

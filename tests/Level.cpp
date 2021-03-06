#include <MOHPC/Formats/BSP.h>
#include <MOHPC/Formats/DCL.h>
#include <MOHPC/Managers/AssetManager.h>
#include <MOHPC/Managers/ShaderManager.h>
#include <MOHPC/Collision/Collision.h>
#include "UnitTest.h"

#include <map>
#include <vector>

class Archive
{
public:
	virtual void serialize(void* value, size_t size) = 0;
};

class ArchiveReader : public Archive
{
private:
	const uint8_t* data;
	size_t dataSize;
	size_t dataPos;

public:
	ArchiveReader(const uint8_t* inData, size_t inDataSize)
		: data(inData)
		, dataSize(inDataSize)
		, dataPos(0)
	{

	}

	template<typename T>
	void operator()(T& value)
	{
		*this >> value;
	}

	virtual void serialize(void* value, size_t size)
	{
		memcpy(value, data + dataPos, size);
		dataPos += size;
	}

	Archive& operator>>(char& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(uint8_t& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(bool& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(int32_t& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(uint32_t& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(int64_t& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(uint64_t& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator>>(float& value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}
};

class ArchiveWriter : public Archive
{
private:
	MOHPC::Container<uint8_t> data;
	size_t pos;

public:
	ArchiveWriter()
		: pos(0)
	{
	}

	template<typename T>
	void operator()(T& value)
	{
		*this << value;
	}

	virtual void serialize(void* value, size_t size)
	{
		if(pos + size >= data.NumObjects()) {
			data.SetNumObjectsUninitialized(data.NumObjects() * 2 + size);
		}

		memcpy(data.Data() + pos, value, size);
		pos += size;
	}

	const MOHPC::Container<uint8_t>& getData() const
	{
		return data;
	}

	Archive& operator<<(char value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(uint8_t value)
	{
		serialize(&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(bool value)
	{
		serialize((void*)&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(int32_t value)
	{
		serialize((void*)&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(uint32_t value)
	{
		serialize((void*)&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(int64_t value)
	{
		serialize((void*)&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(uint64_t value)
	{
		serialize((void*)&value, sizeof(value));
		return *this;
	}

	Archive& operator<<(float value)
	{
		serialize((void*)&value, sizeof(value));
		return *this;
	}
};

/*
template<typename T>
Archive& operator<<(Archive& ar, const T& obj)
{
	obj.serialize(ar);
}

template<typename T>
Archive& operator>>(Archive& ar, const T& obj)
{
	obj.serialize(ar);
}
*/

class CLevelTest : public IUnitTest
{
public:
	virtual const char* name() override
	{
		return "BSP";
	}

	virtual unsigned int priority() override
	{
		return 0;
	}

	virtual void run(const MOHPC::AssetManagerPtr& AM) override
	{
		MOHPC::DCLPtr DCL = AM->LoadAsset<MOHPC::DCL>("/maps/dm/mohdm4.dcl");
		MOHPC::DCLPtr DCLBT = AM->LoadAsset<MOHPC::DCL>("/maps/e1l1.dcl");

		MOHPC::BSPPtr Level = AM->LoadAsset<MOHPC::BSP>("/maps/lib/mp_anzio_lib.bsp");
		MOHPC::BSPPtr Asset = AM->LoadAsset<MOHPC::BSP>("/maps/dm/mohdm6.bsp");
		if(Asset)
		{
			traceTest(Asset);
			leafTesting(Asset);

			MOHPC::BSPData::TerrainCollide collision;
			Asset->GenerateTerrainCollide(Asset->GetTerrainPatch(0), collision);
		}
	}

	void traceTest(MOHPC::BSPPtr Asset)
	{
		using namespace MOHPC;
		CollisionWorldPtr cm = CollisionWorld::create();
		Asset->FillCollisionWorld(*cm);

		trace_t results;
		{
			Vector start(1011.12500f, 1136.81250f ,116.125000f);
			Vector end(1011.12500f, 1136.81250f, 98.1250000f);
			cm->CM_BoxTrace(&results, start, end, Vector(-15, -15, 0), Vector(15, 15, 96), 0, ContentFlags::MASK_PLAYERSOLID, true);
		}

		// Patch testing
		{
			Vector start(499.133942f, -427.044525f, -151.875000f);
			Vector end(499.125824f, -426.720612f, -151.875000f);
			Vector mins(-15, -15, 0);
			Vector maxs(15, 15, 96);
			Vector origin(476.f, -400.f, -150.f);

			//cm.CM_BoxTrace(&results, start, end, Vector(-15, -15, 0), Vector(15, 15, 96), 0, ContentFlags::MASK_PLAYERSOLID, true);
			cm->CM_TransformedBoxTrace(&results, start, end, mins, maxs, 37, ContentFlags::MASK_PLAYERSOLID, origin, vec_origin, true);
			assert(results.fraction < 0.01f);
		}

		Vector start(0, 0, 0);
		Vector end(0, 0, -500);
		cm->CM_BoxTrace(&results, start, end, Vector(), Vector(), 0, ContentFlags::MASK_PLAYERSOLID, true);
		assert(results.fraction < 0.3f);

		ArchiveWriter ar;
		cm->save(ar);

		const Container<uint8_t>& data = ar.getData();

		ArchiveReader arReader(data.Data(), data.NumObjects());
		cm->load(arReader);

		end = Vector(1000, 1000, 0);
		cm->CM_BoxTrace(&results, start, end, Vector(), Vector(), 0, ContentFlags::MASK_PLAYERSOLID, true);
	}

	void leafTesting(MOHPC::BSPPtr Asset)
	{
		uintptr_t leafNum = Asset->PointLeafNum(MOHPC::Vector(0, 0, 0));

		std::map<uintptr_t, uintptr_t> brushRefs;
		std::vector<std::vector<const MOHPC::BSPData::Brush*>> brushArrays;

		size_t numLeafs = Asset->GetNumLeafs();
		brushArrays.resize(numLeafs);

		for (size_t i = 0; i < numLeafs; ++i)
		{
			const MOHPC::BSPData::Leaf* leaf = Asset->GetLeaf(i);

			for (size_t j = 0; j < leaf->numLeafBrushes; ++j)
			{
				uintptr_t brushNum = Asset->GetLeafBrush(leaf->firstLeafBrush + j);

				const size_t brushRef = brushRefs[brushNum]++;
				if (!brushRef)
				{
					const MOHPC::BSPData::Brush* brush = Asset->GetBrush(brushNum);
					brushArrays[i].push_back(brush);
				}
			}
		}

		for (auto it = brushArrays.begin(); it != brushArrays.end(); )
		{
			if (!it->size()) {
				it = brushArrays.erase(it);
			}
			else {
				++it;
			}
		}
	}
};
static CLevelTest unitTest;

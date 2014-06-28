
#include <iostream>
#include <vector>
#include <cassert>

#include "optional.hpp"
typedef std::vector<int> IntVector;

static int live = 0;

struct AllocTrack
{
	AllocTrack()
	{
		++live;
	}

	AllocTrack(const AllocTrack &)
	{
		++live;
	}

	AllocTrack(AllocTrack &&)
	{
		++live;
	}

	AllocTrack &operator =(const AllocTrack &)  { return *this; }
	AllocTrack &operator =(AllocTrack &&)  { return *this; }

	~AllocTrack()
	{
		--live;
	}
};

int main()
{
	using autobind::Optional;

	typedef Optional<std::vector<AllocTrack>> OptionalVec;

	{

		OptionalVec ov1;
		assert(!ov1);

		OptionalVec ov2(std::vector<AllocTrack> {{},{},{}});
		assert(ov2->size() == 3);

		ov1 = ov2;

		assert(ov1->size() == 3);
		assert(ov2->size() == 3);


		OptionalVec ov3;

		ov3 = std::move(ov2);
		assert(!ov2);
		assert(ov3);
		assert(ov3->size() == 3);


		OptionalVec ov4 = ov3;
		assert(ov3);
		assert(ov3->size() == 3);
		assert(ov4);
		assert(ov4->size() == 3);

		OptionalVec ov5 = std::move(ov4);
		assert(!ov4);
		assert(ov5->size() == 3);

		auto ov6 = autobind::makeOptional<int>(0);
	}

	assert(live == 0);


	return 0;

}



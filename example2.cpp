#include <sstream>

#include "autobind.hpp"

pymodule(example2);

class pyexport Point2D
{
	int _x, _y;
public:
	Point2D(int x, int y)
	: _x(x)
	, _y(y)
	{ }

	int x() const
	{
		return _x;
	}

	int y() const
	{
		return _y;
	}

	std::string repr() const
	{
		std::stringstream ss;
		ss << "Point2D(" << _x << ", " << _y << ")";
		return ss.str();
	}

	Point2D translated(int x, int y) const
	{
		return Point2D(_x + x, _y + y);
	}
};


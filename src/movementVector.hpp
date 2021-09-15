class movementVector
{
public:
	double x = 0;
	double y = 0;

	double getMagnitude()
	{
		const double magnitudeFromVector = sqrt(pow(x, 2) + pow(y, 2));
		return magnitudeFromVector;
	}

	//faster due to no sqrt
	double getMagnitudeSqr()
	{
		return x * x + y * y;
	}

	double getDirection()
	{
		double direction = atan2(y, x);
		return direction;
	}

	void setVector(double direction, double magnitude)
	{
		x = magnitude * cos(direction);
		y = magnitude * sin(direction);
	}
};
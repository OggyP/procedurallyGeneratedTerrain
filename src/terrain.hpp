#include <noise.hpp>

class GameTile
{
public:
	int type;
	sf::Color colour;
	GameTile(int blockType, sf::Color blockColour)
	{
		type = blockType;
		colour = blockColour;
	}
	GameTile()
	{
		type = 0;
	}
};

class GameChunk
{
public:
	GameTile tiles[chunkSize][chunkSize];
	int x;
	int y;
	GameChunk()
	{}
	GameChunk(int chunkX, int chunkY, int seed[2])
	{
		const int tileStartCoord[2] = { chunkX * chunkSize * boxSize, chunkY * chunkSize * boxSize };
		for (int yToCalc = 0; yToCalc < chunkSize; yToCalc++)
		{
			for (int xToCalc = 0; xToCalc < chunkSize; xToCalc++)
			{
				double noiseVal = ValueNoise_2D(((double)tileStartCoord[0] + (double)xToCalc * (double)boxSize + (double)seed[0]) / (double)zoomScale / (double)boxSize, ((double)tileStartCoord[1] + (double)yToCalc * (double)boxSize + (double)seed[1]) / (double)zoomScale / (double)boxSize);
				// tiles[yToCalc][xToCalc]
				// cout << noiseVal << "\n";

				// Less than (Lowest First)
				if (noiseVal < -0.13)
				{
					// Ocean
					tiles[yToCalc][xToCalc] = GameTile(2, sf::Color(0, 0, noiseVal * 320 + 200));
				}
				else if (noiseVal < -0.08)
				{
					// Sand
					// tiles[yToCalc][xToCalc] = GameTile(3, sf::Color(242, 204, 31));
					tiles[yToCalc][xToCalc] = GameTile(3, sf::Color(noiseVal * 320 + 200, noiseVal * 320 + 160, 31));
				}

				// Greater Than (Highest First)
				else if (noiseVal > 0.3)
				{
					// Snow
					tiles[yToCalc][xToCalc] = GameTile(5, sf::Color(240, 250, 255));
				}
				else if (noiseVal > 0.2)
				{
					// Mountain
					tiles[yToCalc][xToCalc] = GameTile(4, sf::Color(255 - (noiseVal * 270 + 128), 255 - (noiseVal * 280 + 128), 255 - (noiseVal * 320 + 128)));
				}
				else
				{
					// if no other special case
					// Grass
					tiles[yToCalc][xToCalc] = GameTile(1, sf::Color(13, 255 - (noiseVal * 320 + 128), 3));
				}
			}
		}
	}
};
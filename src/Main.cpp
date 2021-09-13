#include "Platform/Platform.hpp"
#include <math.h>
// noise.hpp is included in terrain.hpp
int boxSize = 10;
const int chunkSize = 6;
#include "terrain.hpp"
#include <time.h>
// #include <map>

using namespace std;
using namespace sf;

// useful stuff
const double currentLocation[2] = { 0, 0 };
int offset[2] = { 0, 0 };

// Threads
vector<std::thread> offScreenThreadList;
int usableVectorSpaces = 0;
bool firstRun = false;

// map of chunks
std::map<std::string, GameChunk> worldMap;

void genNewChunk(string chunkKey, int chunkX, int chunkY, int seed[2]);
void genNewChunk(string chunkKey, int chunkX, int chunkY, int seed[2])
{
	// Key is not present
	GameChunk newChunk = GameChunk(chunkX, chunkY, seed);
	worldMap[chunkKey] = std::move(newChunk);
	return;
}

void checkForMissingOffscreenChunks(int chunksOnScreen[2][2], int seed[2]);
void checkForMissingOffscreenChunks(int chunksOnScreen[2][2], int seed[2])
{
	for (int axis = 0; axis < 2; axis++)
	{
		chunksOnScreen[0][axis] = chunksOnScreen[0][axis] - 1;
		chunksOnScreen[1][axis] = chunksOnScreen[1][axis] + 1;
	}
	int chunkY = chunksOnScreen[0][1];
	int chunkX;
	for (int passes = 0; passes < 2; passes++)
	{
		for (chunkX = chunksOnScreen[0][0]; chunkX < chunksOnScreen[1][0]; chunkX++)
		{
			string chunkKey;
			chunkKey.append(to_string(chunkX));
			chunkKey.push_back('|');
			chunkKey.append(to_string(chunkY));

			std::map<std::string, GameChunk>::iterator it = worldMap.find(chunkKey);
			if (it == worldMap.end())
			{
				// std::cout << " New Chunk - Premptive - " << chunkKey << "\n";
				// void genNewChunk(string& chunkKey, int& chunkX, int& chunkY, int seed[2], sf::RenderWindow& window, sf::RectangleShape& rect)
				genNewChunk(chunkKey, chunkX, chunkY, seed);
			}
		}
		chunkY = chunksOnScreen[1][1];
	}
	chunkX = chunksOnScreen[0][0];
	for (int passes = 0; passes < 2; passes++)
	{
		for (chunkY = chunksOnScreen[0][1] + 1; chunkY < chunksOnScreen[1][1] - 1; chunkY++)
		{
			string chunkKey;
			chunkKey.append(to_string(chunkX));
			chunkKey.push_back('|');
			chunkKey.append(to_string(chunkY));

			std::map<std::string, GameChunk>::iterator it = worldMap.find(chunkKey);
			if (it == worldMap.end())
			{
				// std::cout << " New Chunk - Premptive - " << chunkKey << "\n";
				// void genNewChunk(string& chunkKey, int& chunkX, int& chunkY, int seed[2], sf::RenderWindow& window, sf::RectangleShape& rect)
				genNewChunk(chunkKey, chunkX, chunkY, seed);
			}
		}
		chunkX = chunksOnScreen[1][0];
	}
	return;
}

int main()
{
	util::Platform platform;

#if defined(_DEBUG)
	std::cout << "Hello World!" << std::endl;
#endif
	int screenSize[2] = { 100, 100 };

	// Initialize stuff
	/* initialize random seed: */
	srand(time(NULL));

	int seed[2] = { rand() % 100000 + 1000000, rand() % 100000 + 1000000 };

	// Threads
	std::thread genNewOffscreenThread;

	time_t start = time(0);
	int totalFrames = 0;

	// SFML Window
	sf::RenderWindow window;
	// in Windows at least, this must be called before creating the window
	float screenScalingFactor = platform.getScreenScalingFactor(window.getSystemHandle());
	// Use the screenScalingFactor
	window.create(sf::VideoMode(1000.0f * screenScalingFactor, 1000.0f * screenScalingFactor), "Noise!");

	// SFML Obj
	sf::RectangleShape rect(sf::Vector2f(boxSize, boxSize));
	rect.setFillColor(sf::Color::White);
	platform.setIcon(window.getSystemHandle());

	// Limit the framerate to 60 frames per second (this step is optional)
	window.setFramerateLimit(60);

	sf::Event event;

	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
				// close all offscreen threads
				for (std::thread& th : offScreenThreadList)
				{
					// If thread Object is Joinable then Join that thread.
					if (th.joinable())
						th.join();
				}
				if (genNewOffscreenThread.joinable())
					genNewOffscreenThread.join();
				return 0;
			}

			// catch the resize events
			if (event.type == sf::Event::Resized)
			{
				// update the view to the new size of the window
				sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
				screenSize[0] = event.size.width;
				screenSize[1] = event.size.height;
				window.setView(sf::View(visibleArea));
			}
		}

		if (Keyboard::isKeyPressed(Keyboard::Left))
		{
			offset[0] -= 20;
		}

		if (Keyboard::isKeyPressed(Keyboard::Down))
		{
			offset[1] += 20;
		}

		if (Keyboard::isKeyPressed(Keyboard::Right))
		{
			offset[0] += 20;
		}

		if (Keyboard::isKeyPressed(Keyboard::Up))
		{
			offset[1] -= 20;
		}

		window.clear();

		int chunksOnScreen[2][2] = {
			{ (int)floor(offset[0] / boxSize / chunkSize) - 1, (int)floor(offset[1] / boxSize / chunkSize) - 1 },									 //start pos
			{ (int)floor((offset[0] + screenSize[0]) / boxSize / chunkSize) + 1, (int)floor((offset[1] + screenSize[1]) / boxSize / chunkSize) + 1 } //end pos
		};

		for (int chunkX = chunksOnScreen[0][0]; chunkX < chunksOnScreen[1][0]; chunkX++)
		{
			for (int chunkY = chunksOnScreen[0][1]; chunkY < chunksOnScreen[1][1]; chunkY++)
			{
				string chunkKey;
				chunkKey.append(to_string(chunkX));
				chunkKey.push_back('|');
				chunkKey.append(to_string(chunkY));

				std::map<std::string, GameChunk>::iterator it = worldMap.find(chunkKey);
				if (it == worldMap.end())
				{
					// std::cout << " New Chunk - On Screen";
					// void genNewChunk(string& chunkKey, int& chunkX, int& chunkY, int seed[2], sf::RenderWindow& window, sf::RectangleShape& rect)
					genNewChunk(chunkKey, chunkX, chunkY, seed);
					std::map<std::string, GameChunk>::iterator it = worldMap.find(chunkKey);
					if (it == worldMap.end())
					{}
					else
					{
						GameChunk& currentChunk = it->second;
						for (int xInChunk = 0; xInChunk < chunkSize; xInChunk++)
						{
							for (int yInChunk = 0; yInChunk < chunkSize; yInChunk++)
							{
								GameTile currentTile = currentChunk.tiles[yInChunk][xInChunk];
								rect.setPosition((chunkX * chunkSize * boxSize + xInChunk * boxSize) - offset[0], (chunkY * chunkSize * boxSize + yInChunk * boxSize) - offset[1]);
								rect.setFillColor(currentTile.colour);
								window.draw(rect);
							}
						}
					}
				}
				else
				{
					GameChunk& currentChunk = it->second;
					for (int xInChunk = 0; xInChunk < chunkSize; xInChunk++)
					{
						for (int yInChunk = 0; yInChunk < chunkSize; yInChunk++)
						{
							GameTile currentTile = currentChunk.tiles[yInChunk][xInChunk];
							rect.setPosition((chunkX * chunkSize * boxSize + xInChunk * boxSize) - offset[0], (chunkY * chunkSize * boxSize + yInChunk * boxSize) - offset[1]);
							rect.setFillColor(currentTile.colour);
							window.draw(rect);
						}
					}
				}
			}
		}

		// offScreenThreadList.clear();
		// If thread Object is Joinable then Join that thread.
		if (genNewOffscreenThread.joinable())
			genNewOffscreenThread.join();
		// Check for offscreen chunks and gen themseconds_since_start
		// void genNewChunk(string& chunkKey, int& chunkX, int& chunkY, int seed[2], sf::RenderWindow& window, sf::RectangleShape& rect)
		std::thread offscreenThread(checkForMissingOffscreenChunks, chunksOnScreen, seed);
		genNewOffscreenThread = std::move(offscreenThread);

		totalFrames++;
		double seconds_since_start = difftime(time(0), start);
		cout << "Framerate: " << totalFrames / seconds_since_start << "\n";
		window.display();
	}
	return 0;
}

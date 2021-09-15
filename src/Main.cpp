#include "Platform/Platform.hpp"
#include <math.h>
// noise.hpp is included in terrain.hpp
int boxSize = 10;
float zoomScale = 0.1;
const int chunkSize = 8;
#include "terrain.hpp"
#include <time.h>
// #include <map>

using namespace std;
using namespace sf;

// useful stuff
const double currentLocation[2] = { 0, 0 };
float offset[2] = { 0, 0 };

// Threads6
vector<std::thread> offScreenThreadList;
int usableVectorSpaces = 0;
bool firstRun = false;
bool titleScreen = true;
bool changeBoxSizeSlider = false;

// map of chunks
std::map<std::string, GameChunk> worldMap;

void genNewChunk(string& chunkKey, int& chunkX, int& chunkY, int seed[2]);
void genNewChunk(string& chunkKey, int& chunkX, int& chunkY, int seed[2])
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

double MapValue(double a0, double a1, double b0, double b1, double a);
double MapValue(double a0, double a1, double b0, double b1, double a)
{
	return b0 + (b1 - b0) * ((a - a0) / (a1 - a0));
}

int main()
{
	util::Platform platform;

#if defined(_DEBUG)
	std::cout << "Hello World!" << std::endl;
#endif
	int screenSize[2] = { 1000, 1000 };

	// Initialize stuff
	/* initialize random seed: */
	srand(time(NULL));

	int seed[2] = { rand() % 100000 + 1000000, rand() % 100000 + 1000000 };

	// Threads
	std::thread genNewOffscreenThread;

	time_t start = time(0);
	int totalFrames = 0;

	int mouseCord[2];
	bool mouseBtn[2] = { 0, 0 };

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
	sf::Font font;
	if (!font.loadFromFile("./content/BAHNSCHRIFT.ttf"))
	{
		cout << "Error loading font";
		if (!font.loadFromFile("./src/BAHNSCHRIFT.ttf"))
		{
			cout << "Error loading font";
		}
	}
	sf::Text text;
	text.setFillColor(sf::Color::White);
	text.setCharacterSize(200);
	text.setFont(font);
	sf::RectangleShape sliderBar(sf::Vector2f(100, 100));
	sliderBar.setFillColor(sf::Color(150, 24, 24));
	sf::RectangleShape slider(sf::Vector2f(20, 30));
	slider.setFillColor(sf::Color(28, 24, 150));
	sf::RectangleShape playButton(sf::Vector2f(300, 100));
	playButton.setFillColor(sf::Color(219, 33, 8));
	playButton.setOrigin(sf::Vector2f(150, 50));

	// Limit the framerate to 60 frames per second (this step is optional)
	window.setFramerateLimit(120);

	sf::Event event;

	while (window.isOpen())
	{
		int boxesDrawn = 0;
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

		if (titleScreen)
		{
			mouseBtn[0] = sf::Mouse::isButtonPressed(sf::Mouse::Left);
			mouseBtn[1] = sf::Mouse::isButtonPressed(sf::Mouse::Right);
			mouseCord[0] = sf::Mouse::getPosition(window).x;
			mouseCord[1] = sf::Mouse::getPosition(window).y;

			window.clear();
			text.setString("Xlien");
			text.setCharacterSize(200);
			text.setOrigin(sf::Vector2f(210, 10));
			text.setPosition(sf::Vector2f(screenSize[0] / 2, screenSize[1] / 4));
			text.setFillColor(sf::Color(219, 33, 8));
			window.draw(text);
			text.setFillColor(sf::Color::White);
			// set box size
			if (mouseBtn[0])
			{
				if (mouseCord[1] >= screenSize[1] - 50 && mouseCord[1] <= screenSize[1] - 30 && mouseCord[0] > 50 && mouseCord[0] < screenSize[0] - 50)
				{
					changeBoxSizeSlider = true;
				}
				/*playButton.setFillColor(sf::Color(219, 33, 8));
				playButton.setOrigin(sf::Vector2f(150, 50));*/
				if (mouseCord[0] >= screenSize[0] / 2 - 150 && mouseCord[0] <= screenSize[0] / 2 + 150 && mouseCord[1] >= screenSize[1] / 1.75 - 50 && mouseCord[1] <= screenSize[1] / 1.75 + 50)
				{
					titleScreen = false;
				}
			}
			else
			{
				changeBoxSizeSlider = false;
			}

			if (changeBoxSizeSlider && mouseCord[0] > 50 && mouseCord[0] < screenSize[0] - 50)
			{
				boxSize = round(MapValue(50, screenSize[0] - 50, 1, 100, mouseCord[0]));
				rect.setSize(sf::Vector2f(boxSize, boxSize));
			}
			sliderBar.setSize(sf::Vector2f(screenSize[0] - 100, 20));
			sliderBar.setPosition(sf::Vector2f(50, screenSize[1] - 50));
			window.draw(sliderBar);
			text.setString(((string) "Box Size (heavily affects performance) = ").append(to_string(boxSize)));
			text.setCharacterSize(24);
			text.setOrigin(sf::Vector2f(0, 0));
			text.setPosition(sf::Vector2f(50, screenSize[1] - 100));
			window.draw(text);
			// slider size 20, 30
			// lowest size: 1, highest size: 100
			//double MapValue(double a0, double a1, double b0, double b1, double a);
			slider.setPosition(sf::Vector2f(MapValue(1, 100, 50, screenSize[0] - 50, boxSize) - 10, screenSize[1] - 55));
			window.draw(slider);
			/*sf::RectangleShape playButton(sf::Vector2f(100, 50));
			playButton.setFillColor(sf::Color(18, 223, 234));*/
			playButton.setPosition(sf::Vector2f(screenSize[0] / 2, screenSize[1] / 1.75));
			window.draw(playButton);
			text.setString("Play");
			text.setCharacterSize(75);
			text.setOrigin(sf::Vector2f(74, 55));
			text.setPosition(sf::Vector2f(screenSize[0] / 2, screenSize[1] / 1.75));
			text.setFillColor(sf::Color::Black);
			window.draw(text);

			window.display();
		}
		else
		{

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
				{ (int)floor(offset[0] / (chunkSize * boxSize)), (int)floor(offset[1] / (chunkSize * boxSize)) },											 //start pos
				{ (int)floor((offset[0] + screenSize[0]) / (chunkSize * boxSize)) + 1, (int)floor((offset[1] + screenSize[1]) / (chunkSize * boxSize)) + 1 } //end pos
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
									float xPos = (chunkX * chunkSize * boxSize + xInChunk * boxSize) - offset[0];
									if (xPos < screenSize[0])
									{
										float yPos = (chunkY * chunkSize * boxSize + yInChunk * boxSize) - offset[1];
										if (yPos + boxSize > 0)
										{
											GameTile currentTile = currentChunk.tiles[yInChunk][xInChunk];
											rect.setPosition(xPos, yPos);
											rect.setFillColor(currentTile.colour);
											window.draw(rect);
											boxesDrawn++;
										}
									}
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
								float xPos = (chunkX * chunkSize * boxSize + xInChunk * boxSize) - offset[0];
								if (xPos < screenSize[0])
								{
									float yPos = (chunkY * chunkSize * boxSize + yInChunk * boxSize) - offset[1];
									if (yPos + boxSize > 0)
									{
										GameTile currentTile = currentChunk.tiles[yInChunk][xInChunk];
										rect.setPosition(xPos, yPos);
										rect.setFillColor(currentTile.colour);
										window.draw(rect);
										boxesDrawn++;
									}
								}
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
			cout << "Framerate: " << totalFrames / seconds_since_start << "\nBoxes Drawn: " << boxesDrawn << "\n";
			window.display();
		}
	}
	return 0;
}

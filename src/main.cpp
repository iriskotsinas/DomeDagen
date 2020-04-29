//
//  Main.cpp provided under CC0 license
//
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <glm/gtx/string_cast.hpp>

#include "sgct/sgct.h"

#include "websockethandler.h"
#include "utility.hpp"
#include "game.hpp"
#include "sceneobject.hpp"
#include "player.hpp"
#include "modelmanager.hpp"

namespace {
	std::unique_ptr<WebSocketHandler> wsHandler;

	//Container for deserialized game state info
	std::vector<PlayerData> states;

	//TEMPORARY used to control rotation of all players 
	float updatedRotation{ 0 };

} // namespace

using namespace sgct;

/****************************
	FUNCTIONS DECLARATIONS
*****************************/
void initOGL(GLFWwindow*);
void draw(const RenderData& data);
void cleanup();

std::vector<std::byte> encode();
void decode(const std::vector<std::byte>& data, unsigned int pos);
void preSync();
void postSyncPreDraw();

void keyboard(Key key, Modifier modifier, Action action, int);

void connectionEstablished();
void connectionClosed();
void messageReceived(const void* data, size_t length);

/****************************
		CONSTANTS 
*****************************/
const std::string rootDir = Utility::findRootDir();

/****************************
			MAIN
*****************************/
int main(int argc, char** argv) {
	std::vector<std::string> arg(argv + 1, argv + argc);
	Configuration config = sgct::parseArguments(arg);

	//Choose which config file (.xml) to open
	config.configFilename = rootDir + "/src/configs/fisheye_testing.xml";
	//config.configFilename = rootDir + "/src/configs/simple.xml";
	//config.configFilename = rootDir + "/src/configs/six_nodes.xml";
	//config.configFilename = rootDir + "/src/configs/two_fisheye_nodes.xml";

	config::Cluster cluster = sgct::loadCluster(config.configFilename);

	//Provide functions to engine handles
	Engine::Callbacks callbacks;
	callbacks.initOpenGL = initOGL;
	callbacks.preSync = preSync;
	callbacks.encode = encode;
	callbacks.decode = decode;
	callbacks.postSyncPreDraw = postSyncPreDraw;
	callbacks.draw = draw;
	callbacks.cleanup = cleanup;
	callbacks.keyboard = keyboard;

	//Initialize engine
	try {
		Engine::create(cluster, callbacks, config);		
	}
	catch (const std::runtime_error & e) {
		Log::Error("%s", e.what());
		Engine::destroy();
		return EXIT_FAILURE;
	}

	if (Engine::instance().isMaster()) {
		wsHandler = std::make_unique<WebSocketHandler>(
			"localhost",
			81,
			connectionEstablished,
			connectionClosed,
			messageReceived
		);
		constexpr const int MessageSize = 1024;
		wsHandler->connect("example-protocol", MessageSize);
	}	
	/**********************************/
	/*			 Test Area			  */
	/**********************************/

	Engine::instance().render();

	//Game::destroy();
	Engine::destroy();
	return EXIT_SUCCESS;
}

void draw(const RenderData& data) {
	Game::instance().setMVP(data.modelViewProjectionMatrix);
	Game::instance().setV(data.viewMatrix);


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	Game::instance().render();

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		sgct::Log::Error("GL Error: 0x%x", err);
	}
}

void initOGL(GLFWwindow*) {
	ModelManager::init();
	Game::init();
	assert(std::is_pod<PlayerData>());

	/**********************************/
	/*			 Debug Area			  */
	/**********************************/

	//Game::instance().addPlayer();
}


void keyboard(Key key, Modifier modifier, Action action, int)
{
	if (key == Key::Esc && action == Action::Press) {
		Engine::instance().terminate();
	}
	if (key == Key::Space && modifier == Modifier::Shift && action == Action::Release)
	{
		Log::Info("Released space key, disconnecting");
		wsHandler->disconnect();
	}

	//Left
	if (key == Key::A && (action == Action::Press || action == Action::Repeat))
	{
		Game::instance().rotateAllPlayers(0.1f);
		//Game::instance().disablePlayer(0);
	}
	//Right
	if (key == Key::D && (action == Action::Press || action == Action::Repeat))
	{
		Game::instance().rotateAllPlayers(-0.1f);
		//Game::instance().enablePlayer(0);
	}
}

void preSync() {
	// Do the application simulation step on the server node in here and make sure that
	// the computed state is serialized and deserialized in the encode/decode calls	

	//Run game simulation on master only
	if (Engine::instance().isMaster())
	{
		// This doesn't have to happen every frame, but why not?
		wsHandler->tick();

		Game::instance().update();
	}
}

std::vector<std::byte> encode() {

	return Game::instance().getEncodedPlayerData();
}

void decode(const std::vector<std::byte>& data, unsigned int pos) {
	// These are just two examples;  remove them and replace them with the logic of your
	// application that you need to synchronize

	//Decode position data into states vector
	deserializeObject(data, pos, states);
}

void cleanup() {
	// Cleanup all of your state, particularly the OpenGL state in here.  This function
	// should behave symmetrically to the initOGL function
}

void postSyncPreDraw() {
	//Sync gameobjects' state on clients only
	if (!Engine::instance().isMaster())
	{
		Game::instance().setDecodedPositionData(states);

		//Clear states for next frame, not needed but it's polite
		states.clear();
	}
	else
		return;
}

void connectionEstablished() {
	Log::Info("Connection established");
}

void connectionClosed() {
	Log::Info("Connection closed");
}

void messageReceived(const void* data, size_t length) {
	std::string_view msg = std::string_view(reinterpret_cast<const char*>(data), length);
	//Log::Info("Message received: %s", msg.data());

	std::string message = msg.data();

	if (!message.empty())
	{
		//Get an easily manipulated stream of the message and read type of message
		std::istringstream iss(message);
		char msgType;
		iss >> msgType;

		// If first slot is 'N', a name and unique ID has been sent
		if (msgType == 'N') {
			Log::Info("Player connected: %s", message.c_str());
			Game::instance().addPlayer(Utility::getNewPlayerData(iss));
		}

		// If first slot is 'C', the rotation angle has been sent
		if (msgType == 'C') {
			Game::instance().updateTurnSpeed(Utility::getTurnSpeed(iss));
		}
        
        // If first slot is 'D', player to be deleted has been sent
        if (msgType == 'D') {
            unsigned int playerId;
            iss >> playerId;
            Game::instance().disablePlayer(playerId);
        }
        
        // If first slot is 'I', player's ID has been sent
        if (msgType == 'I') {
            unsigned int playerId;
            iss >> playerId;
            // Send colour information back to server
            std::pair<glm::vec3, glm::vec3> colours = Game::instance().getPlayerColours(playerId);
            std::string colourOne = glm::to_string(colours.first);
            std::string colourTwo = glm::to_string(colours.second);
            
            Log::Info("Player colour 1: %s", colourOne.c_str());
            Log::Info("Player colour 2: %s", colourTwo.c_str());
        }
	}
}

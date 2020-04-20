#include "player.hpp"
#include<glm/common.hpp>
#include<glm/gtx/string_cast.hpp>
#include<iostream>

Player::Player()
	: GameObject{ GameObject::PLAYER, 50.f, glm::quat(glm::vec3(0.f)), 0.f },
	GeometryHandler("player", "fish"),
	mName{ "temp" }, mPoints{ 0 }, mIsAlive{ true }, mSpeed{ 0.5f }
{
	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());
	setShaderData();
}

Player::Player(const std::string name)
	: GameObject{ GameObject::PLAYER, 50.f, glm::quat(glm::vec3(0.f)), 0.f },
	GeometryHandler("player", "fish"),
	mName{ name }, mPoints{ 0 }, mIsAlive{ true }, mSpeed{ 0.5f }
{
	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());
	setShaderData();
}

Player::Player(const std::string & objectModelName, float radius,
	           const glm::quat & position, float orientation,
	           const std::string & name, float speed)
	: GameObject{ GameObject::PLAYER, radius, position, orientation },
	  GeometryHandler("player", objectModelName),
	  mName { name }, mPoints{ 0 }, mIsAlive{ true }, mSpeed{ speed }
{
	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());
	setShaderData();
}

Player::Player(const PlayerData& input)
	: GameObject{ GameObject::PLAYER, input.mRadius, glm::quat{}, input.mOrientation },
	GeometryHandler("player", "fish"),
	mName{ std::string(input.mNameLength, ' ') }, mPoints{ input.mPoints }, mIsAlive{ input.mIsAlive }, mSpeed{ input.mSpeed }
{
	//Copy new player name
	for (size_t i = 0; i < input.mNameLength; i++)
	{
		mName[i] = input.mPlayerName[i];
	}

	glm::quat temp{};
		temp.w = input.mW;
		temp.x = input.mX;
		temp.y = input.mY;
		temp.z = input.mZ;
	setPosition(temp);

	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());
	setShaderData();
}

Player::~Player()
{
	sgct::Log::Info("Player with name=\"%s\" removed", mName.c_str());
}

PlayerData Player::getPlayerData(bool isNewPlayer) const
{
	PlayerData temp;
	temp.mNewPlayer = isNewPlayer;
	temp.mNameLength = getName().length();

	//Positional data
	temp.mOrientation = getOrientation();
	temp.mRadius = getRadius();
	temp.mScale = getScale();
	temp.mSpeed = getSpeed();

	//Quat stuff
	temp.mW = getPosition().w;
	temp.mX = getPosition().x;
	temp.mY = getPosition().y;
	temp.mZ = getPosition().z;

	//Game state data
	temp.mPoints = getPoints();
	temp.mIsAlive = isAlive();
	temp.mEnabled = isEnabled();

	//Send name if this is a new player not present on nodes yet
	if (temp.mNewPlayer)
	{
		for (size_t i = 0; i < mName.length(); i++)
		{
			temp.mPlayerName[i] = mName.c_str()[i];
		}
	}

	return temp;
}

void Player::setPlayerData(const PlayerData& newState)
{
	//Position data
	setOrientation(newState.mOrientation);
	setRadius(newState.mRadius);
	setScale(newState.mScale);
	setSpeed(newState.mSpeed);

	//Quat stuff
	glm::quat newPosition;
		newPosition.w = newState.mW;
		newPosition.x = newState.mX;
		newPosition.y = newState.mY;
		newPosition.z = newState.mZ;
	setPosition(newPosition);

	//Game state data
	setPoints(newState.mPoints);
	setIsAlive(newState.mIsAlive);	
	setEnabled(newState.mEnabled);
}

void Player::update(float deltaTime)
{
	if (!mEnabled)
		return;

	////Update orientation
	//setOrientation(getOrientation() + deltaTime * mTurnSpeed);

	//Update position on sphere
	glm::quat newPos = getPosition();
	newPos *= glm::normalize(glm::quat(
		mSpeed * deltaTime * glm::vec3(cos(getOrientation()), sin(getOrientation()), 0.f)
	));

	//const float orient = getOrientation();
	//const float angle = mSpeed * deltaTime;
	//const glm::vec3 rotAxis(cos(orient), sin(orient), 0.f);
	//newPos *= glm::angleAxis(angle, rotAxis);

	setPosition(glm::normalize(newPos)); //Normalize might not be necessary?

	//TODO Constrain to visible area
}

void Player::render(const glm::mat4& mvp) const
{
	if (!mEnabled)
		return;

	mShaderProgram.bind();

	glUniformMatrix4fv(mMvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniformMatrix4fv(mTransMatrixLoc, 1, GL_FALSE, glm::value_ptr(getTransformation()));
	this->renderModel();

	mShaderProgram.unbind();
}

void Player::setShaderData()
{
	mShaderProgram.bind();

	mMvpMatrixLoc = glGetUniformLocation(mShaderProgram.id(), "mvp");
	mTransMatrixLoc = glGetUniformLocation(mShaderProgram.id(), "transformation");

	mShaderProgram.unbind();
}

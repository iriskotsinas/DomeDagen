#include "player.hpp"
#include<glm/common.hpp>
#include<glm/gtx/string_cast.hpp>
#include<iostream>

Player::Player(const std::string & objectModelName, float radius,
	           const glm::quat & position, float orientation,
	           const std::string & name, float speed)
	: GameObject{ GameObject::PLAYER, radius, position, orientation },
	  GeometryHandler("player", objectModelName),
	  mName { name }, mPoints{ 0 }, mIsAlive{ true }, mSpeed{ speed },
	  mPrimaryColour{ allPlayerColours[rand() % allPlayerColours.size()] }, mSecondaryColour{ allPlayerColours[rand() % allPlayerColours.size()] }
{
	sgct::Log::Info("Player with name=\"%s\" created", mName.c_str());

	mShaderProgram.bind();

	mMvpMatrixLoc = glGetUniformLocation(mShaderProgram.id(), "mvp");
	mTransMatrixLoc = glGetUniformLocation(mShaderProgram.id(), "transformation");

	// frans; More color things
	mPrimaryColLoc = glGetUniformLocation(mShaderProgram.id(), "primaryCol");
	mSecondaryColLoc = glGetUniformLocation(mShaderProgram.id(), "secondaryCol");

	mShaderProgram.unbind();
}

Player::~Player()
{
	sgct::Log::Info("Player with name=\"%s\" removed", mName.c_str());
}

void Player::update(float deltaTime)
{
	//Update orientation
	setOrientation(getOrientation() + deltaTime * mTurnSpeed);

	//Update position on sphere
	glm::quat newPos = getPosition();
	newPos *= glm::quat(
		mSpeed * deltaTime * glm::vec3(cos(getOrientation()), sin(getOrientation()), 0.f)
	);
	setPosition(glm::normalize(newPos)); //Normalize might not be necessary?

	//TODO Constrain to visible area
}

void Player::render(const glm::mat4& mvp) const
{
	mShaderProgram.bind();

	// frans; Even more color things!
	glUniform3fv(mPrimaryColLoc, 1, glm::value_ptr(mPrimaryColour));
	glUniform3fv(mSecondaryColLoc, 1, glm::value_ptr(mSecondaryColour));

	glUniformMatrix4fv(mMvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniformMatrix4fv(mTransMatrixLoc, 1, GL_FALSE, glm::value_ptr(getTransformation()));
	this->renderModel();

	mShaderProgram.unbind();
}
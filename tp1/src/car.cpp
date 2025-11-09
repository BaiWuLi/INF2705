#include "car.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


using namespace gl;
using namespace glm;

    
Car::Car()
: position(0.0f, 0.0f, 0.0f), orientation(0.0f, 0.0f), speed(0.f)
, wheelsRollAngle(0.f), steeringAngle(0.f)
, isHeadlightOn(false), isBraking(false)
, isLeftBlinkerActivated(false), isRightBlinkerActivated(false)
, isBlinkerOn(false), blinkerTimer(0.f)
{}

void Car::loadModels()
{
    frame_.load("../models/frame.ply");
    wheel_.load("../models/wheel.ply");
    blinker_.load("../models/blinker.ply");
    light_.load("../models/light.ply");
}

void Car::update(float deltaTime)
{
    if (isBraking)
    {
        const float LOW_SPEED_THRESHOLD = 0.1f;
        const float BRAKE_APPLIED_SPEED_THRESHOLD = 0.01f;
        const float BRAKING_FORCE = 4.f;
    
        if (fabs(speed) < LOW_SPEED_THRESHOLD)
            speed = 0.f;
            
        if (speed > BRAKE_APPLIED_SPEED_THRESHOLD)
            speed -= BRAKING_FORCE * deltaTime;
        else if (speed < -BRAKE_APPLIED_SPEED_THRESHOLD)
            speed += BRAKING_FORCE * deltaTime;
    }
    
    const float WHEELBASE = 2.7f;
    float angularSpeed = speed * sin(-glm::radians(steeringAngle)) / WHEELBASE;
    orientation.y += angularSpeed * deltaTime;
    
    glm::vec3 positionMod = glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(-speed, 0.f, 0.f, 1.f);
    position += positionMod * deltaTime;
    
    const float WHEEL_RADIUS = 0.2f;
    wheelsRollAngle += speed / (2.f * M_PI * WHEEL_RADIUS) * deltaTime;
    
    if (wheelsRollAngle > M_PI)
        wheelsRollAngle -= 2.f * M_PI;
    else if (wheelsRollAngle < -M_PI)
        wheelsRollAngle += 2.f * M_PI;
        
    if (isRightBlinkerActivated || isLeftBlinkerActivated)
    {
        const float BLINKER_PERIOD_SEC = 0.5f;
        blinkerTimer += deltaTime;
        if (blinkerTimer > BLINKER_PERIOD_SEC)
        {
            blinkerTimer = 0.f;
            isBlinkerOn = !isBlinkerOn;
        }
    }
    else
    {
        isBlinkerOn = true;
        blinkerTimer = 0.f;
    }  
}

void Car::draw(glm::mat4& projView)
{
	mat4 model = mat4(1.0f);
	model = translate(model, position);
	model = rotate(model, orientation.y, vec3(0.0f, 1.0f, 0.0f));
	model = rotate(model, orientation.x, vec3(1.0f, 0.0f, 0.0f));
	mat4 mvp = projView * model;

    drawFrame(mvp);
    drawWheels(mvp);
    drawHeadlights(mvp);
}
    
void Car::drawFrame(mat4& mvp)
{
	mat4 model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.25f, 0.0f));
	mat4 mmvp = mvp * model;

    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, value_ptr(mmvp));
    glUniform3fv(colorModUniformLocation, 1, value_ptr(vec3(1.0f, 1.0f, 1.0f)));
	frame_.draw();
}

void Car::drawWheel(mat4& mvp, vec3& pos)
{
    const float WHEEL_CENTER_OFFSET = 0.10124f;
    bool isFrontWheel = pos[0] < 0.0f;
    bool isLeftWheel = pos[2] > 0.0f;

    mat4 model = mat4(1.0f);

    model = translate(model, vec3(0.0f, 0.0f, -WHEEL_CENTER_OFFSET));
    model = translate(model, pos);

    if (isFrontWheel) {
        model = rotate(model, -radians(steeringAngle), vec3(0.0f, 1.0f, 0.0f));
    }

    model = rotate(model, wheelsRollAngle, vec3(0.0f, 0.0f, 1.0f));

    model = translate(model, vec3(0.0f, 0.0f, WHEEL_CENTER_OFFSET));

    if (isLeftWheel) {
        model = scale(model, vec3(1.0f, 1.0f, -1.0f));
    }

    mat4 mmvp = mvp * model;

    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, value_ptr(mmvp));
	glUniform3fv(colorModUniformLocation, 1, value_ptr(vec3(0.0f, 0.0f, 0.0f)));
	wheel_.draw();
}

void Car::drawWheels(mat4& mvp)
{
    const glm::vec3 WHEEL_POSITIONS[] =
    {
        glm::vec3(-1.29f, 0.245f, -0.57f),
        glm::vec3(-1.29f, 0.245f,  0.57f),
        glm::vec3( 1.4f , 0.245f, -0.57f),
        glm::vec3( 1.4f , 0.245f,  0.57f)
    };

    for (vec3 position : WHEEL_POSITIONS)
    {
		drawWheel(mvp, position);
	}
}

void Car::drawBlinker(mat4& mvp, vec3& pos)
{
    const float BLINKER_Z_POS = -0.06065f;
    
	bool isLeftHeadlight = pos[2] > 0.0f;
    bool isBlinkerActivated = (isLeftHeadlight  && isLeftBlinkerActivated) ||
                              (!isLeftHeadlight && isRightBlinkerActivated);

    const glm::vec3 ON_COLOR (1.0f, 0.7f , 0.3f );
    const glm::vec3 OFF_COLOR(0.5f, 0.35f, 0.15f);
    
	mat4 model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.0f, (isLeftHeadlight ? -1 : 1) * BLINKER_Z_POS));
	model = translate(model, pos);
	mat4 mmvp = mvp * model;

	glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, value_ptr(mmvp));
    if (isBlinkerOn && isBlinkerActivated)
        glUniform3fv(colorModUniformLocation, 1, value_ptr(ON_COLOR));
    else
		glUniform3fv(colorModUniformLocation, 1, value_ptr(OFF_COLOR));

	blinker_.draw();
}

void Car::drawLight(mat4& mvp, vec3& pos)
{
	const float LIGHT_Z_POS = 0.029f;

    const glm::vec3 FRONT_ON_COLOR (1.0f, 1.0f, 1.0f);
    const glm::vec3 FRONT_OFF_COLOR(0.5f, 0.5f, 0.5f);
    const glm::vec3 REAR_ON_COLOR  (1.0f, 0.1f, 0.1f);
    const glm::vec3 REAR_OFF_COLOR (0.5f, 0.1f, 0.1f);

	bool isFrontLight = pos[0] < 0.0f;
	bool isLeftHeadlight = pos[2] > 0.0f;
    
	mat4 model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.0f, (isLeftHeadlight ? -1 : 1) * LIGHT_Z_POS));
	model = translate(model, pos);
	mat4 mmvp = mvp * model;

    glUniformMatrix4fv(mvpUniformLocation, 1, GL_FALSE, value_ptr(mmvp));
    
    if (isFrontLight)
    {
        if (isHeadlightOn)
            glUniform3fv(colorModUniformLocation, 1, value_ptr(FRONT_ON_COLOR));
        else
            glUniform3fv(colorModUniformLocation, 1, value_ptr(FRONT_OFF_COLOR));
    }
    else
    {
        if (isBraking)
            glUniform3fv(colorModUniformLocation, 1, value_ptr(REAR_ON_COLOR));
        else
            glUniform3fv(colorModUniformLocation, 1, value_ptr(REAR_OFF_COLOR));
	}

	light_.draw();
}

void Car::drawHeadlight(mat4& mvp, vec3& pos)
{
    drawLight(mvp, pos);
	drawBlinker(mvp, pos);
}

void Car::drawHeadlights(mat4& mvp)
{
    const glm::vec3 HEADLIGHT_POSITIONS[] =
    {
        glm::vec3(-2.0019f, 0.64f, -0.45f),
        glm::vec3(-2.0019f, 0.64f,  0.45f),
        glm::vec3( 2.0019f, 0.64f, -0.45f),
        glm::vec3( 2.0019f, 0.64f,  0.45f)
    };

    for (vec3 position : HEADLIGHT_POSITIONS)
    {
        drawHeadlight(mvp, position);
    }
}


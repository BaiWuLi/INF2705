#include "car.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <map>

#include "shaders.hpp"


using namespace gl;
using namespace glm;

struct Material
{
    glm::vec4 emission; // vec3, but padded
    glm::vec4 ambient;  // vec3, but padded
    glm::vec4 diffuse;  // vec3, but padded
    glm::vec3 specular;
    GLfloat shininess;
};
    
Car::Car()
: position(0.0f, 0.0f, 0.0f), orientation(0.0f, 0.0f), speed(0.f)
, wheelsRollAngle(0.f), steeringAngle(0.f)
, isHeadlightOn(false), isBraking(false)
, isLeftBlinkerActivated(false), isRightBlinkerActivated(false)
, isBlinkerOn(false), blinkerTimer(0.f)
{}

void Car::loadModels()
{
    const char* WINDOW_MODEL_PATHES[] =
    {
        "../models/window.f.ply",
        "../models/window.r.ply",
        "../models/window.fl.ply",
        "../models/window.fr.ply",
        "../models/window.rl.ply",
        "../models/window.rr.ply"
    };
    for (unsigned int i = 0; i < 6; ++i)
    {
        windows[i].load(WINDOW_MODEL_PATHES[i]);
    }

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

    carModel = glm::mat4(1.0f);
    carModel = glm::translate(carModel, position);
    carModel = glm::rotate(carModel, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Car::draw(glm::mat4& projView, glm::mat4& view)
{
	mat4 model = mat4(1.0f);
	model = translate(model, position);
	model = rotate(model, orientation.y, vec3(0.0f, 1.0f, 0.0f));
	model = rotate(model, orientation.x, vec3(1.0f, 0.0f, 0.0f));
	mat4 mvp = projView * model;

    drawFrame(mvp, view);
    drawWheels(mvp, view);
    drawHeadlights(mvp, view);
}
    
void Car::drawFrame(mat4& mvp, mat4& view)
{
	mat4 model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.25f, 0.0f));
	mat4 frameMvp = mvp * model;

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	celShadingShader->use();
	mat4 worldModel = carModel * model;
	celShadingShader->setMatrices(frameMvp, view, worldModel);
	frame_.draw();
    
    glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	edgeEffectShader->use();
	glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, value_ptr(frameMvp));
    glDepthMask(GL_FALSE);
	frame_.draw();
	glDepthMask(GL_TRUE);
	glDisable(GL_STENCIL_TEST);
}

void Car::drawWindows(glm::mat4& projView, glm::mat4& view)
{
    const glm::vec3 WINDOW_POSITION[] =
    {
        glm::vec3(-0.813, 0.755, 0.0),
        glm::vec3(1.092, 0.761, 0.0),
        glm::vec3(-0.3412, 0.757, 0.51),
        glm::vec3(-0.3412, 0.757, -0.51),
        glm::vec3(0.643, 0.756, 0.508),
        glm::vec3(0.643, 0.756, -0.508)
    };

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::map<float, unsigned int> sorted;
	vec3 cameraPos = vec3(inverse(view)[3]);
    for (unsigned int i = 0; i < 6; i++)
    {
        mat4 model1 = carModel;
		model1 = translate(model1, vec3(0.0f, 0.25f, 0.0f));
		model1 = translate(model1, WINDOW_POSITION[i]);
		float distance = length(cameraPos - vec3(model1[3]));
		sorted[distance] = i;
    }

    for (std::map<float, unsigned int>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
		unsigned int i = it->second;
        mat4 model = mat4(1.0f);
        model = translate(model, position);
        model = rotate(model, orientation.y, vec3(0.0f, 1.0f, 0.0f));
        model = rotate(model, orientation.x, vec3(1.0f, 0.0f, 0.0f));
		model = translate(model, vec3(0.0f, 0.25f, 0.0f));
		mat4 mvp = projView * model;

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 2, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        celShadingShader->use();
		celShadingShader->setMatrices(mvp, view, model);
		windows[i].draw();

        glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		edgeEffectShader->use();
		glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, value_ptr(mvp));
		windows[i].draw();
		glDisable(GL_STENCIL_TEST);
    }

	glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
}

void Car::drawWheel(mat4& mvp, mat4& view, vec3& pos)
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

    mat4 wheelMvp = mvp * model;

	glDisable(GL_CULL_FACE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    celShadingShader->use();
	celShadingShader->setMatrices(wheelMvp, view, model);
	wheel_.draw();

    glStencilFunc(GL_NOTEQUAL, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	edgeEffectShader->use();
	glUniformMatrix4fv(edgeEffectShader->mvpULoc, 1, GL_FALSE, value_ptr(wheelMvp));
	wheel_.draw();
	glDisable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
}

void Car::drawWheels(mat4& mvp, mat4& view)
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
		drawWheel(mvp, view, position);
	}
}

void Car::drawBlinker(mat4& mvp, mat4& view, vec3& pos)
{
    const float BLINKER_Z_POS = -0.06065f;
    
	bool isLeftHeadlight = pos[2] > 0.0f;
    bool isBlinkerActivated = (isLeftHeadlight  && isLeftBlinkerActivated) ||
                              (!isLeftHeadlight && isRightBlinkerActivated);

    const glm::vec3 ON_COLOR (1.0f, 0.7f , 0.3f );
    const glm::vec3 OFF_COLOR(0.5f, 0.35f, 0.15f);

    Material blinkerMat =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {OFF_COLOR, 0.0f},
        {OFF_COLOR, 0.0f},
        {OFF_COLOR},
        10.0f
    };
    
	mat4 model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.0f, (isLeftHeadlight ? -1 : 1) * BLINKER_Z_POS));
	model = translate(model, pos);
	mat4 blinkerMvp = mvp * model;

	celShadingShader->use();
	celShadingShader->setMatrices(blinkerMvp, view, model);

    if (isBlinkerOn && isBlinkerActivated)
        blinkerMat.emission = glm::vec4(ON_COLOR, 0.0f);
    else
        blinkerMat.emission = glm::vec4(OFF_COLOR, 0.0f);
    
	material->updateData(&blinkerMat, 0, sizeof(Material));
	blinker_.draw();
}

void Car::drawLight(mat4& mvp, mat4& view, vec3& pos)
{
	const float LIGHT_Z_POS = 0.029f;

    const glm::vec3 FRONT_ON_COLOR (1.0f, 1.0f, 1.0f);
    const glm::vec3 FRONT_OFF_COLOR(0.5f, 0.5f, 0.5f);
    const glm::vec3 REAR_ON_COLOR  (1.0f, 0.1f, 0.1f);
    const glm::vec3 REAR_OFF_COLOR (0.5f, 0.1f, 0.1f);

    Material lightFrontMat =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {FRONT_OFF_COLOR, 0.0f},
        {FRONT_OFF_COLOR, 0.0f},
        {FRONT_OFF_COLOR},
        10.0f
    };

    Material lightRearMat =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {REAR_OFF_COLOR, 0.0f},
        {REAR_OFF_COLOR, 0.0f},
        {REAR_OFF_COLOR},
        10.0f
    };

	bool isFrontLight = pos[0] < 0.0f;
	bool isLeftHeadlight = pos[2] > 0.0f;
    
	mat4 model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.0f, (isLeftHeadlight ? -1 : 1) * LIGHT_Z_POS));
	model = translate(model, pos);
	mat4 lightMvp = mvp * model;

	celShadingShader->use();
	celShadingShader->setMatrices(lightMvp, view, model);
    
    if (isFrontLight)
    {
        if (isHeadlightOn)
            lightFrontMat.emission = glm::vec4(FRONT_ON_COLOR, 0.0f);
        else
            lightFrontMat.emission = glm::vec4(0.0f);
		material->updateData(&lightFrontMat, 0, sizeof(Material));
    }
    else
    {
        if (isBraking)
            lightRearMat.emission = glm::vec4(REAR_ON_COLOR, 0.0f);
        else
            lightRearMat.emission = glm::vec4(0.0f);
		material->updateData(&lightRearMat, 0, sizeof(Material));
    }

	light_.draw();
}

void Car::drawHeadlight(mat4& mvp, mat4& view, vec3& pos)
{
    drawLight(mvp, view, pos);
	drawBlinker(mvp, view, pos);
}

void Car::drawHeadlights(mat4& mvp, mat4& view)
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
        drawHeadlight(mvp, view, position);
    }
}


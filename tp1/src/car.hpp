#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "model.hpp"

class Car
{   
public:
    Car();
    
    void loadModels();
    
    void update(float deltaTime);
    
    void draw(glm::mat4& projView);
    
private:
    void drawFrame(glm::mat4& mvp);
    
    void drawWheel(glm::mat4& mvp, glm::vec3& pos);
    void drawWheels(glm::mat4& mvp);
    
    void drawBlinker(glm::mat4& mvp, glm::vec3& pos);
    void drawLight(glm::mat4& mvp, glm::vec3& pos);    
    void drawHeadlight(glm::mat4& mvp, glm::vec3& pos);
    void drawHeadlights(glm::mat4& mvp);
    
private:    
    Model frame_;
    Model wheel_;
    Model blinker_;
    Model light_;
    
public:
    glm::vec3 position;
    glm::vec2 orientation;    
    
    float speed;
    float wheelsRollAngle;
    float steeringAngle;
    bool isHeadlightOn;
    bool isBraking;
    bool isLeftBlinkerActivated;
    bool isRightBlinkerActivated;
    
    bool isBlinkerOn;
    float blinkerTimer;
    
    GLuint colorModUniformLocation;
    GLuint mvpUniformLocation;
};



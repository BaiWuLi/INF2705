#pragma once

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "model.hpp"
#include "uniform_buffer.hpp"

class EdgeEffect;
class CelShading;

class Car
{   
public:
    Car();
    
    void loadModels();
    
    void update(float deltaTime);
    
    void draw(glm::mat4& projView, glm::mat4& view); // À besoin de la matrice de vue séparément, pour la partie 3.
    
    void drawWindows(glm::mat4& projView, glm::mat4& view); // Dessin des vitres séparées.
private:
	void drawFrame(glm::mat4& mvp, glm::mat4& view);
    
    void drawWheel(glm::mat4& mvp, glm::mat4& view, glm::vec3& pos);
    void drawWheels(glm::mat4& mvp, glm::mat4& view);
    
    void drawBlinker(glm::mat4& mvp, glm::mat4& view, glm::vec3& pos);
    void drawLight(glm::mat4& mvp, glm::mat4& view, glm::vec3& pos);    
    void drawHeadlight(glm::mat4& mvp, glm::mat4& view, glm::vec3& pos);
    void drawHeadlights(glm::mat4& mvp, glm::mat4& view);
    
private:
    Model windows[6]; // Nouveaux modèles à ajouter.
    Model frame_;
    Model wheel_;
    Model blinker_;
    Model light_;
    
public:
    glm::vec3 position;
    glm::vec2 orientation;
    glm::mat4 carModel;
    
    float speed;
    float wheelsRollAngle;
    float steeringAngle;
    bool isHeadlightOn;
    bool isBraking;
    bool isLeftBlinkerActivated;
    bool isRightBlinkerActivated;
    
    bool isBlinkerOn;
    float blinkerTimer;

    EdgeEffect* edgeEffectShader;
    CelShading* celShadingShader;
    UniformBuffer* material;
};



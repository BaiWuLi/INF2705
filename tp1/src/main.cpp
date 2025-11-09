#include <cstddef>
#include <cstdint>

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "happly.h"
#include <imgui/imgui.h>

#include <inf2705/OpenGLApplication.hpp>

#include "model.hpp"
#include "car.hpp"

#define CHECK_GL_ERROR printGLError(__FILE__, __LINE__)

using namespace gl;
using namespace glm;

struct Vertex {
    vec3 position;
    vec3 color;
};

struct App : public OpenGLApplication
{
    App()
        : nSide_(5)
        , oldNSide_(0)
        , cameraPosition_(0.f, 0.f, 0.f)
        , cameraOrientation_(0.f, 0.f)
        , currentScene_(0)
        , isMouseMotionEnabled_(false)
    {
    }

    void init() override
    {
        // Le message expliquant les touches de clavier.
        setKeybindMessage(
            "ESC : quitter l'application." "\n"
            "T : changer de scène." "\n"
            "W : déplacer la caméra vers l'avant." "\n"
            "S : déplacer la caméra vers l'arrière." "\n"
            "A : déplacer la caméra vers la gauche." "\n"
            "D : déplacer la caméra vers la droite." "\n"
            "Q : déplacer la caméra vers le bas." "\n"
            "E : déplacer la caméra vers le haut." "\n"
            "Flèches : tourner la caméra." "\n"
            "Souris : tourner la caméra" "\n"
            "Espace : activer/désactiver la souris." "\n"
        );

        // Config de base.

        glClearColor(0.5, 0.5, 0.5, 1.0);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

        loadShaderPrograms();

        // Partie 1
        initShapeData();

        // Partie 2
        loadModels();
    }


    void checkShaderCompilingError(const char* name, GLuint id)
    {
        GLint success;
        GLchar infoLog[1024];

        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(id, 1024, NULL, infoLog);
            glDeleteShader(id);
            std::cout << "Shader \"" << name << "\" compile error: " << infoLog << std::endl;
        }
    }


    void checkProgramLinkingError(const char* name, GLuint id)
    {
        GLint success;
        GLchar infoLog[1024];

        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(id, 1024, NULL, infoLog);
            glDeleteProgram(id);
            std::cout << "Program \"" << name << "\" linking error: " << infoLog << std::endl;
        }
    }


    // Appelée à chaque trame. Le buffer swap est fait juste après.
    void drawFrame() override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui::Begin("Scene Parameters");
        ImGui::Combo("Scene", &currentScene_, SCENE_NAMES, N_SCENE_NAMES);
        ImGui::End();

        switch (currentScene_)
        {
        case 0: sceneShape();  break;
        case 1: sceneModels(); break;
        }
    }

    // Appelée lorsque la fenêtre se ferme.
    void onClose() override
    {
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ebo_);
        glDeleteVertexArrays(1, &vao_);
        glDeleteProgram(basicSP_);
        glDeleteProgram(transformSP_);
    }

    // Appelée lors d'une touche de clavier.
    void onKeyPress(const sf::Event::KeyPressed& key) override
    {
        using enum sf::Keyboard::Key;
        switch (key.code)
        {
        case Escape:
            window_.close();
            break;
        case Space:
            isMouseMotionEnabled_ = !isMouseMotionEnabled_;
            if (isMouseMotionEnabled_)
            {
                window_.setMouseCursorGrabbed(true);
                window_.setMouseCursorVisible(false);
            }
            else
            {
                window_.setMouseCursorGrabbed(false);
                window_.setMouseCursorVisible(true);
            }
            break;
        case T:
            currentScene_ = ++currentScene_ < N_SCENE_NAMES ? currentScene_ : 0;
            break;
        default: break;
        }
    }

    void onResize(const sf::Event::Resized& event) override
    {
    }

    void onMouseMove(const sf::Event::MouseMoved& mouseDelta) override
    {
        if (!isMouseMotionEnabled_)
            return;

        const float MOUSE_SENSITIVITY = 0.1;
        float cameraMouvementX = mouseDelta.position.y * MOUSE_SENSITIVITY;
        float cameraMouvementY = mouseDelta.position.x * MOUSE_SENSITIVITY;
        cameraOrientation_.y -= cameraMouvementY * deltaTime_;
        cameraOrientation_.x -= cameraMouvementX * deltaTime_;
    }

    void updateCameraInput()
    {
        if (!window_.hasFocus())
            return;

        if (isMouseMotionEnabled_)
        {
            sf::Vector2u windowSize = window_.getSize();
            sf::Vector2i windowHalfSize(windowSize.x / 2.0f, windowSize.y / 2.0f);
            sf::Mouse::setPosition(windowHalfSize, window_);
        }

        float cameraMouvementX = 0;
        float cameraMouvementY = 0;

        const float KEYBOARD_MOUSE_SENSITIVITY = 1.5f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            cameraMouvementX -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            cameraMouvementX += KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            cameraMouvementY -= KEYBOARD_MOUSE_SENSITIVITY;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            cameraMouvementY += KEYBOARD_MOUSE_SENSITIVITY;

        cameraOrientation_.y -= cameraMouvementY * deltaTime_;
        cameraOrientation_.x -= cameraMouvementX * deltaTime_;

        // Keyboard input
        glm::vec3 positionOffset = glm::vec3(0.0);
        const float SPEED = 10.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            positionOffset.z -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            positionOffset.z += SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            positionOffset.x -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            positionOffset.x += SPEED;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
            positionOffset.y -= SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            positionOffset.y += SPEED;

        positionOffset = glm::rotate(glm::mat4(1.0f), cameraOrientation_.y, glm::vec3(0.0, 1.0, 0.0)) * glm::vec4(positionOffset, 1);
        cameraPosition_ += positionOffset * glm::vec3(deltaTime_);
    }

    void loadModels()
    {
        car_.loadModels();
        tree_.load("../models/tree.ply");
        streetlight_.load("../models/streetlight.ply");
        grass_.load("../models/grass.ply");
        street_.load("../models/street.ply");
    }

    GLuint loadShaderObject(GLenum type, const char* path)
    {
        std::string shaderCode = readFile(path);
        const char* shaderSource = shaderCode.c_str();

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &shaderSource, nullptr);
        glCompileShader(shader);

        App::checkShaderCompilingError(path, shader);

        return shader;
    }

    void loadShaderPrograms()
    {
        // Partie 1
        const char* COLOR_VERTEX_SRC_PATH = "./shaders/basic.vs.glsl";
        const char* COLOR_FRAGMENT_SRC_PATH = "./shaders/basic.fs.glsl";

        GLuint colorVertexShader = loadShaderObject(GL_VERTEX_SHADER, COLOR_VERTEX_SRC_PATH);
        GLuint colorFragmentShader = loadShaderObject(GL_FRAGMENT_SHADER, COLOR_FRAGMENT_SRC_PATH);
        basicSP_ = glCreateProgram();

        glAttachShader(basicSP_, colorVertexShader);
        glAttachShader(basicSP_, colorFragmentShader);
        glLinkProgram(basicSP_);

        App::checkProgramLinkingError("Color Program", basicSP_);

        glDetachShader(basicSP_, colorVertexShader);
        glDetachShader(basicSP_, colorFragmentShader);
        glDeleteShader(colorVertexShader);
        glDeleteShader(colorFragmentShader);

        // Partie 2
        const char* TRANSFORM_VERTEX_SRC_PATH = "./shaders/transform.vs.glsl";
        const char* TRANSFORM_FRAGMENT_SRC_PATH = "./shaders/transform.fs.glsl";

        GLuint transformVertexShader = loadShaderObject(GL_VERTEX_SHADER, TRANSFORM_VERTEX_SRC_PATH);
        GLuint transformFragmentShader = loadShaderObject(GL_FRAGMENT_SHADER, TRANSFORM_FRAGMENT_SRC_PATH);
        transformSP_ = glCreateProgram();

        glAttachShader(transformSP_, transformVertexShader);
        glAttachShader(transformSP_, transformFragmentShader);
        glLinkProgram(transformSP_);

        App::checkProgramLinkingError("Transform Program", transformSP_);

        glDetachShader(transformSP_, transformVertexShader);
        glDetachShader(transformSP_, transformFragmentShader);
        glDeleteShader(transformVertexShader);
        glDeleteShader(transformFragmentShader);

		mvpUniformLocation_ = glGetUniformLocation(transformSP_, "mvp");
		colorModUniformLocation_ = glGetUniformLocation(transformSP_, "colorMod");
		car_.mvpUniformLocation = mvpUniformLocation_;
		car_.colorModUniformLocation = colorModUniformLocation_;
    }

    void generateNgon(void* vertices, void* elements, unsigned int side)
    {       
        const float RADIUS = 0.7f;

        vertices_[0].position = vec3(0.0f, 0.0f, 0.0f);
        vertices_[0].color = vec3(1.0f, 1.0f, 1.0f);

        for (unsigned int i = 0; i < side; i++) {
            float angle = 2.0f * M_PI * i / side + M_PI / 2;
            float x = RADIUS * cos(angle);
            float y = RADIUS * sin(angle);
            vertices_[i + 1].position = vec3(x, y, 0.0f);

            vec3 color = vec3(0.0f, 0.0f, 0.0f);
            color[i % 3] = 1.0f;
			vertices_[(side - i) % side + 1].color = color; // Les couleurs sont attribuées en ordre inverse
        }

        for (unsigned int i = 0; i < side; i++) {
            elements_[i * 3 + 0] = 0;
            elements_[i * 3 + 1] = i + 1;
            elements_[i * 3 + 2] = i + 2 <= side ? i + 2 : 1;
        }
    }

    void initShapeData()
    {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(1);
        
        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements_), nullptr, GL_DYNAMIC_DRAW);
        
        glBindVertexArray(0);
    }

    void sceneShape()
    {
        ImGui::Begin("Scene Parameters");
        ImGui::SliderInt("Sides", &nSide_, MIN_N_SIDES, MAX_N_SIDES);
        ImGui::End();

        bool hasNumberOfSidesChanged = nSide_ != oldNSide_;
        if (hasNumberOfSidesChanged)
        {
            oldNSide_ = nSide_;
            generateNgon(vertices_, elements_, nSide_);

            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_), vertices_);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(elements_), elements_);
        }

        glUseProgram(basicSP_);
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, nSide_ * 3, GL_UNSIGNED_INT, 0);
    }

    void drawStreetlights(glm::mat4& projView)
    {
		//unsigned int seed = 0;
        //srand(seed);

		auto randBetween = [](float min, float max) {
            return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
        };

        unsigned int numStreetlights = 7;
		float distanceFromRoad = 2.5f;
		float x = -50.0f;

        glUseProgram(transformSP_);

        for (unsigned int i = 0; i < numStreetlights; i++) {
            x += randBetween(10, 20);
			float y = -0.15f;
			float z = -(distanceFromRoad + 0.5f);

			float angle = M_PI / 2;

			mat4 model = mat4(1.0f);
			model = translate(model, vec3(x, y, z));
			model = rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
			mat4 mvp = projView * model;

			glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, value_ptr(mvp));
			glUniform3fv(colorModUniformLocation_, 1, value_ptr(vec3(1.0f, 1.0f, 1.0f)));
			streetlight_.draw();
        }
    }

    void drawTrees(glm::mat4& projView)
    {
		unsigned int seed = 0;
		srand(seed);

        auto randBetween = [](float min, float max) {
			return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
        };

        unsigned int numTrees = 10;
        float distanceFromRoad = 2.5f;
		float x = -50.0f;

        glUseProgram(transformSP_);

        for (unsigned int i = 0; i < numTrees; i++) {
            x += randBetween(5, 11);
			float y = -0.15f;
            
            float z = distanceFromRoad + randBetween(1.5f, 3.5f);

			float angle = randBetween(0, 2 * M_PI);
			float scale = randBetween(0.6f, 1.2f);

			mat4 model = mat4(1.0f);
			model = translate(model, vec3(x, y, z));
			model = rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, vec3(scale, scale, scale));
			mat4 mvp = projView * model;

			glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, value_ptr(mvp));
			glUniform3fv(colorModUniformLocation_, 1, value_ptr(vec3(1.0f, 1.0f, 1.0f)));
			tree_.draw();
        }
    }

    void drawGround(glm::mat4& projView)
    {
        {
            mat4 model = scale(mat4(1.0f), vec3(100.0f, 1.0f, 5.0f));
            mat4 mvp = projView * model;
            glUseProgram(transformSP_);
			glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, value_ptr(mvp));
			glUniform3fv(colorModUniformLocation_, 1, value_ptr(vec3(1.0f, 1.0f, 1.0f)));
            street_.draw();
        }
        {
            glDisable(GL_CULL_FACE);
			mat4 model = mat4(1.0f);
			model = translate(model, vec3(0.0f, -0.1f, 0.0f));
			model = scale(model, vec3(100.0f, 1.0f, 50.0f));
			mat4 mvp = projView * model;
			glUseProgram(transformSP_);
			glUniformMatrix4fv(mvpUniformLocation_, 1, GL_FALSE, value_ptr(mvp));
			glUniform3fv(colorModUniformLocation_, 1, value_ptr(vec3(1.0f, 1.0f, 1.0f)));
			grass_.draw();
            glEnable(GL_CULL_FACE);
        }
    }

    glm::mat4 getViewMatrix()
    {
		mat4 view = mat4(1.0f);

		view = rotate(view, -cameraOrientation_.x, vec3(1.0f, 0.0f, 0.0f));
		view = rotate(view, -cameraOrientation_.y, vec3(0.0f, 1.0f, 0.0f));
		view = translate(view, -cameraPosition_);

        return view;
    }

    glm::mat4 getPerspectiveProjectionMatrix()
    {
        float fov = radians(70.0f);
        sf::Vector2u windowSize = window_.getSize();
		float aspectRatio = (float)windowSize.x / (float)windowSize.y;
        float near = 0.1f;
        float far = 100.0f;

		return glm::perspective(fov, aspectRatio, near, far);
    }

    void sceneModels()
    {
        ImGui::Begin("Scene Parameters");
        ImGui::SliderFloat("Car speed", &car_.speed, -10.0f, 10.0f, "%.2f m/s");
        ImGui::SliderFloat("Steering Angle", &car_.steeringAngle, -30.0f, 30.0f, "%.2f°");
        if (ImGui::Button("Reset steering"))
            car_.steeringAngle = 0.f;
        ImGui::Checkbox("Headlight", &car_.isHeadlightOn);
        ImGui::Checkbox("Left Blinker", &car_.isLeftBlinkerActivated);
        ImGui::Checkbox("Right Blinker", &car_.isRightBlinkerActivated);
        ImGui::Checkbox("Brake", &car_.isBraking);
        ImGui::End();

        updateCameraInput();
        car_.update(deltaTime_);

		mat4 view = getViewMatrix();
		mat4 proj = getPerspectiveProjectionMatrix();
		mat4 projView = proj * view;

        drawGround(projView);
        drawTrees(projView);
        drawStreetlights(projView);
        car_.draw(projView);
    }

private:
    // Shaders
    GLuint basicSP_;
    GLuint transformSP_;
    GLuint colorModUniformLocation_;
    GLuint mvpUniformLocation_;

    // Partie 1
    GLuint vbo_, ebo_, vao_;

    static constexpr unsigned int MIN_N_SIDES = 5;
    static constexpr unsigned int MAX_N_SIDES = 12;

    Vertex  vertices_[MAX_N_SIDES + 1];
    GLuint  elements_[MAX_N_SIDES * 3];

    int nSide_, oldNSide_;

    // Partie 2
    Model tree_;
    Model streetlight_;
    Model grass_;
    Model street_;

    Car car_;

    glm::vec3 cameraPosition_;
    glm::vec2 cameraOrientation_;

    static constexpr unsigned int N_TREES = 12;
    static constexpr unsigned int N_STREETLIGHTS = 5;
    glm::mat4 treeModelMatrices_[N_TREES];
    glm::mat4 streetlightModelMatrices_[N_STREETLIGHTS];

    // Imgui var
    const char* const SCENE_NAMES[2] = {
        "Introduction",
        "3D Model & transformation",
    };
    const int N_SCENE_NAMES = sizeof(SCENE_NAMES) / sizeof(SCENE_NAMES[0]);
    int currentScene_;

    bool isMouseMotionEnabled_;
};


int main(int argc, char* argv[])
{
    WindowSettings settings = {};
    settings.fps = 60;
    settings.context.depthBits = 24;
    settings.context.stencilBits = 8;
    settings.context.antiAliasingLevel = 4;
    settings.context.majorVersion = 3;
    settings.context.minorVersion = 3;
    settings.context.attributeFlags = sf::ContextSettings::Attribute::Core;

    App app;
    app.run(argc, argv, "Tp1", settings);
}

#include "model.hpp"
#include <glm/glm.hpp>
#include "happly.h"

using namespace gl;
using namespace glm;

struct Vertex {
    vec3 position;
    vec3 color;
};

void Model::load(const char* path)
{
    // Chargement des données du fichier .ply.
    // Ne modifier pas cette partie.
    happly::PLYData plyIn(path);

    happly::Element& vertex = plyIn.getElement("vertex");
    std::vector<float> positionX = vertex.getProperty<float>("x");
    std::vector<float> positionY = vertex.getProperty<float>("y");
    std::vector<float> positionZ = vertex.getProperty<float>("z");
    
    std::vector<unsigned char> colorRed   = vertex.getProperty<unsigned char>("red");
    std::vector<unsigned char> colorGreen = vertex.getProperty<unsigned char>("green");
    std::vector<unsigned char> colorBlue  = vertex.getProperty<unsigned char>("blue");

    // Tableau de faces, une face est un tableau d'indices.
    // Les faces sont toutes des triangles dans nos modèles (donc 3 indices par face).
    std::vector<std::vector<unsigned int>> facesIndices = plyIn.getFaceIndices<unsigned int>();
    
	std::vector<Vertex> vertices;

    for (size_t i = 0; i < positionX.size(); i++) {
        Vertex vertex;
        vertex.position = vec3(positionX[i], positionY[i], positionZ[i]);
        vertex.color = vec3(colorRed[i] / 255.0f, colorGreen[i] / 255.0f, colorBlue[i] / 255.0f);
        vertices.push_back(vertex);
	}
    
    std::vector<unsigned int> indices;

    for (size_t i = 0; i < facesIndices.size(); i++) {
		indices.push_back(facesIndices[i][0]);
		indices.push_back(facesIndices[i][1]);
		indices.push_back(facesIndices[i][2]);
    }
    
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

	glGenBuffers(1, &ebo_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	count_ = (GLsizei)indices.size();
}

Model::~Model()
{
	glDeleteBuffers(1, &ebo_);
	glDeleteBuffers(1, &vbo_);
	glDeleteVertexArrays(1, &vao_);
}

void Model::draw()
{
	glBindVertexArray(vao_);
	glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, 0);
}


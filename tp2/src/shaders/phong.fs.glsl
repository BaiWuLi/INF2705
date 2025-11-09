#version 330 core

#define MAX_SPOT_LIGHTS 9
#define MAX_POINT_LIGHTS 4

in ATTRIBS_VS_OUT
{
    vec2 texCoords;
    vec3 normal;
    vec3 color;
} attribsIn;

in LIGHTS_VS_OUT
{
    vec3 obsPos;
    vec3 dirLightDir;
    
    vec3 spotLightsDir[MAX_SPOT_LIGHTS];
    vec3 spotLightsSpotDir[MAX_SPOT_LIGHTS];
    
    //vec3 pointLightsDir[MAX_POINT_LIGHTS];
} lightsIn;


struct Material
{
    vec3 emission;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirectionalLight
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 direction;
};

struct SpotLight
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 position;
    vec3 direction;
    float exponent;
    float openingAngle;
};

uniform int nSpotLights;
uniform vec3 globalAmbient;

layout (std140) uniform MaterialBlock
{
    Material mat;
};

layout (std140) uniform LightingBlock
{
    DirectionalLight dirLight;
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

uniform sampler2D diffuseSampler;

out vec4 FragColor;

float computeSpot(in float openingAngle, in float exponent, in vec3 spotDir, in vec3 lightDir, in vec3 normal)
{
    float alpha = dot(normalize(lightDir), normalize(spotDir));
    float cosAngle = cos(radians(openingAngle));

    if (alpha <= cosAngle)
        return 0.0;

    return pow(alpha, exponent);
}

vec3 computeSpotLightsColor(in vec3 texColor)
{
    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    for (int i = 0; i < nSpotLights; i++)
    {
        ambient += mat.ambient * spotLights[i].ambient;

        float attenuation = computeSpot(
            spotLights[i].openingAngle,
            spotLights[i].exponent,
            lightsIn.spotLightsSpotDir[i],
            lightsIn.spotLightsDir[i],
            attribsIn.normal
        );

        if (attenuation <= 0.0f)
            continue;

        vec3 n = normalize(gl_FrontFacing ? attribsIn.normal : -attribsIn.normal);
        vec3 l = normalize(lightsIn.spotLightsDir[i]);
        float nl = dot(n, l);
        if (nl <= 0.0f)
            continue;
        diffuse += attenuation * mat.diffuse * spotLights[i].diffuse * nl;

        vec3 r = reflect(-l, n);
        vec3 v = normalize(lightsIn.obsPos);
        float rv = dot(r, v);
        if (rv <= 0.0f)
            continue;
        specular += attenuation * mat.specular * spotLights[i].specular * pow(rv, mat.shininess);
    }

    return (ambient + diffuse) * texColor + specular;
}

vec3 computeDirectionalLightColor(in vec3 texColor)
{
    vec3 ambient = mat.ambient * dirLight.ambient;

    vec3 n = normalize(gl_FrontFacing ? attribsIn.normal : -attribsIn.normal);
    vec3 l = normalize(lightsIn.dirLightDir);
    float nl = dot(n, l);
    if (nl <= 0.0f)
    {
        return ambient * texColor;
    }
    vec3 diffuse = mat.diffuse * dirLight.diffuse * nl;

    vec3 r = reflect(-l, n);
    vec3 v = normalize(lightsIn.obsPos);
    float rv = dot(r, v);
    if (rv <= 0.0f)
    {
        return (ambient + diffuse) * texColor;
    }
    vec3 specular = mat.specular * dirLight.specular * pow(rv, mat.shininess);

    return (ambient + diffuse) * texColor + specular;
}

void main()
{
    vec4 texColor = texture(diffuseSampler, attribsIn.texCoords);

    vec3 fragColor = vec3(0.0f);

    fragColor += mat.emission;

    fragColor += mat.ambient * globalAmbient;

    fragColor *= texColor.rgb;

    fragColor += computeDirectionalLightColor(texColor.rgb);

    fragColor += computeSpotLightsColor(texColor.rgb);

    FragColor = vec4(fragColor, texColor.a);
}

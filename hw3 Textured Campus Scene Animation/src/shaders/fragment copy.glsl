#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D diffuseMap;
uniform bool hasDiffuseMap;

uniform vec3 material_Kd;

void main()
{
    // 只顯示紋理或材質顏色,不做光照計算
    vec3 objectColor;
    if (hasDiffuseMap)
    {
        objectColor = texture(diffuseMap, TexCoord).rgb;
    }
    else
    {
        objectColor = length(material_Kd) > 0.01 ? material_Kd : vec3(0.8);
    }
    
    FragColor = vec4(objectColor, 1.0);
}
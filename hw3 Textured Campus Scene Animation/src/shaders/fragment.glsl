#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform bool hasDiffuseMap;
uniform bool hasSpecularMap;

uniform vec3 material_Ka;
uniform vec3 material_Kd;
uniform vec3 material_Ks;
uniform float material_Ns;
uniform float material_d;

uniform vec3 lightColor;

void main()
{
    // === 獲取基礎顏色 ===
    vec3 objectColor;
    if (hasDiffuseMap)
    {
        objectColor = texture(diffuseMap, TexCoord).rgb;
    }
    else
    {
        objectColor = length(material_Kd) > 0.01 ? material_Kd : vec3(0.8, 0.8, 0.8);
    }
    
    // === 1. Ambient (降低環境光) ===
    vec3 ambient = 0.3 * objectColor;
    
    // === 2. Diffuse ===
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor * lightColor;
    
    // === 3. Specular ===
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float shininess = material_Ns > 1.0 ? material_Ns : 32.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    vec3 specularColor;
    if (hasSpecularMap)
    {
        specularColor = texture(specularMap, TexCoord).rgb;
    }
    else
    {
        specularColor = vec3(0.2);  // 降低高光強度
    }
    vec3 specular = spec * specularColor * lightColor;
    
    // === 4. 合成 ===
    vec3 result = ambient + diffuse + specular;
    
    FragColor = vec4(result, material_d);
}
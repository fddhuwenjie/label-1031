#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float Height;
in float Velocity;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 waterColorDeep;
uniform vec3 waterColorShallow;
uniform float time;

void main() {
    // 基于高度混合深浅水颜色
    float heightFactor = clamp((Height + 0.5) / 1.0, 0.0, 1.0);
    vec3 waterColor = mix(waterColorDeep, waterColorShallow, heightFactor);
    
    // 环境光
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * 0.5;
    
    // 镜面反射（Blinn-Phong）
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor * 0.8;
    
    // 菲涅尔效果
    float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    fresnel = clamp(fresnel, 0.0, 1.0);
    
    // 添加一些波光效果
    float sparkle = pow(spec, 8.0) * 2.0;
    
    // 雨滴溅射高光：速度绝对值超过阈值时叠加衰减白色光斑
    float splashHighlight = 0.0;
    float velocityThreshold = 0.3;
    float absVel = abs(Velocity);
    if (absVel > velocityThreshold) {
        splashHighlight = (absVel - velocityThreshold) / (2.0 - velocityThreshold);
        splashHighlight = clamp(splashHighlight, 0.0, 1.0);
        splashHighlight = splashHighlight * splashHighlight;
    }
    
    // 最终颜色
    vec3 result = (ambient + diffuse) * waterColor + specular + vec3(sparkle) + vec3(splashHighlight);
    
    // 透明度基于菲涅尔和高度
    float alpha = mix(0.7, 0.95, fresnel);
    
    FragColor = vec4(result, alpha);
}

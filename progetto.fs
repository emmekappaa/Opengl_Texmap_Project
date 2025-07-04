#version 330 core
// Fragment shader per normal mapping con spotlight e spotlight extra
// Calcola l'illuminazione combinando una spotlight (che segue la camera) e una spotlight extra, usando una normal map e shadowmap

// Input dal vertex shader
in VS_OUT {
    vec2 TexCoords;
    vec3 TangentLightDir;
    vec3 TangentViewDir;
    vec3 TangentSpotDir;
    // --- LUCE DX ---
    vec3 TangentLuceDxDir;
    vec3 TangentLuceDxConeDir;
    vec4 FragPosLuceDxSpace;
    // --- LUCE SX ---
    vec3 TangentLuceSxDir;
    vec3 TangentLuceSxConeDir;
    vec4 FragPosLuceSxSpace;
} fs_in;

out vec4 FragColor;

// Texture diffuse (colore), normal map, gloss e shadow map
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;
// --- LUCE DX ---
uniform sampler2D shadowMapLuceDx;
uniform mat4 luceDxSpaceMatrix;
uniform float luceDxAngle;
// --- LUCE SX ---
uniform sampler2D shadowMapLuceSx;
uniform mat4 luceSxSpaceMatrix;
uniform float luceSxAngle;
// --- LUCE CENTRALE ---
uniform sampler2D shadowMapLuceCentro;
uniform mat4 luceCentroSpaceMatrix;

uniform float intensitaLuciLaterali;


float ShadowCalculation(vec4 fragPosLightSpace, sampler2D shadowMap)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.002;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    if(projCoords.z > 1.0)
        shadow = 0.0;
    return shadow;
}

void main()
{
    vec3 normal = texture(texture_normal1, fs_in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    vec3 color = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 ambient = 0.28 * color;
    float gloss = texture(texture_specular1, fs_in.TexCoords).r;
    float shininess = mix(8.0, 128.0, gloss);

    // --- Spotlight (segue la camera) ---
    vec3 lightDir = normalize(fs_in.TangentLightDir);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    vec3 viewDir = normalize(fs_in.TangentViewDir);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = vec3(0.2) * spec;
    vec3 spotDir = normalize(fs_in.TangentSpotDir);
    float theta = dot(lightDir, spotDir);
    float epsilon = 0.15;
    float intensity = smoothstep(cos(radians(12.5)), cos(radians(12.5 + epsilon)), theta) * 0.55;
    vec3 spotlightResult = intensity * (diffuse + specular);

    // --- Luce DX ---
    //vec3 luceDxDir = normalize(fs_in.TangentLuceDxDir);
    //float luceDxDiff = max(dot(luceDxDir, normal), 0.0);
    //vec3 luceDxDiffuse = luceDxDiff * color;
    //vec3 luceDxHalfwayDir = normalize(luceDxDir + viewDir);
    //float luceDxSpec = pow(max(dot(normal, luceDxHalfwayDir), 0.0), shininess);
    //vec3 luceDxSpecular = vec3(0.2) * luceDxSpec;
    //vec3 luceDxConeDir = normalize(fs_in.TangentLuceDxConeDir);
    //float luceDxTheta = dot(luceDxDir, luceDxConeDir);
    //float luceDxEpsilon = 0.05;
    //float luceDxIntensity = intensitaLuciLaterali * smoothstep(cos(radians(luceDxAngle + luceDxEpsilon)), cos(radians(luceDxAngle)), luceDxTheta) * 1.0;
    //float shadowLuceDx = ShadowCalculation(fs_in.FragPosLuceDxSpace, shadowMapLuceDx);
    //vec3 luceDxResult = luceDxIntensity * (1.0 - shadowLuceDx) * (luceDxDiffuse + luceDxSpecular);
    
    // --- Luce SX ---
    //vec3 luceSxDir = normalize(fs_in.TangentLuceSxDir);
    //float luceSxDiff = max(dot(luceSxDir, normal), 0.0);
    //vec3 luceSxDiffuse = luceSxDiff * color;
    //vec3 luceSxHalfwayDir = normalize(luceSxDir + viewDir);
    //float luceSxSpec = pow(max(dot(normal, luceSxHalfwayDir), 0.0), shininess);
    //vec3 luceSxSpecular = vec3(0.2) * luceSxSpec;
    //vec3 luceSxConeDir = normalize(fs_in.TangentLuceSxConeDir);
    //float luceSxTheta = dot(luceSxDir, luceSxConeDir);
    //float luceSxEpsilon = 0.05;
    //float luceSxIntensity = intensitaLuciLaterali * smoothstep(cos(radians(luceSxAngle + luceSxEpsilon)), cos(radians(luceSxAngle)), luceSxTheta) * 1.0;
    //float shadowLuceSx = ShadowCalculation(fs_in.FragPosLuceSxSpace, shadowMapLuceSx);
    //vec3 luceSxResult = luceSxIntensity * (1.0 - shadowLuceSx) * (luceSxDiffuse + luceSxSpecular);


    // --- Luce Centrale (media tra DX e SX) ---
    vec3 luceCentroDir = normalize(normalize(fs_in.TangentLuceDxDir) + normalize(fs_in.TangentLuceSxDir));
    float luceCentroDiff = max(dot(luceCentroDir, normal), 0.0);
    vec3 luceCentroDiffuse = luceCentroDiff * color;
    vec3 luceCentroHalfwayDir = normalize(luceCentroDir + viewDir);
    float luceCentroSpec = pow(max(dot(normal, luceCentroHalfwayDir), 0.0), shininess);
    vec3 luceCentroSpecular = vec3(0.2) * luceCentroSpec;
    vec3 luceCentroConeDir = normalize(normalize(fs_in.TangentLuceDxConeDir) + normalize(fs_in.TangentLuceSxConeDir));
    float luceCentroTheta = dot(luceCentroDir, luceCentroConeDir);
    float luceCentroEpsilon = 0.05;
    float luceCentroAngle = 0.5 * (luceDxAngle + luceSxAngle) + 8.0;
    float luceCentroIntensity = intensitaLuciLaterali * smoothstep(cos(radians(luceCentroAngle + luceCentroEpsilon)), cos(radians(luceCentroAngle)), luceCentroTheta) * 1.0;
    // Calcolo posizione interpolata nello spazio luce centrale
    vec4 fragPosLuceCentroSpace = 0.5 * fs_in.FragPosLuceDxSpace + 0.5 * fs_in.FragPosLuceSxSpace;
    float shadowLuceCentro = ShadowCalculation(fragPosLuceCentroSpace, shadowMapLuceCentro);
    vec3 luceCentroResult = luceCentroIntensity * (1.0 - shadowLuceCentro) * (luceCentroDiffuse + luceCentroSpecular);

    vec3 result = ambient + spotlightResult + luceCentroResult;
    FragColor = vec4(result, 1.0);
}

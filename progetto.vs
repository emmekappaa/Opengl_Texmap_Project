#version 330 core
// Vertex shader per normal mapping con spotlight e spotlight extra
// Trasforma le posizioni e le direzioni delle luci/osservatore nello spazio tangente per il fragment shader

// Attributi di input dal VAO
layout (location = 0) in vec3 aPos;        // Posizione del vertice
layout (location = 1) in vec3 aNormal;     // Normale del vertice
layout (location = 2) in vec2 aTexCoords;  // Coordinate texture
layout (location = 3) in vec3 aTangent;    // Tangente del vertice
layout (location = 4) in vec3 aBitangent;  // Bitangente del vertice

// Output verso il fragment shader
out VS_OUT {
    vec2 TexCoords;                // Coordinate texture
    vec3 TangentLightDir;          // Direzione della luce principale (spotlight)
    vec3 TangentViewDir;           // Direzione della vista (camera)
    vec3 TangentSpotDir;           // Direzione della spotlight
    vec3 TangentLuceDxDir;         // Direzione della luce dx
    vec3 TangentLuceDxConeDir;     // Direzione del cono della luce dx
    vec4 FragPosLuceDxSpace;       // Posizione del frammento nello spazio della luce dx per shadow mapping
    vec3 TangentLuceSxDir;         // Direzione della luce sx
    vec3 TangentLuceSxConeDir;     // Direzione del cono della luce sx
    vec4 FragPosLuceSxSpace;       // Posizione del frammento nello spazio della luce sx per shadow mapping
} vs_out;

// Uniform per le trasformazioni e le posizioni delle luci
uniform mat4 model;         // Matrice modello
uniform mat4 view;          // Matrice vista
uniform mat4 projection;    // Matrice proiezione
uniform vec3 lightPos;      // Posizione spotlight
uniform vec3 viewPos;       // Posizione camera
uniform vec3 spotlightDir;  // Direzione spotlight (gi  normalizzata)
uniform vec3 luceDxPos;     // Posizione luce dx
uniform vec3 luceDxDir;     // Direzione luce dx
uniform mat4 luceDxSpaceMatrix; // Matrice per shadow mapping della luce dx
uniform vec3 luceSxPos;     // Posizione luce sx
uniform vec3 luceSxDir;     // Direzione luce sx
uniform mat4 luceSxSpaceMatrix; // Matrice per shadow mapping della luce sx

void main()
{
    // Calcolo della matrice TBN (Tangente, Bitangente, Normale) per passare da world space a tangent space
    vec3 T = normalize(mat3(model) * aTangent);   // Tangente trasformata
    vec3 B = normalize(mat3(model) * aBitangent); // Bitangente trasformata
    vec3 N = normalize(mat3(model) * aNormal);    // Normale trasformata
    mat3 TBN = mat3(T, B, N);
    
    // Per trasformare da world space a tangent space si usa la trasposta della TBN
    mat3 TBN_inv = transpose(TBN);

    // Calcolo della posizione del frammento in world space
    vec3 fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    
    // Trasforma le direzioni delle luci e della vista nello spazio tangente
    vs_out.TangentLightDir = TBN_inv * (lightPos - fragPos);           // Direzione spotlight
    vs_out.TangentViewDir  = TBN_inv * (viewPos - fragPos);            // Direzione osservatore
    vs_out.TangentSpotDir = TBN_inv * normalize(spotlightDir);         // Spotlight: gi  una direzione, non serve sottrarre fragPos
    vs_out.TangentLuceDxDir = TBN_inv * (luceDxPos - fragPos);         // Direzione luce dx
    vs_out.TangentLuceDxConeDir = TBN_inv * normalize(luceDxDir);      // Direzione del cono della luce dx
    vs_out.FragPosLuceDxSpace = luceDxSpaceMatrix * vec4(fragPos, 1.0); // Luce dx
    vs_out.TangentLuceSxDir = TBN_inv * (luceSxPos - fragPos);         // Direzione luce sx
    vs_out.TangentLuceSxConeDir = TBN_inv * normalize(luceSxDir);      // Direzione del cono della luce sx
    vs_out.FragPosLuceSxSpace = luceSxSpaceMatrix * vec4(fragPos, 1.0); // Luce sx

    // Calcola la posizione finale del vertice nello spazio clip
    gl_Position = projection * view * vec4(fragPos, 1.0);
}

/******************************************************************************
 * File:        test_tessiture.cpp
 * Author:      Michele Cipriani - Tommaso Vilotto
 * Description: This project is designed to test various materials
                and observe how light interacts with them, particularly through
                the effects of their normal maps.
 *****************************************************************************/

#include <glad/glad.h> 
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui/imconfig.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h> 
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 
#include <learnopengl/shader.h> 
#include <learnopengl/camera.h> 
#include <learnopengl/model.h> 
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
#include <iostream> 


 // Callback per ridimensionamento finestra: aggiorna viewport OpenGL
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// Callback per movimento mouse: aggiorna orientamento camera
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
// Callback per scroll del mouse: aggiorna zoom camera
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// Gestione input tastiera: aggiorna posizione camera
void processInput(GLFWwindow* window);
// Caricamento texture da file (non usata nel main, ma utile per estensioni)
unsigned int loadTexture(const char* path);
// Renderizza la scena per shadow mapping
void RenderScene(Shader &shader);

// Impostazioni finestra (risoluzione)
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1200;

// Shadow map resolution
const unsigned int SHADOW_WIDTH = 8096, SHADOW_HEIGHT = 8096;

// Camera globale (gestisce posizione e orientamento dell'osservatore)
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0; // Ultima posizione X del mouse
float lastY = (float)SCR_HEIGHT / 2.0; // Ultima posizione Y del mouse
bool firstMouse = true; // Serve per evitare salti all'inizio

// Variabili per il tempo (utile per movimenti smooth e indipendenti dal frame rate)
float deltaTime = 0.0f; // Tempo tra un frame e l'altro
float lastFrame = 0.0f; // Tempo dell'ultimo frame

// === Floor (Pavimento) ===
// Vertici del piano: posizione (3), normale (3), texcoord (2), tangente (3), bitangente (3)
// Le texcoord vanno da 0 a 10 per ripetere la texture 10 volte su X e Z
float planeVertices[] = {
    // positions          // normals         // texcoords   // tangent           // bitangente
    -1.0f, 0.0f, -1.0f,   0,1,0,            0.0f, 0.0f,   1,0,0,              0,0,1,
     1.0f, 0.0f, -1.0f,   0,1,0,           10.0f, 0.0f,   1,0,0,              0,0,1,
     1.0f, 0.0f,  1.0f,   0,1,0,           10.0f,10.0f,   1,0,0,              0,0,1,
    -1.0f, 0.0f,  1.0f,   0,1,0,            0.0f,10.0f,   1,0,0,              0,0,1
};

unsigned int planeIndices[] = {
    0, 1, 2,
    2, 3, 0
};
unsigned int planeVAO = 0, planeVBO = 0, planeEBO = 0;
unsigned int floorDiffuse, floorNormal, floorRoughness;

// === Wall (Muri) ===
// Vertici del muro: posizione (3), normale (3), texcoord (2), tangente (3), bitangente (3)
// Le texcoord sono proporzionali alle dimensioni del muro per evitare stretching
constexpr float wall_tex_x = 9.288005f; // larghezza reale muro
constexpr float wall_tex_y = 2.0f;      // aumenta ancora ripetizione verticale per mattoni più corti
float wallVertices[] = {
    // positions          // normals      // texcoords         // tangent      // bitangent
    -1.0f, 0.0f, 0.0f,    0,0,1,         0.0f, 0.0f,         1,0,0,         0,1,0,
     1.0f, 0.0f, 0.0f,    0,0,1,         wall_tex_x, 0.0f,   1,0,0,         0,1,0,
     1.0f, 1.0f, 0.0f,    0,0,1,         wall_tex_x, wall_tex_y, 1,0,0,      0,1,0,
    -1.0f, 1.0f, 0.0f,    0,0,1,         0.0f, wall_tex_y,   1,0,0,         0,1,0
};

unsigned int wallIndices[] = { 0, 1, 2, 2, 3, 0 };
unsigned int wallVAO = 0, wallVBO = 0, wallEBO = 0;
unsigned int wallDiffuse, wallNormal;

// posizione faro sinistro
glm::vec3 farettoSxPos = glm::vec3(
    -2.73f,         // manteniamo X simile
    1.181563f,      // altezza invariata
    3.0f         // spostato in avanti rispetto a prima (~3.20 → 1.35)
);
// Risultato: circa (-2.735169, 2.181563, 3.202862)


// posizione faro destro 
glm::vec3 farettoDxPos = glm::vec3(
    1.78f,          // simmetrico
    1.181563f,      // altezza invariata
    3.0f           // stesso Z del faretto sinistro
);
// Risultato: circa (1.782247, 2.181563, 3.363492)

// Soffitto
unsigned int ceilingDiffuse, ceilingNormal, ceilingRoughness;

// Limiti della stanza (calcolati in base alla posizione e dimensione dei muri)
const float room_min_x = -0.0029815f - 9.288005f/2.0f - 4.14f + 0.5f; // +0.5f margine per non attraversare il muro
const float room_max_x = -0.0029815f + 9.288005f/2.0f + 4.14f - 0.5f;
const float room_min_z =  1.5337835f - 5.676001f/2.0f - 2.34f + 0.5f;
const float room_max_z =  1.5337835f + 5.676001f/2.0f + 2.34f - 0.5f;
const float room_min_y = 0.0f;      // pavimento
const float room_max_y = 3.0f - 0.1f; // soffitto (con piccolo margine)

// === Modelli globali come puntatori ===
Model* personaggio = nullptr;
Model* farettodx = nullptr;
Model* farettosx = nullptr;
Model* telo = nullptr;
Model* ventola = nullptr;
Model* divanetto = nullptr;
Model* divanetto2 = nullptr;
Model* tavolino = nullptr;
Model* fotocamera = nullptr;
Model* wall_e = nullptr;
Model* arcade = nullptr;

struct MaterialSet {
    std::string diffuse;
    std::string gloss;
    std::string normal;
};

// Array di materiali disponibili
MaterialSet materiali[] = {
    { // boucleFabric
        "./Progetto/x64/Debug/tex/boucleFabric/rp_manuel_animated_001_dif.jpg",
        "./Progetto/x64/Debug/tex/boucleFabric/rp_manuel_animated_001_gloss.jpg",
        "./Progetto/x64/Debug/tex/boucleFabric/rp_manuel_animated_001_norm.jpg"
    },
    { // leather
        "./Progetto/x64/Debug/tex/leather/rp_manuel_animated_001_dif.jpg",
        "./Progetto/x64/Debug/tex/leather/rp_manuel_animated_001_gloss.jpg",
        "./Progetto/x64/Debug/tex/leather/rp_manuel_animated_001_norm.jpg"
    },
    { // redCotton
        "./Progetto/x64/Debug/tex/redCotton/rp_manuel_animated_001_dif.jpg",
        "./Progetto/x64/Debug/tex/redCotton/rp_manuel_animated_001_gloss.jpg",
        "./Progetto/x64/Debug/tex/redCotton/rp_manuel_animated_001_norm.jpg"
    },
    { // simiLino
        "./Progetto/x64/Debug/tex/simiLino/rp_manuel_animated_001_dif.jpg",
        "./Progetto/x64/Debug/tex/simiLino/rp_manuel_animated_001_gloss.jpg",
        "./Progetto/x64/Debug/tex/simiLino/rp_manuel_animated_001_norm.jpg"
    },
    { // towelCotton
        "./Progetto/x64/Debug/tex/towelCotton/rp_manuel_animated_001_dif.jpg",
        "./Progetto/x64/Debug/tex/towelCotton/rp_manuel_animated_001_gloss_nera.jpg",
        "./Progetto/x64/Debug/tex/towelCotton/rp_manuel_animated_001_norm.jpg"
    }
};
const int numMateriali = sizeof(materiali) / sizeof(MaterialSet);
int materialeCorrente = 0;

string materiali_sel[] = {
    "Boucle Fabric",
    "Leather",
    "Red Cotton",
    "Simil Lino",
    "Towel Cotton"
};

unsigned int personaggioDiffuse[numMateriali];
unsigned int personaggioGloss[numMateriali];
unsigned int personaggioNormal[numMateriali];

// Variabile per dimmare la luminosità delle due luci laterali
float intensitaLuciLaterali = 0.3f;
int livelloIntensitaLuci = 1; // 0=spento, 1=bassa, 2=media, 3=alta

const char* intensitaLabels[] = { "Off", "Low", "Medium", "High" };


int main()
{
    // Inizializza GLFW e imposta versione OpenGL
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crea la finestra principale
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Graphics Programming Univr - Fabric Simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Imposta le callback per input e resize
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Disabilita il cursore per un'esperienza FPS (mouse catturato)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inizializza GLAD per caricare le funzioni OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // === IMGUI: Inizializzazione ===
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImFont* font = io.Fonts->AddFontFromFileTTF("Progetto/x64/Debug/Font/Timeline.ttf", 18.0f * (SCR_HEIGHT / 1080.0f));
    io.FontDefault = font;



    // Carica i modelli 3D DOPO aver creato il contesto OpenGL
    personaggio = new Model("Progetto/x64/Debug/manuel.obj");
    farettodx = new Model("Progetto/x64/Debug/faretto_dx.obj");
    farettosx = new Model("Progetto/x64/Debug/faretto_sx.obj");
    telo = new Model("Progetto/x64/Debug/studio.obj");
    ventola = new Model("Progetto/x64/Debug/ceiling_fan_(OBJ).obj");
    divanetto = new Model("Progetto/x64/Debug/leather_chair(OBJ).obj");
    divanetto2 = new Model("Progetto/x64/Debug/leather_chair(OBJ).obj");
    tavolino = new Model("Progetto/x64/Debug/Table.obj");
    fotocamera = new Model("Progetto/x64/Debug/camera.obj");
    wall_e = new Model("Progetto/x64/Debug/wall-e.obj");
    arcade = new Model("Progetto/x64/Debug/arcade.obj");

    // === Caricamento texture per tutti i materiali del personaggio ===

    for (int i = 0; i < numMateriali; ++i) {
        personaggioDiffuse[i] = loadTexture(materiali[i].diffuse.c_str());
        personaggioGloss[i] = loadTexture(materiali[i].gloss.c_str());
        personaggioNormal[i] = loadTexture(materiali[i].normal.c_str());
    }


    // Abilita il depth test per la corretta visualizzazione 3D (gestione profondità)
    glEnable(GL_DEPTH_TEST);

    // Carica e compila gli shader (vertex e fragment)
    Shader shader("progetto.vs", "progetto.fs");
    Shader shadowMappingShader("shadow_mapping.vs", "shadow_mapping.fs");

    // Configurazione shadow mapping per luceDx e luceSx
    unsigned int depthMapFBOLuceDx, depthMapFBOLuceSx;
    glGenFramebuffers(1, &depthMapFBOLuceDx);
    glGenFramebuffers(1, &depthMapFBOLuceSx);
    unsigned int depthMapLuceDx, depthMapLuceSx;
    glGenTextures(1, &depthMapLuceDx);
    glGenTextures(1, &depthMapLuceSx);
    // LuceDx
    glBindTexture(GL_TEXTURE_2D, depthMapLuceDx);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOLuceDx);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapLuceDx, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // LuceSx
    glBindTexture(GL_TEXTURE_2D, depthMapLuceSx);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOLuceSx);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapLuceSx, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // === Inizializzazione VAO/VBO/EBO per il piano ===
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);
    // posizione
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normale
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texcoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // tangente
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    // bitangente
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glBindVertexArray(0);

    // === Caricamento texture per il Soffitto ===
    ceilingDiffuse = loadTexture("./Progetto/x64/Debug/tex/soffitto/Ceiling_Drop_Tiles_001_basecolor.jpg");
    ceilingNormal = loadTexture("./Progetto/x64/Debug/tex/soffitto/Ceiling_Drop_Tiles_001_normal.jpg");
    ceilingRoughness = loadTexture("./Progetto/x64/Debug/tex/soffitto/Ceiling_Drop_Tiles_001_roughness.jpg");

    // === Caricamento texture Poliigon per il pavimento ===
    floorDiffuse = loadTexture("./Progetto/x64/Debug/tex/poligon_woodfloor/Poliigon_WoodFloorAsh_4186_BaseColor.jpg");
    floorNormal = loadTexture("./Progetto/x64/Debug/tex/poligon_woodfloor/Poliigon_WoodFloorAsh_4186_Normal.png");
    floorRoughness = loadTexture("./Progetto/x64/Debug/tex/poligon_woodfloor/Poliigon_WoodFloorAsh_4186_Roughness.jpg");

    // === Inizializzazione VAO/VBO/EBO per il muro ===
    glGenVertexArrays(1, &wallVAO);
    glGenBuffers(1, &wallVBO);
    glGenBuffers(1, &wallEBO);

    glBindVertexArray(wallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wallIndices), wallIndices, GL_STATIC_DRAW);
    // posizione
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normale
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texcoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // tangente
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    // bitangente
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glBindVertexArray(0);

    // === Caricamento texture per i muri ===
    wallDiffuse = loadTexture("./Progetto/x64/Debug/tex/muri/color.png");
    wallNormal = loadTexture("./Progetto/x64/Debug/tex/muri/normal_muro.png");



    // Ciclo di rendering principale
    while (!glfwWindowShouldClose(window))
    {
        // Calcola il tempo trascorso tra un frame e l'altro (per movimenti smooth)
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Gestione input tastiera/mouse
        processInput(window);



        // Rendering shadow map per luceDx
        glm::vec3 luceDxPos(1.25f, 1.9f, 1.6f);
        glm::vec3 luceDxTarget(0.5f, 1.4f, 0.5f);
        glm::vec3 luceDxDir = glm::normalize(luceDxTarget - luceDxPos);
        float near_plane = 0.1f, far_plane = 20.0f;
        float ortho_size = 10.0f;
        glm::mat4 luceDxProjection = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, near_plane, far_plane);
        glm::mat4 luceDxView = glm::lookAt(luceDxPos, luceDxTarget, glm::vec3(0, 1, 0));
        glm::mat4 luceDxSpaceMatrix = luceDxProjection * luceDxView;
        shadowMappingShader.use();
        shadowMappingShader.setMat4("lightSpaceMatrix", luceDxSpaceMatrix);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOLuceDx);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderScene(shadowMappingShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Rendering shadow map per luceSx
        glm::vec3 luceSxPos(-1.25f, 1.9f, 1.6f);
        glm::vec3 luceSxTarget(-0.4f, 1.4f, 0.4f);
        glm::vec3 luceSxDir = glm::normalize(luceSxTarget - luceSxPos);
        glm::mat4 luceSxProjection = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, near_plane, far_plane);
        glm::mat4 luceSxView = glm::lookAt(luceSxPos, luceSxTarget, glm::vec3(0, 1, 0));
        glm::mat4 luceSxSpaceMatrix = luceSxProjection * luceSxView;
        shadowMappingShader.use();
        shadowMappingShader.setMat4("lightSpaceMatrix", luceSxSpaceMatrix);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOLuceSx);
        glClear(GL_DEPTH_BUFFER_BIT);
        RenderScene(shadowMappingShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Rendering normale della scena con shadow mapping
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        // Calcola le matrici di proiezione e vista (telecamera)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        shader.setMat4("projection", projection); // Passa la matrice di proiezione allo shader
        shader.setMat4("view", view); // Passa la matrice di vista allo shader

        // Matrice modello identità 
        glm::mat4 model = glm::mat4(1.0f);
        // Riduci la scala del modello
        model = glm::scale(model, glm::vec3(1.0f));
        shader.setMat4("model", model); // Passa la matrice modello allo shader

        // Passa la posizione della camera sia come viewPos che come lightPos (spotlight)
        shader.setVec3("viewPos", camera.Position); // Posizione osservatore
        shader.setVec3("lightPos", camera.Position); // La luce segue la camera
        // Passa anche la direzione della camera come spotlightDir
        shader.setVec3("spotlightDir", camera.Front); // Direzione della spotlight


        shader.setVec3("luceDxPos", luceDxPos);
        shader.setVec3("luceDxDir", luceDxDir);
        shader.setFloat("luceDxAngle", 191.0f);
        shader.setMat4("luceDxSpaceMatrix", luceDxSpaceMatrix);
        shader.setVec3("luceSxPos", luceSxPos);
        shader.setVec3("luceSxDir", luceSxDir);
        shader.setFloat("luceSxAngle", 191.0f);
        shader.setMat4("luceSxSpaceMatrix", luceSxSpaceMatrix);
        shader.setFloat("intensitaLuciLaterali", intensitaLuciLaterali);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, depthMapLuceDx);
        shader.setInt("shadowMapLuceDx", 5);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, depthMapLuceSx);
        shader.setInt("shadowMapLuceSx", 6);

        // Renderizza la scena
        RenderScene(shader);


        // Inizio frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Finestra custom
        ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH * 0.19f, SCR_HEIGHT * 0.08f), ImGuiCond_Always);
        ImGui::Begin("Material Info");
        ImGui::Text("Materiale corrente: %s", materiali_sel[materialeCorrente].c_str());
        ImGui::Text("Intensita luci laterali: %s", intensitaLabels[livelloIntensitaLuci]);
        ImGui::End();

        // Rendering ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        // Scambia i buffer e gestisce gli eventi di input
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Libera le risorse e termina l'applicazione
    delete personaggio;
    delete farettodx;
    delete farettosx;
    delete telo;
    delete ventola;
    delete divanetto;
    delete divanetto2;
    delete tavolino;
    delete fotocamera;
	delete wall_e;
    delete arcade;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

// Gestione input tastiera: aggiorna la posizione della camera in base ai tasti premuti
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    static bool mPressed = false;
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mPressed) {
        materialeCorrente = (materialeCorrente + 1) % numMateriali;
        mPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        mPressed = false;
    }

    // --- Gestione intensità luci laterali con L ---
    static bool lPressed = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lPressed) {
        livelloIntensitaLuci = (livelloIntensitaLuci + 1) % 4;
        switch (livelloIntensitaLuci) {
        case 0: intensitaLuciLaterali = 0.0f; break; // spento
        case 1: intensitaLuciLaterali = 0.1f; break; // bassa
        case 2: intensitaLuciLaterali = 0.2f; break; // media
        case 3: intensitaLuciLaterali = 0.3f; break; // alta
        }
        lPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lPressed = false;
    }


    // Vincola la posizione della camera all'interno delle mura
    camera.Position.x = glm::clamp(camera.Position.x, room_min_x, room_max_x);
    camera.Position.y = glm::clamp(camera.Position.y, room_min_y+0.2f, room_max_y-0.2f);
    camera.Position.z = glm::clamp(camera.Position.z, room_min_z, room_max_z);
}

// Callback per il ridimensionamento della finestra: aggiorna la viewport OpenGL
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Callback per il movimento del mouse: aggiorna l'orientamento della camera
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertito perché le y crescono dal basso verso l'alto

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Callback per lo scroll del mouse: aggiorna lo zoom della camera
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// Carica una texture da file e restituisce l'ID OpenGL della texture
// (Non usata direttamente nel main, ma utile per estensioni future)
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Imposta i parametri di wrapping e filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Renderizza la scena per shadow mapping
void RenderScene(Shader &shader)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, personaggioDiffuse[materialeCorrente]);
    shader.setInt("texture_diffuse1", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, personaggioNormal[materialeCorrente]);
    shader.setInt("texture_normal1", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, personaggioGloss[materialeCorrente]);
    shader.setInt("texture_specular1", 2);

    // Modello del personaggio
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if (personaggio) personaggio->Draw(shader);

    // Modello del faretto dx
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if(farettodx) farettodx->Draw(shader);

    // Modello del faretto sx
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if(farettosx) farettosx->Draw(shader);

    // Modello del telo/rampa
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.01f, 0.0f));
    model = glm::scale(model, glm::vec3(0.6f));
    shader.setMat4("model", model);
    if(telo) telo->Draw(shader);

    // Modello della ventola
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 2.7f, 0.0f));
    model = glm::scale(model, glm::vec3(0.6f));
    shader.setMat4("model", model);
    if(ventola) ventola->Draw(shader);

    // Modello del divanetto
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.01f, 5.5f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if(divanetto) divanetto->Draw(shader);

    // Modello del divanetto 2
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 0.01f, 5.5f));
    model = glm::rotate(model, glm::radians(160.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if(divanetto2) divanetto2->Draw(shader);

    // Modello del tavolino
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.01f, 5.2f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(0.03f));
    shader.setMat4("model", model);
    if(tavolino) tavolino->Draw(shader);

    // Modello della fotocamera
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.78f, 5.2f));
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if(fotocamera) fotocamera->Draw(shader);

    // Modello della wall_e
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(8.5f, 0.01f, 6.2f));
    model = glm::rotate(model, glm::radians(50.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(0.008f));
    shader.setMat4("model", model);
    if (wall_e) wall_e->Draw(shader);

    // Modello della macchina arcade
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-8.2f, 0.01f, 6.2f));
    model = glm::rotate(model, glm::radians(120.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(1.0f));
    shader.setMat4("model", model);
    if (arcade) arcade->Draw(shader);


    // === Pavimento ===
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorDiffuse);
    shader.setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, floorNormal);
    shader.setInt("texture_normal1", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, floorRoughness);
    shader.setInt("texture_specular1", 2);

    float floor_size_x = 9.288005f;
    float floor_size_z = 5.676001f;
    glm::vec3 floor_center_position = glm::vec3(-0.0029815f, 0.0f, 1.5337835f);
    model = glm::mat4(1.0f);
    model = glm::translate(model, floor_center_position);
    model = glm::scale(model, glm::vec3(floor_size_x, 1.0f, floor_size_z));
    shader.setMat4("model", model);
    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // === Muri ===
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallDiffuse);
    shader.setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, wallNormal);
    shader.setInt("texture_normal1", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, floorRoughness); // Usa la stessa roughness del pavimento
    shader.setInt("texture_specular1", 2);

    float wall_height = 3.0f;
    float wall_thickness = 1.0f;
    // Back wall
    model = glm::mat4(1.0f);
    model = glm::translate(model, floor_center_position + glm::vec3(0.0f, 0.0f, -floor_size_z/2.0f - wall_thickness/2.0f - 2.34f));
    model = glm::scale(model, glm::vec3(floor_size_x, wall_height, wall_thickness));
    shader.setMat4("model", model);
    glBindVertexArray(wallVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // Front wall
    model = glm::mat4(1.0f);
    model = glm::translate(model, floor_center_position + glm::vec3(0.0f, 0.0f, floor_size_z/2.0f + wall_thickness/2.0f + 2.34f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(floor_size_x, wall_height, wall_thickness));
    shader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // Left wall
    model = glm::mat4(1.0f);
    model = glm::translate(model, floor_center_position + glm::vec3(-floor_size_x/2.0f - wall_thickness/2.0f - 4.14f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
    model = glm::scale(model, glm::vec3(floor_size_z, wall_height, wall_thickness));
    shader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // Right wall
    model = glm::mat4(1.0f);
    model = glm::translate(model, floor_center_position + glm::vec3(floor_size_x/2.0f + wall_thickness/2.0f + 4.14f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0));
    model = glm::scale(model, glm::vec3(floor_size_z, wall_height, wall_thickness));
    shader.setMat4("model", model);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // === Soffitto ===
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ceilingDiffuse);
    shader.setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ceilingNormal);
    shader.setInt("texture_normal1", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ceilingRoughness);
    shader.setInt("texture_specular1", 2);

    model = glm::mat4(1.0f);
    model = glm::translate(model, floor_center_position + glm::vec3(0.0f, 3.0f, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
    model = glm::scale(model, glm::vec3(floor_size_x, 1.0f, floor_size_z));
    shader.setMat4("model", model);
    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

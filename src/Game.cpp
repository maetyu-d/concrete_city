#include "Game.hpp"

#include "Hash.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <raymath.h>
#include <rlgl.h>

namespace {
constexpr float Pi = 3.14159265f;
constexpr float TurnSpeed = 3.25f;
int WindowWidth = 1600;
int WindowHeight = 900;
constexpr int ShadowMapResolution = 2048;
constexpr float MouseSensitivity = 0.0024f;
constexpr float PlayerCollisionRadius = 0.22f;
constexpr float RainSoundtrackVolume = 0.55f;
constexpr float RainSoundtrackCrossfadeSeconds = 30.0f;
constexpr int RainSoundtrackBufferFrames = 49152;
constexpr const char* RainSoundtrackPath = "assets/audio/rain.wav";
constexpr int CurrentRenderWidth = 1600;
constexpr int CurrentRenderHeight = 900;
constexpr int FullHdRenderWidth = 1920;
constexpr int FullHdRenderHeight = 1080;

int positiveMod(int value, int divisor) {
    const int mod = value % divisor;
    return mod < 0 ? mod + divisor : mod;
}

TileCoord worldPositionToTile(Vector2 position) {
    return {
        static_cast<int>(std::floor(position.x / static_cast<float>(TileSize))),
        static_cast<int>(std::floor(position.y / static_cast<float>(TileSize)))
    };
}

Vector2 add(Vector2 a, Vector2 b) {
    return {a.x + b.x, a.y + b.y};
}

Vector2 subtract(Vector2 a, Vector2 b) {
    return {a.x - b.x, a.y - b.y};
}

Vector2 multiply(Vector2 value, float scalar) {
    return {value.x * scalar, value.y * scalar};
}

Vector2 directionFromAngle(float angle) {
    return {std::cos(angle), std::sin(angle)};
}

Vector2 rightFromAngle(float angle) {
    return {std::cos(angle + Pi * 0.5f), std::sin(angle + Pi * 0.5f)};
}

float dot(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

Color mixColor(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        static_cast<unsigned char>(a.r * (1.0f - t) + b.r * t),
        static_cast<unsigned char>(a.g * (1.0f - t) + b.g * t),
        static_cast<unsigned char>(a.b * (1.0f - t) + b.b * t),
        static_cast<unsigned char>(a.a * (1.0f - t) + b.a * t)
    };
}

float objectHeight(ObjectType type) {
    switch (type) {
        case ObjectType::Tree: return 98.0f;
        case ObjectType::Stone: return 42.0f;
        case ObjectType::Ruin: return 78.0f;
        case ObjectType::Marker: return 58.0f;
        case ObjectType::Anomaly: return 120.0f;
    }
    return 48.0f;
}

float objectWidth(ObjectType type) {
    switch (type) {
        case ObjectType::Tree: return 54.0f;
        case ObjectType::Stone: return 38.0f;
        case ObjectType::Ruin: return 86.0f;
        case ObjectType::Marker: return 32.0f;
        case ObjectType::Anomaly: return 62.0f;
    }
    return 42.0f;
}

Color adjust(Color color, int amount);

float billboardProfile(ObjectType type, float t) {
    const float centered = std::abs(t * 2.0f - 1.0f);
    switch (type) {
        case ObjectType::Tree: return std::max(0.28f, 1.0f - centered * centered * 0.45f);
        case ObjectType::Stone: return std::max(0.35f, 1.0f - centered * 0.55f);
        case ObjectType::Ruin: return 0.88f + (centered < 0.25f ? 0.12f : 0.0f);
        case ObjectType::Marker: return std::max(0.22f, 1.0f - centered * 0.9f);
        case ObjectType::Anomaly: return 0.75f + std::sin(t * Pi * 3.0f) * 0.16f;
    }
    return 1.0f;
}

float tileHeight3D(TileType type) {
    switch (type) {
        case TileType::DeepWater: return 0.10f;
        case TileType::ShallowWater: return 0.12f;
        case TileType::Grass: return 0.18f;
        case TileType::Forest: return 0.26f;
        case TileType::Stone: return 0.72f;
        case TileType::Strange: return 0.58f;
    }
    return 0.18f;
}

float tileBase3D(TileType type) {
    switch (type) {
        case TileType::DeepWater: return -0.18f;
        case TileType::ShallowWater: return -0.14f;
        case TileType::Grass: return -0.09f;
        case TileType::Forest: return -0.08f;
        case TileType::Stone: return -0.05f;
        case TileType::Strange: return -0.04f;
    }
    return -0.09f;
}

Color distanceShade(Color color, float distanceTiles) {
    return mixColor(color, {12, 17, 22, 255}, std::clamp(distanceTiles / 50.0f, 0.0f, 1.0f));
}

bool isCityStreet(TileCoord tile) {
    const int avenueX = positiveMod(tile.x, 16);
    const int avenueY = positiveMod(tile.y, 20);
    const int serviceX = positiveMod(tile.x + 37, 41);
    const int serviceY = positiveMod(tile.y - 11, 47);
    return avenueX <= 3 || avenueY <= 3 || serviceX <= 1 || serviceY <= 1;
}

bool isCityCourtyard(TileCoord tile) {
    const int blockX = static_cast<int>(std::floor(static_cast<float>(tile.x) / 9.0f));
    const int blockY = static_cast<int>(std::floor(static_cast<float>(tile.y) / 9.0f));
    const int localX = positiveMod(tile.x, 9);
    const int localY = positiveMod(tile.y, 9);
    const bool core = localX >= 2 && localX <= 6 && localY >= 2 && localY <= 6;
    return core && hashToUnit(stableHash(0xB807A1157ULL, blockX, blockY, 0xC0U)) > 0.56;
}

bool isCityBuilding(TileCoord tile) {
    return !isCityStreet(tile) && !isCityCourtyard(tile);
}

bool isLampPostTile(TileCoord tile) {
    return isCityStreet(tile) && positiveMod(tile.x * 5 - tile.y * 3, 74) == 0;
}

bool circleIntersectsAabb(Vector2 center, float radius, float minX, float minY, float maxX, float maxY) {
    const float closestX = std::clamp(center.x, minX, maxX);
    const float closestY = std::clamp(center.y, minY, maxY);
    const float dx = center.x - closestX;
    const float dy = center.y - closestY;
    return dx * dx + dy * dy < radius * radius;
}

float cityBuildingHeight(TileCoord tile) {
    const int blockX = static_cast<int>(std::floor(static_cast<float>(tile.x) / 4.0f));
    const int blockY = static_cast<int>(std::floor(static_cast<float>(tile.y) / 4.0f));
    const double mass = hashToUnit(stableHash(0x5A71E71C17ULL, blockX, blockY, 0xB10C));
    const double spike = hashToUnit(stableHash(0x5A71E71C17ULL, blockX, blockY, 0x717));
    const double distance = std::sqrt(static_cast<double>(tile.x * tile.x + tile.y * tile.y));
    const double distancePush = std::min(distance / 260.0, 1.0) * 5.0;
    float height = 5.0f + static_cast<float>(mass * 12.0 + distancePush);
    if (spike > 0.86) {
        height += static_cast<float>((spike - 0.86) * 44.0);
    }
    if (positiveMod(tile.x + tile.y, 5) == 0) {
        height -= 1.1f;
    }
    return std::max(height, 3.6f);
}

Color cityConcreteColor(TileCoord tile, float height) {
    const int variation = static_cast<int>(stableHash(0xC011CEULL, tile.x, tile.y, 0xFACADE) % 41) - 20;
    Color base = height > 15.0f ? Color{142, 144, 136, 255} : Color{68, 73, 72, 255};
    if (hashToUnit(stableHash(0xC011CEULL, tile.x / 4, tile.y / 4, 0x51A8)) > 0.76) {
        base = mixColor(base, {24, 32, 36, 255}, 0.52f);
    }
    return adjust(base, variation);
}

Color adjust(Color color, int amount) {
    const auto clamp = [](int value) {
        return static_cast<unsigned char>(std::clamp(value, 0, 255));
    };
    return {
        clamp(static_cast<int>(color.r) + amount),
        clamp(static_cast<int>(color.g) + amount),
        clamp(static_cast<int>(color.b) + amount),
        color.a
    };
}

float distanceToSegment(float px, float py, float ax, float ay, float bx, float by) {
    const float vx = bx - ax;
    const float vy = by - ay;
    const float wx = px - ax;
    const float wy = py - ay;
    const float lengthSquared = vx * vx + vy * vy;
    const float t = lengthSquared > 0.0f ? std::clamp((wx * vx + wy * vy) / lengthSquared, 0.0f, 1.0f) : 0.0f;
    const float cx = ax + vx * t;
    const float cy = ay + vy * t;
    const float dx = px - cx;
    const float dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

Texture2D makeMaterialTexture(Color base, Color dark, Color light, std::uint64_t salt, bool cracks) {
    constexpr int size = 96;
    std::vector<Color> pixels(size * size);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double grain = hashToUnit(stableHash(salt, x, y, 0x111));
            const double pebble = hashToUnit(stableHash(salt, x / 3, y / 3, 0x222));
            const double stain = hashToUnit(stableHash(salt, x / 13, y / 13, 0x333));
            Color color = adjust(base, static_cast<int>(grain * 54.0) - 27);

            if (pebble > 0.78) {
                color = mixColor(color, light, 0.28f);
            }
            if (stain > 0.66) {
                color = mixColor(color, dark, 0.34f);
            }

            const double aggregate = hashToUnit(stableHash(salt, x / 7, y / 5, 0x444));
            if (aggregate > 0.74) {
                color = mixColor(color, aggregate > 0.88 ? light : dark, 0.20f);
            }

            if (cracks) {
                for (int i = 0; i < 7; ++i) {
                    const float ax = static_cast<float>(hashToUnit(stableHash(salt, i, 0, 0xCAFE)) * size);
                    const float ay = static_cast<float>(hashToUnit(stableHash(salt, i, 1, 0xCAFE)) * size);
                    const float bx = static_cast<float>(hashToUnit(stableHash(salt, i, 2, 0xCAFE)) * size);
                    const float by = static_cast<float>(hashToUnit(stableHash(salt, i, 3, 0xCAFE)) * size);
                    const float width = 0.45f + static_cast<float>(hashToUnit(stableHash(salt, i, 4, 0xCAFE))) * 1.15f;
                    const float d = distanceToSegment(static_cast<float>(x), static_cast<float>(y), ax, ay, bx, by);
                    if (d < width) {
                        color = mixColor(color, dark, 0.78f * (1.0f - d / width));
                    }
                }
            }

            pixels[static_cast<std::size_t>(y * size + x)] = color;
        }
    }

    Image image{};
    image.data = pixels.data();
    image.width = size;
    image.height = size;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    Texture2D texture = LoadTextureFromImage(image);
    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    return texture;
}

RenderTexture2D loadShadowmapRenderTexture(int width, int height) {
    RenderTexture2D target{};
    target.id = rlLoadFramebuffer();
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = PIXELFORMAT_UNCOMPRESSED_R32;
        target.depth.mipmaps = 1;

        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        if (rlFramebufferComplete(target.id)) {
            TraceLog(LOG_INFO, "FBO: [ID %i] Shadow map framebuffer created successfully", target.id);
        }

        rlDisableFramebuffer();
    } else {
        TraceLog(LOG_WARNING, "FBO: Shadow map framebuffer could not be created");
    }

    return target;
}

void unloadShadowmapRenderTexture(RenderTexture2D target) {
    if (target.id > 0) {
        rlUnloadFramebuffer(target.id);
    }
}

bool renderTextureLoaded(RenderTexture2D target) {
    return target.id > 0;
}
}

Game::Game()
    : m_generator(0xC0BA171977ULL)
    , m_chunks(m_generator)
    , m_deltas("markers.txt")
    , m_player({0.0f, 0.0f}) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WindowWidth, WindowHeight, "Zone Drifter");
    const int monitor = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    if (!IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    SetTargetFPS(60);
    DisableCursor();
    HideCursor();
    loadRainSoundtrack();
    resizeRenderTargets(WindowWidth, WindowHeight);
    m_camera.target = m_player.position();
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

    loadVisualShaders();

    m_cubeModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    m_sphereModel = LoadModelFromMesh(GenMeshSphere(0.5f, 24, 16));
    m_cylinderModel = LoadModelFromMesh(GenMeshCylinder(0.5f, 1.0f, 16));
    m_cubeModel.materials[0].shader = m_lightingShader;
    m_sphereModel.materials[0].shader = m_lightingShader;
    m_cylinderModel.materials[0].shader = m_lightingShader;

    m_concreteTexture = makeMaterialTexture({94, 98, 96, 255}, {7, 10, 12, 255}, {222, 215, 184, 255}, 0xC0C0C0, true);
    m_groundTexture = makeMaterialTexture({24, 28, 30, 255}, {2, 3, 5, 255}, {110, 108, 92, 255}, 0x6A551, false);
    m_waterTexture = makeMaterialTexture({8, 16, 24, 255}, {0, 1, 4, 255}, {100, 172, 214, 255}, 0x0CE4A, false);
    m_mossTexture = makeMaterialTexture({22, 39, 36, 255}, {2, 7, 8, 255}, {96, 122, 101, 255}, 0xF0257, false);
    m_barkTexture = makeMaterialTexture({64, 43, 27, 255}, {12, 8, 6, 255}, {134, 105, 70, 255}, 0xBA44, true);
    m_anomalyTexture = makeMaterialTexture({175, 54, 166, 255}, {24, 3, 30, 255}, {255, 143, 238, 255}, 0xA110, true);
    m_shadowMap = loadShadowmapRenderTexture(ShadowMapResolution, ShadowMapResolution);

    m_deltas.load();
}

Game::~Game() {
    unloadRainSoundtrack();
    UnloadTexture(m_concreteTexture);
    UnloadTexture(m_groundTexture);
    UnloadTexture(m_waterTexture);
    UnloadTexture(m_mossTexture);
    UnloadTexture(m_barkTexture);
    UnloadTexture(m_anomalyTexture);
    unloadShadowmapRenderTexture(m_shadowMap);
    unloadVisualShaders();
    unloadRenderTargets();
    UnloadModel(m_cubeModel);
    UnloadModel(m_sphereModel);
    UnloadModel(m_cylinderModel);
    CloseWindow();
}

void Game::unloadVisualShaders() {
    if (m_paintingShader.id > 0) {
        UnloadShader(m_paintingShader);
        m_paintingShader = {};
    }
    if (m_lightingShader.id > 0) {
        UnloadShader(m_lightingShader);
        m_lightingShader = {};
    }
}

void Game::loadVisualShaders() {
    unloadVisualShaders();

    const char* lightingPath = m_useMeshVisuals ? "assets/shaders/mesh_lit.fs" : "assets/shaders/zone_lit.fs";
    const char* postPath = m_useMeshVisuals ? "assets/shaders/mesh_presentation.fs" : "assets/shaders/bleached_painting.fs";

    m_lightingShader = LoadShader("assets/shaders/zone_lit.vs", lightingPath);
    m_lightingShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(m_lightingShader, "matModel");
    m_lightingShader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(m_lightingShader, "matNormal");
    m_lightingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(m_lightingShader, "viewPos");
    m_viewPosLoc = m_lightingShader.locs[SHADER_LOC_VECTOR_VIEW];
    m_lightDirLoc = GetShaderLocation(m_lightingShader, "lightDir");
    m_materialDepthLoc = GetShaderLocation(m_lightingShader, "materialDepth");
    m_textureScaleLoc = GetShaderLocation(m_lightingShader, "textureScale");
    m_lightingTimeLoc = GetShaderLocation(m_lightingShader, "time");
    m_lightVPLoc = GetShaderLocation(m_lightingShader, "lightVP");
    m_shadowMapLoc = GetShaderLocation(m_lightingShader, "shadowMap");
    m_shadowMapResolutionLoc = GetShaderLocation(m_lightingShader, "shadowMapResolution");
    m_useShadowMapLoc = GetShaderLocation(m_lightingShader, "useShadowMap");

    const float lightDir[3] = {-0.18f, -0.94f, -0.28f};
    const float lightColor[4] = {1.42f, 1.13f, 0.74f, 1.0f};
    const float ambientColor[4] = {0.019f, 0.027f, 0.040f, 1.0f};
    const float fogColor[4] = {0.005f, 0.008f, 0.013f, 1.0f};
    const float fogStart = 12.0f;
    const float fogEnd = 58.0f;
    const float materialDepth = 1.15f;
    const float textureScale = 4.0f;
    const int shadowMapResolution = ShadowMapResolution;
    const int useShadowMap = 1;
    SetShaderValue(m_lightingShader, m_lightDirLoc, lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_lightingShader, GetShaderLocation(m_lightingShader, "lightColor"), lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(m_lightingShader, GetShaderLocation(m_lightingShader, "ambientColor"), ambientColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(m_lightingShader, GetShaderLocation(m_lightingShader, "fogColor"), fogColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(m_lightingShader, GetShaderLocation(m_lightingShader, "fogStart"), &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_lightingShader, GetShaderLocation(m_lightingShader, "fogEnd"), &fogEnd, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_lightingShader, m_materialDepthLoc, &materialDepth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_lightingShader, m_textureScaleLoc, &textureScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_lightingShader, m_shadowMapResolutionLoc, &shadowMapResolution, SHADER_UNIFORM_INT);
    SetShaderValue(m_lightingShader, m_useShadowMapLoc, &useShadowMap, SHADER_UNIFORM_INT);

    m_paintingShader = LoadShader(nullptr, postPath);
    m_paintingFeedbackLoc = GetShaderLocation(m_paintingShader, "feedbackTexture");
    m_paintingResolutionLoc = GetShaderLocation(m_paintingShader, "resolution");
    m_paintingTimeLoc = GetShaderLocation(m_paintingShader, "time");
    m_paintingLookLoc = GetShaderLocation(m_paintingShader, "lookInfluence");
    const float paintingResolution[2] = {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)};
    SetShaderValue(m_paintingShader, m_paintingResolutionLoc, paintingResolution, SHADER_UNIFORM_VEC2);

    m_activeMaterialTextureId = -1;
    m_shadowMapDirty = true;

    if (m_cubeModel.meshCount > 0) {
        m_cubeModel.materials[0].shader = m_lightingShader;
    }
    if (m_sphereModel.meshCount > 0) {
        m_sphereModel.materials[0].shader = m_lightingShader;
    }
    if (m_cylinderModel.meshCount > 0) {
        m_cylinderModel.materials[0].shader = m_lightingShader;
    }
}

void Game::unloadRenderTargets() {
    if (renderTextureLoaded(m_feedbackTargets[0])) {
        UnloadRenderTexture(m_feedbackTargets[0]);
        m_feedbackTargets[0] = {};
    }
    if (renderTextureLoaded(m_feedbackTargets[1])) {
        UnloadRenderTexture(m_feedbackTargets[1]);
        m_feedbackTargets[1] = {};
    }
    if (renderTextureLoaded(m_sceneTarget)) {
        UnloadRenderTexture(m_sceneTarget);
        m_sceneTarget = {};
    }
}

void Game::resizeRenderTargets(int width, int height) {
    if (width == WindowWidth && height == WindowHeight && renderTextureLoaded(m_sceneTarget)) {
        return;
    }

    unloadRenderTargets();
    WindowWidth = width;
    WindowHeight = height;

    m_sceneTarget = LoadRenderTexture(WindowWidth, WindowHeight);
    SetTextureFilter(m_sceneTarget.texture, TEXTURE_FILTER_BILINEAR);
    m_feedbackTargets[0] = LoadRenderTexture(WindowWidth, WindowHeight);
    m_feedbackTargets[1] = LoadRenderTexture(WindowWidth, WindowHeight);
    SetTextureFilter(m_feedbackTargets[0].texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_feedbackTargets[1].texture, TEXTURE_FILTER_BILINEAR);

    BeginTextureMode(m_feedbackTargets[0]);
    ClearBackground({18, 20, 20, 255});
    EndTextureMode();
    BeginTextureMode(m_feedbackTargets[1]);
    ClearBackground({18, 20, 20, 255});
    EndTextureMode();

    m_camera.offset = {WindowWidth * 0.5f, WindowHeight * 0.5f};
    m_feedbackWriteIndex = 0;

    if (m_paintingShader.id > 0 && m_paintingResolutionLoc >= 0) {
        const float paintingResolution[2] = {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)};
        SetShaderValue(m_paintingShader, m_paintingResolutionLoc, paintingResolution, SHADER_UNIFORM_VEC2);
    }
}

void Game::run() {
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        update(dt);
        render();
    }
}

void Game::loadRainSoundtrack() {
    // A larger stream buffer prevents rain underruns when the GPU has a heavy frame.
    SetAudioStreamBufferSizeDefault(RainSoundtrackBufferFrames);
    InitAudioDevice();
    m_audioReady = IsAudioDeviceReady();
    if (!m_audioReady) {
        std::cout << "Audio device unavailable; rain soundtrack disabled.\n";
        return;
    }

    // The soundtrack is streamed twice so the end can overlap the beginning.
    // That gives us a 30 second crossfade without storing the whole track in memory.
    m_rainMusicPrimary = LoadMusicStream(RainSoundtrackPath);
    m_rainMusicSecondary = LoadMusicStream(RainSoundtrackPath);
    m_rainPrimaryLoaded = m_rainMusicPrimary.stream.buffer != nullptr;
    m_rainSecondaryLoaded = m_rainMusicSecondary.stream.buffer != nullptr;

    if (!m_rainPrimaryLoaded) {
        std::cout << "Could not load rain soundtrack: " << RainSoundtrackPath << "\n";
        return;
    }

    m_rainMusicPrimary.looping = false;
    m_rainMusicSecondary.looping = false;
    SetMusicVolume(m_rainMusicPrimary, RainSoundtrackVolume);
    SetMusicVolume(m_rainMusicSecondary, 0.0f);
    PlayMusicStream(m_rainMusicPrimary);
}

void Game::unloadRainSoundtrack() {
    if (!m_audioReady) {
        return;
    }

    if (m_rainPrimaryLoaded) {
        UnloadMusicStream(m_rainMusicPrimary);
        m_rainPrimaryLoaded = false;
    }
    if (m_rainSecondaryLoaded) {
        UnloadMusicStream(m_rainMusicSecondary);
        m_rainSecondaryLoaded = false;
    }

    CloseAudioDevice();
    m_audioReady = false;
}

void Game::updateRainSoundtrack() {
    if (!m_audioReady || !m_rainPrimaryLoaded) {
        return;
    }

    UpdateMusicStream(m_rainMusicPrimary);

    const float length = GetMusicTimeLength(m_rainMusicPrimary);
    const float played = GetMusicTimePlayed(m_rainMusicPrimary);
    if (length <= 0.0f) {
        return;
    }

    const float crossfadeDuration = std::min(RainSoundtrackCrossfadeSeconds, std::max(1.0f, length * 0.5f));
    if (!m_rainCrossfading && length - played <= crossfadeDuration) {
        startRainCrossfade(crossfadeDuration);
    }

    if (!m_rainCrossfading) {
        if (!IsMusicStreamPlaying(m_rainMusicPrimary)) {
            SeekMusicStream(m_rainMusicPrimary, 0.0f);
            PlayMusicStream(m_rainMusicPrimary);
            SetMusicVolume(m_rainMusicPrimary, RainSoundtrackVolume);
        }
        return;
    }

    if (!m_rainSecondaryLoaded) {
        if (!IsMusicStreamPlaying(m_rainMusicPrimary)) {
            SeekMusicStream(m_rainMusicPrimary, 0.0f);
            PlayMusicStream(m_rainMusicPrimary);
            SetMusicVolume(m_rainMusicPrimary, RainSoundtrackVolume);
            m_rainCrossfading = false;
        }
        return;
    }

    UpdateMusicStream(m_rainMusicSecondary);

    const float elapsed = static_cast<float>(GetTime()) - m_rainCrossfadeStart;
    const float fade = std::clamp(elapsed / m_rainCrossfadeDuration, 0.0f, 1.0f);
    SetMusicVolume(m_rainMusicPrimary, RainSoundtrackVolume * (1.0f - fade));
    SetMusicVolume(m_rainMusicSecondary, RainSoundtrackVolume * fade);

    if (fade >= 1.0f || !IsMusicStreamPlaying(m_rainMusicPrimary)) {
        finishRainCrossfade();
    }
}

void Game::startRainCrossfade(float duration) {
    if (!m_rainSecondaryLoaded) {
        return;
    }

    m_rainCrossfadeDuration = std::max(1.0f, duration);
    m_rainCrossfadeStart = static_cast<float>(GetTime());
    StopMusicStream(m_rainMusicSecondary);
    SeekMusicStream(m_rainMusicSecondary, 0.0f);
    SetMusicVolume(m_rainMusicSecondary, 0.0f);
    PlayMusicStream(m_rainMusicSecondary);
    m_rainCrossfading = true;
}

void Game::finishRainCrossfade() {
    if (!m_rainSecondaryLoaded) {
        m_rainCrossfading = false;
        return;
    }

    StopMusicStream(m_rainMusicPrimary);
    std::swap(m_rainMusicPrimary, m_rainMusicSecondary);
    SetMusicVolume(m_rainMusicPrimary, RainSoundtrackVolume);
    StopMusicStream(m_rainMusicSecondary);
    SeekMusicStream(m_rainMusicSecondary, 0.0f);
    SetMusicVolume(m_rainMusicSecondary, 0.0f);
    m_rainCrossfading = false;
}

void Game::update(float dt) {
    updateRainSoundtrack();
    if (!IsCursorHidden()) {
        HideCursor();
    }

    if (IsKeyPressed(KEY_M)) {
        m_resolutionMenuOpen = !m_resolutionMenuOpen;
    }

    if (m_resolutionMenuOpen) {
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
            resizeRenderTargets(CurrentRenderWidth, CurrentRenderHeight);
            m_resolutionMenuOpen = false;
        }
        if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
            resizeRenderTargets(FullHdRenderWidth, FullHdRenderHeight);
            m_resolutionMenuOpen = false;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            m_resolutionMenuOpen = false;
        }
        updateWindowTitle();
        return;
    }

    if (IsKeyPressed(KEY_P)) {
        m_deltas.toggleMarker(playerTile());
    }
    if (IsKeyPressed(KEY_F3)) {
        printDebugTile();
    }
    if (IsKeyPressed(KEY_I)) {
        m_useMeshVisuals = !m_useMeshVisuals;
        loadVisualShaders();
    }
    const Vector2 mouseDelta = GetMouseDelta();
    m_firstPersonAngle += mouseDelta.x * MouseSensitivity;
    m_firstPersonPitch = std::clamp(m_firstPersonPitch - mouseDelta.y * MouseSensitivity, -1.18f, 1.05f);

    Vector2 movement{0.0f, 0.0f};
    if (IsKeyDown(KEY_LEFT)) {
        m_firstPersonAngle -= TurnSpeed * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        m_firstPersonAngle += TurnSpeed * dt;
    }

    const Vector2 forward = directionFromAngle(m_firstPersonAngle);
    const Vector2 right = rightFromAngle(m_firstPersonAngle);
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        movement = add(movement, forward);
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        movement = subtract(movement, forward);
    }
    if (IsKeyDown(KEY_A)) {
        movement = subtract(movement, right);
    }
    if (IsKeyDown(KEY_D)) {
        movement = add(movement, right);
    }
    m_player.setMovementTuning(122.5f, 1650.0f, 3.4f);

    const Vector2 previousPosition = m_player.position();
    m_player.setMovementInput(movement);
    m_player.update(dt);
    resolvePlayerCollision(previousPosition);
    m_camera.target = m_player.position();
    m_chunks.updateAround(playerTile());
    updateWindowTitle();
}

void Game::render() {
    const Vector2 playerWorld = m_player.position();
    const Vector2 playerTilePosition{
        playerWorld.x / static_cast<float>(TileSize),
        playerWorld.y / static_cast<float>(TileSize)
    };
    constexpr int shadowDrawRadius = 23;
    const TileCoord currentTile = playerTile();
    if (m_shadowMapDirty || !(currentTile == m_shadowTile)) {
        updateShadowMap(playerTilePosition, shadowDrawRadius);
        m_shadowTile = currentTile;
        m_shadowMapDirty = false;
    }

    BeginTextureMode(m_sceneTarget);
    renderFirstPerson();
    EndTextureMode();

    BeginTextureMode(m_feedbackTargets[m_feedbackWriteIndex]);
    drawBleachedPaintingPass();
    EndTextureMode();

    BeginDrawing();
    drawTextureToScreen(m_feedbackTargets[m_feedbackWriteIndex].texture);
    if (m_resolutionMenuOpen) {
        drawResolutionMenu();
    }
    EndDrawing();

    m_feedbackWriteIndex = 1 - m_feedbackWriteIndex;
}

void Game::renderTopDown() {
    ClearBackground({8, 10, 9, 255});
    BeginMode2D(m_camera);

    const Vector2 center = m_player.position();
    const int minTileX = static_cast<int>(std::floor((center.x - WindowWidth * 0.55f) / TileSize));
    const int maxTileX = static_cast<int>(std::ceil((center.x + WindowWidth * 0.55f) / TileSize));
    const int minTileY = static_cast<int>(std::floor((center.y - WindowHeight * 0.60f) / TileSize));
    const int maxTileY = static_cast<int>(std::ceil((center.y + WindowHeight * 0.60f) / TileSize));

    for (int y = minTileY; y <= maxTileY; ++y) {
        for (int x = minTileX; x <= maxTileX; ++x) {
            const TileCoord tile{x, y};
            DrawRectangle(x * TileSize, y * TileSize, TileSize, TileSize, m_chunks.tileColor(m_chunks.tileAt(tile), tile));
        }
    }

    for (const auto& object : m_chunks.objectsNear(playerTile(), 2)) {
        drawTopDownObject(object);
    }

    for (const auto& marker : m_deltas.markers()) {
        DrawRectangle(marker.x * TileSize + 2, marker.y * TileSize + 2, TileSize - 4, TileSize - 4, {245, 214, 93, 255});
        DrawRectangleLines(marker.x * TileSize + 2, marker.y * TileSize + 2, TileSize - 4, TileSize - 4, {30, 24, 18, 255});
    }

    DrawCircleV(m_player.position(), 7.0f, {232, 220, 186, 255});
    DrawCircleLines(static_cast<int>(m_player.position().x), static_cast<int>(m_player.position().y), 8.0f, {28, 24, 20, 255});
    EndMode2D();
    drawScreenAtmosphere();
}

void Game::drawTopDownObject(const WorldObject& object) {
    const float x = static_cast<float>(object.tile.x * TileSize);
    const float y = static_cast<float>(object.tile.y * TileSize);
    const Color base = m_chunks.objectColor(object.type);

    switch (object.type) {
        case ObjectType::Tree:
            DrawCircle(static_cast<int>(x + TileSize * 0.52f), static_cast<int>(y + TileSize * 0.45f), TileSize * 0.42f, base);
            DrawCircleLines(static_cast<int>(x + TileSize * 0.52f), static_cast<int>(y + TileSize * 0.45f), TileSize * 0.42f, {7, 24, 13, 180});
            DrawRectangle(static_cast<int>(x + TileSize * 0.42f), static_cast<int>(y + TileSize * 0.55f), 3, 5, {61, 45, 30, 255});
            break;
        case ObjectType::Stone:
            DrawPoly({x + TileSize * 0.50f, y + TileSize * 0.54f}, 7, TileSize * 0.32f, 18.0f, base);
            break;
        case ObjectType::Ruin:
            DrawRectangle(static_cast<int>(x + 2), static_cast<int>(y + 1), TileSize - 4, TileSize - 2, base);
            DrawRectangle(static_cast<int>(x + 4), static_cast<int>(y + 4), TileSize - 7, 2, {45, 41, 36, 255});
            break;
        case ObjectType::Marker:
            DrawPoly({x + TileSize * 0.50f, y + TileSize * 0.45f}, 4, TileSize * 0.34f, 45.0f, base);
            break;
        case ObjectType::Anomaly:
            DrawCircle(static_cast<int>(x + TileSize * 0.5f), static_cast<int>(y + TileSize * 0.5f), TileSize * 0.45f, {base.r, base.g, base.b, 78});
            DrawPoly({x + TileSize * 0.5f, y + TileSize * 0.5f}, 5, TileSize * 0.30f, 18.0f, base);
            break;
    }
}

void Game::renderFirstPerson() {
    ClearBackground(m_useMeshVisuals ? BLACK : Color{18, 24, 27, 255});

    const Vector2 playerWorld = m_player.position();
    const Vector2 playerTilePosition{
        playerWorld.x / static_cast<float>(TileSize),
        playerWorld.y / static_cast<float>(TileSize)
    };
    const Vector2 forward = directionFromAngle(m_firstPersonAngle);
    const float pitchCos = std::cos(m_firstPersonPitch);

    Camera3D camera{};
    camera.position = {playerTilePosition.x, 1.58f, playerTilePosition.y};
    camera.target = {
        playerTilePosition.x + forward.x * pitchCos * 4.0f,
        1.58f + std::sin(m_firstPersonPitch) * 4.0f,
        playerTilePosition.y + forward.y * pitchCos * 4.0f
    };
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 66.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    updateLightingShader(camera);

    constexpr int drawRadius = 53;

    rlEnableShader(m_lightingShader.id);
    int shadowSlot = 10;
    rlActiveTextureSlot(shadowSlot);
    rlEnableTexture(m_shadowMap.depth.id);
    rlSetUniform(m_shadowMapLoc, &shadowSlot, SHADER_UNIFORM_INT, 1);

    BeginMode3D(camera);
    renderFirstPersonScene(playerTilePosition, drawRadius, forward, true);
    EndMode3D();

    DrawRectangleGradientV(0, 0, WindowWidth, 210, {1, 4, 8, 205}, {8, 13, 17, 0});
    DrawRectangleGradientV(0, WindowHeight - 240, WindowWidth, 240, {0, 0, 0, 0}, {0, 0, 0, 150});
    DrawRectangle(0, 0, WindowWidth, WindowHeight, {18, 25, 34, 42});
}

void Game::renderFirstPersonScene(Vector2 playerTilePosition, int drawRadius, Vector2 viewForward, bool cullToView) {
    const int centerX = static_cast<int>(std::floor(playerTilePosition.x));
    const int centerY = static_cast<int>(std::floor(playerTilePosition.y));
    constexpr float ViewCullBehindTolerance = -0.30f;

    for (int y = centerY - drawRadius; y <= centerY + drawRadius; ++y) {
        for (int x = centerX - drawRadius; x <= centerX + drawRadius; ++x) {
            const TileCoord tile{x, y};
            const float dx = static_cast<float>(x) + 0.5f - playerTilePosition.x;
            const float dy = static_cast<float>(y) + 0.5f - playerTilePosition.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared > static_cast<float>(drawRadius * drawRadius)) {
                continue;
            }
            if (cullToView && distanceSquared > 9.0f) {
                const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                const float forwardDot = (dx * viewForward.x + dy * viewForward.y) * inverseDistance;
                if (forwardDot < ViewCullBehindTolerance) {
                    continue;
                }
            }
            if (cullToView && isCityBuilding(tile) && distanceSquared > 41.0f * 41.0f) {
                const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                const float forwardDot = (dx * viewForward.x + dy * viewForward.y) * inverseDistance;
                if (forwardDot < 0.08f) {
                    continue;
                }
            }
            if (cullToView && !isCityBuilding(tile) && distanceSquared > 43.5f * 43.5f) {
                continue;
            }
            draw3DTerrainTile(tile, m_chunks.tileAt(tile), playerTilePosition);
        }
    }

    for (const auto& marker : m_deltas.markers()) {
        const Vector2 markerPos{static_cast<float>(marker.x), static_cast<float>(marker.y)};
        const float dx = markerPos.x - playerTilePosition.x;
        const float dy = markerPos.y - playerTilePosition.y;
        if (dx * dx + dy * dy < drawRadius * drawRadius) {
            drawContactShadow({marker.x + 0.5f, 0.01f, marker.y + 0.5f}, 0.34f, 74.0f);
            drawLitCube({marker.x + 0.5f, 0.35f, marker.y + 0.5f}, {0.18f, 0.70f, 0.18f}, {245, 214, 93, 255}, m_concreteTexture);
            DrawCubeWires({marker.x + 0.5f, 0.35f, marker.y + 0.5f}, 0.18f, 0.70f, 0.18f, {30, 24, 18, 255});
        }
    }

    if (!m_renderingShadowMap) {
        for (const auto& object : m_chunks.objectsNear(playerTile(), 2)) {
            if (object.type == ObjectType::Anomaly || object.type == ObjectType::Marker) {
                draw3DObject(object, playerTilePosition);
            }
        }
    }
}

void Game::updateShadowMap(Vector2 playerTilePosition, int drawRadius) {
    if (m_shadowMap.id == 0 || m_shadowMap.depth.id == 0) {
        return;
    }

    const Vector3 lightDir = Vector3Normalize({-0.42f, -0.86f, -0.28f});
    const Vector3 lightTarget{playerTilePosition.x, 0.0f, playerTilePosition.y};
    Camera3D lightCamera{};
    lightCamera.position = Vector3Subtract(lightTarget, Vector3Scale(lightDir, 30.0f));
    lightCamera.target = lightTarget;
    lightCamera.up = {0.0f, 1.0f, 0.0f};
    lightCamera.fovy = 34.0f;
    lightCamera.projection = CAMERA_ORTHOGRAPHIC;

    Matrix lightView{};
    Matrix lightProjection{};
    const int useShadowMap = 0;
    SetShaderValue(m_lightingShader, m_useShadowMapLoc, &useShadowMap, SHADER_UNIFORM_INT);

    BeginTextureMode(m_shadowMap);
    ClearBackground(WHITE);
    BeginMode3D(lightCamera);
    lightView = rlGetMatrixModelview();
    lightProjection = rlGetMatrixProjection();
    m_renderingShadowMap = true;
    renderFirstPersonScene(playerTilePosition, drawRadius);
    m_renderingShadowMap = false;
    EndMode3D();
    EndTextureMode();

    const Matrix lightViewProjection = MatrixMultiply(lightView, lightProjection);
    const int enableShadowMap = 1;
    SetShaderValueMatrix(m_lightingShader, m_lightVPLoc, lightViewProjection);
    SetShaderValue(m_lightingShader, m_useShadowMapLoc, &enableShadowMap, SHADER_UNIFORM_INT);
}

void Game::draw3DTerrainTile(TileCoord tile, TileType type, Vector2 playerTilePosition) {
    const float dx = static_cast<float>(tile.x) + 0.5f - playerTilePosition.x;
    const float dy = static_cast<float>(tile.y) + 0.5f - playerTilePosition.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    const bool street = isCityStreet(tile);
    const bool courtyard = isCityCourtyard(tile);
    Color paving = street ? Color{24, 29, 30, 255} : Color{82, 83, 75, 255};
    if (courtyard) {
        paving = {31, 49, 41, 255};
    }
    paving = distanceShade(adjust(paving, static_cast<int>(stableHash(0xDAB5ULL, tile.x, tile.y, 0xA5F) % 17) - 8), distance);

    const float slabHeight = street ? 0.10f : 0.16f;
    drawLitCube({tile.x + 0.5f, -0.06f + slabHeight * 0.5f, tile.y + 0.5f}, {1.0f, slabHeight, 1.0f}, paving, street ? m_groundTexture : m_concreteTexture);

    if (!m_renderingShadowMap && !isCityBuilding(tile) && distance < 41.0f) {
        drawReflectivePuddle(tile, paving, distance);
    }

    if (!m_renderingShadowMap && street && distance < 29.0f && positiveMod(tile.x + tile.y * 3, 11) == 0) {
        drawLitCube({tile.x + 0.5f, 0.018f, tile.y + 0.5f}, {0.78f, 0.025f, 0.16f}, mixColor(paving, {176, 170, 142, 255}, 0.42f), m_concreteTexture);
    }

    if (!isCityBuilding(tile)) {
        if (!m_renderingShadowMap && isLampPostTile(tile)) {
            const Vector3 lampBase{tile.x + 0.5f, 0.03f, tile.y + 0.5f};
            drawContactShadow({lampBase.x, 0.012f, lampBase.z}, 0.58f, 150.0f);
            drawLitCube({lampBase.x, 0.075f, lampBase.z}, {0.56f, 0.15f, 0.56f}, {126, 48, 31, 255}, m_concreteTexture);
            drawLitCylinder({lampBase.x, 0.30f, lampBase.z}, 0.145f, 0.45f, {102, 35, 24, 255}, m_concreteTexture);
            drawLitCylinder({lampBase.x, 2.15f, lampBase.z}, 0.085f, 3.70f, {154, 55, 32, 255}, m_concreteTexture);
            drawLitCube({lampBase.x + 0.035f, 2.15f, lampBase.z - 0.035f}, {0.035f, 3.42f, 0.018f}, {214, 86, 42, 255}, m_concreteTexture);
            drawLitCube({lampBase.x, 2.02f, lampBase.z}, {0.32f, 0.11f, 0.32f}, {151, 58, 34, 255}, m_concreteTexture);
            drawLitCube({lampBase.x, 3.90f, lampBase.z}, {0.82f, 0.13f, 0.18f}, {95, 31, 22, 255}, m_concreteTexture);
            drawLitCube({lampBase.x, 3.68f, lampBase.z}, {0.42f, 0.36f, 0.42f}, {129, 43, 26, 255}, m_concreteTexture);
            drawGlowCube({lampBase.x, 3.60f, lampBase.z}, {0.58f, 0.34f, 0.58f}, {255, 238, 154, 255});
            drawLitCube({lampBase.x, 3.56f, lampBase.z}, {0.36f, 0.22f, 0.36f}, {255, 226, 126, 255}, m_concreteTexture);
            drawLitCube({lampBase.x - 0.33f, 3.48f, lampBase.z}, {0.24f, 0.12f, 0.20f}, {255, 231, 145, 255}, m_concreteTexture);
            drawLitCube({lampBase.x + 0.33f, 3.48f, lampBase.z}, {0.24f, 0.12f, 0.20f}, {255, 231, 145, 255}, m_concreteTexture);
            DrawCubeWires({lampBase.x, 3.68f, lampBase.z}, 0.48f, 0.42f, 0.48f, {255, 249, 202, 255});
            DrawLine3D({lampBase.x, 0.42f, lampBase.z}, {lampBase.x, 3.76f, lampBase.z}, {220, 74, 38, 255});
            DrawLine3D({lampBase.x + 0.06f, 0.42f, lampBase.z + 0.01f}, {lampBase.x + 0.06f, 3.76f, lampBase.z + 0.01f}, {96, 24, 18, 255});
            DrawPoint3D({lampBase.x, 3.60f, lampBase.z}, {255, 246, 194, 255});
        }
        return;
    }

    const float height = cityBuildingHeight(tile);
    Color concrete = distanceShade(cityConcreteColor(tile, height), distance);
    const float inset = hashToUnit(stableHash(0x1E55ULL, tile.x, tile.y, 0x5150)) > 0.78 ? 0.16f : 0.06f;
    const float lobbyHeight = 2.8f;
    const float towerHeight = std::max(height - lobbyHeight, 1.0f);
    drawLitCube({tile.x + 0.5f, lobbyHeight + towerHeight * 0.5f, tile.y + 0.5f}, {1.0f - inset, towerHeight, 1.0f - inset}, concrete, m_concreteTexture);

    if (!m_renderingShadowMap && distance < 33.0f) {
        const Color columnColor = mixColor(concrete, {18, 22, 22, 255}, 0.24f);
        const float columnHeight = lobbyHeight * 0.94f;
        drawLitCube({tile.x + 0.16f, columnHeight * 0.5f, tile.y + 0.16f}, {0.15f, columnHeight, 0.15f}, columnColor, m_concreteTexture);
        drawLitCube({tile.x + 0.84f, columnHeight * 0.5f, tile.y + 0.16f}, {0.15f, columnHeight, 0.15f}, columnColor, m_concreteTexture);
        drawLitCube({tile.x + 0.16f, columnHeight * 0.5f, tile.y + 0.84f}, {0.15f, columnHeight, 0.15f}, columnColor, m_concreteTexture);
        drawLitCube({tile.x + 0.84f, columnHeight * 0.5f, tile.y + 0.84f}, {0.15f, columnHeight, 0.15f}, columnColor, m_concreteTexture);
    }

    if (!m_renderingShadowMap && distance < 29.0f && hashToUnit(stableHash(0x10987ULL, tile.x, tile.y, 0x10BB)) > 0.58) {
        const Color innerLight = hashToUnit(stableHash(0x10987ULL, tile.x, tile.y, 0xAA1)) > 0.55
            ? Color{255, 205, 94, 245}
            : Color{116, 222, 255, 220};
        drawGlowCube({tile.x + 0.5f, 2.36f, tile.y + 0.5f}, {0.82f, 0.14f, 0.82f}, innerLight);
    }

    if (height > 9.0f && distance < 43.5f) {
        const float capHeight = 0.22f + static_cast<float>(hashToUnit(stableHash(0xB4F0ULL, tile.x, tile.y, 0xCA9))) * 0.22f;
        drawLitCube({tile.x + 0.5f, height + capHeight * 0.5f, tile.y + 0.5f}, {1.06f, capHeight, 1.06f}, mixColor(concrete, {215, 210, 178, 255}, 0.24f), m_concreteTexture);
    }

    if (!m_renderingShadowMap && distance < 25.5f) {
        drawCityFacadeDetails(tile, height, concrete, playerTilePosition);
    }
}

void Game::drawCityFacadeDetails(TileCoord tile, float height, Color color, Vector2 playerTilePosition) {
    const float x = tile.x + 0.5f;
    const float z = tile.y + 0.5f;
    const Color rib = mixColor(color, {14, 17, 17, 255}, 0.38f);
    const Color glass = {1, 5, 9, 255};
    const Color warmWindow = {255, 198, 82, 255};
    const Color coldWindow = {76, 215, 255, 235};

    const auto windowColor = [](TileCoord coord, int floor, int side, Color warm, Color cold) {
        const double lit = hashToUnit(stableHash(0xFACADEULL, coord.x, coord.y, floor * 11 + side));
        if (lit < 0.54) {
            return Color{1, 5, 9, 255};
        }
        return lit > 0.84 ? cold : warm;
    };

    if (isCityStreet({tile.x - 1, tile.y})) {
        drawLitCube({x - 0.485f, height * 0.48f, z}, {0.035f, height * 0.84f, 0.78f}, rib, m_concreteTexture);
        int floor = 0;
        for (float y = 3.4f; y < height - 0.8f; y += 2.1f, ++floor) {
            const Color window = windowColor(tile, floor, 0, warmWindow, coldWindow);
            drawLitCube({x - 0.508f, y, z}, {0.018f, 0.22f, 0.52f}, glass, m_waterTexture);
            if (!m_renderingShadowMap && window.r > 20) {
                drawGlowCube({x - 0.522f, y, z}, {0.018f, 0.34f, 0.64f}, window);
            }
        }
    }
    if (isCityStreet({tile.x + 1, tile.y})) {
        drawLitCube({x + 0.485f, height * 0.48f, z}, {0.035f, height * 0.84f, 0.78f}, rib, m_concreteTexture);
        int floor = 0;
        for (float y = 3.4f; y < height - 0.8f; y += 2.1f, ++floor) {
            const Color window = windowColor(tile, floor, 1, warmWindow, coldWindow);
            drawLitCube({x + 0.508f, y, z}, {0.018f, 0.22f, 0.52f}, glass, m_waterTexture);
            if (!m_renderingShadowMap && window.r > 20) {
                drawGlowCube({x + 0.522f, y, z}, {0.018f, 0.34f, 0.64f}, window);
            }
        }
    }
    if (isCityStreet({tile.x, tile.y - 1})) {
        drawLitCube({x, height * 0.48f, z - 0.485f}, {0.78f, height * 0.84f, 0.035f}, rib, m_concreteTexture);
        int floor = 0;
        for (float y = 3.4f; y < height - 0.8f; y += 2.1f, ++floor) {
            const Color window = windowColor(tile, floor, 2, warmWindow, coldWindow);
            drawLitCube({x, y, z - 0.508f}, {0.52f, 0.22f, 0.018f}, glass, m_waterTexture);
            if (!m_renderingShadowMap && window.r > 20) {
                drawGlowCube({x, y, z - 0.522f}, {0.64f, 0.34f, 0.018f}, window);
            }
        }
    }
    if (isCityStreet({tile.x, tile.y + 1})) {
        drawLitCube({x, height * 0.48f, z + 0.485f}, {0.78f, height * 0.84f, 0.035f}, rib, m_concreteTexture);
        int floor = 0;
        for (float y = 3.4f; y < height - 0.8f; y += 2.1f, ++floor) {
            const Color window = windowColor(tile, floor, 3, warmWindow, coldWindow);
            drawLitCube({x, y, z + 0.508f}, {0.52f, 0.22f, 0.018f}, glass, m_waterTexture);
            if (!m_renderingShadowMap && window.r > 20) {
                drawGlowCube({x, y, z + 0.522f}, {0.64f, 0.34f, 0.018f}, window);
            }
        }
    }

    const double aerial = hashToUnit(stableHash(0xA17EULL, tile.x, tile.y, 0x700F));
    if (aerial > 0.86 && height > 11.0f) {
        drawLitCube({x, height + 0.55f, z}, {0.16f, 1.10f, 0.16f}, mixColor(color, {232, 230, 192, 255}, 0.30f), m_concreteTexture);
    }
}

void Game::draw3DObject(const WorldObject& object, Vector2 playerTilePosition) {
    const float x = object.tile.x + 0.5f;
    const float z = object.tile.y + 0.5f;
    const float dx = x - playerTilePosition.x;
    const float dz = z - playerTilePosition.y;
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (distance > 46.0f) {
        return;
    }

    const Color base = distanceShade(m_chunks.objectColor(object.type), distance);
    switch (object.type) {
        case ObjectType::Tree:
            drawContactShadow({x, 0.02f, z}, 0.58f, 80.0f);
            drawLitCylinder({x, 0.74f, z}, 0.10f, 1.48f, {54, 44, 36, 255}, m_barkTexture);
            drawLitSphere({x, 1.66f, z}, 0.52f, base, textureForObject(object.type));
            break;
        case ObjectType::Stone:
            drawContactShadow({x, 0.02f, z}, 0.62f, 92.0f);
            drawLitCube({x, 0.42f, z}, {0.82f, 0.84f, 0.66f}, base, textureForObject(object.type));
            drawLitCube({x + 0.18f, 0.84f, z - 0.12f}, {0.40f, 0.34f, 0.48f}, mixColor(base, {70, 74, 70, 255}, 0.22f), textureForObject(object.type));
            DrawCubeWires({x, 0.42f, z}, 0.82f, 0.84f, 0.66f, mixColor(base, {0, 0, 0, 255}, 0.34f));
            break;
        case ObjectType::Ruin:
            drawContactShadow({x, 0.02f, z}, 0.95f, 120.0f);
            drawLitCube({x - 0.26f, 1.12f, z}, {0.36f, 2.24f, 1.12f}, base, textureForObject(object.type));
            drawLitCube({x + 0.22f, 0.82f, z - 0.22f}, {0.66f, 1.64f, 0.34f}, mixColor(base, {64, 67, 64, 255}, 0.20f), textureForObject(object.type));
            drawLitCube({x + 0.12f, 1.85f, z + 0.22f}, {1.16f, 0.24f, 0.42f}, mixColor(base, {144, 146, 138, 255}, 0.18f), textureForObject(object.type));
            drawLitCube({x - 0.46f, 0.42f, z - 0.34f}, {0.44f, 0.84f, 0.34f}, mixColor(base, {52, 55, 52, 255}, 0.24f), textureForObject(object.type));
            DrawCubeWires({x - 0.26f, 1.12f, z}, 0.36f, 2.24f, 1.12f, {34, 36, 34, 255});
            break;
        case ObjectType::Marker:
            drawContactShadow({x, 0.02f, z}, 0.42f, 76.0f);
            drawLitCube({x, 0.72f, z}, {0.14f, 1.44f, 0.14f}, base, textureForObject(object.type));
            drawLitCube({x, 1.34f, z}, {0.52f, 0.16f, 0.12f}, mixColor(base, {255, 150, 90, 255}, 0.18f), textureForObject(object.type));
            break;
        case ObjectType::Anomaly:
            drawContactShadow({x, 0.02f, z}, 0.72f, 68.0f);
            drawLitSphere({x, 1.10f, z}, 0.58f, {base.r, base.g, base.b, 220}, textureForObject(object.type));
            DrawCylinder({x, 1.10f, z}, 0.68f, 0.22f, 0.12f, 5, {base.r, base.g, base.b, 120});
            break;
    }
}

void Game::drawReflectivePuddle(TileCoord tile, Color paving, float distance) {
    const double puddleChance = hashToUnit(stableHash(0xCAFEF100DULL, tile.x / 2, tile.y / 2, 0x7755));
    const double localShape = hashToUnit(stableHash(0xCAFEF100DULL, tile.x, tile.y, 0xA17));
    const bool street = isCityStreet(tile);
    const bool courtyard = isCityCourtyard(tile);
    if ((!street && !courtyard) || puddleChance < 0.26 || localShape < 0.08) {
        return;
    }

    const float wide = 0.62f + static_cast<float>(hashToUnit(stableHash(0xCAFEF100DULL, tile.x, tile.y, 0x501))) * 0.52f;
    const float deep = 0.42f + static_cast<float>(hashToUnit(stableHash(0xCAFEF100DULL, tile.x, tile.y, 0x502))) * 0.56f;
    const float offsetX = (static_cast<float>(hashToUnit(stableHash(0xCAFEF100DULL, tile.x, tile.y, 0x503))) - 0.5f) * 0.16f;
    const float offsetZ = (static_cast<float>(hashToUnit(stableHash(0xCAFEF100DULL, tile.x, tile.y, 0x504))) - 0.5f) * 0.16f;
    const Color darkWater = mixColor({12, 52, 78, 245}, paving, 0.06f);
    drawLitCube(
        {tile.x + 0.5f + offsetX, 0.055f, tile.y + 0.5f + offsetZ},
        {wide, 0.030f, deep},
        darkWater,
        m_waterTexture
    );

    if (distance < 22.0f && localShape > 0.66) {
        const Color brightRim{205, 236, 255, 185};
        drawLitCube(
            {tile.x + 0.5f + offsetX * 0.5f, 0.078f, tile.y + 0.5f + offsetZ * 0.5f},
            {wide * 0.72f, 0.014f, 0.040f},
            brightRim,
            m_waterTexture
        );
    }
}

void Game::drawGlowCube(Vector3 position, Vector3 scale, Color color) {
    DrawCube(position, scale.x, scale.y, scale.z, color);
    DrawCube(position, scale.x * 2.25f, scale.y * 1.95f, scale.z * 2.25f, {color.r, color.g, color.b, static_cast<unsigned char>(color.a * 0.30f)});
    DrawCube(position, scale.x * 3.25f, scale.y * 2.55f, scale.z * 3.25f, {color.r, color.g, color.b, static_cast<unsigned char>(color.a * 0.10f)});
}

void Game::drawLitCube(Vector3 position, Vector3 scale, Color tint, Texture2D texture) {
    setMaterialUniforms(texture);
    m_cubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    DrawModelEx(m_cubeModel, position, {0.0f, 1.0f, 0.0f}, 0.0f, scale, tint);
}

void Game::drawLitSphere(Vector3 position, float radius, Color tint, Texture2D texture) {
    setMaterialUniforms(texture);
    m_sphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    const float diameter = radius * 2.0f;
    DrawModelEx(m_sphereModel, position, {0.0f, 1.0f, 0.0f}, 0.0f, {diameter, diameter, diameter}, tint);
}

void Game::drawLitCylinder(Vector3 position, float radius, float height, Color tint, Texture2D texture) {
    setMaterialUniforms(texture);
    m_cylinderModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    DrawModelEx(m_cylinderModel, position, {0.0f, 1.0f, 0.0f}, 0.0f, {radius * 2.0f, height, radius * 2.0f}, tint);
}

Texture2D Game::textureForTile(TileType type) const {
    switch (type) {
        case TileType::DeepWater:
        case TileType::ShallowWater:
            return m_waterTexture;
        case TileType::Grass:
            return m_groundTexture;
        case TileType::Forest:
            return m_mossTexture;
        case TileType::Stone:
            return m_concreteTexture;
        case TileType::Strange:
            return m_anomalyTexture;
    }
    return m_concreteTexture;
}

Texture2D Game::textureForObject(ObjectType type) const {
    switch (type) {
        case ObjectType::Tree: return m_mossTexture;
        case ObjectType::Stone:
        case ObjectType::Ruin:
        case ObjectType::Marker:
            return m_concreteTexture;
        case ObjectType::Anomaly:
            return m_anomalyTexture;
    }
    return m_concreteTexture;
}

void Game::setMaterialUniforms(Texture2D texture) {
    if (m_activeMaterialTextureId == static_cast<int>(texture.id)) {
        return;
    }
    m_activeMaterialTextureId = static_cast<int>(texture.id);

    float depth = 0.80f;
    float scale = 4.0f;

    if (texture.id == m_concreteTexture.id) {
        depth = 1.15f;
        scale = 5.4f;
    } else if (texture.id == m_groundTexture.id) {
        depth = 0.90f;
        scale = 3.6f;
    } else if (texture.id == m_waterTexture.id) {
        depth = 0.22f;
        scale = 7.2f;
    } else if (texture.id == m_mossTexture.id) {
        depth = 1.00f;
        scale = 4.8f;
    } else if (texture.id == m_barkTexture.id) {
        depth = 1.35f;
        scale = 8.0f;
    } else if (texture.id == m_anomalyTexture.id) {
        depth = 1.55f;
        scale = 6.5f;
    }

    SetShaderValue(m_lightingShader, m_materialDepthLoc, &depth, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_lightingShader, m_textureScaleLoc, &scale, SHADER_UNIFORM_FLOAT);
}

void Game::drawContactShadow(Vector3 position, float radius, float alpha) {
    if (m_renderingShadowMap) {
        return;
    }

    const auto clampAlpha = [](float value) {
        return static_cast<unsigned char>(std::clamp(value, 0.0f, 255.0f));
    };

    DrawCylinder(position, radius * 1.10f, radius * 0.78f, 0.014f, 32, {0, 0, 0, clampAlpha(alpha * 0.52f)});
    DrawCylinder({position.x + 0.10f, position.y + 0.004f, position.z + 0.06f}, radius * 0.78f, radius * 0.48f, 0.010f, 32, {0, 0, 0, clampAlpha(alpha * 0.48f)});
    DrawCylinder({position.x + 0.22f, position.y + 0.006f, position.z + 0.13f}, radius * 0.52f, radius * 0.24f, 0.008f, 24, {0, 0, 0, clampAlpha(alpha * 0.28f)});
}

void Game::updateLightingShader(Camera3D camera) {
    SetShaderValue(m_lightingShader, m_viewPosLoc, &camera.position.x, SHADER_UNIFORM_VEC3);
    const float time = GetTime();
    SetShaderValue(m_lightingShader, m_lightingTimeLoc, &time, SHADER_UNIFORM_FLOAT);
}

void Game::drawBleachedPaintingPass() {
    ClearBackground(m_useMeshVisuals ? BLACK : Color{210, 206, 188, 255});
    const float time = GetTime();
    SetShaderValue(m_paintingShader, m_paintingTimeLoc, &time, SHADER_UNIFORM_FLOAT);
    const float lookInfluence[2] = {
        std::sin(m_firstPersonAngle) * 0.72f,
        std::clamp(m_firstPersonPitch, -0.85f, 0.85f)
    };
    SetShaderValue(m_paintingShader, m_paintingLookLoc, lookInfluence, SHADER_UNIFORM_VEC2);

    BeginShaderMode(m_paintingShader);
    int feedbackSlot = 1;
    rlActiveTextureSlot(feedbackSlot);
    rlEnableTexture(m_feedbackTargets[1 - m_feedbackWriteIndex].texture.id);
    rlSetUniform(m_paintingFeedbackLoc, &feedbackSlot, SHADER_UNIFORM_INT, 1);
    DrawTexturePro(
        m_sceneTarget.texture,
        {0.0f, 0.0f, static_cast<float>(m_sceneTarget.texture.width), -static_cast<float>(m_sceneTarget.texture.height)},
        {0.0f, 0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)},
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
    EndShaderMode();
}

void Game::drawTextureToScreen(Texture2D texture) {
    ClearBackground(BLACK);
    DrawTexturePro(
        texture,
        {0.0f, 0.0f, static_cast<float>(texture.width), -static_cast<float>(texture.height)},
        {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}

void Game::drawResolutionMenu() {
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    const int panelWidth = 560;
    const int panelHeight = 280;
    const int panelX = (screenWidth - panelWidth) / 2;
    const int panelY = (screenHeight - panelHeight) / 2;

    DrawRectangle(0, 0, screenWidth, screenHeight, {0, 0, 0, 120});
    DrawRectangle(panelX - 8, panelY - 8, panelWidth + 16, panelHeight + 16, {245, 226, 165, 38});
    DrawRectangle(panelX, panelY, panelWidth, panelHeight, {12, 15, 17, 232});
    DrawRectangleLinesEx({static_cast<float>(panelX), static_cast<float>(panelY), static_cast<float>(panelWidth), static_cast<float>(panelHeight)}, 2.0f, {230, 214, 166, 180});

    DrawText("Shader Resolution", panelX + 34, panelY + 30, 32, {238, 226, 191, 255});
    DrawText("M closes this menu. Movement pauses while it is open.", panelX + 36, panelY + 74, 18, {174, 184, 184, 255});

    const bool currentSelected = WindowWidth == CurrentRenderWidth && WindowHeight == CurrentRenderHeight;
    const bool fullHdSelected = WindowWidth == FullHdRenderWidth && WindowHeight == FullHdRenderHeight;
    const Color selected = {255, 221, 132, 255};
    const Color normal = {204, 211, 202, 255};

    DrawText(currentSelected ? "> 1  Current 1600 x 900" : "  1  Current 1600 x 900", panelX + 52, panelY + 130, 24, currentSelected ? selected : normal);
    DrawText(fullHdSelected ? "> 2  1080p 1920 x 1080" : "  2  1080p 1920 x 1080", panelX + 52, panelY + 172, 24, fullHdSelected ? selected : normal);
    DrawText("Esc cancels   |   P places a marker", panelX + 52, panelY + 228, 18, {156, 162, 153, 255});
}

void Game::drawScreenAtmosphere() {
    DrawRectangle(0, 0, WindowWidth, 110, {0, 0, 0, 46});
    DrawRectangle(0, WindowHeight - 135, WindowWidth, 135, {0, 0, 0, 72});
    DrawRectangle(0, 0, 120, WindowHeight, {0, 0, 0, 34});
    DrawRectangle(WindowWidth - 120, 0, 120, WindowHeight, {0, 0, 0, 34});
}

void Game::updateWindowTitle() {
    const TileCoord tile = playerTile();
    const ChunkCoord chunk = m_chunks.chunkCoordForTile(tile);
    std::ostringstream title;
    title << "Zone Drifter | pos " << tile.x << "," << tile.y
          << " | chunk " << chunk.x << "," << chunk.y
          << " | shader " << WindowWidth << "x" << WindowHeight
          << (m_useMeshVisuals ? " | mesh shader" : " | psychedelic shader")
          << " | loaded " << m_chunks.loadedChunkCount()
          << " | mouse look | WASD move | M resolution | P marker | I shader | F3 debug";
    SetWindowTitle(title.str().c_str());
}

void Game::printDebugTile() {
    const TileCoord tile = playerTile();
    const ChunkCoord chunk = m_chunks.chunkCoordForTile(tile);
    const TileType type = m_chunks.tileAt(tile);

    std::cout << "Player tile: " << tile.x << "," << tile.y
              << " chunk: " << chunk.x << "," << chunk.y
              << " chunk seed: " << m_generator.chunkSeed(chunk)
              << " tile type: " << tileTypeName(type)
              << '\n';
}

bool Game::collidesWithWorld(Vector2 worldPosition) const {
    const Vector2 tilePosition{
        worldPosition.x / static_cast<float>(TileSize),
        worldPosition.y / static_cast<float>(TileSize)
    };
    const int centerX = static_cast<int>(std::floor(tilePosition.x));
    const int centerY = static_cast<int>(std::floor(tilePosition.y));

    for (int y = centerY - 1; y <= centerY + 1; ++y) {
        for (int x = centerX - 1; x <= centerX + 1; ++x) {
            const TileCoord tile{x, y};

            if (isCityBuilding(tile)) {
                const float inset = 0.04f;
                if (circleIntersectsAabb(tilePosition, PlayerCollisionRadius, x + inset, y + inset, x + 1.0f - inset, y + 1.0f - inset)) {
                    return true;
                }
            }

            if (isLampPostTile(tile)) {
                const float dx = tilePosition.x - (static_cast<float>(x) + 0.5f);
                const float dy = tilePosition.y - (static_cast<float>(y) + 0.5f);
                const float radius = PlayerCollisionRadius + 0.17f;
                if (dx * dx + dy * dy < radius * radius) {
                    return true;
                }
            }
        }
    }

    return false;
}

void Game::resolvePlayerCollision(Vector2 previousPosition) {
    const Vector2 current = m_player.position();
    if (!collidesWithWorld(current)) {
        return;
    }

    const Vector2 xOnly{current.x, previousPosition.y};
    if (!collidesWithWorld(xOnly)) {
        m_player.setPosition(xOnly);
        return;
    }

    const Vector2 yOnly{previousPosition.x, current.y};
    if (!collidesWithWorld(yOnly)) {
        m_player.setPosition(yOnly);
        return;
    }

    m_player.setPosition(previousPosition);
}

Color Game::shadeForDistance(Color color, float distance) const {
    const float fog = std::clamp(distance / 760.0f, 0.0f, 1.0f);
    const Color fogColor{10, 14, 17, 255};
    return mixColor(color, fogColor, fog);
}

float Game::terrainHeight(TileType type, TileCoord tile) const {
    if (m_deltas.hasMarker(tile)) {
        return 42.0f;
    }

    switch (type) {
        case TileType::Forest: return 64.0f;
        case TileType::Stone: return 42.0f;
        case TileType::Strange: return 86.0f;
        case TileType::DeepWater:
        case TileType::ShallowWater:
        case TileType::Grass:
            return 0.0f;
    }
    return 0.0f;
}

TileCoord Game::playerTile() const {
    return worldPositionToTile(m_player.position());
}

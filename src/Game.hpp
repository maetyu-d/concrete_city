#pragma once

#include "ChunkManager.hpp"
#include "DeltaLayer.hpp"
#include "Player.hpp"
#include "WorldGenerator.hpp"

#include <raylib.h>

enum class ViewMode {
    TopDown,
    FirstPerson
};

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void update(float dt);
    void render();
    void renderTopDown();
    void renderFirstPerson();
    void drawTopDownObject(const WorldObject& object);
    void draw3DTerrainTile(TileCoord tile, TileType type, Vector2 playerTilePosition);
    void draw3DObject(const WorldObject& object, Vector2 playerTilePosition);
    void drawCityFacadeDetails(TileCoord tile, float height, Color color, Vector2 playerTilePosition);
    void drawGlowCube(Vector3 position, Vector3 scale, Color color);
    void drawLitCube(Vector3 position, Vector3 scale, Color tint, Texture2D texture);
    void drawLitSphere(Vector3 position, float radius, Color tint, Texture2D texture);
    void drawLitCylinder(Vector3 position, float radius, float height, Color tint, Texture2D texture);
    void drawReflectivePuddle(TileCoord tile, Color paving, float distance);
    Texture2D textureForTile(TileType type) const;
    Texture2D textureForObject(ObjectType type) const;
    void setMaterialUniforms(Texture2D texture);
    void drawContactShadow(Vector3 position, float radius, float alpha);
    void renderFirstPersonScene(Vector2 playerTilePosition, int drawRadius);
    void updateShadowMap(Vector2 playerTilePosition, int drawRadius);
    void loadVisualShaders();
    void unloadVisualShaders();
    void updateLightingShader(Camera3D camera);
    void drawBleachedPaintingPass();
    void drawTextureToScreen(Texture2D texture);
    void drawScreenAtmosphere();
    void updateWindowTitle();
    void printDebugTile();
    bool collidesWithWorld(Vector2 worldPosition) const;
    void resolvePlayerCollision(Vector2 previousPosition);
    Color shadeForDistance(Color color, float distance) const;
    float terrainHeight(TileType type, TileCoord tile) const;
    TileCoord playerTile() const;

    WorldGenerator m_generator;
    ChunkManager m_chunks;
    DeltaLayer m_deltas;
    Player m_player;
    Camera2D m_camera{};
    Shader m_lightingShader{};
    Model m_cubeModel{};
    Model m_sphereModel{};
    Model m_cylinderModel{};
    Texture2D m_concreteTexture{};
    Texture2D m_groundTexture{};
    Texture2D m_waterTexture{};
    Texture2D m_mossTexture{};
    Texture2D m_barkTexture{};
    Texture2D m_anomalyTexture{};
    RenderTexture2D m_shadowMap{};
    RenderTexture2D m_sceneTarget{};
    RenderTexture2D m_feedbackTargets[2]{};
    Shader m_paintingShader{};
    int m_lightDirLoc = -1;
    int m_viewPosLoc = -1;
    int m_materialDepthLoc = -1;
    int m_textureScaleLoc = -1;
    int m_lightingTimeLoc = -1;
    int m_lightVPLoc = -1;
    int m_shadowMapLoc = -1;
    int m_shadowMapResolutionLoc = -1;
    int m_useShadowMapLoc = -1;
    int m_paintingFeedbackLoc = -1;
    int m_paintingResolutionLoc = -1;
    int m_paintingTimeLoc = -1;
    int m_feedbackWriteIndex = 0;
    bool m_renderingShadowMap = false;
    bool m_useMeshVisuals = false;
    ViewMode m_viewMode = ViewMode::TopDown;
    float m_firstPersonAngle = -1.5707963f;
    float m_firstPersonPitch = -0.02f;
};

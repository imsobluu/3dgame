#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Item
{
    int lane;
    float z;
    bool active;
    int heightUnits;
};

enum class GameState
{
    Running,
    Win,
    Lose
};

// Window settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Game tuning
const float ROAD_BLOCK_SIZE = 2.0f;
const float ROAD_SEGMENT_SPACING = ROAD_BLOCK_SIZE;
const float LANE_X[3] = {-ROAD_BLOCK_SIZE, 0.0f, ROAD_BLOCK_SIZE};
const float PLAYER_Z = 0.0f;
const float TRACK_LENGTH = 360.0f;
const int TRACK_TILE_COUNT = 120;
const float ROAD_YAW_DEG = 42.0f;
const float FORWARD_SPEED_START = 18.0f;
const float FORWARD_ACCEL = 0.45f;
const float LANE_SWITCH_SPEED = 10.0f;
const float JUMP_VELOCITY = 8.5f;
const float GRAVITY = 20.0f;
const float COLLISION_Z_THRESHOLD = 0.75f;
const float COLLISION_X_THRESHOLD = 0.6f;
const int TARGET_SCORE = 25;
const float ROUND_TIME = 60.0f;
const int OBSTACLE_POOL_SIZE = 36;

// Global timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float travelDistance = 0.0f;
float rollAngle = 0.0f;

// Input edge detection
bool laneLeftLatch = false;
bool laneRightLatch = false;
bool jumpLatch = false;

// Cube rendering helpers
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window, GameState &gameState, int &targetLane, float &playerYVel, float playerY);
glm::vec3 localToWorld(float localX, float localZ, float y);
void initCube();
void renderCube();
unsigned int createSolidTexture(const glm::vec3 &color);

bool fileExists(const std::string &path)
{
    std::ifstream file(path);
    return file.good();
}

std::string resolveExistingPath(std::initializer_list<std::string> candidates)
{
    for (const auto &candidate : candidates)
    {
        if (fileExists(candidate))
        {
            return candidate;
        }
    }
    return candidates.size() > 0 ? *candidates.begin() : std::string{};
}

struct ModelBounds
{
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 center;
    float height;
};

ModelBounds computeModelBounds(const Model &model)
{
    glm::vec3 minBound(std::numeric_limits<float>::max());
    glm::vec3 maxBound(std::numeric_limits<float>::lowest());

    for (const auto &mesh : model.meshes)
    {
        for (const auto &vertex : mesh.vertices)
        {
            minBound.x = std::min(minBound.x, vertex.Position.x);
            minBound.y = std::min(minBound.y, vertex.Position.y);
            minBound.z = std::min(minBound.z, vertex.Position.z);
            maxBound.x = std::max(maxBound.x, vertex.Position.x);
            maxBound.y = std::max(maxBound.y, vertex.Position.y);
            maxBound.z = std::max(maxBound.z, vertex.Position.z);
        }
    }

    ModelBounds bounds{};
    bounds.min = minBound;
    bounds.max = maxBound;
    bounds.center = glm::vec3(
        (minBound.x + maxBound.x) * 0.5f,
        (minBound.y + maxBound.y) * 0.5f,
        (minBound.z + maxBound.z) * 0.5f);
    bounds.height = maxBound.y - minBound.y;
    return bounds;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Coin Runner", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    const std::string vertexPath = resolveExistingPath({
        "7.coin_runner.vs",
        "src/7.coin_runner/7.coin_runner.vs",
        "../src/7.coin_runner/7.coin_runner.vs"});
    const std::string fragmentPath = resolveExistingPath({
        "7.coin_runner.fs",
        "src/7.coin_runner/7.coin_runner.fs",
        "../src/7.coin_runner/7.coin_runner.fs"});
    Shader colorShader(vertexPath.c_str(), fragmentPath.c_str());

    const std::string modelVertexPath = resolveExistingPath({
        "7.coin_runner_model.vs",
        "src/7.coin_runner/7.coin_runner_model.vs",
        "../src/7.coin_runner/7.coin_runner_model.vs"});
    const std::string modelFragmentPath = resolveExistingPath({
        "7.coin_runner_model.fs",
        "src/7.coin_runner/7.coin_runner_model.fs",
        "../src/7.coin_runner/7.coin_runner_model.fs"});
    Shader modelShader(modelVertexPath.c_str(), modelFragmentPath.c_str());
    initCube();

    const std::string kirbyModelPath = resolveExistingPath({
        "resources/kirby-ball/source/Kirby Ball.obj",
        "../resources/kirby-ball/source/Kirby Ball.obj"});
    Model kirbyModel(kirbyModelPath);
    std::cout << "Kirby model loaded from: " << kirbyModelPath << "\n"
              << "Meshes: " << kirbyModel.meshes.size()
              << " | Textures: " << kirbyModel.textures_loaded.size() << std::endl;
    const ModelBounds kirbyBounds = computeModelBounds(kirbyModel);
    const float kirbyTargetHeight = 0.9f;
    const float kirbyScale = kirbyBounds.height > 0.0f ? (kirbyTargetHeight / kirbyBounds.height) : 1.0f;
    const float kirbyRadius = kirbyTargetHeight * 0.5f;
    const float kirbyGroundOffset = kirbyBounds.center.y - kirbyBounds.min.y;
    const bool kirbyHasTextures = !kirbyModel.textures_loaded.empty();
    const unsigned int kirbyFallbackTexture = kirbyHasTextures ? 0 : createSolidTexture(glm::vec3(1.0f));

    // Deterministic random for repeatable class demo runs.
    std::mt19937 rng(77);
    std::uniform_int_distribution<int> laneDist(0, 2);
    std::uniform_int_distribution<int> obstacleHeightDist(1, 2);
    std::uniform_real_distribution<float> coinSpacingJitter(2.0f, 6.0f);
    std::uniform_real_distribution<float> obstacleSpacingJitter(4.0f, 9.0f);
    std::uniform_int_distribution<int> patternDist(0, 3);
    std::uniform_int_distribution<int> mirrorDist(0, 1);

    std::vector<Item> coins;
    std::vector<Item> obstacles;

    const int initialCoinCount = 28;

    float nextCoinZ = 20.0f;
    for (int i = 0; i < initialCoinCount; ++i)
    {
        coins.push_back({laneDist(rng), nextCoinZ, true, 1});
        nextCoinZ += 12.0f + coinSpacingJitter(rng);
    }

    for (int i = 0; i < OBSTACLE_POOL_SIZE; ++i)
    {
        obstacles.push_back({1, 0.0f, false, 1});
    }

    auto countInactiveObstacles = [&]()
    {
        int count = 0;
        for (const Item &o : obstacles)
        {
            if (!o.active)
            {
                ++count;
            }
        }
        return count;
    };

    auto spawnObstacleAt = [&](int lane, float z, int heightOverride)
    {
        for (Item &o : obstacles)
        {
            if (!o.active)
            {
                o.lane = lane;
                o.z = z;
                o.active = true;
                o.heightUnits = heightOverride > 0 ? heightOverride : obstacleHeightDist(rng);
                return true;
            }
        }
        return false;
    };

    auto trySpawnPattern = [&](float baseZ)
    {
        const bool mirror = mirrorDist(rng) == 1;
        const int pattern = patternDist(rng);

        std::vector<std::pair<int, float>> cells;
        if (pattern == 0)
        {
            // ___ solid row
            cells = {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}};
        }
        else if (pattern == 1)
        {
            // Side walls with middle hole
            cells = {{0, 0.0f}, {2, 0.0f}};
        }
        else if (pattern == 2)
        {
            // Two-block pair on the left or right
            if (!mirror)
            {
                cells = {{0, 0.0f}, {1, 0.0f}};
            }
            else
            {
                cells = {{1, 0.0f}, {2, 0.0f}};
            }
        }
        else
        {
            // Single pillar in a random lane (keep variety without depth)
            int lane = laneDist(rng);
            cells = {{lane, 0.0f}};
        }

        if (countInactiveObstacles() < static_cast<int>(cells.size()))
        {
            return false;
        }

        const int heightOverride = (pattern == 0) ? 1 : 0;
        for (const auto &cell : cells)
        {
            if (!spawnObstacleAt(cell.first, baseZ + cell.second, heightOverride))
            {
                return false;
            }
        }
        return true;
    };

    float obstacleSpawnCursorZ = 26.0f;
    while (obstacleSpawnCursorZ < 240.0f)
    {
        if (trySpawnPattern(obstacleSpawnCursorZ))
        {
            obstacleSpawnCursorZ += 20.0f + obstacleSpacingJitter(rng);
        }
        else
        {
            break;
        }
    }

    GameState gameState = GameState::Running;
    float elapsed = 0.0f;
    float forwardSpeed = FORWARD_SPEED_START;
    int score = 0;

    int currentLane = 1;
    int targetLane = 1;
    float playerX = LANE_X[currentLane];
    float playerY = 0.5f;
    float playerYVel = 0.0f;

    float hudTimer = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, gameState, targetLane, playerYVel, playerY);

        if (gameState == GameState::Running)
        {
            elapsed += deltaTime;
            forwardSpeed += FORWARD_ACCEL * deltaTime;
            travelDistance += forwardSpeed * deltaTime;
            if (kirbyRadius > 0.0f)
            {
                rollAngle += (forwardSpeed * deltaTime) / kirbyRadius;
            }

            // Smooth lane switching
            float desiredX = LANE_X[targetLane];
            float step = LANE_SWITCH_SPEED * deltaTime;
            if (std::fabs(desiredX - playerX) <= step)
            {
                playerX = desiredX;
                currentLane = targetLane;
            }
            else
            {
                playerX += (desiredX > playerX ? 1.0f : -1.0f) * step;
            }

            // Jump physics
            playerYVel -= GRAVITY * deltaTime;
            playerY += playerYVel * deltaTime;
            if (playerY <= 0.5f)
            {
                playerY = 0.5f;
                playerYVel = 0.0f;
            }

            for (Item &c : coins)
            {
                // Items travel from far positive-Z toward the player.
                c.z -= forwardSpeed * deltaTime;
                if (c.z < -12.0f)
                {
                    float farthestZ = 0.0f;
                    for (const Item &other : coins)
                    {
                        if (&other == &c)
                        {
                            continue;
                        }
                        farthestZ = std::max(farthestZ, other.z);
                    }
                    c.z = farthestZ + 12.0f + coinSpacingJitter(rng);
                    c.lane = laneDist(rng);
                    c.active = true;
                }

                if (c.active)
                {
                    float dx = std::fabs(LANE_X[c.lane] - playerX);
                    float dz = std::fabs(c.z - PLAYER_Z);
                    float dy = std::fabs(1.0f - playerY);
                    if (dx < COLLISION_X_THRESHOLD && dz < COLLISION_Z_THRESHOLD && dy < 0.9f)
                    {
                        c.active = false;
                        score += 1;
                    }
                }
            }

            for (Item &o : obstacles)
            {
                if (!o.active)
                {
                    continue;
                }

                o.z -= forwardSpeed * deltaTime;
                if (o.z < -12.0f)
                {
                    o.active = false;
                    continue;
                }

                if (o.active)
                {
                    float dx = std::fabs(LANE_X[o.lane] - playerX);
                    float dz = std::fabs(o.z - PLAYER_Z);
                    float hitYThreshold = 1.35f + (o.heightUnits - 1) * 0.9f;
                    // Allow jumping over obstacles when high enough.
                    if (dx < COLLISION_X_THRESHOLD && dz < COLLISION_Z_THRESHOLD && playerY < hitYThreshold)
                    {
                        gameState = GameState::Lose;
                    }
                }
            }

            while (countInactiveObstacles() >= 2)
            {
                if (!trySpawnPattern(obstacleSpawnCursorZ))
                {
                    break;
                }
                obstacleSpawnCursorZ += 20.0f + obstacleSpacingJitter(rng);
            }

            if (score >= TARGET_SCORE)
            {
                gameState = GameState::Win;
            }
            else if (elapsed >= ROUND_TIME)
            {
                gameState = GameState::Lose;
            }
        }

        hudTimer += deltaTime;
        if (hudTimer > 0.2f)
        {
            float timeLeft = std::max(0.0f, ROUND_TIME - elapsed);
            std::ostringstream title;
            title << std::fixed << std::setprecision(1);
            title << "Coin Runner | Score: " << score << "/" << TARGET_SCORE
                  << " | Time: " << timeLeft;
            if (gameState == GameState::Win)
            {
                title << " | YOU WIN! (Press R to restart)";
            }
            else if (gameState == GameState::Lose)
            {
                title << " | YOU LOSE! (Press R to restart)";
            }
            glfwSetWindowTitle(window, title.str().c_str());
            hudTimer = 0.0f;
        }

        // Neon-inspired palette to match your reference mood.
        glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    colorShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(46.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 700.0f);
        // Diagonal chase camera: near road enters bottom-left and converges to horizon.
        glm::vec3 camPos = localToWorld(playerX - 14.0f, -21.0f, 8.2f);
        glm::vec3 camTarget = localToWorld(playerX + 0.1f, 62.0f, 0.8f);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    colorShader.setMat4("projection", projection);
    colorShader.setMat4("view", view);

        auto drawOutlinedCube = [&](const glm::mat4 &model, const glm::vec3 &fillColor)
        {
            // Fill pass
            colorShader.setMat4("model", model);
            colorShader.setVec3("objectColor", fillColor);
            renderCube();

            // Outline pass
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            colorShader.setVec3("objectColor", glm::vec3(0.02f, 0.08f, 0.12f));
            renderCube();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        };

        // Draw runway as chunky 3D blocks (one block per lane segment).
        const float roadCubeSize = ROAD_BLOCK_SIZE;
        const float trackLength = TRACK_TILE_COUNT * ROAD_SEGMENT_SPACING;
        for (int i = 0; i < TRACK_TILE_COUNT; ++i)
        {
            float z = (i * ROAD_SEGMENT_SPACING) - travelDistance;
            z = std::fmod(z, trackLength);
            if (z < 0.0f)
            {
                z += trackLength;
            }
            z -= trackLength * 0.5f;
            for (int lane = 0; lane < 3; ++lane)
            {
                // Keep cube top aligned near y=0 so player/obstacle heights still make sense.
                glm::vec3 blockPos = localToWorld(LANE_X[lane], z, -roadCubeSize * 0.5f);
                glm::mat4 blockModel = glm::mat4(1.0f);
                blockModel = glm::translate(blockModel, blockPos);
                blockModel = glm::rotate(blockModel, glm::radians(ROAD_YAW_DEG), glm::vec3(0.0f, 1.0f, 0.0f));
                blockModel = glm::scale(blockModel, glm::vec3(roadCubeSize, roadCubeSize, roadCubeSize));
                glm::vec3 blockColor;
                if (((i + lane) % 2) == 0)
                {
                    blockColor = glm::vec3(0.10f, 0.70f, 0.86f);
                }
                else
                {
                    blockColor = glm::vec3(0.07f, 0.44f, 0.62f);
                }
                drawOutlinedCube(blockModel, blockColor);
            }
        }

    // Draw player
        glm::vec3 playerPos = localToWorld(playerX, PLAYER_Z, playerY);
        glm::mat4 playerModel = glm::mat4(1.0f);
        playerModel = glm::translate(playerModel, playerPos);
    playerModel = glm::rotate(playerModel, glm::radians(ROAD_YAW_DEG), glm::vec3(0.0f, 1.0f, 0.0f));
    playerModel = glm::translate(playerModel, glm::vec3(0.0f, kirbyGroundOffset * kirbyScale, 0.0f));
    playerModel = glm::rotate(playerModel, rollAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    playerModel = glm::scale(playerModel, glm::vec3(kirbyScale));
    playerModel = glm::translate(playerModel, -kirbyBounds.center);
        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        modelShader.setMat4("model", playerModel);
        modelShader.setVec3("baseColor", glm::vec3(1.0f, 0.72f, 0.84f));
        modelShader.setVec3("lightDir", glm::vec3(-0.35f, -1.0f, -0.2f));
        modelShader.setBool("useTexture", kirbyHasTextures);
        modelShader.setInt("texture_diffuse1", 0);
        if (!kirbyHasTextures)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, kirbyFallbackTexture);
        }
        kirbyModel.Draw(modelShader);
        colorShader.use();
        colorShader.setMat4("projection", projection);
        colorShader.setMat4("view", view);

        // Draw coins
        for (const Item &c : coins)
        {
            if (!c.active)
            {
                continue;
            }
            glm::vec3 coinPos = localToWorld(LANE_X[c.lane], c.z, 1.0f);
            glm::mat4 coinModel = glm::mat4(1.0f);
            coinModel = glm::translate(coinModel, coinPos);
            coinModel = glm::rotate(coinModel, glm::radians(ROAD_YAW_DEG), glm::vec3(0.0f, 1.0f, 0.0f));
            coinModel = glm::scale(coinModel, glm::vec3(0.35f, 0.35f, 0.35f));
            drawOutlinedCube(coinModel, glm::vec3(1.0f, 0.85f, 0.25f));
        }

        // Draw obstacles
        for (const Item &o : obstacles)
        {
            if (!o.active)
            {
                continue;
            }
            float obstacleHeight = ROAD_BLOCK_SIZE * static_cast<float>(o.heightUnits);
            glm::vec3 obstaclePos = localToWorld(LANE_X[o.lane], o.z, obstacleHeight * 0.5f);
            glm::mat4 obstacleModel = glm::mat4(1.0f);
            obstacleModel = glm::translate(obstacleModel, obstaclePos);
            obstacleModel = glm::rotate(obstacleModel, glm::radians(ROAD_YAW_DEG), glm::vec3(0.0f, 1.0f, 0.0f));
            obstacleModel = glm::scale(obstacleModel, glm::vec3(ROAD_BLOCK_SIZE, obstacleHeight, ROAD_BLOCK_SIZE));
            drawOutlinedCube(obstacleModel, glm::vec3(1.0f, 0.1f, 0.35f));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        {
            if (gameState != GameState::Running)
            {
                // Reset all game state for quick replay.
                gameState = GameState::Running;
                elapsed = 0.0f;
                forwardSpeed = FORWARD_SPEED_START;
                score = 0;

                currentLane = 1;
                targetLane = 1;
                playerX = LANE_X[currentLane];
                playerY = 0.5f;
                playerYVel = 0.0f;

                float resetCoinZ = 20.0f;
                for (Item &c : coins)
                {
                    c.z = resetCoinZ;
                    resetCoinZ += 12.0f + coinSpacingJitter(rng);
                    c.lane = laneDist(rng);
                    c.active = true;
                    c.heightUnits = 1;
                }

                float resetObstacleZ = 26.0f;
                for (Item &o : obstacles)
                {
                    o.lane = 1;
                    o.z = 0.0f;
                    o.active = false;
                    o.heightUnits = 1;
                }

                obstacleSpawnCursorZ = resetObstacleZ;
                while (obstacleSpawnCursorZ < 240.0f)
                {
                    if (!trySpawnPattern(obstacleSpawnCursorZ))
                    {
                        break;
                    }
                    obstacleSpawnCursorZ += 20.0f + obstacleSpacingJitter(rng);
                }
            }
        }
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window, GameState &gameState, int &targetLane, float &playerYVel, float playerY)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if (gameState != GameState::Running)
    {
        return;
    }

    bool leftPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool rightPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (leftPressed && !laneLeftLatch)
    {
        targetLane = std::min(2, targetLane + 1);
    }
    if (rightPressed && !laneRightLatch)
    {
        targetLane = std::max(0, targetLane - 1);
    }

    if (jumpPressed && !jumpLatch && playerY <= 0.51f)
    {
        playerYVel = JUMP_VELOCITY;
    }

    laneLeftLatch = leftPressed;
    laneRightLatch = rightPressed;
    jumpLatch = jumpPressed;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

glm::vec3 localToWorld(float localX, float localZ, float y)
{
    // Rotate local lane space around Y to get a diagonal runway in world space.
    const float yaw = glm::radians(ROAD_YAW_DEG);
    const float c = std::cos(yaw);
    const float s = std::sin(yaw);

    const float worldX = localX * c + localZ * s;
    const float worldZ = -localX * s + localZ * c;
    return glm::vec3(worldX, y, worldZ);
}

void initCube()
{
    float vertices[] = {
        // back face
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        // front face
        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        // left face
        -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        // right face
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        // bottom face
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f,
        // top face
        -0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f};

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderCube()
{
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int createSolidTexture(const glm::vec3 &color)
{
    unsigned int textureId = 0;
    unsigned char pixel[3] = {
        static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f)};

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    return textureId;
}


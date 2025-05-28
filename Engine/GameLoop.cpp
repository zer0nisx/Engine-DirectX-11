#include "GameLoop.h"
#include <algorithm>
#include <thread>

GameLoop::GameLoop()
    : m_targetUPS(60)
    , m_targetFPS(0) // Unlimited by default
    , m_vSyncEnabled(true)
    , m_isRunning(false)
    , m_deltaTime(0.0f)
    , m_fixedDeltaTime(1.0f / 60.0f)
    , m_interpolation(0.0f)
    , m_currentFPS(0)
    , m_currentUPS(0)
    , m_frameCount(0)
    , m_updateCount(0)
    , m_frameTime(0.0)
    , m_updateTime(0.0)
    , m_historyIndex(0)
{
    SetTargetUPS(60); // This will set m_fixedTimestep

    // Initialize performance history
    for (int i = 0; i < STATS_HISTORY_SIZE; ++i)
    {
        m_frameTimeHistory[i] = 0.0;
        m_updateTimeHistory[i] = 0.0;
    }
}

GameLoop::~GameLoop()
{
    Stop();
}

void GameLoop::SetTargetUPS(int updatesPerSecond)
{
    m_targetUPS = std::max(1, updatesPerSecond);
    m_fixedTimestep = std::chrono::duration<double>(1.0 / m_targetUPS);
    m_fixedDeltaTime = static_cast<float>(m_fixedTimestep.count());
}

void GameLoop::SetTargetFPS(int framesPerSecond)
{
    m_targetFPS = std::max(0, framesPerSecond);
    if (m_targetFPS > 0)
    {
        m_frameTimeLimit = std::chrono::duration<double>(1.0 / m_targetFPS);
    }
    else
    {
        m_frameTimeLimit = std::chrono::duration<double>(0.0); // Unlimited
    }
}

void GameLoop::SetVSyncEnabled(bool enabled)
{
    m_vSyncEnabled = enabled;
}

void GameLoop::SetUpdateFunction(UpdateFunction updateFunc)
{
    m_updateFunction = updateFunc;
}

void GameLoop::SetRenderFunction(RenderFunction renderFunc)
{
    m_renderFunction = renderFunc;
}

void GameLoop::SetInputFunction(InputFunction inputFunc)
{
    m_inputFunction = inputFunc;
}

void GameLoop::Start()
{
    m_isRunning = true;
    m_lastTime = std::chrono::high_resolution_clock::now();
    m_currentTime = m_lastTime;
    m_lastStatsUpdate = m_lastTime;
    m_accumulator = std::chrono::duration<double>(0.0);

    ResetStats();
}

void GameLoop::Stop()
{
    m_isRunning = false;
}

void GameLoop::Run()
{
    Start();

    while (m_isRunning)
    {
        UpdateTiming();
        ProcessInput();
        UpdateLogic();
        Render();

        // Frame rate limiting (if not using VSync and target FPS is set)
        if (!m_vSyncEnabled && m_targetFPS > 0)
        {
            auto frameEnd = std::chrono::high_resolution_clock::now();
            auto frameDuration = frameEnd - m_currentTime;

            if (frameDuration < m_frameTimeLimit)
            {
                auto sleepTime = m_frameTimeLimit - frameDuration;
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }
}

void GameLoop::UpdateTiming()
{
    m_currentTime = std::chrono::high_resolution_clock::now();
    auto frameTime = m_currentTime - m_lastTime;
    m_lastTime = m_currentTime;

    // Convert to seconds
    m_deltaTime = static_cast<float>(frameTime.count() * 1e-9);

    // Clamp delta time to prevent spiral of death
    m_deltaTime = std::min(m_deltaTime, 0.05f); // Max 50ms per frame

    // Add to accumulator for fixed timestep
    m_accumulator += frameTime;

    // Update performance stats
    auto statsTime = std::chrono::duration<double>(m_currentTime - m_lastStatsUpdate).count();
    if (statsTime >= 1.0) // Update stats every second
    {
        m_currentFPS = m_frameCount;
        m_currentUPS = m_updateCount;
        m_frameCount = 0;
        m_updateCount = 0;
        m_lastStatsUpdate = m_currentTime;
    }
}

void GameLoop::ProcessInput()
{
    if (m_inputFunction)
    {
        m_inputFunction();
    }
}

void GameLoop::UpdateLogic()
{
    auto updateStart = std::chrono::high_resolution_clock::now();

    // Fixed timestep update loop
    while (m_accumulator >= m_fixedTimestep)
    {
        if (m_updateFunction)
        {
            m_updateFunction(m_fixedDeltaTime);
        }

        m_accumulator -= m_fixedTimestep;
        m_updateCount++;
    }

    // Calculate interpolation for smooth rendering
    m_interpolation = static_cast<float>(m_accumulator / m_fixedTimestep);

    auto updateEnd = std::chrono::high_resolution_clock::now();
    m_updateTime = std::chrono::duration<double>(updateEnd - updateStart).count() * 1000.0; // Convert to milliseconds

    // Store in history for averaging
    m_updateTimeHistory[m_historyIndex] = m_updateTime;
}

void GameLoop::Render()
{
    auto renderStart = std::chrono::high_resolution_clock::now();

    if (m_renderFunction)
    {
        m_renderFunction(m_interpolation);
    }

    m_frameCount++;

    auto renderEnd = std::chrono::high_resolution_clock::now();
    m_frameTime = std::chrono::duration<double>(renderEnd - renderStart).count() * 1000.0; // Convert to milliseconds

    // Store in history for averaging
    m_frameTimeHistory[m_historyIndex] = m_frameTime;

    // Update history index
    m_historyIndex = (m_historyIndex + 1) % STATS_HISTORY_SIZE;
}

void GameLoop::ResetStats()
{
    m_frameCount = 0;
    m_updateCount = 0;
    m_currentFPS = 0;
    m_currentUPS = 0;
    m_frameTime = 0.0;
    m_updateTime = 0.0;
    m_historyIndex = 0;

    for (int i = 0; i < STATS_HISTORY_SIZE; ++i)
    {
        m_frameTimeHistory[i] = 0.0;
        m_updateTimeHistory[i] = 0.0;
    }
}

double GameLoop::GetAverageFrameTime() const
{
    double total = 0.0;
    int count = 0;

    for (int i = 0; i < STATS_HISTORY_SIZE; ++i)
    {
        if (m_frameTimeHistory[i] > 0.0)
        {
            total += m_frameTimeHistory[i];
            count++;
        }
    }

    return count > 0 ? total / count : 0.0;
}

double GameLoop::GetAverageUpdateTime() const
{
    double total = 0.0;
    int count = 0;

    for (int i = 0; i < STATS_HISTORY_SIZE; ++i)
    {
        if (m_updateTimeHistory[i] > 0.0)
        {
            total += m_updateTimeHistory[i];
            count++;
        }
    }

    return count > 0 ? total / count : 0.0;
}

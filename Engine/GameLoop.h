#pragma once

#include <chrono>
#include <functional>

class GameLoop
{
public:
    using UpdateFunction = std::function<void(float)>;
    using RenderFunction = std::function<void(float)>;
    using InputFunction = std::function<void()>;

    GameLoop();
    ~GameLoop();

    // Configuration
    void SetTargetUPS(int updatesPerSecond); // Updates Per Second (game logic)
    void SetTargetFPS(int framesPerSecond);  // Frames Per Second (rendering), 0 = unlimited
    void SetVSyncEnabled(bool enabled);

    // Callback functions
    void SetUpdateFunction(UpdateFunction updateFunc);
    void SetRenderFunction(RenderFunction renderFunc);
    void SetInputFunction(InputFunction inputFunc);

    // Loop control
    void Start();
    void Stop();
    void Run();

    // Timing information
    float GetDeltaTime() const { return m_deltaTime; }
    float GetInterpolation() const { return m_interpolation; }
    int GetCurrentFPS() const { return m_currentFPS; }
    int GetCurrentUPS() const { return m_currentUPS; }
    double GetFrameTime() const { return m_frameTime; }
    double GetUpdateTime() const { return m_updateTime; }

    // Performance stats
    void ResetStats();
    double GetAverageFrameTime() const;
    double GetAverageUpdateTime() const;

private:
    void UpdateTiming();
    void ProcessInput();
    void UpdateLogic();
    void Render();

    // Timing configuration
    int m_targetUPS;
    int m_targetFPS;
    bool m_vSyncEnabled;
    bool m_isRunning;

    // Fixed timestep for game logic
    std::chrono::duration<double> m_fixedTimestep;
    std::chrono::duration<double> m_frameTimeLimit;

    // Timing variables
    std::chrono::high_resolution_clock::time_point m_lastTime;
    std::chrono::high_resolution_clock::time_point m_currentTime;
    std::chrono::duration<double> m_accumulator;

    float m_deltaTime;        // Delta time for rendering (variable)
    float m_fixedDeltaTime;   // Fixed delta time for game logic
    float m_interpolation;    // Interpolation factor for smooth rendering

    // Performance monitoring
    int m_currentFPS;
    int m_currentUPS;
    int m_frameCount;
    int m_updateCount;
    double m_frameTime;
    double m_updateTime;
    std::chrono::high_resolution_clock::time_point m_lastStatsUpdate;

    // Performance history for averaging
    static const int STATS_HISTORY_SIZE = 60;
    double m_frameTimeHistory[STATS_HISTORY_SIZE];
    double m_updateTimeHistory[STATS_HISTORY_SIZE];
    int m_historyIndex;

    // Callback functions
    UpdateFunction m_updateFunction;
    RenderFunction m_renderFunction;
    InputFunction m_inputFunction;
};

#if !defined(THREAD_HPP)
#define THREAD_HPP

#include <windows.h>

// Thread Manager Class
class ThreadManager {
public:
    // Thread management
    bool StartConsoleThread();
    bool StartWindowThread();
    void WaitForThreadsToFinish();
    void SignalExit();
    bool ShouldExit() const { return *m_globalExitFlag; }
    
    // Cleanup
    void Cleanup();
    
    // Constructor/Destructor
    ThreadManager(volatile bool* globalExitFlag);
    ~ThreadManager();

private:
    // Thread handles and IDs
    HANDLE m_hConsoleThread = nullptr;
    HANDLE m_hWindowThread = nullptr;
    DWORD m_consoleThreadId = 0;
    DWORD m_windowThreadId = 0;
    
    // Global exit flag reference
    volatile bool* m_globalExitFlag = nullptr;
    
    // Helper methods
    void ForceTerminateThreads();
};

// Thread function declarations (to be called by ThreadManager)
DWORD WINAPI ConsoleThreadProc(LPVOID lpParam);
DWORD WINAPI WindowThreadProc(LPVOID lpParam);

#endif // THREAD_HPP
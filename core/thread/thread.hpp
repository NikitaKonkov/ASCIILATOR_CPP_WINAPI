#if !defined(THREAD_HPP)
#define THREAD_HPP

#include <windows.h>
#include <map>
#include <string>

// Thread types enumeration
enum class ThreadType {
    CONSOLE_THREAD,
    WINDOW_THREAD,
    SOUND_THREAD,
    RENDER_THREAD
};

// Thread information structure
struct ThreadInfo {
    HANDLE handle = nullptr;
    DWORD threadId = 0;
    ThreadType type;
    std::string name;
};

// Thread Manager Class
class ThreadManager {
public:
    // Generic thread management
    bool CreateThread(const std::string& threadId, ThreadType type);
    bool IsThreadRunning(const std::string& threadId);
    void WaitForThreadsToFinish();
    void WaitForThread(const std::string& threadId);
    void SignalExit();
    bool ShouldExit() const { return *m_globalExitFlag; }
    
    // Thread lifecycle
    void KillThread(const std::string& threadId);
    void KillAllThreads();
    void Cleanup();
    
    // Constructor/Destructor
    ThreadManager(volatile bool* globalExitFlag);
    ~ThreadManager();

private:
    // Thread storage
    std::map<std::string, ThreadInfo> m_threads;
    
    // Global exit flag reference
    volatile bool* m_globalExitFlag = nullptr;
    
    // Helper methods
    void ForceTerminateThread(const std::string& threadId);
    LPTHREAD_START_ROUTINE GetThreadProcedure(ThreadType type);
};

// Thread function declarations (to be called by ThreadManager)
DWORD WINAPI ConsoleThreadProc(LPVOID lpParam);
DWORD WINAPI WindowThreadProc(LPVOID lpParam);
DWORD WINAPI SoundThreadProc(LPVOID lpParam);
DWORD WINAPI RenderThreadProc(LPVOID lpParam);

#endif // THREAD_HPP
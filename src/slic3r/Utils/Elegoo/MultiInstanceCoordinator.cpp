#include "MultiInstanceCoordinator.hpp"
#include "libslic3r/Utils.hpp"
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/convert.hpp>
#include <mutex>
#include <condition_variable>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#endif

namespace Slic3r {

namespace fs = boost::filesystem;

MultiInstanceCoordinator::MultiInstanceCoordinator()
#ifdef _WIN32
    : mMasterLockHandle(nullptr)
#else
    : mMasterLockFd(-1)
#endif
{
    mIsMaster.store(false);
    mRunning.store(false);
}

MultiInstanceCoordinator::~MultiInstanceCoordinator() {
    uninit();
}

bool MultiInstanceCoordinator::init() {
    // Try to acquire master lock
    if (tryAcquireMasterLock()) {
        mIsMaster.store(true);
        BOOST_LOG_TRIVIAL(info) << "MultiInstanceCoordinator: This instance is master";
        
        // Start monitoring thread
        mRunning.store(true);
        mMonitorThread = std::thread([this]() { monitorMasterLock(); });
        
        return true;
    } else {
        mIsMaster.store(false);
        BOOST_LOG_TRIVIAL(info) << "MultiInstanceCoordinator: This instance is slave (another instance is master)";
        
        // Start monitoring thread to detect when master exits
        mRunning.store(true);
        mMonitorThread = std::thread([this]() { monitorMasterLock(); });
        
        return false;
    }
}

void MultiInstanceCoordinator::uninit() {
    // Stop monitor thread
    mRunning.store(false);
    {
        std::lock_guard<std::mutex> lk(mCvMutex);
        mCv.notify_all();
    }
    
    if (mMonitorThread.joinable()) {
        mMonitorThread.join();
    }
    
    // Release lock if held
    releaseMasterLock();
    mIsMaster.store(false);
}

std::string MultiInstanceCoordinator::getMasterLockPath() {
    static std::string lockPath;
    if (lockPath.empty()) {
        fs::path dataDir = fs::path(Slic3r::data_dir());
        fs::path path = dataDir / "cache" / "elegoo_master_instance.lock";
        lockPath = path.string();
    }
    return lockPath;
}

bool MultiInstanceCoordinator::tryAcquireMasterLock() {
    std::string lockPath = getMasterLockPath();
    
    // Ensure parent directory exists
    fs::path lockFilePath(lockPath);
    try {
        if (!fs::exists(lockFilePath.parent_path())) {
            fs::create_directories(lockFilePath.parent_path());
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "MultiInstanceCoordinator: Failed to ensure lock directory: " << e.what();
        return false;
    }
    
#ifdef _WIN32
    // Windows: open or create the file with exclusive access (share mode = 0)
    HANDLE h = CreateFileW(
        boost::nowide::widen(lockPath).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // no sharing -> exclusive
        NULL,
        OPEN_ALWAYS, // keep file persistent
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (h == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION) {
            // another process has opened with exclusive access => master exists
            return false;
        }
        BOOST_LOG_TRIVIAL(error) << "MultiInstanceCoordinator: CreateFileW failed: " << error;
        return false;
    }
    
    // Successfully opened with exclusive access -> we are master
    mMasterLockHandle = h;
    return true;
#else
    // POSIX: open file and use flock for exclusive non-blocking lock
    int fd = open(lockPath.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        BOOST_LOG_TRIVIAL(error) << "MultiInstanceCoordinator: Failed to open lock file: " << strerror(errno);
        return false;
    }
    
    // Try non-blocking exclusive flock
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        // We got the lock -> keep fd open
        mMasterLockFd = fd;
        return true;
    } else {
        // Failed to get lock
        int err = errno;
        close(fd);
        if (err == EWOULDBLOCK || err == EAGAIN) {
            // Another process holds the lock
            return false;
        }
        BOOST_LOG_TRIVIAL(error) << "MultiInstanceCoordinator: flock failed: " << strerror(err);
        return false;
    }
#endif
}

void MultiInstanceCoordinator::releaseMasterLock() {
#ifdef _WIN32
    if (mMasterLockHandle != nullptr && mMasterLockHandle != INVALID_HANDLE_VALUE) {
        // Close handle - this releases exclusivity
        CloseHandle(mMasterLockHandle);
        mMasterLockHandle = nullptr;
    }
#else
    if (mMasterLockFd >= 0) {
        // Release flock and close fd
        flock(mMasterLockFd, LOCK_UN);
        close(mMasterLockFd);
        mMasterLockFd = -1;
    }
#endif
}

bool MultiInstanceCoordinator::isMasterAlive() const {
    std::string lockPath = getMasterLockPath();
    
#ifdef _WIN32
    // If this instance is master, check our handle validity
    if (mIsMaster.load()) {
        if (mMasterLockHandle != nullptr && mMasterLockHandle != INVALID_HANDLE_VALUE) {
            // Quick check: GetFileInformationByHandle should succeed while handle is valid
            BY_HANDLE_FILE_INFORMATION info;
            if (GetFileInformationByHandle(mMasterLockHandle, &info)) {
                return true;
            }
        }
        return false;
    }
    
    // Slave: try to open with exclusive access. If sharing violation -> master alive.
    HANDLE h = CreateFileW(
        boost::nowide::widen(lockPath).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // try exclusive
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (h == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            // Lock file missing -> master likely not alive
            return false;
        }
        if (error == ERROR_SHARING_VIOLATION) {
            // file exists but opened exclusively by master -> master alive
            return true;
        }
        // Other errors: assume master not alive (conservative)
        return false;
    }
    
    // We managed to open file with exclusive access -> no master holding exclusive lock
    CloseHandle(h);
    return false;
#else
    // POSIX
    if (mIsMaster.load()) {
        if (mMasterLockFd >= 0) {
            struct stat st;
            if (fstat(mMasterLockFd, &st) == 0) {
                return true;
            }
        }
        return false;
    }
    
    // Slave path: try to open and attempt to acquire flock non-blocking to test
    int fd = open(lockPath.c_str(), O_RDONLY);
    if (fd < 0) {
        // File doesn't exist or cannot be opened -> master not alive
        return false;
    }
    
    // Try to acquire exclusive lock non-blocking
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        // We could get the lock -> master not alive. Release immediately.
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    } else {
        // Could not acquire lock
        int err = errno;
        close(fd);
        if (err == EWOULDBLOCK || err == EAGAIN) {
            // Someone else holds the lock -> master alive
            return true;
        }
        // On other errors, assume master not alive
        return false;
    }
#endif
}

void MultiInstanceCoordinator::monitorMasterLock() {
    std::unique_lock<std::mutex> cv_lk(mCvMutex);
    while (mRunning.load()) {
        // Wait for up to 1 second or until notify (so uninit can wake us quickly)
        mCv.wait_for(cv_lk, std::chrono::seconds(1));
        
        if (!mRunning.load()) break;
        
        if (mIsMaster.load()) {
            // Master: verify lock is still held; if lost -> demote
            if (!isMasterAlive()) {
                BOOST_LOG_TRIVIAL(warning) << "MultiInstanceCoordinator: Master lock lost!";
                releaseMasterLock();
                mIsMaster.store(false);
                
                // Notify callback about demotion (copy under lock)
                MasterStatusCallback cb;
                {
                    std::lock_guard<std::mutex> g(mCallbackMutex);
                    cb = mMasterStatusCallback;
                }
                if (cb) {
                    try {
                        cb(false);
                    } catch (...) {
                        BOOST_LOG_TRIVIAL(error) << "MultiInstanceCoordinator: Exception in master status callback (demotion)";
                    }
                }
            }
        } else {
            // Slave: check if master alive. If not, try to become master
            if (!isMasterAlive()) {
                BOOST_LOG_TRIVIAL(info) << "MultiInstanceCoordinator: Detected no master, attempting to acquire master lock...";
                
                if (tryAcquireMasterLock()) {
                    mIsMaster.store(true);
                    BOOST_LOG_TRIVIAL(info) << "MultiInstanceCoordinator: Successfully became master instance";
                    // Re-initialize network components now that we're master
                    // Notify callback about becoming master
                    MasterStatusCallback cb;
                    {
                        std::lock_guard<std::mutex> g(mCallbackMutex);
                        cb = mMasterStatusCallback;
                    }
                    if (cb) {
                        try {
                            cb(true);
                        } catch (...) {
                            BOOST_LOG_TRIVIAL(error) << "MultiInstanceCoordinator: Exception in master status callback (promotion)";
                        }
                    }
                } else {
                    // Failed to acquire lock: someone else likely became master immediately
                    BOOST_LOG_TRIVIAL(info) << "MultiInstanceCoordinator: Failed to become master (another instance acquired lock).";
                }
            }
        }
    }
}

void MultiInstanceCoordinator::registerMasterStatusCallback(MasterStatusCallback callback) {
    std::lock_guard<std::mutex> g(mCallbackMutex);
    mMasterStatusCallback = std::move(callback);
}

} // namespace Slic3r


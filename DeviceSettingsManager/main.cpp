#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <curl/curl.h>
#include <iostream>
#include <cstdint>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>
#include <sys/resource.h>

// RDK specific includes
#include "device_settings.h"  // Thunder client library for DeviceSettings
// #include "libIBus.h"           // Uncomment if using IARM Bus
// #include "libIBusDaemon.h"      // Uncomment if using IARM Bus daemon

using namespace std;

// RDK/Yocto specific constants
#define MAX_DEBUG_LOG_BUFF_SIZE 8192
#define DEVICE_SETTINGS_ERROR_NONE 0
#define DEVICE_SETTINGS_ERROR_GENERAL 1
#define DEVICE_SETTINGS_ERROR_UNAVAILABLE 2
#define DEVICE_SETTINGS_ERROR_NOT_EXIST 43

// Define logging macros
#define LOGI(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Global variables
static pthread_mutex_t gCurlInitMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gDSAppMutex = PTHREAD_MUTEX_INITIALIZER;
CURLSH* mCurlShared = nullptr;
char* gDebugPrintBuffer = nullptr;

// RDK specific globals
static bool gAppInitialized = false;
static bool gSignalHandlersSet = false;

// Function declarations (DeviceSettings functions are in device_settings.h)
void getDSClientInterface();
void test_getDSClientInterface();
int setup_signals();
int breakpad_ExceptionHandler();
void cleanup_resources();

// Menu system functions
void showMainMenu();
void handleFPDModule();
void handleAudioPortsModule();
void handleVideoPortsModule();
void handleHDMIInModule();
void handleCompositeInModule();
int getUserChoice();
void clearScreen();

// DeviceSettings functions - forward declarations for compilation
void DeviceSettings_Init();
bool DeviceSettings_IsOperational();
uint32_t DeviceSettings_Connect();
void DeviceSettings_Term();


int main(int argc, char **argv)
{
    // Initialize syslog for RDK environment
    openlog("DSApp", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    printf("\n=== DeviceSettings Interactive Test Application ===\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -h, --help    Show this help message\n");
            printf("  --version     Show version information\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("DSApp version 1.0.0 (RDK/Yocto build)\n");
            return 0;
        }
    }
    
    // Set up signal handlers early
    if (setup_signals() != 0) {
        LOGE("Failed to setup signal handlers");
        return -1;
    }

    // Allocate debug buffer with error checking
    gDebugPrintBuffer = new(std::nothrow) char[MAX_DEBUG_LOG_BUFF_SIZE];
    if (!gDebugPrintBuffer) {
        LOGE("Failed to allocate debug buffer of size %d", MAX_DEBUG_LOG_BUFF_SIZE);
        return -1;
    }
    memset(gDebugPrintBuffer, 0, MAX_DEBUG_LOG_BUFF_SIZE);
    
    // Initialize curl for network operations
    CURLcode curl_result = curl_global_init(CURL_GLOBAL_ALL);
    if (curl_result != CURLE_OK) {
        LOGE("Failed to initialize curl: %s", curl_easy_strerror(curl_result));
        cleanup_resources();
        return -1;
    }

    // Initialize breakpad for crash reporting
    if (breakpad_ExceptionHandler() != 0) {
        LOGE("Failed to initialize exception handler");
        // Continue anyway, not critical
    }

    // Initialize Device Settings
    /* printf("\nInitializing Device Settings...\n");
    DeviceSettings_Init();
    printf("Device Settings initialized successfully!\n");
    */

    getDSClientInterface();

    // Set application as initialized
    pthread_mutex_lock(&gDSAppMutex);
    gAppInitialized = true;
    pthread_mutex_unlock(&gDSAppMutex);

    // Main interactive menu loop
    int result = 0;
    try {
        showMainMenu();
        printf("\nApplication completed successfully\n");
    } catch (const std::exception& e) {
        LOGE("Application exception: %s", e.what());
        result = -1;
    } catch (...) {
        LOGE("Unknown application exception");
        result = -1;
    }

    // Cleanup before exit
    cleanup_resources();
    
    printf("\nDevice Settings Application exiting with code %d\n", result);
    closelog();
    return result;
}

void test_getDSClientInterface()
{
    // After your existing initialization
    DeviceSettings_Init();
    DeviceSettings_Connect(); // This may return ERROR_NOT_EXIST

/*    // If Operational(true) isn't called after a reasonable time:
    if (!DeviceSettings_IsOperational()) {
        printf("Operational callback not triggered, trying manual check...\n");
        if (DeviceSettings_ForceCheckOperational()) {
            printf("Successfully acquired interface manually!\n");
        }
    }
*/
    // Wait a bit for any async operations
    sleep(2);

    // Check detailed connection state
    DeviceSettings_DebugConnectionState();

    // Try the API call
    uint32_t brightness = 0;
    uint32_t result = DeviceSettings_GetFPDBrightness(FPD_INDICATOR_POWER, &brightness);
    printf("Final result: %u, brightness: %u\n", result, brightness);
}

void getDSClientInterface()
{
    LOGI("Initializing Device Settings Client Interface...");

    // Initialize the device settings subsystem
    DeviceSettings_Init();
    LOGI("Device Settings subsystem initialized");

    int retry_count = 0;
    const int max_retries = 30; // Reduced for production environment
    const int retry_delay_base = 100; // Base delay in milliseconds
    
    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    LOGI("Attempting to connect to Device Settings service...");

    while (!DeviceSettings_IsOperational() && retry_count < max_retries)
    {
        uint32_t status = DeviceSettings_Connect();

        if (DEVICE_SETTINGS_ERROR_NONE == status)
        {
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 + 
                             (current_time.tv_nsec - start_time.tv_nsec) / 1000000;
            LOGI("Successfully connected to Device Settings service (elapsed: %ld ms)", elapsed_ms);
            return;
        } 

        // Enhanced error reporting for RDK environment
        const char* error_desc = "Unknown error";
        switch (status) {
            case DEVICE_SETTINGS_ERROR_UNAVAILABLE:
                error_desc = "Thunder/WPE Framework is unavailable";
                break;
            case DEVICE_SETTINGS_ERROR_NOT_EXIST:
                error_desc = "DeviceSettingsManager plugin not loaded";
                break;
            default:
                error_desc = "Unknown Device Settings error";
                break;
        }
        
        LOGE("Connection attempt %d/%d failed: %s (error code: %u)", 
             retry_count + 1, max_retries, error_desc, status);
        
        retry_count++;
        
        // Exponential backoff with jitter for production stability
        int delay_ms = retry_delay_base * (1 << (retry_count / 5)); // Exponential component
        delay_ms += (rand() % 50); // Add jitter
        if (delay_ms > 2000) delay_ms = 2000; // Cap at 2 seconds
        
        LOGD("Waiting %d ms before next attempt...", delay_ms);
        usleep(delay_ms * 1000);
    }

    if (DeviceSettings_IsOperational())
    {
        LOGI("Device Settings service became operational");
        return;
    }

    // Final failure handling
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    long total_elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 + 
                           (current_time.tv_nsec - start_time.tv_nsec) / 1000000;
    
    LOGE("Failed to connect to Device Settings service after %d attempts over %ld ms", 
         max_retries, total_elapsed_ms);
    LOGE("Possible causes:");
    LOGE("  1. Thunder/WPE Framework not running");
    LOGE("  2. DeviceSettingsManager plugin not loaded");
    LOGE("  3. System resources exhausted");
    LOGE("  4. Network connectivity issues");
    
    throw std::runtime_error("Device Settings connection failed");
}

// Signal handler for graceful shutdown
static void signal_handler(int signum) {
    const char* signal_name = "UNKNOWN";
    switch (signum) {
        case SIGTERM: signal_name = "SIGTERM"; break;
        case SIGINT:  signal_name = "SIGINT"; break;
        case SIGHUP:  signal_name = "SIGHUP"; break;
        case SIGUSR1: signal_name = "SIGUSR1"; break;
        case SIGUSR2: signal_name = "SIGUSR2"; break;
    }
    
    syslog(LOG_INFO, "[DSApp] Received signal %s (%d), initiating graceful shutdown", signal_name, signum);
    
    // Mark application as not initialized to trigger cleanup
    pthread_mutex_lock(&gDSAppMutex);
    gAppInitialized = false;
    pthread_mutex_unlock(&gDSAppMutex);
    
    // Cleanup and exit
    cleanup_resources();
    closelog();
    exit(0);
}

int setup_signals() {
    LOGI("Setting up signal handlers for RDK environment...");
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    // Handle common termination signals
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGTERM handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGINT handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGHUP, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGHUP handler: %s", strerror(errno));
        return -1;
    }
    
    // Handle user-defined signals for debugging
    if (sigaction(SIGUSR1, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGUSR1 handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGUSR2, &sa, nullptr) == -1) {
        LOGE("Failed to set SIGUSR2 handler: %s", strerror(errno));
        return -1;
    }
    
    // Ignore SIGPIPE to handle broken pipe gracefully
    signal(SIGPIPE, SIG_IGN);
    
    gSignalHandlersSet = true;
    LOGI("Signal handlers configured successfully");
    return 0;
}

int breakpad_ExceptionHandler() {
    LOGI("Initializing crash reporting for RDK environment...");
    
    // Set core dump parameters for RDK debugging
    struct rlimit core_limit;
    core_limit.rlim_cur = RLIM_INFINITY;
    core_limit.rlim_max = RLIM_INFINITY;
    
    if (setrlimit(RLIMIT_CORE, &core_limit) != 0) {
        LOGE("Failed to set core dump limit: %s", strerror(errno));
    } else {
        LOGI("Core dump enabled for crash analysis");
    }
    
    // Set up crash dump directory (typical RDK location)
    const char* crash_dir = "/opt/logs/crashes";
    if (access(crash_dir, W_OK) == 0) {
        LOGI("Crash dumps will be written to: %s", crash_dir);
    } else {
        LOGD("Crash directory %s not accessible, using default location", crash_dir);
    }
    
    // TODO: Initialize actual breakpad/crashpad when available
    // For now, rely on system core dumps and signal handlers
    
    LOGI("Crash reporting initialization complete");
    return 0;
}

// DeviceSettings_Init() is now implemented in device_settings.cpp

// DeviceSettings_IsOperational() is now implemented in device_settings.cpp

// DeviceSettings_Connect() is now implemented in device_settings.cpp

void cleanup_resources() {
    LOGI("Cleaning up application resources...");
    
    // Mark application as shutting down
    pthread_mutex_lock(&gDSAppMutex);
    gAppInitialized = false;
    pthread_mutex_unlock(&gDSAppMutex);
    
    // Clean up curl resources
    if (mCurlShared) {
        curl_share_cleanup(mCurlShared);
        mCurlShared = nullptr;
        LOGD("CURL shared handle cleaned up");
    }
    
    curl_global_cleanup();
    LOGD("CURL global cleanup completed");
    
    // Free debug buffer
    if (gDebugPrintBuffer) {
        delete[] gDebugPrintBuffer;
        gDebugPrintBuffer = nullptr;
        LOGD("Debug buffer freed");
    }
    
    // Clean up RDK-specific resources
    LOGD("Terminating Device Settings connection...");
    DeviceSettings_Term();  // Call actual cleanup from device_settings.cpp
    
    // TODO: Add IARM Bus cleanup if used
    // IARM_Bus_Disconnect();
    // IARM_Bus_Term();
    
    // Destroy mutexes
    pthread_mutex_destroy(&gDSAppMutex);
    pthread_mutex_destroy(&gCurlInitMutex);
    LOGD("Mutexes destroyed");
    
    LOGI("Resource cleanup complete");
}

// Menu system implementation
void clearScreen() {
    printf("\033[2J\033[H"); // ANSI escape codes to clear screen
}

int getUserChoice() {
    int choice;
    printf("\nEnter your choice: ");
    if (scanf("%d", &choice) != 1) {
        // Clear input buffer on invalid input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return -1;
    }
    return choice;
}

void showMainMenu() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== DeviceSettings Module Test Menu ===\n");
        printf("1. FPD (Front Panel Display)\n");
        printf("2. Audio Ports\n");
        printf("3. Video Ports\n");
        printf("4. HDMI Input\n");
        printf("5. Composite Input\n");
        printf("0. Exit\n");
        printf("========================================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                handleFPDModule();
                break;
            case 2:
                handleAudioPortsModule();
                break;
            case 3:
                handleVideoPortsModule();
                break;
            case 4:
                handleHDMIInModule();
                break;
            case 5:
                handleCompositeInModule();
                break;
            case 0:
                printf("\nExiting...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); // consume newline
                getchar(); // wait for Enter
                break;
        }
    } while (choice != 0);
}

void handleFPDModule() {
    int choice;

    do {
        clearScreen();
        printf("\n=== FPD (Front Panel Display) Module ===\n");
        printf("1. DeviceSettings_GetFPDBrightness\n");
        printf("2. DeviceSettings_SetFPDBrightness\n");
        printf("3. DeviceSettings_GetFPDState\n");
        printf("4. DeviceSettings_SetFPDState\n");
        printf("5. DeviceSettings_GetFPDColor\n");
        printf("6. DeviceSettings_SetFPDColor\n");
        printf("7. DeviceSettings_GetFPDTextDisplay\n");
        printf("8. DeviceSettings_SetFPDTextDisplay\n");
        printf("0. Back to Main Menu\n");
        printf("=========================================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1: {
                printf("\nCalling DeviceSettings_GetFPDBrightness()...\n");
                uint32_t brightness;
                DeviceSettings_GetFPDBrightness(FPD_INDICATOR_POWER, &brightness);
                printf("Brightness: %u\n", brightness);
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 2: {
                int brightness;
                printf("\nEnter brightness level (0-100): ");
                if (scanf("%d", &brightness) == 1) {
                    printf("Calling DeviceSettings_SetFPDBrightness(%d)...\n", brightness);
                    DeviceSettings_SetFPDBrightness(FPD_INDICATOR_POWER, brightness);
                    printf("Function called successfully!\n");
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 3: {
                printf("\nCalling DeviceSettings_GetFPDState()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 4: {
                int state;
                printf("\nEnter FPD state (0=Off, 1=On): ");
                if (scanf("%d", &state) == 1) {
                    printf("Calling DeviceSettings_SetFPDState(%d)...\n", state);
                    // TODO: Add actual function call
                    printf("Function called successfully!\n");
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 5: {
                printf("\nCalling DeviceSettings_GetFPDColor()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 6: {
                int color;
                printf("\nEnter FPD color (0-7): ");
                if (scanf("%d", &color) == 1) {
                    printf("Calling DeviceSettings_SetFPDColor(%d)...\n", color);
                    // TODO: Add actual function call
                    printf("Function called successfully!\n");
                } else {
                    printf("Invalid input!\n");
                }
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 7: {
                printf("\nCalling DeviceSettings_GetFPDTextDisplay()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            }
            case 8: {
                char text[256];
                printf("\nEnter text to display: ");
                getchar(); // consume newline
                if (fgets(text, sizeof(text), stdin)) {
                    // Remove newline if present
                    text[strcspn(text, "\n")] = 0;
                    printf("Calling DeviceSettings_SetFPDTextDisplay(\"%s\")...\n", text);
                    // TODO: Add actual function call
                    printf("Function called successfully!\n");
                }
                printf("Press Enter to continue...");
                getchar();
                break;
            }
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleAudioPortsModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== Audio Ports Module ===\n");
        printf("1. DeviceSettings_GetAudioPort\n");
        printf("2. DeviceSettings_SetAudioPort\n");
        printf("3. DeviceSettings_GetAudioFormat\n");
        printf("4. DeviceSettings_SetAudioFormat\n");
        printf("5. DeviceSettings_GetAudioCompression\n");
        printf("6. DeviceSettings_SetAudioCompression\n");
        printf("7. DeviceSettings_GetDialogEnhancement\n");
        printf("8. DeviceSettings_SetDialogEnhancement\n");
        printf("9. DeviceSettings_GetDolbyVolumeMode\n");
        printf("10. DeviceSettings_SetDolbyVolumeMode\n");
        printf("0. Back to Main Menu\n");
        printf("===============================\n");

        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetAudioPort()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 2:
                printf("\nCalling DeviceSettings_SetAudioPort()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleVideoPortsModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== Video Ports Module ===\n");
        printf("1. DeviceSettings_GetVideoPort\n");
        printf("2. DeviceSettings_SetVideoPort\n");
        printf("3. DeviceSettings_GetVideoFormat\n");
        printf("4. DeviceSettings_SetVideoFormat\n");
        printf("5. DeviceSettings_GetVideoResolution\n");
        printf("6. DeviceSettings_SetVideoResolution\n");
        printf("7. DeviceSettings_GetVideoFrameRate\n");
        printf("8. DeviceSettings_SetVideoFrameRate\n");
        printf("0. Back to Main Menu\n");
        printf("===============================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetVideoPort()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleHDMIInModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== HDMI Input Module ===\n");
        printf("1. DeviceSettings_GetHDMIInput\n");
        printf("2. DeviceSettings_SetHDMIInput\n");
        printf("3. DeviceSettings_GetHDMIInputStatus\n");
        printf("4. DeviceSettings_GetHDMIInputSignalStatus\n");
        printf("5. DeviceSettings_GetHDMIVersion\n");
        printf("0. Back to Main Menu\n");
        printf("==========================\n");

        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetHDMIInput()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 2:
                printf("\nCalling DeviceSettings_SetHDMIInput()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 3:
                printf("\nCalling DeviceSettings_GetHDMIInputStatus()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 4:
                printf("\nCalling DeviceSettings_GetHDMIInputSignalStatus()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 5:
                printf("\nCalling DeviceSettings_GetHDMIVersion()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}

void handleCompositeInModule() {
    int choice;
    
    do {
        clearScreen();
        printf("\n=== Composite Input Module ===\n");
        printf("1. DeviceSettings_GetCompositeInput\n");
        printf("2. DeviceSettings_SetCompositeInput\n");
        printf("3. DeviceSettings_GetCompositeInputStatus\n");
        printf("0. Back to Main Menu\n");
        printf("===============================\n");
        
        choice = getUserChoice();
        
        switch (choice) {
            case 1:
                printf("\nCalling DeviceSettings_GetCompositeInput()...\n");
                // TODO: Add actual function call
                printf("Function called successfully!\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
            // Add more cases as needed
            case 0:
                printf("\nReturning to main menu...\n");
                break;
            default:
                printf("\nInvalid choice! Please try again.\n");
                printf("Press Enter to continue...");
                getchar(); getchar();
                break;
        }
    } while (choice != 0);
}
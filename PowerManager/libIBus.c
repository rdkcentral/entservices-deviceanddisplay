#include "libIBus.h"
#include <stdio.h>
#include <string.h>

IARM_Result_t IARM_Bus_Init(const char *name) {
    printf("IARM_Bus_Init called with name: %s\n", name);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Term(void) {
    printf("IARM_Bus_Term called\n");
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Connect(void) {
    printf("IARM_Bus_Connect called\n");
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Disconnect(void) {
    printf("IARM_Bus_Disconnect called\n");
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_GetContext(void **context) {
    printf("IARM_Bus_GetContext called\n");
    if (context) {
        *context = NULL; // Stub context
        return IARM_RESULT_SUCCESS;
    }
    return IARM_RESULT_INVALID_PARAM;
}

IARM_Result_t IARM_Bus_BroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *data, size_t len) {
    printf("IARM_Bus_BroadcastEvent called with ownerName: %s, eventId: %d, len: %zu\n", ownerName, eventId, len);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_IsConnected(const char *memberName, int *isRegistered) {
    printf("IARM_Bus_IsConnected called for memberName: %s\n", memberName);
    if (isRegistered) {
        *isRegistered = 1; // Stub: always connected
        return IARM_RESULT_SUCCESS;
    }
    return IARM_RESULT_INVALID_PARAM;
}

IARM_Result_t IARM_Bus_RegisterEventHandler(const char *ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
    printf("IARM_Bus_RegisterEventHandler called for ownerName: %s, eventId: %d\n", ownerName, eventId);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_UnRegisterEventHandler(const char *ownerName, IARM_EventId_t eventId) {
    printf("IARM_Bus_UnRegisterEventHandler called for ownerName: %s, eventId: %d\n", ownerName, eventId);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_RemoveEventHandler(const char *ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
    printf("IARM_Bus_RemoveEventHandler called for ownerName: %s, eventId: %d\n", ownerName, eventId);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_RegisterCall(const char *methodName, IARM_BusCall_t handler) {
    printf("IARM_Bus_RegisterCall called for methodName: %s\n", methodName);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Call(const char *ownerName, const char *methodName, void *arg, size_t argLen) {
    printf("IARM_Bus_Call called for ownerName: %s, methodName: %s, argLen: %zu\n", ownerName, methodName, argLen);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_Call_with_IPCTimeout(const char *ownerName, const char *methodName, void *arg, size_t argLen, int timeout) {
    printf("IARM_Bus_Call_with_IPCTimeout called for ownerName: %s, methodName: %s, timeout: %d\n", ownerName, methodName, timeout);
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t IARM_Bus_RegisterEvent(IARM_EventId_t maxEventId) {
    printf("IARM_Bus_RegisterEvent called with maxEventId: %d\n", maxEventId);
    return IARM_RESULT_SUCCESS;
}

void IARM_Bus_WritePIDFile(const char *path) {
    printf("IARM_Bus_WritePIDFile called with path: %s\n", path);
}


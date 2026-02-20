#pragma once

// ============================================================
//  EditorIDMapperAPI.h
//
//  Drop-in header for any OBSE plugin that needs EditorID
//  lookups at runtime.
//
//  USAGE
//  -----
//  1. Copy this header into your project.
//  2. In OBSEPlugin_Load:
//
//       g_msgIntfc->RegisterListener(g_pluginHandle,
//           "EditorIDMapper", EditorIDMapper::MessageHandler);
//
//  3. Wait for kMessage_Ready before calling Lookup/ReverseLookup.
//     The ready flag is set automatically in MessageHandler.
//
//  4. Resolve editor IDs:
//
//       UInt32 formID = EditorIDMapper::Lookup("WeapIronDagger");
//       const char* edid = EditorIDMapper::ReverseLookup(0x00019171);
//
//  REQUIREMENTS
//  ------------
//  EditorIDMapper.dll must be in Data\OBSE\Plugins\.
//  If it is not loaded, IsReady() returns false and all
//  lookups return 0 / nullptr safely.
// ============================================================

#include "obse/PluginAPI.h"
#include <cstring>

namespace EditorIDMapper
{
    // ---- Message types dispatched to/from EditorIDMapper.dll ----

    // Broadcast by EditorIDMapper when all forms have been captured.
    // data = nullptr.
    static const UInt32 kMessage_Ready = 'EMRD';

    // EditorID -> FormID query (synchronous).
    // Caller fills QueryData::editorID, dispatches kMessage_Query,
    // reads back QueryData::formID.
    static const UInt32 kMessage_Query = 'EMQR';

    // FormID -> EditorID reverse query (synchronous).
    // Caller fills QueryData::formID, dispatches kMessage_ReverseQuery,
    // reads back QueryData::editorID (points into EditorIDMapper's
    // internal storage  do not free or modify).
    static const UInt32 kMessage_ReverseQuery = 'EMRQ';

    static const UInt32 kMessage_RequestReady = 'EMRR';


    // ---- Interop struct used for both query types ----

    struct QueryData
    {
        const char* editorID;   // in  (Query) / out (ReverseQuery)
        UInt32      formID;     // out (Query) / in  (ReverseQuery)
    };

    // ---- Client state ----

    inline bool s_ready = false;
    inline OBSEMessagingInterface* s_msgIntfc = nullptr;
    inline PluginHandle s_pluginHandle = kPluginHandle_Invalid;

    // ---- MessageHandler ----
    // Register this with:
    //   g_msgIntfc->RegisterListener(pluginHandle, "EditorIDMapper", MessageHandler);

    inline void MessageHandler(OBSEMessagingInterface::Message* msg)
    {
        if (msg && msg->type == kMessage_Ready)
        {
            s_ready = true;
            _MESSAGE("EditorIDMapper: received ready signal");
        }
    }

    // ---- Init ----
    // Call from OBSEPlugin_Load (runtime path only).

    inline void Init(OBSEMessagingInterface* msgIntfc, PluginHandle pluginHandle)
    {
        s_msgIntfc = msgIntfc;
        s_pluginHandle = pluginHandle;

    }



    // ---- IsReady ----

    inline bool IsReady()
    {
        if (!s_msgIntfc)
        {
            _MESSAGE("EditorIDMapper: messaging interface missing");
        }

        if (!s_ready)
        {
            _MESSAGE("EditorIDMapper: Lookup before ready");
            return 0;
        }

        return s_ready && s_msgIntfc != nullptr;
    }

    // ---- Lookup: EditorID -> FormID ----
    // Returns 0 if not found or EditorIDMapper not ready.

    inline UInt32 Lookup(const char* editorID)
    {
        if (!IsReady() || !editorID) return 0;

        QueryData data = { editorID, 0 };
        bool sent = s_msgIntfc->Dispatch(s_pluginHandle, kMessage_Query,
            &data, sizeof(QueryData), nullptr);

        return data.formID;
    }

    // Convenience overload for std::string
    inline UInt32 Lookup(const std::string& editorID)
    {
        return Lookup(editorID.c_str());
    }

    // ---- ReverseLookup: FormID -> EditorID ----
    // Returns nullptr if not found or EditorIDMapper not ready.
    // The returned pointer is owned by EditorIDMapper do not free.

    inline const char* ReverseLookup(UInt32 formID)
    {
        if (!IsReady()) return nullptr;

        QueryData data = { nullptr, formID };
        s_msgIntfc->Dispatch(s_pluginHandle, kMessage_ReverseQuery,
            &data, sizeof(QueryData), nullptr);
        return data.editorID;
    }
}
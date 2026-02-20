#include "EditorIDMapper.h"
#include "EditorIDMapperAPI.h"

#include "obse/PluginAPI.h"
#include "obse_common/SafeWrite.h"
#include "obse/GameObjects.h"

IDebugLog        gLog("EditorIDMapper.log");
PluginHandle     g_pluginHandle = kPluginHandle_Invalid;
OBSEMessagingInterface* g_msgIntfc = nullptr;
OBSEScriptInterface* g_scriptIntfc = nullptr;
OBSEStringVarInterface* g_strVarIntfc = nullptr;

static bool Cmd_GetRuntimeEditorID_Execute(COMMAND_ARGS)
{
    TESForm* BaseObject = NULL;

    if (!g_scriptIntfc->ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &BaseObject) ||
        (thisObj == NULL && BaseObject == NULL))
    {
        g_strVarIntfc->Assign(PASS_COMMAND_ARGS, "");
        return true;
    }

    UInt32 FormID = 0;
    if (thisObj)
        FormID = thisObj->refID;
    else if (BaseObject)
        FormID = BaseObject->refID;

    const char* EditorID = EditorIDMapper::ReverseLookup(FormID);
    if (EditorID == NULL && BaseObject)
        EditorID = BaseObject->GetEditorID();

    if (EditorID == NULL && BaseObject)
        EditorID = BaseObject->GetEditorID2();

    if (EditorID == NULL && BaseObject)
        EditorID = BaseObject->GetEditorName();

    if (EditorID == NULL)
        g_strVarIntfc->Assign(PASS_COMMAND_ARGS, "");
    else
    {
        g_strVarIntfc->Assign(PASS_COMMAND_ARGS, EditorID);

        if (IsConsoleMode())
            Console_Print("EditorID: %s", EditorID);
    }

    return true;
}

static ParamInfo kParams_GetRuntimeEditorID[1] =
{
    {	"base object",	kParamType_InventoryObject,	1	},
};

CommandInfo kCommandInfo_GetRuntimeEditorID =
{
    "GetRuntimeEditorID",
    "",
    0,
    "Returns the editorID of form",
    0,
    1,
    kParams_GetRuntimeEditorID,

    Cmd_GetRuntimeEditorID_Execute
};

// ============================================================
//  Message handler
//  Handles synchronous Query and ReverseQuery from other plugins.
//  Also handles OBSE lifecycle messages.
// ============================================================

void MessageHandler(OBSEMessagingInterface::Message* msg)
{

    if (!msg)
        return;

    if (msg->type == EditorIDMapper::kMessage_Query &&
        msg->dataLen == sizeof(EditorIDMapper::QueryData))
    {
        auto* data = static_cast<EditorIDMapper::QueryData*>(msg->data);
        data->formID = g_editorIDMap.Lookup(data->editorID);
    }

    if (msg->type == EditorIDMapper::kMessage_ReverseQuery)
    {
        if (msg->dataLen != sizeof(EditorIDMapper::QueryData))
            return;

        auto* data =
            static_cast<EditorIDMapper::QueryData*>(msg->data);

        data->editorID =
            g_editorIDMap.ReverseLookup(data->formID);

        return;
    }

    if (msg->type == EditorIDMapper::kMessage_RequestReady)
    {
        if (EditorIDMapper::s_ready)
        {
            _MESSAGE("EditorIDMapper: replying to ready request");

            EditorIDMapper::s_msgIntfc->Dispatch(
                g_pluginHandle,
                EditorIDMapper::kMessage_Ready,
                nullptr,
                0,
                nullptr
            );
        }
    }
}

void MessageHandlerOBSE(OBSEMessagingInterface::Message* msg)
{

    if (!msg)
        return;

    if (msg->type == OBSEMessagingInterface::kMessage_GameInitialized)
    {
        _MESSAGE("EditorIDMapper: broadcasting ready (%u entries)",
            g_editorIDMap.Size());

        g_msgIntfc->Dispatch(g_pluginHandle,
            EditorIDMapper::kMessage_Ready,
            nullptr,
            0,
            nullptr);

        g_msgIntfc->RegisterListener(g_pluginHandle, nullptr, MessageHandler);
    }
}


// ============================================================
//  Plugin entry points
// ============================================================

extern "C"
{

    bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
    {
        _MESSAGE("EditorIDMapper Query");

        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = "EditorIDMapper";
        info->version = 1;

        if (obse->isEditor)
            return true;

        if (obse->obseVersion < OBSE_VERSION_INTEGER)
            return false;

        if (obse->oblivionVersion != OBLIVION_VERSION)
            return false;

        EditorIDMapper_Install();

        return true;
    }

    bool OBSEPlugin_Load(const OBSEInterface* obse)
    {
        _MESSAGE("EditorIDMapper Load");

        g_pluginHandle = obse->GetPluginHandle();

        obse->RegisterCommand(&kCommandInfo_GetRuntimeEditorID);

        g_msgIntfc = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
        if (!g_msgIntfc)
        {
            _ERROR("EditorIDMapper: messaging interface not found");
            return false;
        }
        g_msgIntfc->RegisterListener(g_pluginHandle, "OBSE", MessageHandlerOBSE);

        _MESSAGE("EditorIDMapper loaded successfully");
        return true;
    }

}
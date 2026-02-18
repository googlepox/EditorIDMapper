#pragma once

#include "obse/PluginAPI.h"
#include "obse/GameForms.h"
#include "EditorIDMapperAPI.h"

#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
//  EditorIDMap
//  Internal bidirectional map maintained by EditorIDMapper.dll
// ============================================================

class EditorIDMap
{
public:
    // Called from the VTBL hook when the game calls SetEditorID
    void Capture(const char* editorID, TESForm* form);

    // EditorID -> FormID (highest mod index wins)
    UInt32 Lookup(const char* editorID) const;

    // FormID -> EditorID
    const char* ReverseLookup(UInt32 formID) const;

    std::size_t Size() const { return m_formIDMap.size(); }

private:
    static std::string ToLower(const char* s);

    // lower-cased editorID -> list of forms (may have multiple for overrides)
    std::unordered_map<std::string, std::vector<TESForm*>> m_editorIDMap;

    // formID -> pointer into m_editorIDMap key (stable, never reallocated)
    std::unordered_map<UInt32, const std::string*>         m_formIDMap;
};

extern EditorIDMap g_editorIDMap;

// Install VTBL patches
void EditorIDMapper_Install();
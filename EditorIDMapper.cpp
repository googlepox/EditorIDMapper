#include "EditorIDMapper.h"

#include "obse/GameAPI.h"
#include "obse_common/SafeWrite.h"

#include <algorithm>
#include <cctype>

// ============================================================
//  Global map instance
// ============================================================

EditorIDMap g_editorIDMap;

// ============================================================
//  EditorIDMap implementation
// ============================================================

std::string EditorIDMap::ToLower(const char* s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return out;
}

void EditorIDMap::Capture(const char* editorID, TESForm* form)
{
    if (!editorID || !editorID[0] || !form) return;

    std::string lower = ToLower(editorID);

    auto& formList = m_editorIDMap[lower];
    formList.push_back(form);

    // Point the FormID map at the stable key inside m_editorIDMap
    const std::string* keyPtr = &m_editorIDMap.find(lower)->first;
    m_formIDMap[form->refID] = keyPtr;
}

UInt32 EditorIDMap::Lookup(const char* editorID) const
{
    if (!editorID || !editorID[0]) return 0;

    std::string lower = ToLower(editorID);

    auto it = m_editorIDMap.find(lower);
    if (it == m_editorIDMap.end() || it->second.empty())
        return 0;

    // Return the form with the highest mod index (last-loaded mod wins)
    TESForm* best = nullptr;
    UInt8    bestModIndex = 0;

    for (TESForm* form : it->second)
    {
        if (!form) continue;
        UInt8 modIndex = static_cast<UInt8>(form->refID >> 24);
        if (!best || modIndex > bestModIndex)
        {
            best = form;
            bestModIndex = modIndex;
        }
    }

    return best ? best->refID : 0;
}

const char* EditorIDMap::ReverseLookup(UInt32 formID) const
{
    auto it = m_formIDMap.find(formID);
    if (it == m_formIDMap.end()) return nullptr;
    return it->second->c_str();
}

// ============================================================
//  VTBL hook
// ============================================================

struct VTBLEntry
{
    UInt32      address;
    const char* className;
};

static const UInt32 kSetEditorID_VTBLOffset = 0xD8;
static const UInt32 kGetEditorID_VTBLOffset = 0xD4;

static const VTBLEntry kVTBLTable[] =
{
    { 0x00A52C6C, "TESClass"        },
    { 0x00A53524, "TESFaction"      },
    { 0x00A53654, "TESHair"         },
    { 0x00A53414, "TESEyes"         },
    { 0x00A548FC, "TESRace"         },
    { 0x00A54EFC, "TESSkill"        },
    { 0x00A32B14, "EffectSetting"   },
    { 0x00A49DA4, "Script"          },
    { 0x00A4656C, "TESLandTexture"  },
    { 0x00A33C84, "EnchantmentItem" },
    { 0x00A34C3C, "SpellItem"       },
    { 0x00A52524, "BirthSign"       },
    { 0x00A43CB4, "TESObjectACTI"   },
    { 0x00A43FB4, "TESObjectAPPA"   },
    { 0x00A441BC, "TESObjectARMO"   },
    { 0x00A44454, "TESObjectBOOK"   },
    { 0x00A44644, "TESObjectCLOT"   },
    { 0x00A44824, "TESObjectCONT"   },
    { 0x00A44A54, "TESObjectDOOR"   },
    { 0x00A33F5C, "IngredientItem"  },
    { 0x00A4319C, "TESObjectLIGH"   },
    { 0x00A44CA4, "TESObjectMISC"   },
    { 0x00A44DEC, "TESObjectSTAT"   },
    { 0x00A4280C, "TESGrass"        },
    { 0x00A44FB4, "TESObjectTREE"   },
    { 0x00A41B1C, "TESFlora"        },
    { 0x00A41D1C, "TESFurniture"    },
    { 0x00A45354, "TESObjectWEAP"   },
    { 0x00A4068C, "TESAmmo"         },
    { 0x00A53DD4, "TESNPC"          },
    { 0x00A5324C, "TESCreature"     },
    { 0x00A42B9C, "TESLevCreature"  },
    { 0x00A456F4, "TESSoulGem"      },
    { 0x00A42A4C, "TESKey"          },
    { 0x00A3217C, "AlchemyItem"     },
    { 0x00A4592C, "TESSubSpace"     },
    { 0x00A45534, "TESSigilStone"   },
    { 0x00A42DC4, "TESLevItem"      },
    { 0x00A47FEC, "TESWeather"      },
    { 0x00A45C9C, "TESClimate"      },
    { 0x00A3FEE4, "TESRegion"       },
    { 0x00A46C44, "TESObjectREFR"   },
    { 0x00A53774, "TESIdleForm"     },
    { 0x00A6743C, "TESPackage"      },
    { 0x00A407F4, "TESCombatStyle"  },
    { 0x00A4997C, "TESLoadScreen"   },
    { 0x00A42F1C, "TESLevSpell"     },
    { 0x00A43DFC, "TESObjectANIO"   },
    { 0x00A47F0C, "TESWaterForm"    },
    { 0x00A419EC, "TESEffectShader" },
    { 0x00A6FC9C, "Character"       },
    { 0x00A6E074, "Actor"           },
    { 0x00A710F4, "Creature"        },
};

static const UInt32 kVTBLTableSize = sizeof(kVTBLTable) / sizeof(kVTBLTable[0]);


static void __fastcall Hook_SetEditorID(TESForm* form, void* edx, const char* editorID)
{
    g_editorIDMap.Capture(editorID, form);
}

static void __fastcall Hook_SetEditorIDCELL(TESForm* form, void* edx, const char* editorID)
{
    g_editorIDMap.Capture(editorID, form);
    ThisStdCall(0x004C9FC0, form, editorID);
}

static void __fastcall Hook_SetEditorIDWRLD(TESForm* form, void* edx, const char* editorID)
{
    g_editorIDMap.Capture(editorID, form);
    ThisStdCall(0x004EF3C0, form, editorID);
}

static void __fastcall Hook_GetEditorID(TESForm* form, void* edx, const char* editorID)
{
    g_editorIDMap.Capture(editorID, form);
}

void EditorIDMapper_Install()
{
    for (UInt32 i = 0; i < kVTBLTableSize; ++i)
    {
        UInt32 addr = kVTBLTable[i].address + kSetEditorID_VTBLOffset;
        SafeWrite32(addr, (UInt32)Hook_SetEditorID);
        _MESSAGE("EditorIDMapper: patched SetEditorID for %s at 0x%08X", kVTBLTable[i].className, addr);
    }
    UInt32 addrWRLD = 0xA48374;
    SafeWrite32(addrWRLD, (UInt32)Hook_SetEditorIDWRLD);
    _MESSAGE("EditorIDMapper: patched TESWorldspace at 0x00A48374");

    UInt32 addrCELL = 0xA46AAC;
    SafeWrite32(addrCELL, (UInt32)Hook_SetEditorIDCELL);
    _MESSAGE("EditorIDMapper: patched TESObjectCELL at 0x00A46AAC");

    _MESSAGE("EditorIDMapper: %u VTBLs patched", kVTBLTableSize);
}
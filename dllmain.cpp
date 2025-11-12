#include "pch.h"

#include "base-dllmain.h" // Don't forget to add `version.lib` to Linker -> Input -> Additional Dependencies. Linker will give you unresolved symbols without it.
#include "AllocateMemory.h"
#include "AoBSwap.h"
#include "Common.h"
#include "Logger.h"
#include "PatchThreading.h"
#include "Util.h"

BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD ulReasonForCall, const LPVOID lpReserved) {
    SetupLog(GetLogPathAsCurrentDllDotLog());
    if (EndsWith(GetFullModulePath(), ".asi")) {
        EmptyDllProxy proxy;
        return BaseDllMain(hModule, ulReasonForCall, lpReserved, proxy);
    }
    WinHttpDllProxy proxy;
    return BaseDllMain(hModule, ulReasonForCall, lpReserved, proxy);
}

bool DisableHumanSeedDecayTimer(const std::string& moduleName, const PTR_SIZE moduleAddress, AllocateMemory* allocator, LogBuffer* logBuffer) {
    // Search for seconds as a float.
    return DoSimplePatch(moduleName, moduleAddress, "Disable Human Seed Decay Timer", "F3 0F 5C FE F3 0F 11 BB 18 03 00 00", FOUR_BYTE_NOP, logBuffer);
}

void DoInjection() {
    const auto moduleName   = "VoyageSteam-Win64-Shipping.exe";
    const auto moduleHandle = GetModuleHandle(moduleName);

    if (moduleHandle == nullptr) {
        LOG("Unable to find module, aborting.");
        return;
    }

    const auto moduleAddress = reinterpret_cast<const PTR_SIZE>(moduleHandle);
    LOG("Module base address: " << std::uppercase << std::hex << moduleAddress);

    const auto moduleInfo = GetModuleInfo(GetCurrentProcess(), moduleHandle);
    LOG("Module size: " << std::uppercase << std::hex << moduleInfo.SizeOfImage);

    AllocateMemory allocator;
    /*
    if (!allocator.AllocateGlobalAddresses(moduleName, moduleAddress)) {
        MessageBoxW(nullptr, L"Patching failed.", L"Patching Failed", MB_ICONERROR | MB_OK);
        return;
    }
    */

    const std::vector<PatchFunction> injectorFunctions{
        &DisableHumanSeedDecayTimer,
    };

    const auto result = DoPatchFunctionsAsync(moduleName, moduleAddress, &allocator, injectorFunctions);

    LOG("");

    if (result) {
        //MessageBoxW(nullptr, L"Patching done!", L"Patching Done", MB_ICONINFORMATION | MB_OK);
    } else {
        MessageBoxW(nullptr, L"Patching failed.", L"Patching Failed", MB_ICONERROR | MB_OK);
    }

    LOG("Patching done!");

    //HookGame(moduleName, moduleAddress, nullptr);
}
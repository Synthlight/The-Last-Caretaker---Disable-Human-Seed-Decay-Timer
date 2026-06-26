#include "pch.h"

#include "base-dllmain.h" // Don't forget to add `version.lib` to Linker -> Input -> Additional Dependencies. Linker will give you unresolved symbols without it.
#include "AllocateMemory.h"
#include "AoBSwap.h"
#include "Common.h"
#include "Logger.h"
#include "PatchThreading.h"
#include "Util.h"

// For v0.8.0.612115.

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
    /*
    VoyageSteam-Win64-Shipping.exe+513A8A6 - F3 0F10 8B 20030000   - movss xmm1,[rbx+00000320]
    VoyageSteam-Win64-Shipping.exe+513A8AE - 48 8B CB              - mov rcx,rbx
    VoyageSteam-Win64-Shipping.exe+513A8B1 - F3 0F5C CE            - subss xmm1,xmm6 // <------ NOP
    VoyageSteam-Win64-Shipping.exe+513A8B5 - F3 0F11 8B 20030000   - movss [rbx+00000320],xmm1
    */
    return DoSimplePatch(moduleName, moduleAddress, "Disable Human Seed Decay Timer", "F3 0F 5C ?? F3 0F 11 ?? 20 03 00 00", FOUR_BYTE_NOP, logBuffer);
}

void DoInjection() {
    LOG("Initializing v1.3.0.");

    auto moduleName   = "VoyageSteam-Win64-Shipping.exe";
    auto moduleHandle = GetModuleHandle(moduleName);

    if (moduleHandle == nullptr) {
        LOG("Unable to find `VoyageSteam-Win64-Shipping.exe` module, trying again for `VoyageEpic-Win64-Shipping.exe`.");
        moduleName   = "VoyageEpic-Win64-Shipping.exe";
        moduleHandle = GetModuleHandle(moduleName);
    }

    if (moduleHandle == nullptr) {
        LOG("Unable to find module, aborting.");
        MessageBoxW(nullptr, L"Disable-Human-Seed-Decay-Timer Patching failed.", L"Patching Failed", MB_ICONERROR | MB_OK);
        return;
    }

    const auto moduleAddress = reinterpret_cast<const PTR_SIZE>(moduleHandle);
    LOG("Module base address: " << std::uppercase << std::hex << moduleAddress);

    const auto moduleInfo = GetModuleInfo(GetCurrentProcess(), moduleHandle);
    LOG("Module size: " << std::uppercase << std::hex << moduleInfo.SizeOfImage);

    AllocateMemory allocator;
    /*
    if (!allocator.AllocateGlobalAddresses(moduleName, moduleAddress)) {
        MessageBoxW(nullptr, L"Disable-Human-Seed-Decay-Timer Patching failed.", L"Patching Failed", MB_ICONERROR | MB_OK);
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
        MessageBoxW(nullptr, L"Disable-Human-Seed-Decay-Timer Patching failed.", L"Patching Failed", MB_ICONERROR | MB_OK);
    }

    LOG("Patching done!");

    //HookGame(moduleName, moduleAddress, nullptr);
}
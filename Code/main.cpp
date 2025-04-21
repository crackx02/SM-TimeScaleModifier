#include "GraphicsOptionsMenu.hpp"
#include "Utils/Console.hpp"

#include <MinHook.h>

static bool ms_mhInitialized = false;
static bool ms_mhHooksAttached = false;

#define DEFINE_HOOK(address, detour, original) \
	MH_CreateHook((LPVOID)(v_mod_base + address), (LPVOID)detour, (LPVOID*)&original)

#define EASY_HOOK(address, func_name) \
	MH_CreateHook((LPVOID)(v_mod_base + address), (LPVOID)h_ ## func_name, (LPVOID*)&o_ ## func_name)

#define EASY_CLASS_HOOK(address, class_name, func_name) \
	MH_CreateHook((LPVOID)(v_mod_base + address), (LPVOID)class_name::h_##func_name, (LPVOID*)&class_name::o_##func_name)

constexpr uintptr_t Offset_ContraptionUpdate = 0x02DA770;
constexpr uintptr_t Offset_ContraptionRender = 0x02D9180;
constexpr uintptr_t Offset_GameInstanceUpdate = 0x0343030;
constexpr uintptr_t Offset_PlayStateUpdate = 0x042E600;
constexpr uintptr_t Offset_GraphicsOptionsMenu_Setup = 0x03031C0;

static void (*o_ContraptionUpdate)(void*, float) = nullptr;
static void (*o_ContraptionRender)(void*, float) = nullptr;
static void (*o_GameInstanceUpdate)(void*, float) = nullptr;
static void (*o_PlayStateUpdate)(void*, float) = nullptr;

static void h_ContraptionUpdate(void* self, float delta_time)
{
	o_ContraptionUpdate(self, delta_time * GraphicsOptionsMenu::TimeScale);
}

static void h_ContraptionRender(void* self, float delta_time)
{
	o_ContraptionRender(self, delta_time * GraphicsOptionsMenu::TimeScale);
}

static void h_GameInstanceUpdate(void* self, float delta_time)
{
	o_GameInstanceUpdate(self, delta_time * GraphicsOptionsMenu::TimeScale);
}

static void h_PlayStateUpdate(void* self, float delta_time)
{
	o_PlayStateUpdate(self, delta_time * GraphicsOptionsMenu::TimeScale);
}

static void process_attach(HMODULE hMod)
{
	AttachDebugConsole();

	if (MH_Initialize() != MH_OK)
	{
		DebugErrorL("Couldn't initialize minHook");
		return;
	}

	ms_mhInitialized = true;

	const std::uintptr_t v_mod_base = std::uintptr_t(GetModuleHandle(NULL));
	if (DEFINE_HOOK(Offset_ContraptionUpdate, h_ContraptionUpdate, o_ContraptionUpdate) != MH_OK) return;
	if (DEFINE_HOOK(Offset_ContraptionRender, h_ContraptionRender, o_ContraptionRender) != MH_OK) return;
	if (DEFINE_HOOK(Offset_GameInstanceUpdate, h_GameInstanceUpdate, o_GameInstanceUpdate) != MH_OK) return;
	if (DEFINE_HOOK(Offset_PlayStateUpdate, h_PlayStateUpdate, o_PlayStateUpdate) != MH_OK) return;

	// Gui hook
	if (DEFINE_HOOK(Offset_GraphicsOptionsMenu_Setup, GraphicsOptionsMenu::h_CreateWidgets, GraphicsOptionsMenu::o_CreateWidgets)) return;
	
	ms_mhHooksAttached = MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

static void process_detach(HMODULE hmod)
{
	if (ms_mhInitialized)
	{
		if (ms_mhHooksAttached)
			MH_DisableHook(MH_ALL_HOOKS);

		MH_Uninitialize();
	}

	FreeLibrary(hmod);
}

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		process_attach(hModule);
		break;
	case DLL_PROCESS_DETACH:
		process_detach(hModule);
		break;
	}

	return TRUE;
}
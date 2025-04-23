
#include <format>

#include "GraphicsOptionsMenu.hpp"
#include "Utils/Console.hpp"

#include "intrin.h"
#include <MinHook.h>
#include "lua.hpp"

#define DEFINE_HOOK(address, detour, original) \
	MH_CreateHook((LPVOID)(v_mod_base + address), (LPVOID)detour, (LPVOID*)&original)

constexpr uintptr_t Offset_ContraptionUpdate = 0x02DA770;
constexpr uintptr_t Offset_ContraptionRender = 0x02D9180;
constexpr uintptr_t Offset_GameInstanceUpdate = 0x0343030;
constexpr uintptr_t Offset_PlayStateUpdate = 0x042E600;
constexpr uintptr_t Offset_GraphicsOptionsMenu_Setup = 0x03031C0;
constexpr uintptr_t Offset_LuaVM_SetupEnv = 0x054A7F0;
constexpr uintptr_t Offset_LuaVM_Destruct = 0x054A4A0;
constexpr uintptr_t Offset_SMConsole = 0x1267600;

struct LuaVM {
	lua_State* L;
};

struct SMConsole {
	virtual ~SMConsole() {};
	virtual void logString(const std::string* pMessage, uint16_t color, int32_t type) {};
};

enum class LuaEnvType : uint32_t {
	Game,
	Terrain
};


static bool ms_mhInitialized = false;
static bool ms_mhHooksAttached = false;
static SMConsole** g_ppSMConsole = nullptr;
std::atomic<LuaVM*> g_thisLuaVM = nullptr;

// Utils

static void LogWarning(const std::string& message) {
	if ( g_ppSMConsole && *g_ppSMConsole )
		// 14 = yellow, 1 << 30 = warning
		(*g_ppSMConsole)->logString(&message, 14, 1 << 30);
}

static void lua_CheckArgCount(lua_State* L, int expected) {
	int top = lua_gettop(L);
	if ( top != expected )
		luaL_error(L, "Expected %i arguments (got %i)", expected, top);
}

// C API

extern "C" __declspec(dllexport) float TS_GetMin() {
	return GraphicsOptionsMenu::TimeScaleMin;
}

extern "C" __declspec(dllexport) float TS_GetMax() {
	return GraphicsOptionsMenu::TimeScaleMax;
}

extern "C" __declspec(dllexport) float TS_Get() {
	return GraphicsOptionsMenu::TimeScale;
}

extern "C" __declspec(dllexport) void TS_Set(float scale) {
	if ( scale < TS_GetMin() || scale > TS_GetMax() ) {
		LogWarning(
			std::format(
				"TimeScale: Value being set is outside of known good range, unexpected behavior may occur! Value: {}, min: {}, max: {}",
				scale, TS_GetMin(), TS_GetMax()
			)
		);
	}
	GraphicsOptionsMenu::TimeScale = scale;
}

// Lua API

static int Lua_TS_GetMin(lua_State* L) {
	lua_CheckArgCount(L, 0);
	lua_pushnumber(L, TS_GetMin());
	return 1;
}

static int Lua_TS_GetMax(lua_State* L) {
	lua_CheckArgCount(L, 0);
	lua_pushnumber(L, TS_GetMax());
	return 1;
}

static int Lua_TS_Get(lua_State* L) {
	lua_CheckArgCount(L, 0);
	lua_pushnumber(L, TS_Get());
	return 1;
}

static int Lua_TS_Set(lua_State* L) {
	lua_CheckArgCount(L, 1);
	TS_Set(float(luaL_checknumber(L, 1)));
	return 0;
}

// Hooks

static void (*o_LuaVM_SetupEnv)(LuaVM*, void*, LuaEnvType) = nullptr;
static void h_LuaVM_SetupEnv(LuaVM* self, void* ptr, LuaEnvType envType) {
	o_LuaVM_SetupEnv(self, ptr, envType);
	if ( envType == LuaEnvType::Game ) {
		g_thisLuaVM = self;

		lua_State* L = self->L;
		int top = lua_gettop(L);
		// Get the env table
		lua_getglobal(L, "unsafe_env");
		if ( !lua_istable(L, -1) ) {
			DebugErrorL("TimeScale: unsafe_env wasn't a table");
			return lua_settop(L, top);
		}

		// Get the sm table
		lua_pushstring(L, "sm");
		lua_rawget(L, -2);
		if ( !lua_istable(L, -1) ) {
			DebugErrorL("TimeScale: unsafe_env.sm wasn't a table");
			return lua_settop(L, top);
		}

		// Create timeScale table
		lua_pushstring(L, "timeScale");
		lua_createtable(L, 0, 2);

		// Set timeScale.getMin
		lua_pushstring(L, "getMin");
		lua_pushcfunction(L, Lua_TS_GetMin);
		lua_rawset(L, -3);

		// Set timeScale.getMax
		lua_pushstring(L, "getMax");
		lua_pushcfunction(L, Lua_TS_GetMax);
		lua_rawset(L, -3);

		// Set timeScale.get
		lua_pushstring(L, "get");
		lua_pushcfunction(L, Lua_TS_Get);
		lua_rawset(L, -3);

		// Set timeScale.set
		lua_pushstring(L, "set");
		lua_pushcfunction(L, Lua_TS_Set);
		lua_rawset(L, -3);

		// Set the table to sm.timeScale
		lua_rawset(L, -3);

		// Reset the stack - pops unsafe_env and sm tables
		lua_settop(L, top);

		DebugOutL("TimeScale: Injected sm.timeScale Lua functions");

		// Also use this to reset the value upon world load
		GraphicsOptionsMenu::TimeScale = 1.0f;
	}
}

static void (*o_LuaVM_Destruct)(LuaVM*) = nullptr;
static void h_LuaVM_Destruct(LuaVM* self) {
	if ( g_thisLuaVM == self ) {
		g_thisLuaVM = nullptr;
		GraphicsOptionsMenu::TimeScale = 1.0f;
	}
	o_LuaVM_Destruct(self);
}

static void (*o_ContraptionUpdate)(void*, float) = nullptr;
static void h_ContraptionUpdate(void* self, float delta_time)
{
	o_ContraptionUpdate(self, delta_time * GraphicsOptionsMenu::TimeScale);
}


static void (*o_ContraptionRender)(void*, float) = nullptr;
static void h_ContraptionRender(void* self, float delta_time)
{
	o_ContraptionRender(self, delta_time * GraphicsOptionsMenu::TimeScale);
}


static void (*o_GameInstanceUpdate)(void*, float) = nullptr;
static void h_GameInstanceUpdate(void* self, float delta_time)
{
	o_GameInstanceUpdate(self, delta_time * GraphicsOptionsMenu::TimeScale);
}


static void (*o_PlayStateUpdate)(void*, float) = nullptr;
static void h_PlayStateUpdate(void* self, float delta_time)
{
	o_PlayStateUpdate(self, delta_time * GraphicsOptionsMenu::TimeScale);
}

// Init

static void process_attach(HMODULE hMod)
{
	AttachDebugConsole();

	if (MH_Initialize() != MH_OK)
	{
		DebugErrorL("TimeScale: Couldn't initialize minHook");
		return;
	}

	ms_mhInitialized = true;

	const uintptr_t v_mod_base = std::uintptr_t(GetModuleHandle(NULL));
	if (DEFINE_HOOK(Offset_ContraptionUpdate, h_ContraptionUpdate, o_ContraptionUpdate) != MH_OK) return;
	if (DEFINE_HOOK(Offset_ContraptionRender, h_ContraptionRender, o_ContraptionRender) != MH_OK) return;
	if (DEFINE_HOOK(Offset_GameInstanceUpdate, h_GameInstanceUpdate, o_GameInstanceUpdate) != MH_OK) return;
	if (DEFINE_HOOK(Offset_PlayStateUpdate, h_PlayStateUpdate, o_PlayStateUpdate) != MH_OK) return;
	if (DEFINE_HOOK(Offset_LuaVM_SetupEnv, h_LuaVM_SetupEnv, o_LuaVM_SetupEnv) != MH_OK) return;
	if (DEFINE_HOOK(Offset_LuaVM_Destruct, h_LuaVM_Destruct, o_LuaVM_Destruct) != MH_OK) return;
	g_ppSMConsole = (SMConsole**)(v_mod_base + Offset_SMConsole);

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
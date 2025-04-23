# SM-TimeScaleModifier

### Supported version: 0.7.3.776

This is an updated version of the original archived mod repository from QM.

TimeScaleModifier is a mod for Scrap Mechanic that adds a slider in the settings which allows for increasing or decreasing the in-game time speed.

It also implements a C API and Lua API to allow other DLLs and Lua scripts to interact with the time scale value.

To use this mod, inject it into the game using any DLL injector of your choice.

To build the mod, use the provided Visual Studio solution.

## API Documentation

Below you can find documentation for the C and Lua APIs which this mod provides.

### Lua API
The mod injects a Lua API into the game, which can be used from Lua scripts to read or manipulate the time scale.  
This API is added to the global `sm` table, as `sm.timeScale`.  
To check if this mod is loaded and available, simply check for the existence of this table: `if sm.timeScale ~= nil then`

The Lua API functions are documented below.

`sm.timeScale.getMin() -> number`  
Returns the minimum value of the time scale slider in the settings GUI.  
At the time of writing, this is always `0.0`.

`sm.timeScale.getMax() -> number`  
Returns the maximum value of the time scale slider in the settings GUI.  
At the time of writing, this is always `20.0`.

`sm.timeScale.get() -> number`  
Returns the current time scale multiplier value.  
Note: While the 'getMin' and 'getMax' functions may suggest a limit, no actual limit is enforced in the API - the returned value may be outside of the min/max range!

`sm.timeScale.set([number] scale)`  
Sets the current time scale multiplier value.  
Note: While the 'getMin' and 'getMax' functions may suggest a limit, no actual limit is enforced in the API.  
However, setting a value outside of the min/max range will log a warning message in the developer console.



### C API
The mod provides a C API for use by other DLLs.

The API can be accessed by getting its function pointers, using methods such as `GetProcAddress` and similar functions.

The C API functions are documented below.

`float TS_GetMin()`  
Returns the minimum value of the time scale slider in the settings GUI.  
At the time of writing, this is always `0.0f`.

`float TS_GetMax()`  
Returns the maximum value of the time scale slider in the settings GUI.  
At the time of writing, this is always `20.0f`.

`float TS_Get()`  
Returns the current time scale multiplier value.  
Note: While the 'getMin' and 'getMax' functions may suggest a limit, no actual limit is enforced in the API - the returned value may be outside of the min/max range!

`void TS_Set(float scale)`  
Sets the current time scale multiplier value.  
Note: While the 'getMin' and 'getMax' functions may suggest a limit, no actual limit is enforced in the API.  
However, setting a value outside of the min/max range will log a warning message in the developer console.

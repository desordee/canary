/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#include "otpch.h"

#include "lua/functions/core/libs/result_functions.hpp"

int ResultFunctions::luaResultGet8(lua_State* L) {
	// result.get8(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->get8(resultString));
	return 1;
}

int ResultFunctions::luaResultGet16(lua_State* L) {
	// result.get16(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->get16(resultString));
	return 1;
}

int ResultFunctions::luaResultGet32(lua_State* L) {
	// result.get32(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->get32(resultString));
	return 1;
}

int ResultFunctions::luaResultGet64(lua_State* L) {
	// result.get64(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, static_cast<lua_Number>(result->get64(resultString)));
	return 1;
}

int ResultFunctions::luaResultGetU8(lua_State* L) {
	// result.getU8(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->getU8(resultString));
	return 1;
}

int ResultFunctions::luaResultGetU16(lua_State* L) {
	// result.getU16(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->getU16(resultString));
	return 1;
}

int ResultFunctions::luaResultGetU32(lua_State* L) {
	// result.getU32(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->getU32(resultString));
	return 1;
}

int ResultFunctions::luaResultGetU64(lua_State* L) {
	// result.getU64(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, static_cast<lua_Number>(result->getU64(resultString)));
	return 1;
}

int ResultFunctions::luaResultGetTime(lua_State* L) {
	// result.getTime(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, static_cast<lua_Number>(result->getTime(resultString)));
	return 1;
}

int ResultFunctions::luaResultGetBoolean(lua_State* L) {
	// result.getBoolean(result, tableName)
	DBResult_ptr result = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!result) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& resultString = getString(L, 2);
	lua_pushnumber(L, result->getBoolean(resultString));
	return 1;
}

int ResultFunctions::luaResultGetString(lua_State* L) {
	// result.getString(result, tableName)
	DBResult_ptr res = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!res) {
		pushBoolean(L, false);
		return 1;
	}

	const std::string& s = getString(L, 2);
	pushString(L, res->getString(s));
	return 1;
}

int ResultFunctions::luaResultGetStream(lua_State* L) {
	// result.getStream(result, tableName)
	DBResult_ptr res = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, 1));
	if (!res) {
		pushBoolean(L, false);
		return 1;
	}

	unsigned long length;
	const char* stream = res->getStream(getString(L, 2), length);
	lua_pushlstring(L, stream, length);
	lua_pushnumber(L, length);
	return 2;
}

int ResultFunctions::luaResultNext(lua_State* L) {
	// result.next()
	DBResult_ptr res = ScriptEnvironment::getResultByID(getNumber<uint32_t>(L, -1));
	if (!res) {
		pushBoolean(L, false);
		return 1;
	}

	pushBoolean(L, res->next());
	return 1;
}

int ResultFunctions::luaResultFree(lua_State* L) {
	// result.free()
	pushBoolean(L, ScriptEnvironment::removeResult(getNumber<uint32_t>(L, -1)));
	return 1;
}

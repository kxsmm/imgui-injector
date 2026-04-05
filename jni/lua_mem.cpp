// Mem.cpp
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

static pid_t GetPIDViaSyscall() {
	return syscall(SYS_getpid);
}

static int MemRead(lua_State* L, size_t size) {
	pid_t pid = (pid_t)luaL_checkinteger(L, 1);
	uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 2);
	
	struct iovec local[1], remote[1];
	uint8_t buf[size];
	local[0].iov_base = buf;
	local[0].iov_len = size;
	remote[0].iov_base = (void*)addr;
	remote[0].iov_len = size;
	
	ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
	if (nread != (ssize_t)size) {
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		return 2;
	}
	
	if (size == 1) {
		lua_pushinteger(L, buf[0]);
	} else if (size == 2) {
		uint16_t val = *(uint16_t*)buf;
		lua_pushinteger(L, val);
	} else if (size == 4) {
		uint32_t val = *(uint32_t*)buf;
		lua_pushinteger(L, val);
	} else if (size == 8) {
		uint64_t val = *(uint64_t*)buf;
		lua_pushinteger(L, (lua_Integer)val);
	} else {
		lua_pushlstring(L, (char*)buf, size);
	}
	return 1;
}

// 通用内存写入辅助
static int MemWrite(lua_State* L, size_t size) {
	pid_t pid = (pid_t)luaL_checkinteger(L, 1);
	uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 2);
	
	uint8_t buf[8];
	if (size <= 8) {
		lua_Integer val = luaL_checkinteger(L, 3);
		for (size_t i = 0; i < size; i++)
			buf[i] = (val >> (i * 8)) & 0xFF;
	} else {
		size_t len;
		const char* str = luaL_checklstring(L, 3, &len);
		if (len > size) len = size;
		memcpy(buf, str, len);
		if (len < size) memset(buf + len, 0, size - len);
	}
	
	struct iovec local[1], remote[1];
	local[0].iov_base = buf;
	local[0].iov_len = size;
	remote[0].iov_base = (void*)addr;
	remote[0].iov_len = size;
	
	ssize_t nwrite = process_vm_writev(pid, local, 1, remote, 1, 0);
	if (nwrite != (ssize_t)size) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, strerror(errno));
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

static int lua_Mem_GetPID(lua_State* L) {
	lua_pushinteger(L, GetPIDViaSyscall());
	return 1;
}

static int lua_Mem_ReadByte(lua_State* L) { return MemRead(L, 1); }
static int lua_Mem_ReadShort(lua_State* L) { return MemRead(L, 2); }
static int lua_Mem_ReadInt(lua_State* L) { return MemRead(L, 4); }
static int lua_Mem_ReadLong(lua_State* L) { return MemRead(L, 8); }
static int lua_Mem_ReadBytes(lua_State* L) {
	pid_t pid = (pid_t)luaL_checkinteger(L, 1);
	uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 2);
	size_t len = (size_t)luaL_checkinteger(L, 3);
	if (len == 0) {
		lua_pushstring(L, "");
		return 1;
	}
	char* buf = (char*)malloc(len);
	struct iovec local = { buf, len }, remote = { (void*)addr, len };
	ssize_t nread = process_vm_readv(pid, &local, 1, &remote, 1, 0);
	if (nread != (ssize_t)len) {
		free(buf);
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		return 2;
	}
	lua_pushlstring(L, buf, len);
	free(buf);
	return 1;
}

static int lua_Mem_WriteByte(lua_State* L) { return MemWrite(L, 1); }
static int lua_Mem_WriteShort(lua_State* L) { return MemWrite(L, 2); }
static int lua_Mem_WriteInt(lua_State* L) { return MemWrite(L, 4); }
static int lua_Mem_WriteLong(lua_State* L) { return MemWrite(L, 8); }
static int lua_Mem_WriteBytes(lua_State* L) {
	pid_t pid = (pid_t)luaL_checkinteger(L, 1);
	uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 2);
	size_t len;
	const char* data = luaL_checklstring(L, 3, &len);
	if (len == 0) {
		lua_pushboolean(L, 1);
		return 1;
	}
	struct iovec local = { (void*)data, len }, remote = { (void*)addr, len };
	ssize_t nwrite = process_vm_writev(pid, &local, 1, &remote, 1, 0);
	if (nwrite != (ssize_t)len) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, strerror(errno));
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

static const luaL_Reg MemLib[] = {
	{"GetPID", lua_Mem_GetPID},
	{"ReadByte", lua_Mem_ReadByte},
	{"ReadShort", lua_Mem_ReadShort},
	{"ReadInt", lua_Mem_ReadInt},
	{"ReadLong", lua_Mem_ReadLong},
	{"ReadBytes", lua_Mem_ReadBytes},
	{"WriteByte", lua_Mem_WriteByte},
	{"WriteShort", lua_Mem_WriteShort},
	{"WriteInt", lua_Mem_WriteInt},
	{"WriteLong", lua_Mem_WriteLong},
	{"WriteBytes", lua_Mem_WriteBytes},
	{NULL, NULL}
};

extern "C" int luaopen_Mem(lua_State* L) {
	luaL_newlib(L, MemLib);
	return 1;
}
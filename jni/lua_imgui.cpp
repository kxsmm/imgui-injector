extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "imgui.h"
#include <string>
#include <cstring>
#include <vector>

// ----------------------------------------------------------------------
// 辅助函数：Lua <-> ImVec2 / ImVec4 转换
// ----------------------------------------------------------------------
static ImVec2 to_ImVec2(lua_State* L, int idx) {
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "x");
	lua_getfield(L, idx, "y");
	float x = (float)luaL_optnumber(L, -2, 0);
	float y = (float)luaL_optnumber(L, -1, 0);
	lua_pop(L, 2);
	return ImVec2(x, y);
}

static void push_ImVec2(lua_State* L, const ImVec2& v) {
	lua_newtable(L);
	lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
	lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
}

static ImVec4 to_ImVec4(lua_State* L, int idx) {
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "r");
	lua_getfield(L, idx, "g");
	lua_getfield(L, idx, "b");
	lua_getfield(L, idx, "a");
	float r = (float)luaL_optnumber(L, -4, 0);
	float g = (float)luaL_optnumber(L, -3, 0);
	float b = (float)luaL_optnumber(L, -2, 0);
	float a = (float)luaL_optnumber(L, -1, 1);
	lua_pop(L, 4);
	return ImVec4(r, g, b, a);
}

static void push_ImVec4(lua_State* L, const ImVec4& v) {
	lua_newtable(L);
	lua_pushnumber(L, v.x); lua_setfield(L, -2, "r");
	lua_pushnumber(L, v.y); lua_setfield(L, -2, "g");
	lua_pushnumber(L, v.z); lua_setfield(L, -2, "b");
	lua_pushnumber(L, v.w); lua_setfield(L, -2, "a");
}

// 将 Lua 表 {r, g, b, a} (0-255 或 0-1) 转换为 ImU32 (ABGR 格式)
static ImU32 to_ImU32(lua_State* L, int idx) {
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "r"); float r = (float)luaL_optnumber(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, idx, "g"); float g = (float)luaL_optnumber(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, idx, "b"); float b = (float)luaL_optnumber(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, idx, "a"); float a = (float)luaL_optnumber(L, -1, 255); lua_pop(L, 1);
	// 如果值大于1，认为是0-255范围，转换为0-1
	if (r > 1 || g > 1 || b > 1 || a > 1) {
		r /= 255.0f; g /= 255.0f; b /= 255.0f; a /= 255.0f;
	}
	return ImGui::GetColorU32(ImVec4(r, g, b, a));
}

// ----------------------------------------------------------------------
// 基础窗口函数
// ----------------------------------------------------------------------
static int lua_ImGui_Begin(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	bool* p_open = nullptr;
	if (lua_isboolean(L, 2)) {
		static bool open_flag = lua_toboolean(L, 2);
		p_open = &open_flag;
	}
	int flags = (int)luaL_optinteger(L, 3, 0);
	bool ret = ImGui::Begin(name, p_open, flags);
	lua_pushboolean(L, ret);
	return 1;
}

static int lua_ImGui_End(lua_State* L) {
	ImGui::End();
	return 0;
}

static int lua_ImGui_BeginChild(lua_State* L) {
	const char* str_id = luaL_checkstring(L, 1);
	ImVec2 size = ImVec2(0, 0);
	if (lua_istable(L, 2)) size = to_ImVec2(L, 2);
	bool border = lua_toboolean(L, 3);
	int flags = (int)luaL_optinteger(L, 4, 0);
	bool ret = ImGui::BeginChild(str_id, size, border, flags);
	lua_pushboolean(L, ret);
	return 1;
}

static int lua_ImGui_EndChild(lua_State* L) {
	ImGui::EndChild();
	return 0;
}

// ----------------------------------------------------------------------
// 文本和简单控件
// ----------------------------------------------------------------------
static int lua_ImGui_Text(lua_State* L) {
	const char* text = luaL_checkstring(L, 1);
	ImGui::Text("%s", text);
	return 0;
}

static int lua_ImGui_TextColored(lua_State* L) {
	ImVec4 col = to_ImVec4(L, 1);
	const char* text = luaL_checkstring(L, 2);
	ImGui::TextColored(col, "%s", text);
	return 0;
}

static int lua_ImGui_Button(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	ImVec2 size = ImVec2(0, 0);
	if (lua_istable(L, 2)) size = to_ImVec2(L, 2);
	bool ret = ImGui::Button(label, size);
	lua_pushboolean(L, ret);
	return 1;
}

static int lua_ImGui_SmallButton(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	bool ret = ImGui::SmallButton(label);
	lua_pushboolean(L, ret);
	return 1;
}

static int lua_ImGui_Checkbox(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	bool val = lua_toboolean(L, 2);
	if (ImGui::Checkbox(label, &val)) {
		lua_pushboolean(L, val);
		return 1;
	}
	return 0;
}

static int lua_ImGui_CheckboxFlags(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	int flags = (int)luaL_checkinteger(L, 2);
	int flags_value = (int)luaL_checkinteger(L, 3);
	if (ImGui::CheckboxFlags(label, &flags, flags_value)) {
		lua_pushinteger(L, flags);
		return 1;
	}
	return 0;
}

// ----------------------------------------------------------------------
// 滑块
// ----------------------------------------------------------------------
static int lua_ImGui_SliderFloat(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	float v = (float)luaL_checknumber(L, 2);
	float v_min = (float)luaL_checknumber(L, 3);
	float v_max = (float)luaL_checknumber(L, 4);
	const char* format = luaL_optstring(L, 5, "%.3f");
	if (ImGui::SliderFloat(label, &v, v_min, v_max, format)) {
		lua_pushnumber(L, v);
		return 1;
	}
	return 0;
}

static int lua_ImGui_SliderInt(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	int v = (int)luaL_checkinteger(L, 2);
	int v_min = (int)luaL_checkinteger(L, 3);
	int v_max = (int)luaL_checkinteger(L, 4);
	const char* format = luaL_optstring(L, 5, "%d");
	if (ImGui::SliderInt(label, &v, v_min, v_max, format)) {
		lua_pushinteger(L, v);
		return 1;
	}
	return 0;
}

// ----------------------------------------------------------------------
// 拖拽控件
// ----------------------------------------------------------------------
static int lua_ImGui_DragFloat(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	float v = (float)luaL_checknumber(L, 2);
	float v_speed = (float)luaL_optnumber(L, 3, 1.0f);
	float v_min = (float)luaL_optnumber(L, 4, 0.0f);
	float v_max = (float)luaL_optnumber(L, 5, 0.0f);
	const char* format = luaL_optstring(L, 6, "%.3f");
	if (ImGui::DragFloat(label, &v, v_speed, v_min, v_max, format)) {
		lua_pushnumber(L, v);
		return 1;
	}
	return 0;
}

static int lua_ImGui_DragInt(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	int v = (int)luaL_checkinteger(L, 2);
	float v_speed = (float)luaL_optnumber(L, 3, 1.0f);
	int v_min = (int)luaL_optinteger(L, 4, 0);
	int v_max = (int)luaL_optinteger(L, 5, 0);
	const char* format = luaL_optstring(L, 6, "%d");
	if (ImGui::DragInt(label, &v, v_speed, v_min, v_max, format)) {
		lua_pushinteger(L, v);
		return 1;
	}
	return 0;
}

// ----------------------------------------------------------------------
// 输入框
// ----------------------------------------------------------------------
static int lua_ImGui_InputText(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	std::string buf = luaL_checkstring(L, 2);
	size_t buf_size = std::max(buf.size() + 1, (size_t)256);
	std::vector<char> buffer(buf_size);
	strcpy(buffer.data(), buf.c_str());
	int flags = (int)luaL_optinteger(L, 3, 0);
	if (ImGui::InputText(label, buffer.data(), buf_size, flags)) {
		lua_pushstring(L, buffer.data());
		return 1;
	}
	return 0;
}

static int lua_ImGui_InputTextMultiline(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	std::string buf = luaL_checkstring(L, 2);
	ImVec2 size = ImVec2(0, 0);
	if (lua_istable(L, 3)) size = to_ImVec2(L, 3);
	int flags = (int)luaL_optinteger(L, 4, 0);
	size_t buf_size = std::max(buf.size() + 1, (size_t)1024);
	std::vector<char> buffer(buf_size);
	strcpy(buffer.data(), buf.c_str());
	if (ImGui::InputTextMultiline(label, buffer.data(), buf_size, size, flags)) {
		lua_pushstring(L, buffer.data());
		return 1;
	}
	return 0;
}

static int lua_ImGui_InputFloat(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	float v = (float)luaL_checknumber(L, 2);
	float step = (float)luaL_optnumber(L, 3, 0.0f);
	float step_fast = (float)luaL_optnumber(L, 4, 0.0f);
	const char* format = luaL_optstring(L, 5, "%.3f");
	if (ImGui::InputFloat(label, &v, step, step_fast, format)) {
		lua_pushnumber(L, v);
		return 1;
	}
	return 0;
}

static int lua_ImGui_InputInt(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	int v = (int)luaL_checkinteger(L, 2);
	int step = (int)luaL_optinteger(L, 3, 1);
	int step_fast = (int)luaL_optinteger(L, 4, 100);
	if (ImGui::InputInt(label, &v, step, step_fast)) {
		lua_pushinteger(L, v);
		return 1;
	}
	return 0;
}

// ----------------------------------------------------------------------
// 组合框
// ----------------------------------------------------------------------
static int lua_ImGui_Combo(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	int current_item = (int)luaL_checkinteger(L, 2);
	// 支持两种形式：items 作为 table 或者 "item1\0item2\0\0"
	if (lua_istable(L, 3)) {
		std::vector<const char*> items;
		lua_pushnil(L);
		while (lua_next(L, 3) != 0) {
			items.push_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		if (ImGui::Combo(label, &current_item, items.data(), items.size())) {
			lua_pushinteger(L, current_item);
			return 1;
		}
	} else if (lua_isstring(L, 3)) {
		const char* items_separated = lua_tostring(L, 3);
		if (ImGui::Combo(label, &current_item, items_separated)) {
			lua_pushinteger(L, current_item);
			return 1;
		}
	}
	return 0;
}

// ----------------------------------------------------------------------
// 颜色编辑
// ----------------------------------------------------------------------
static int lua_ImGui_ColorEdit3(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	ImVec4 col = to_ImVec4(L, 2);
	int flags = (int)luaL_optinteger(L, 3, 0);
	if (ImGui::ColorEdit3(label, &col.x, flags)) {
		push_ImVec4(L, col);
		return 1;
	}
	return 0;
}

static int lua_ImGui_ColorEdit4(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	ImVec4 col = to_ImVec4(L, 2);
	int flags = (int)luaL_optinteger(L, 3, 0);
	if (ImGui::ColorEdit4(label, &col.x, flags)) {
		push_ImVec4(L, col);
		return 1;
	}
	return 0;
}

static int lua_ImGui_ColorPicker3(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	ImVec4 col = to_ImVec4(L, 2);
	int flags = (int)luaL_optinteger(L, 3, 0);
	if (ImGui::ColorPicker3(label, &col.x, flags)) {
		push_ImVec4(L, col);
		return 1;
	}
	return 0;
}

static int lua_ImGui_ColorPicker4(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	ImVec4 col = to_ImVec4(L, 2);
	int flags = (int)luaL_optinteger(L, 3, 0);
	if (ImGui::ColorPicker4(label, &col.x, flags)) {
		push_ImVec4(L, col);
		return 1;
	}
	return 0;
}

// ----------------------------------------------------------------------
// 布局
// ----------------------------------------------------------------------
static int lua_ImGui_SameLine(lua_State* L) {
	float offset_x = (float)luaL_optnumber(L, 1, 0.0f);
	float spacing_w = (float)luaL_optnumber(L, 2, -1.0f);
	ImGui::SameLine(offset_x, spacing_w);
	return 0;
}

static int lua_ImGui_NewLine(lua_State* L) {
	ImGui::NewLine();
	return 0;
}

static int lua_ImGui_Spacing(lua_State* L) {
	ImGui::Spacing();
	return 0;
}

static int lua_ImGui_Separator(lua_State* L) {
	ImGui::Separator();
	return 0;
}

static int lua_ImGui_Dummy(lua_State* L) {
	ImVec2 size = to_ImVec2(L, 1);
	ImGui::Dummy(size);
	return 0;
}

static int lua_ImGui_Indent(lua_State* L) {
	float indent_w = (float)luaL_optnumber(L, 1, 0.0f);
	ImGui::Indent(indent_w);
	return 0;
}

static int lua_ImGui_Unindent(lua_State* L) {
	float indent_w = (float)luaL_optnumber(L, 1, 0.0f);
	ImGui::Unindent(indent_w);
	return 0;
}

// ----------------------------------------------------------------------
// 窗口大小/位置操作
// ----------------------------------------------------------------------
static int lua_ImGui_SetNextWindowSize(lua_State* L) {
	ImVec2 size = to_ImVec2(L, 1);
	int cond = (int)luaL_optinteger(L, 2, 0);
	ImGui::SetNextWindowSize(size, cond);
	return 0;
}

static int lua_ImGui_SetNextWindowPos(lua_State* L) {
	ImVec2 pos = to_ImVec2(L, 1);
	int cond = (int)luaL_optinteger(L, 2, 0);
	ImVec2 pivot = ImVec2(0,0);
	if (lua_istable(L, 3)) pivot = to_ImVec2(L, 3);
	ImGui::SetNextWindowPos(pos, cond, pivot);
	return 0;
}

static int lua_ImGui_GetWindowSize(lua_State* L) {
	push_ImVec2(L, ImGui::GetWindowSize());
	return 1;
}

static int lua_ImGui_GetWindowPos(lua_State* L) {
	push_ImVec2(L, ImGui::GetWindowPos());
	return 1;
}

static int lua_ImGui_SetWindowSize(lua_State* L) {
	ImVec2 size = to_ImVec2(L, 1);
	int cond = (int)luaL_optinteger(L, 2, 0);
	ImGui::SetWindowSize(size, cond);
	return 0;
}

static int lua_ImGui_SetWindowPos(lua_State* L) {
	ImVec2 pos = to_ImVec2(L, 1);
	int cond = (int)luaL_optinteger(L, 2, 0);
	ImGui::SetWindowPos(pos, cond);
	return 0;
}

// ----------------------------------------------------------------------
// 进度条
// ----------------------------------------------------------------------
static int lua_ImGui_ProgressBar(lua_State* L) {
	float fraction = (float)luaL_checknumber(L, 1);
	ImVec2 size = ImVec2(0,0);
	if (lua_istable(L, 2)) size = to_ImVec2(L, 2);
	const char* overlay = luaL_optstring(L, 3, NULL);
	ImGui::ProgressBar(fraction, size, overlay);
	return 0;
}

// ----------------------------------------------------------------------
// 获取IO信息
// ----------------------------------------------------------------------
static int lua_ImGui_GetIO(lua_State* L) {
	ImGuiIO& io = ImGui::GetIO();
	lua_newtable(L);
	push_ImVec2(L, io.DisplaySize);	   lua_setfield(L, -2, "DisplaySize");
	push_ImVec2(L, io.DisplayFramebufferScale); lua_setfield(L, -2, "DisplayFramebufferScale");
	lua_pushnumber(L, io.DeltaTime);	  lua_setfield(L, -2, "DeltaTime");
	lua_pushnumber(L, io.Framerate);	  lua_setfield(L, -2, "Framerate");
	lua_pushboolean(L, io.MouseDown[0]);  lua_setfield(L, -2, "MouseDown");
	push_ImVec2(L, io.MousePos);		  lua_setfield(L, -2, "MousePos");
	lua_pushnumber(L, io.MouseWheel);	 lua_setfield(L, -2, "MouseWheel");
	return 1;
}

// ----------------------------------------------------------------------
// 样式和颜色
// ----------------------------------------------------------------------
static int lua_ImGui_PushStyleColor(lua_State* L) {
	int idx = (int)luaL_checkinteger(L, 1);
	ImVec4 col = to_ImVec4(L, 2);
	ImGui::PushStyleColor(idx, col);
	return 0;
}

static int lua_ImGui_PopStyleColor(lua_State* L) {
	int count = (int)luaL_optinteger(L, 1, 1);
	ImGui::PopStyleColor(count);
	return 0;
}

static int lua_ImGui_PushStyleVar(lua_State* L) {
	int idx = (int)luaL_checkinteger(L, 1);
	if (lua_isnumber(L, 2)) {
		float val = (float)luaL_checknumber(L, 2);
		ImGui::PushStyleVar(idx, val);
	} else if (lua_istable(L, 2)) {
		ImVec2 val = to_ImVec2(L, 2);
		ImGui::PushStyleVar(idx, val);
	}
	return 0;
}

static int lua_ImGui_PopStyleVar(lua_State* L) {
	int count = (int)luaL_optinteger(L, 1, 1);
	ImGui::PopStyleVar(count);
	return 0;
}

// ----------------------------------------------------------------------
// 工具函数
// ----------------------------------------------------------------------
static int lua_ImGui_GetVersion(lua_State* L) {
	lua_pushstring(L, IMGUI_VERSION);
	return 1;
}

static int lua_ImGui_ShowDemoWindow(lua_State* L) {
	bool* p_open = nullptr;
	bool local_open = true;
	int arg_type = lua_type(L, 1);
	if (arg_type == LUA_TTABLE) {
		lua_getfield(L, 1, "open");
		if (lua_isboolean(L, -1)) {
			local_open = lua_toboolean(L, -1);
			p_open = &local_open;
		}
		lua_pop(L, 1);
	} 
	else if (arg_type == LUA_TBOOLEAN) {
		local_open = lua_toboolean(L, 1);
		p_open = &local_open;
	}
	ImGui::ShowDemoWindow(p_open);
	if (arg_type == LUA_TTABLE && p_open) {
		lua_pushboolean(L, *p_open);
		lua_setfield(L, 1, "open");
		return 0;
	}
	else if (arg_type == LUA_TBOOLEAN && p_open) {
		lua_pushboolean(L, *p_open);
		return 1;
	}
	else {
		return 0;
	}
}

static int lua_ImGui_GetStyle(lua_State* L) {
	ImGuiStyle& style = ImGui::GetStyle();
	lua_newtable(L);
	// ImVec2 类型字段
	push_ImVec2(L, style.WindowPadding);		  lua_setfield(L, -2, "WindowPadding");
	push_ImVec2(L, style.FramePadding);		   lua_setfield(L, -2, "FramePadding");
	push_ImVec2(L, style.ItemSpacing);			lua_setfield(L, -2, "ItemSpacing");
	push_ImVec2(L, style.ItemInnerSpacing);	   lua_setfield(L, -2, "ItemInnerSpacing");
	push_ImVec2(L, style.TouchExtraPadding);	  lua_setfield(L, -2, "TouchExtraPadding");
	push_ImVec2(L, style.CellPadding);			lua_setfield(L, -2, "CellPadding");
	push_ImVec2(L, style.ButtonTextAlign);		lua_setfield(L, -2, "ButtonTextAlign");
	push_ImVec2(L, style.DisplayWindowPadding);   lua_setfield(L, -2, "DisplayWindowPadding");
	push_ImVec2(L, style.DisplaySafeAreaPadding); lua_setfield(L, -2, "DisplaySafeAreaPadding");
	// float 类型字段
	lua_pushnumber(L, style.IndentSpacing);	   lua_setfield(L, -2, "IndentSpacing");
	lua_pushnumber(L, style.ScrollbarSize);	   lua_setfield(L, -2, "ScrollbarSize");
	lua_pushnumber(L, style.GrabMinSize);		 lua_setfield(L, -2, "GrabMinSize");
	lua_pushnumber(L, style.WindowBorderSize);	lua_setfield(L, -2, "WindowBorderSize");
	lua_pushnumber(L, style.ChildBorderSize);	 lua_setfield(L, -2, "ChildBorderSize");
	lua_pushnumber(L, style.PopupBorderSize);	 lua_setfield(L, -2, "PopupBorderSize");
	lua_pushnumber(L, style.FrameBorderSize);	 lua_setfield(L, -2, "FrameBorderSize");
	lua_pushnumber(L, style.TabBorderSize);	   lua_setfield(L, -2, "TabBorderSize");
	// 以下为浮点 rounding 等
	lua_pushnumber(L, style.WindowRounding);	  lua_setfield(L, -2, "WindowRounding");
	lua_pushnumber(L, style.ChildRounding);	   lua_setfield(L, -2, "ChildRounding");
	lua_pushnumber(L, style.FrameRounding);	   lua_setfield(L, -2, "FrameRounding");
	lua_pushnumber(L, style.PopupRounding);	   lua_setfield(L, -2, "PopupRounding");
	lua_pushnumber(L, style.ScrollbarRounding);   lua_setfield(L, -2, "ScrollbarRounding");
	lua_pushnumber(L, style.GrabRounding);		lua_setfield(L, -2, "GrabRounding");
	lua_pushnumber(L, style.TabRounding);		 lua_setfield(L, -2, "TabRounding");
	lua_pushnumber(L, style.Alpha);			   lua_setfield(L, -2, "Alpha");
	lua_pushnumber(L, style.DisabledAlpha);	   lua_setfield(L, -2, "DisabledAlpha");
	lua_pushnumber(L, style.ColorButtonPosition); lua_setfield(L, -2, "ColorButtonPosition");
	lua_pushnumber(L, style.MouseCursorScale);	lua_setfield(L, -2, "MouseCursorScale");
	lua_pushnumber(L, style.CurveTessellationTol); lua_setfield(L, -2, "CurveTessellationTol");
	lua_pushnumber(L, style.CircleTessellationMaxError); lua_setfield(L, -2, "CircleTessellationMaxError");
	// 布尔字段
	lua_pushboolean(L, style.AntiAliasedLines);		 lua_setfield(L, -2, "AntiAliasedLines");
	lua_pushboolean(L, style.AntiAliasedLinesUseTex);   lua_setfield(L, -2, "AntiAliasedLinesUseTex");
	lua_pushboolean(L, style.AntiAliasedFill);		  lua_setfield(L, -2, "AntiAliasedFill");
	// 颜色数组
	lua_newtable(L);
	for (int i = 0; i < ImGuiCol_COUNT; ++i) {
		push_ImVec4(L, style.Colors[i]);
		lua_rawseti(L, -2, i);
	}
	lua_setfield(L, -2, "Colors");
	return 1;
}

// 主题切换函数
static int lua_ImGui_StyleColorsDark(lua_State* L) {
	ImGui::StyleColorsDark(nullptr);   // 应用深色样式（默认）
	return 0;
}

static int lua_ImGui_StyleColorsLight(lua_State* L) {
	ImGui::StyleColorsLight(nullptr);  // 应用亮色样式
	return 0;
}

static int lua_ImGui_StyleColorsClassic(lua_State* L) {
	ImGui::StyleColorsClassic(nullptr); // 应用经典样式
	return 0;
}

static int lua_ImGui_ShowStyleSelector(lua_State* L) {
	const char* label = luaL_checkstring(L, 1);
	static int style_idx = 0;   // 保存当前样式索引
	const char* items = "Dark\0Light\0Classic\0";   // 组合框选项
	
	if (ImGui::Combo(label, &style_idx, items)) {
		switch (style_idx) {
			case 0: ImGui::StyleColorsDark(); break;
			case 1: ImGui::StyleColorsLight(); break;
			case 2: ImGui::StyleColorsClassic(); break;
		}
		lua_pushboolean(L, 1);   // 返回 true 表示样式已改变
		return 1;
	}
	lua_pushboolean(L, 0);
	return 1;
}
// 获取 ImDrawList 对象（通过 lightuserdata）
static int lua_ImDrawList_AddLine(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 p1 = to_ImVec2(L, 2);
	ImVec2 p2 = to_ImVec2(L, 3);
	ImU32 col = to_ImU32(L, 4);
	float thickness = (float)luaL_optnumber(L, 5, 1.0f);
	dl->AddLine(p1, p2, col, thickness);
	return 0;
}

static int lua_ImDrawList_AddRect(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 p_min = to_ImVec2(L, 2);
	ImVec2 p_max = to_ImVec2(L, 3);
	ImU32 col = to_ImU32(L, 4);
	float rounding = (float)luaL_optnumber(L, 5, 0.0f);
	int flags = (int)luaL_optinteger(L, 6, 0);
	float thickness = (float)luaL_optnumber(L, 7, 1.0f);
	dl->AddRect(p_min, p_max, col, rounding, flags, thickness);
	return 0;
}

static int lua_ImDrawList_AddRectFilled(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 p_min = to_ImVec2(L, 2);
	ImVec2 p_max = to_ImVec2(L, 3);
	ImU32 col = to_ImU32(L, 4);
	float rounding = (float)luaL_optnumber(L, 5, 0.0f);
	int flags = (int)luaL_optinteger(L, 6, 0);
	dl->AddRectFilled(p_min, p_max, col, rounding, flags);
	return 0;
}

static int lua_ImDrawList_AddCircle(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 center = to_ImVec2(L, 2);
	float radius = (float)luaL_checknumber(L, 3);
	ImU32 col = to_ImU32(L, 4);
	int num_segments = (int)luaL_optinteger(L, 5, 0);
	float thickness = (float)luaL_optnumber(L, 6, 1.0f);
	dl->AddCircle(center, radius, col, num_segments, thickness);
	return 0;
}

static int lua_ImDrawList_AddCircleFilled(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 center = to_ImVec2(L, 2);
	float radius = (float)luaL_checknumber(L, 3);
	ImU32 col = to_ImU32(L, 4);
	int num_segments = (int)luaL_optinteger(L, 5, 0);
	dl->AddCircleFilled(center, radius, col, num_segments);
	return 0;
}

static int lua_ImDrawList_AddTriangle(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 p1 = to_ImVec2(L, 2);
	ImVec2 p2 = to_ImVec2(L, 3);
	ImVec2 p3 = to_ImVec2(L, 4);
	ImU32 col = to_ImU32(L, 5);
	float thickness = (float)luaL_optnumber(L, 6, 1.0f);
	dl->AddTriangle(p1, p2, p3, col, thickness);
	return 0;
}

static int lua_ImDrawList_AddTriangleFilled(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 p1 = to_ImVec2(L, 2);
	ImVec2 p2 = to_ImVec2(L, 3);
	ImVec2 p3 = to_ImVec2(L, 4);
	ImU32 col = to_ImU32(L, 5);
	dl->AddTriangleFilled(p1, p2, p3, col);
	return 0;
}

static int lua_ImDrawList_AddText(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	ImVec2 pos = to_ImVec2(L, 2);
	ImU32 col = to_ImU32(L, 3);
	const char* text = luaL_checkstring(L, 4);
	dl->AddText(pos, col, text);
	return 0;
}

static int lua_ImDrawList_AddPolyline(lua_State* L) {
	ImDrawList* dl = (ImDrawList*)lua_touserdata(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	int points_count = (int)luaL_len(L, 2);
	std::vector<ImVec2> points;
	for (int i = 1; i <= points_count; i++) {
		lua_rawgeti(L, 2, i);
		points.push_back(to_ImVec2(L, -1));
		lua_pop(L, 1);
	}
	ImU32 col = to_ImU32(L, 3);
	int flags = (int)luaL_optinteger(L, 4, 0);
	float thickness = (float)luaL_optnumber(L, 5, 1.0f);
	dl->AddPolyline(points.data(), points_count, col, flags, thickness);
	return 0;
}

extern "C" void create_imdrawlist_metatable(lua_State* L) {
	luaL_newmetatable(L, "ImDrawList");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	static const luaL_Reg methods[] = {
		{"AddLine", lua_ImDrawList_AddLine},
		{"AddRect", lua_ImDrawList_AddRect},
		{"AddRectFilled", lua_ImDrawList_AddRectFilled},
		{"AddCircle", lua_ImDrawList_AddCircle},
		{"AddCircleFilled", lua_ImDrawList_AddCircleFilled},
		{"AddTriangle", lua_ImDrawList_AddTriangle},
		{"AddTriangleFilled", lua_ImDrawList_AddTriangleFilled},
		{"AddText", lua_ImDrawList_AddText},
		{"AddPolyline", lua_ImDrawList_AddPolyline},
		{NULL, NULL}
	};
	luaL_setfuncs(L, methods, 0);
	lua_pop(L, 1);
}
static int lua_ImGui_GetWindowDrawList(lua_State* L) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	lua_pushlightuserdata(L, draw_list);
	luaL_getmetatable(L, "ImDrawList");
	lua_setmetatable(L, -2);
	return 1;
}

static int lua_ImGui_GetForegroundDrawList(lua_State* L) {
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	lua_pushlightuserdata(L, draw_list);
	luaL_getmetatable(L, "ImDrawList");
	lua_setmetatable(L, -2);
	return 1;
}
// ----------------------------------------------------------------------
// 模块注册表
// ----------------------------------------------------------------------
static const luaL_Reg ImGuiLib[] = {
	// 窗口
	{"Begin", lua_ImGui_Begin},
	{"End", lua_ImGui_End},
	{"BeginChild", lua_ImGui_BeginChild},
	{"EndChild", lua_ImGui_EndChild},
	// 文本
	{"Text", lua_ImGui_Text},
	{"TextColored", lua_ImGui_TextColored},
	// 按钮
	{"Button", lua_ImGui_Button},
	{"SmallButton", lua_ImGui_SmallButton},
	// 复选框
	{"Checkbox", lua_ImGui_Checkbox},
	{"CheckboxFlags", lua_ImGui_CheckboxFlags},
	// 滑块
	{"SliderFloat", lua_ImGui_SliderFloat},
	{"SliderInt", lua_ImGui_SliderInt},
	// 拖拽
	{"DragFloat", lua_ImGui_DragFloat},
	{"DragInt", lua_ImGui_DragInt},
	// 输入框
	{"InputText", lua_ImGui_InputText},
	{"InputTextMultiline", lua_ImGui_InputTextMultiline},
	{"InputFloat", lua_ImGui_InputFloat},
	{"InputInt", lua_ImGui_InputInt},
	// 组合框
	{"Combo", lua_ImGui_Combo},
	// 颜色编辑
	{"ColorEdit3", lua_ImGui_ColorEdit3},
	{"ColorEdit4", lua_ImGui_ColorEdit4},
	{"ColorPicker3", lua_ImGui_ColorPicker3},
	{"ColorPicker4", lua_ImGui_ColorPicker4},
	// 布局
	{"SameLine", lua_ImGui_SameLine},
	{"NewLine", lua_ImGui_NewLine},
	{"Spacing", lua_ImGui_Spacing},
	{"Separator", lua_ImGui_Separator},
	{"Dummy", lua_ImGui_Dummy},
	{"Indent", lua_ImGui_Indent},
	{"Unindent", lua_ImGui_Unindent},
	// 窗口操作
	{"SetNextWindowSize", lua_ImGui_SetNextWindowSize},
	{"SetNextWindowPos", lua_ImGui_SetNextWindowPos},
	{"GetWindowSize", lua_ImGui_GetWindowSize},
	{"GetWindowPos", lua_ImGui_GetWindowPos},
	{"SetWindowSize", lua_ImGui_SetWindowSize},
	{"SetWindowPos", lua_ImGui_SetWindowPos},
	// 进度条
	{"ProgressBar", lua_ImGui_ProgressBar},
	// IO
	{"GetIO", lua_ImGui_GetIO},
	{"GetStyle", lua_ImGui_GetStyle},
	// 样式
	{"PushStyleColor", lua_ImGui_PushStyleColor},
	{"PopStyleColor", lua_ImGui_PopStyleColor},
	{"PushStyleVar", lua_ImGui_PushStyleVar},
	{"PopStyleVar", lua_ImGui_PopStyleVar},
	// 工具
	{"GetVersion", lua_ImGui_GetVersion},
	{"ShowDemoWindow", lua_ImGui_ShowDemoWindow},
	// 主题
	{"StyleColorsDark", lua_ImGui_StyleColorsDark},
	{"StyleColorsLight", lua_ImGui_StyleColorsLight},
	{"StyleColorsClassic", lua_ImGui_StyleColorsClassic},
	{"ShowStyleSelector", lua_ImGui_ShowStyleSelector},
	// 绘图
	{"GetWindowDrawList", lua_ImGui_GetWindowDrawList},
	{"GetForegroundDrawList", lua_ImGui_GetForegroundDrawList},
	{NULL, NULL}
};

extern "C" int luaopen_ImGui(lua_State* L) {
	luaL_newlib(L, ImGuiLib);
	return 1;
}
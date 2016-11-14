/*
 * Nuklear - v1.00 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 * adapted to LOVE in 2016 by Kevin Harrison
 */

#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#define NK_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_PRIVATE
#include "nuklear/nuklear.h"

/*
 * ===============================================================
 *
 *                          INTERNAL
 *
 * ===============================================================
 */
 
#define NK_LOVE_MAX_POINTS 256
#define NK_LOVE_EDIT_BUFFER_LEN (1024 * 1024)
#define NK_LOVE_COMBOBOX_MAX_ITEMS 1024
#define NK_LOVE_MAX_FONTS 1024
#define NK_LOVE_MAX_RATIOS 1024

static lua_State *L;
static struct nk_context context;
static struct nk_user_font *fonts;
static int font_count;
static char *edit_buffer;
static const char **combobox_items;
static struct nk_cursor cursors[NK_CURSOR_COUNT];
static float *layout_ratios;
static int layout_ratio_count;

static void nk_love_configureGraphics(int line_thickness, struct nk_color col)
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");
	lua_remove(L, -2);
	if (line_thickness >= 0) {
		lua_getfield(L, -1, "setLineWidth");
		lua_pushnumber(L, line_thickness);
		lua_call(L, 1, 0);
	}
	lua_getfield(L, -1, "setColor");
	lua_pushnumber(L, col.r);
	lua_pushnumber(L, col.g);
	lua_pushnumber(L, col.b);
	lua_pushnumber(L, col.a);
	lua_call(L, 4, 0);
}

static void nk_love_scissor(int x, int y, int w, int h)
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");
	lua_getfield(L, -1, "setScissor");
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	lua_call(L, 4, 0);
	lua_pop(L, 2);
}

static void nk_love_draw_line(int x0, int y0, int x1, int y1,
	int line_thickness, struct nk_color col)
{
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "line");
	lua_pushnumber(L, x0);
	lua_pushnumber(L, y0);
	lua_pushnumber(L, x1);
	lua_pushnumber(L, y1);
	lua_call(L, 4, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_rect(int x, int y,	unsigned int w,
	unsigned int h, unsigned int r, int line_thickness,
	struct nk_color col)
{
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "rectangle");
	if (line_thickness >= 0) {
		lua_pushstring(L, "line");
	} else {
		lua_pushstring(L, "fill");
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	lua_pushnumber(L, r);
	lua_pushnumber(L, r);
	lua_call(L, 7, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_triangle(int x0, int y0, int x1, int y1,
	int x2, int y2,	int line_thickness, struct nk_color col)
{
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "polygon");
	if (line_thickness >= 0) {
		lua_pushstring(L, "line");
	} else {
		lua_pushstring(L, "fill");
	}
	lua_pushnumber(L, x0);
	lua_pushnumber(L, y0);
	lua_pushnumber(L, x1);
	lua_pushnumber(L, y1);
	lua_pushnumber(L, x2);
	lua_pushnumber(L, y2);
	lua_call(L, 7, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_polygon(const struct nk_vec2i *pnts, int count,
	int line_thickness, struct nk_color col)
{
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "polygon");
	if (line_thickness >= 0) {
		lua_pushstring(L, "line");
	} else {
		lua_pushstring(L, "fill");
	}
	int i;
	for (i = 0; (i < count) && (i < NK_LOVE_MAX_POINTS); ++i) {
		lua_pushnumber(L, pnts[i].x);
		lua_pushnumber(L, pnts[i].y);
	}
	lua_call(L, 1 + count * 2, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_polyline(const struct nk_vec2i *pnts,
	int count, int line_thickness, struct nk_color col)
{
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "line");
	int i;
	for (i = 0; (i < count) && (i < NK_LOVE_MAX_POINTS); ++i) {
		lua_pushnumber(L, pnts[i].x);
		lua_pushnumber(L, pnts[i].y);
	}
	lua_call(L, count * 2, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_circle(int x, int y, unsigned int w,
	unsigned int h, int line_thickness, struct nk_color col)
{
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "ellipse");
	if (line_thickness >= 0) {
		lua_pushstring(L, "line");
	} else {
		lua_pushstring(L, "fill");
	}
	lua_pushnumber(L, x + w / 2);
	lua_pushnumber(L, y + h / 2);
	lua_pushnumber(L, w / 2);
	lua_pushnumber(L, h / 2);
	lua_call(L, 5, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_curve(struct nk_vec2i p1, struct nk_vec2i p2,
	struct nk_vec2i p3, struct nk_vec2i p4, unsigned int num_segments,
	int line_thickness, struct nk_color col)
{
	unsigned int i_step;
	float t_step;
	struct nk_vec2i last = p1;

	if (num_segments < 1) {
		num_segments = 1;
	}
	t_step = 1.0f/(float)num_segments;
	nk_love_configureGraphics(line_thickness, col);
	lua_getfield(L, -1, "line");
	for (i_step = 1; i_step <= num_segments; ++i_step) {
		float t = t_step * (float)i_step;
		float u = 1.0f - t;
		float w1 = u*u*u;
		float w2 = 3*u*u*t;
		float w3 = 3*u*t*t;
		float w4 = t * t *t;
		float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
		float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
	}
	lua_call(L, (num_segments + 1) * 2, 0);
	lua_pop(L, 1);
}

static float nk_love_get_text_width(nk_handle handle, float height,
	const char *text, int len)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "font");
	lua_rawgeti(L, -1, handle.id);
	lua_getfield(L, -1, "getWidth");
	lua_insert(L, -2);
	lua_pushlstring(L, text, len);
	lua_call(L, 2, 1);
	float width = lua_tonumber(L, -1);
	lua_pop(L, 3);
	return width;
}

static void nk_love_draw_text(int fontref, struct nk_color cbg,
	struct nk_color cfg, int x, int y, unsigned int w, unsigned int h,
	float height, int len, const char *text)
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");

	lua_getfield(L, -1, "setColor");
	lua_pushnumber(L, cbg.r);
	lua_pushnumber(L, cbg.g);
	lua_pushnumber(L, cbg.b);
	lua_pushnumber(L, cbg.a);
	lua_call(L, 4, 0);

	lua_getfield(L, -1, "rectangle");
	lua_pushstring(L, "fill");
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	lua_call(L, 5, 0);

	lua_getfield(L, -1, "setColor");
	lua_pushnumber(L, cfg.r);
	lua_pushnumber(L, cfg.g);
	lua_pushnumber(L, cfg.b);
	lua_pushnumber(L, cfg.a);
	lua_call(L, 4, 0);

	lua_getfield(L, -1, "setFont");
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "font");
	lua_rawgeti(L, -1, fontref);
	lua_replace(L, -3);
	lua_pop(L, 1);
	lua_call(L, 1, 0);

	lua_getfield(L, -1, "print");
	lua_pushlstring(L, text, len);
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_call(L, 3, 0);

	lua_pop(L, 2);
}

static void interpolate_color(struct nk_color c1, struct nk_color c2,
	struct nk_color *result, float fraction)
{
	float r = c1.r + (c2.r - c1.r) * fraction;
	float g = c1.g + (c2.g - c1.g) * fraction;
	float b = c1.b + (c2.b - c1.b) * fraction;
	float a = c1.a + (c2.a - c1.a) * fraction;

	result->r = (nk_byte)NK_CLAMP(0, r, 255);
	result->g = (nk_byte)NK_CLAMP(0, g, 255);
	result->b = (nk_byte)NK_CLAMP(0, b, 255);
	result->a = (nk_byte)NK_CLAMP(0, a, 255);
}

static void nk_love_draw_rect_multi_color(int x, int y, unsigned int w,
	unsigned int h, struct nk_color left, struct nk_color top,
	struct nk_color right, struct nk_color bottom)
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");

	lua_getfield(L, -1, "setPointSize");
	lua_pushnumber(L, 0);
	lua_call(L, 1, 0);
	
	struct nk_color X1, X2, Y;
	float fraction_x, fraction_y;
	int i,j;
	
	lua_getfield(L, -1, "points");
	lua_createtable(L, w * h, 0);
	
	for (j = 0; j < h; j++) {
		fraction_y = ((float)j) / h;
		for (i = 0; i < w; i++) {
			fraction_x = ((float)i) / w;
			interpolate_color(left, top, &X1, fraction_x);
			interpolate_color(right, bottom, &X2, fraction_x);
			interpolate_color(X1, X2, &Y, fraction_y);
			lua_createtable(L, 6, 0);
			lua_pushnumber(L, x + i);
			lua_rawseti(L, -2, 1);
			lua_pushnumber(L, y + j);
			lua_rawseti(L, -2, 2);
			lua_pushnumber(L, Y.r);
			lua_rawseti(L, -2, 3);
			lua_pushnumber(L, Y.g);
			lua_rawseti(L, -2, 4);
			lua_pushnumber(L, Y.b);
			lua_rawseti(L, -2, 5);
			lua_pushnumber(L, Y.a);
			lua_rawseti(L, -2, 6);
			lua_rawseti(L, -2, i + j * w + 1);
		}
	}
	
	lua_call(L, 1, 0);
	lua_pop(L, 2);
}

static void nk_love_clear(struct nk_color col)
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");
	lua_getfield(L, -1, "clear");
	lua_pushnumber(L, col.r);
	lua_pushnumber(L, col.g);
	lua_pushnumber(L, col.b);
	lua_pushnumber(L, col.a);
	lua_call(L, 4, 0);
	lua_pop(L, 2);
}

static void nk_love_blit()
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");
	lua_getfield(L, -1, "present");
	lua_call(L, 0, 0);
	lua_pop(L, 2);
}

static void nk_love_draw_image(int x, int y, unsigned int w, unsigned int h,
	struct nk_image image, struct nk_color color)
{
	nk_love_configureGraphics(-1, color);
	lua_getfield(L, -1, "draw");
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "image");
	lua_rawgeti(L, -1, image.handle.id);
	lua_getfield(L, -5, "newQuad");
	lua_pushnumber(L, image.region[0]);
	lua_pushnumber(L, image.region[1]);
	lua_pushnumber(L, image.region[2]);
	lua_pushnumber(L, image.region[3]);
	lua_pushnumber(L, image.w);
	lua_pushnumber(L, image.h);
	lua_call(L, 6, 1);
	lua_replace(L, -3);
	lua_replace(L, -3);
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, 0);
	lua_pushnumber(L, (double) w / image.region[2]);
	lua_pushnumber(L, (double) h / image.region[3]);
	lua_call(L, 7, 0);
	lua_pop(L, 1);
}

static void nk_love_draw_arc(int cx, int cy, unsigned int r,
	int line_thickness, float a1, float a2, struct nk_color color)
{
	nk_love_configureGraphics(line_thickness, color);
	lua_getfield(L, -1, "arc");
	if (line_thickness >= 0) {
		lua_pushstring(L, "line");
	} else {
		lua_pushstring(L, "fill");
	}
	lua_pushnumber(L, cx);
	lua_pushnumber(L, cy);
	lua_pushnumber(L, r);
	lua_pushnumber(L, a1);
	lua_pushnumber(L, a2);
	lua_call(L, 6, 0);
	lua_pop(L, 1);
}

static void nk_love_clipbard_paste(nk_handle usr, struct nk_text_edit *edit)
{
	(void)usr;
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "system");
	lua_getfield(L, -1, "getClipboardText");
	lua_call(L, 0, 1);
	const char *text = lua_tostring(L, -1);
	if (text) nk_textedit_paste(edit, text, nk_strlen(text));
	lua_pop(L, 3);
}

static void nk_love_clipbard_copy(nk_handle usr, const char *text, int len)
{
	(void)usr;
	char *str = 0;
	if (!len) return;
	str = (char*)malloc((size_t)len+1);
	if (!str) return;
	memcpy(str, text, (size_t)len);
	str[len] = '\0';
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "system");
	lua_getfield(L, -1, "setClipboardText");
	lua_pushstring(L, str);
	free(str);
	lua_call(L, 1, 0);
	lua_pop(L, 2);
}

static int nk_love_is_active(struct nk_context *ctx)
{
	struct nk_window *iter;
	NK_ASSERT(ctx);
	if (!ctx) return 0;
	iter = ctx->begin;
	while (iter) {
		/* check if window is being hovered */
		if (iter->flags & NK_WINDOW_MINIMIZED) {
			struct nk_rect header = iter->bounds;
			header.h = ctx->style.font->height + 2 * ctx->style.window.header.padding.y;
			if (nk_input_is_mouse_hovering_rect(&ctx->input, header))
				return 1;
		} else if (nk_input_is_mouse_hovering_rect(&ctx->input, iter->bounds)) {
			return 1;
		}
		/* check if window popup is being hovered */
		if (iter->popup.active && iter->popup.win && nk_input_is_mouse_hovering_rect(&ctx->input, iter->popup.win->bounds))
			return 1;
		if (iter->edit.active & NK_EDIT_ACTIVE)
			return 1;
		iter = iter->next;
	}
	return 0;
}

static int nk_love_keyevent(const char *key, const char *scancode,
	int isrepeat, int down)
{
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "keyboard");
	lua_getfield(L, -1, "isScancodeDown");
	lua_pushstring(L, "lctrl");
	lua_call(L, 1, 1);
	int lctrl = lua_toboolean(L, -1);
	lua_pop(L, 3);
	
	if (!strcmp(key, "rshift") || !strcmp(key, "lshift"))
		nk_input_key(&context, NK_KEY_SHIFT, down);
	else if (!strcmp(key, "delete"))
		nk_input_key(&context, NK_KEY_DEL, down);
	else if (!strcmp(key, "return"))
		nk_input_key(&context, NK_KEY_ENTER, down);
	else if (!strcmp(key, "tab"))
		nk_input_key(&context, NK_KEY_TAB, down);
	else if (!strcmp(key, "backspace"))
		nk_input_key(&context, NK_KEY_BACKSPACE, down);
	else if (!strcmp(key, "home")) {
		nk_input_key(&context, NK_KEY_TEXT_LINE_START, down);
	} else if (!strcmp(key, "end")) {
		nk_input_key(&context, NK_KEY_TEXT_LINE_END, down);
	} else if (!strcmp(key, "pagedown")) {
		nk_input_key(&context, NK_KEY_SCROLL_DOWN, down);
	} else if (!strcmp(key, "pageup")) {
		nk_input_key(&context, NK_KEY_SCROLL_UP, down);
	} else if (!strcmp(key, "z"))
		nk_input_key(&context, NK_KEY_TEXT_UNDO, down && lctrl);
	else if (!strcmp(key, "r"))
		nk_input_key(&context, NK_KEY_TEXT_REDO, down && lctrl);
	else if (!strcmp(key, "c"))
		nk_input_key(&context, NK_KEY_COPY, down && lctrl);
	else if (!strcmp(key, "v"))
		nk_input_key(&context, NK_KEY_PASTE, down && lctrl);
	else if (!strcmp(key, "x"))
		nk_input_key(&context, NK_KEY_CUT, down && lctrl);
	else if (!strcmp(key, "b"))
		nk_input_key(&context, NK_KEY_TEXT_LINE_START, down && lctrl);
	else if (!strcmp(key, "e"))
		nk_input_key(&context, NK_KEY_TEXT_LINE_END, down && lctrl);
	else if (!strcmp(key, "left")) {
		if (lctrl)
			nk_input_key(&context, NK_KEY_TEXT_WORD_LEFT, down);
		else
			nk_input_key(&context, NK_KEY_LEFT, down);
	} else if (!strcmp(key, "right")) {
		if (lctrl)
			nk_input_key(&context, NK_KEY_TEXT_WORD_RIGHT, down);
		else
			nk_input_key(&context, NK_KEY_RIGHT, down);
	} else if (!strcmp(key, "up"))
		nk_input_key(&context, NK_KEY_UP, down);
	else if (!strcmp(key, "down"))
		nk_input_key(&context, NK_KEY_DOWN, down);
	else
		return 0;
	return nk_love_is_active(&context);
}

static int nk_love_clickevent(int x, int y, int button, int istouch, int down)
{
	if (button == 1)
		nk_input_button(&context, NK_BUTTON_LEFT, x, y, down);
	else if (button == 3)
		nk_input_button(&context, NK_BUTTON_MIDDLE, x, y, down);
	else if (button == 2)
		nk_input_button(&context, NK_BUTTON_RIGHT, x, y, down);
	else
		return 0;
	return nk_window_is_any_hovered(&context);
}

static int nk_love_mousemoved_event(int x, int y, int dx, int dy, int istouch)
{
	nk_input_motion(&context, x, y);
	return nk_window_is_any_hovered(&context);
}

static int nk_love_textinput_event(const char *text)
{
	nk_rune rune;
	nk_utf_decode(text, &rune, strlen(text));
	nk_input_unicode(&context, rune);
	return nk_love_is_active(&context);
}

static int nk_love_wheelmoved_event(int x, int y)
{
	nk_input_scroll(&context,(float)y);
	return nk_window_is_any_hovered(&context);
}

/*
 * ===============================================================
 *
 *                          WRAPPER
 *
 * ===============================================================
 */

static void nk_love_error(const char *msg)
{
	lua_getglobal(L, "error");
	lua_pushstring(L, msg);
	lua_call(L, 1, 0);
}

static void nk_love_assert(int pass, const char *msg)
{
	if (!pass) {
		nk_love_error(msg);
	}
}

static void nk_love_assert_type(int index, const char *type, const char *message)
{
	if (index < 0) {
		index += lua_gettop(L) + 1;
	}
	nk_love_assert(lua_type(L, index) == LUA_TUSERDATA, message);
	lua_getfield(L, index, "typeOf");
	nk_love_assert(lua_type(L, -1) == LUA_TFUNCTION, message);
	lua_pushvalue(L, index);
	lua_pushstring(L, type);
	lua_call(L, 2, 1);
	nk_love_assert(lua_type(L, -1) == LUA_TBOOLEAN, message);
	int is_type = lua_toboolean(L, -1);
	nk_love_assert(is_type, message);
	lua_pop(L, 1);
}

static void nk_love_assert_hex(char c, const char *message)
{
	nk_love_assert((c >= '0' && c <= '9')
	            || (c >= 'a' && c <= 'f')
	            || (c >= 'A' && c <= 'F'), message); 
}

static void nk_love_assert_color(int index, const char *message)
{
	nk_love_assert(lua_type(L, index) == LUA_TSTRING, message);
	const char *color_string = lua_tostring(L, index);
	size_t len = strlen(color_string);
	if (len == 7 || len == 9) {
		nk_love_assert(color_string[0] == '#', message);
		int i;
		for (i = 1; i < len; ++i) {
			nk_love_assert_hex(color_string[i], message); 
		}
	} else {
		nk_love_error(message);
	}
}

static nk_flags nk_love_parse_window_flags(int flags_begin) {
	int argc = lua_gettop(L);
	nk_flags flags = NK_WINDOW_NO_SCROLLBAR;
	int i;
	for (i = flags_begin; i <= argc; ++i) {
		nk_love_assert(lua_type(L, i) == LUA_TSTRING, "window flags must be strings");
		const char *flag = lua_tostring(L, i);
		if (!strcmp(flag, "border"))
			flags |= NK_WINDOW_BORDER;
		else if (!strcmp(flag, "movable"))
			flags |= NK_WINDOW_MOVABLE;
		else if (!strcmp(flag, "scalable"))
			flags |= NK_WINDOW_SCALABLE;
		else if (!strcmp(flag, "closable"))
			flags |= NK_WINDOW_CLOSABLE;
		else if (!strcmp(flag, "minimizable"))
			flags |= NK_WINDOW_MINIMIZABLE;
		else if (!strcmp(flag, "scrollbar"))
			flags &= ~NK_WINDOW_NO_SCROLLBAR;
		else if (!strcmp(flag, "title"))
			flags |= NK_WINDOW_TITLE;
		else if (!strcmp(flag, "scroll auto hide"))
			flags |= NK_WINDOW_SCROLL_AUTO_HIDE;
		else if (!strcmp(flag, "background"))
			flags |= NK_WINDOW_BACKGROUND;
		else
			nk_love_error("unrecognized window flag");
		
	}
	return flags;
}

static void nk_love_tofont(struct nk_user_font *font)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "font");
	lua_pushvalue(L, -3);
	int ref = luaL_ref(L, -2);
	lua_getfield(L, -3, "getHeight");
	lua_pushvalue(L, -4);
	lua_call(L, 1, 1);
	float height = lua_tonumber(L, -1);
	font->userdata = nk_handle_id(ref);
	font->height = height;
	font->width = nk_love_get_text_width;
	lua_pop(L, 4);
}

static void nk_love_toimage(struct nk_image *image)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "image");
	lua_pushvalue(L, -3);
	int ref = luaL_ref(L, -2);
	lua_getfield(L, -3, "getDimensions");
	lua_pushvalue(L, -4);
	lua_call(L, 1, 2);
	int width = lua_tointeger(L, -2);
	int height = lua_tointeger(L, -1);
	image->handle = nk_handle_id(ref);
	image->w = width;
	image->h = height;
	image->region[0] = 0;
	image->region[1] = 0;
	image->region[2] = width;
	image->region[3] = height;
	lua_pop(L, 5);
}

static int nk_love_parse_symbol(const char *s, enum nk_symbol_type *symbol)
{
	if (!strcmp(s, "none")) {
		*symbol = NK_SYMBOL_NONE;
	} else if (!strcmp(s, "x")) {
		*symbol = NK_SYMBOL_X;
	} else if (!strcmp(s, "underscore")) {
		*symbol = NK_SYMBOL_UNDERSCORE;
	} else if (!strcmp(s, "circle solid")) {
		*symbol = NK_SYMBOL_CIRCLE_SOLID;
	} else if (!strcmp(s, "circle outline")) {
		*symbol = NK_SYMBOL_CIRCLE_OUTLINE;
	} else if (!strcmp(s, "rect solid")) {
		*symbol = NK_SYMBOL_RECT_SOLID;
	} else if (!strcmp(s, "rect outline")) {
		*symbol = NK_SYMBOL_RECT_OUTLINE;
	} else if (!strcmp(s, "triangle up")) {
		*symbol = NK_SYMBOL_TRIANGLE_UP;
	} else if (!strcmp(s, "triangle down")) {
		*symbol = NK_SYMBOL_TRIANGLE_DOWN;
	} else if (!strcmp(s, "triangle left")) {
		*symbol = NK_SYMBOL_TRIANGLE_LEFT;
	} else if (!strcmp(s, "triangle right")) {
		*symbol = NK_SYMBOL_TRIANGLE_RIGHT;
	} else if (!strcmp(s, "plus")) {
		*symbol = NK_SYMBOL_PLUS;
	} else if (!strcmp(s, "minus")) {
		*symbol = NK_SYMBOL_MINUS;
	} else if (!strcmp(s, "max")) {
		*symbol = NK_SYMBOL_MAX;
	} else {
		return 0;
	}
	return 1;
}

static int nk_love_parse_align(const char *s, nk_flags *align)
{
	if (!strcmp(s, "left")) {
		*align = NK_TEXT_LEFT;
	} else if (!strcmp(s, "centered")) {
		*align = NK_TEXT_CENTERED;
	} else if (!strcmp(s, "right")) {
		*align = NK_TEXT_RIGHT;
	} else if (!strcmp(s, "top left")) {
		*align = NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_LEFT;
	} else if (!strcmp(s, "top centered")) {
		*align = NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED;
	} else if (!strcmp(s, "top right")) {
		*align = NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_RIGHT;
	} else if (!strcmp(s, "bottom left")) {
		*align = NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_LEFT;
	} else if (!strcmp(s, "bottom centered")) {
		*align = NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED;
	} else if (!strcmp(s, "bottom right")) {
		*align = NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_RIGHT;
	} else {
		return 0;
	}
	return 1;
}

static int nk_love_parse_button(const char *s, enum nk_buttons *button)
{
	if (!strcmp(s, "left")) {
		*button = NK_BUTTON_LEFT;
	} else if (!strcmp(s, "right")) {
		*button = NK_BUTTON_RIGHT;
	} else if (!strcmp(s, "middle")) {
		*button = NK_BUTTON_MIDDLE;
	} else {
		return 0;
	}
	return 1;
}

static int nk_love_init(lua_State *luaState)
{
	L = luaState;
	nk_love_assert(lua_gettop(L) == 0, "nk.init: wrong number of arguments");
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_newtable(L);
	lua_setfield(L, -2, "font");
	lua_newtable(L);
	lua_setfield(L, -2, "image");
	lua_newtable(L);
	lua_setfield(L, -2, "stack");
	fonts = malloc(sizeof(struct nk_user_font) * NK_LOVE_MAX_FONTS);
	nk_love_assert(fonts != NULL, "nk.init: out of memory");
	lua_getglobal(L, "love");
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "nk.init: requires LOVE environment");
	lua_getfield(L, -1, "graphics");
	lua_getfield(L, -1, "getFont");
	lua_call(L, 0, 1);
	nk_love_tofont(&fonts[0]);
	nk_init_default(&context, &fonts[0]);
	font_count = 1;
	context.clip.copy = nk_love_clipbard_copy;
	context.clip.paste = nk_love_clipbard_paste;
	context.clip.userdata = nk_handle_ptr(0);
	edit_buffer = malloc(NK_LOVE_EDIT_BUFFER_LEN);
	nk_love_assert(edit_buffer != NULL, "nk.init: out of memory");
	combobox_items = malloc(sizeof(char*) * NK_LOVE_COMBOBOX_MAX_ITEMS);
	nk_love_assert(combobox_items != NULL, "nk.init: out of memory");
	layout_ratios = malloc(sizeof(float) * NK_LOVE_MAX_RATIOS);
	nk_love_assert(layout_ratios != NULL, "nk.init: out of memory");
	return 0;
}

static int nk_love_shutdown(lua_State *luaState)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.shutdown: wrong number of arguments");
	nk_free(&context);
	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "nuklear");
	L = NULL;
	free(fonts);
	fonts = NULL;
	free(edit_buffer);
	edit_buffer = NULL;
	free(combobox_items);
	combobox_items = NULL;
	free(layout_ratios);
	layout_ratios = NULL;
	return 0;
}

static int nk_love_keypressed(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 3, "nk.keypressed: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.keypressed: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.keypressed: arg 2 should be a string");
	nk_love_assert(lua_type(L, 3) == LUA_TBOOLEAN, "nk.keypressed: arg 3 should be a boolean");
	const char *key = lua_tostring(L, 1);
	const char *scancode = lua_tostring(L, 2);
	int isrepeat = lua_toboolean(L, 3);
	int consume = nk_love_keyevent(key, scancode, isrepeat, 1);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_keyreleased(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.keyreleased: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.keyreleased: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.keyreleased: arg 2 should be a string");
	const char *key = lua_tostring(L, 1);
	const char *scancode = lua_tostring(L, 2);
	int consume = nk_love_keyevent(key, scancode, 0, 0);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_mousepressed(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.mousepressed: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.mousepressed: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.mousepressed: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.mousepressed: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TBOOLEAN, "nk.mousepressed: arg 4 should be a boolean");
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int button = lua_tointeger(L, 3);
	int istouch = lua_toboolean(L, 4);
	int consume = nk_love_clickevent(x, y, button, istouch, 1);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_mousereleased(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.mousereleased: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.mousereleased: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.mousereleased: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.mousereleased: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TBOOLEAN, "nk.mousereleased: arg 4 should be a boolean");
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int button = lua_tointeger(L, 3);
	int istouch = lua_toboolean(L, 4);
	int consume = nk_love_clickevent(x, y, button, istouch, 0);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_mousemoved(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 5, "nk.mousemoved: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.mousemoved: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.mousemoved: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.mousemoved: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.mousemoved: arg 4 should be a number");
	nk_love_assert(lua_type(L, 5) == LUA_TBOOLEAN, "nk.mousemoved: arg 5 should be a boolean");
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int dx = lua_tointeger(L, 3);
	int dy = lua_tointeger(L, 4);
	int istouch = lua_toboolean(L, 5);
	int consume = nk_love_mousemoved_event(x, y, dx, dy, istouch);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_textinput(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.textinput: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.textinput: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	int consume = nk_love_textinput_event(text);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_wheelmoved(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.wheelmoved: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.wheelmoved: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.wheelmoved: arg 2 should be a number");
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int consume = nk_love_wheelmoved_event(x, y);
	lua_pushboolean(L, consume);
	return 1;
}

static int nk_love_draw(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.draw: wrong number of arguments");
	
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "graphics");
	
	lua_getfield(L, -1, "push");
	lua_pushstring(L, "all");
	lua_call(L, 1, 0);
	
	const struct nk_command *cmd;
	nk_foreach(cmd, &context)
	{
		switch (cmd->type) {
		case NK_COMMAND_NOP: break;
		case NK_COMMAND_SCISSOR: {
			const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
			nk_love_scissor(s->x, s->y, s->w, s->h);
		} break;
		case NK_COMMAND_LINE: {
			const struct nk_command_line *l = (const struct nk_command_line *)cmd;
			nk_love_draw_line(l->begin.x, l->begin.y, l->end.x,
				l->end.y, l->line_thickness, l->color);
		} break;
		case NK_COMMAND_RECT: {
			const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
			nk_love_draw_rect(r->x, r->y, r->w, r->h,
				(unsigned int)r->rounding, r->line_thickness, r->color);
		} break;
		case NK_COMMAND_RECT_FILLED: {
			const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
			nk_love_draw_rect(r->x, r->y, r->w, r->h, (unsigned int)r->rounding, -1, r->color);
		} break;
		case NK_COMMAND_CIRCLE: {
			const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
			nk_love_draw_circle(c->x, c->y, c->w, c->h, c->line_thickness, c->color);
		} break;
		case NK_COMMAND_CIRCLE_FILLED: {
			const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
			nk_love_draw_circle(c->x, c->y, c->w, c->h, -1, c->color);
		} break;
		case NK_COMMAND_TRIANGLE: {
			const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
			nk_love_draw_triangle(t->a.x, t->a.y, t->b.x, t->b.y,
				t->c.x, t->c.y, t->line_thickness, t->color);
		} break;
		case NK_COMMAND_TRIANGLE_FILLED: {
			const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
			nk_love_draw_triangle(t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y, -1, t->color);
		} break;
		case NK_COMMAND_POLYGON: {
			const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
			nk_love_draw_polygon(p->points, p->point_count, p->line_thickness, p->color);
		} break;
		case NK_COMMAND_POLYGON_FILLED: {
			const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled*)cmd;
			nk_love_draw_polygon(p->points, p->point_count, -1, p->color);
		} break;
		case NK_COMMAND_POLYLINE: {
			const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
			nk_love_draw_polyline(p->points, p->point_count, p->line_thickness, p->color);
		} break;
		case NK_COMMAND_TEXT: {
			const struct nk_command_text *t = (const struct nk_command_text*)cmd;
			nk_love_draw_text(t->font->userdata.id, t->background,
				t->foreground, t->x, t->y, t->w, t->h,
				t->height, t->length, (const char*)t->string);
		} break;
		case NK_COMMAND_CURVE: {
			const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
			nk_love_draw_curve(q->begin, q->ctrl[0], q->ctrl[1],
				q->end, 22, q->line_thickness, q->color);
		} break;
		case NK_COMMAND_RECT_MULTI_COLOR: {
			const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
			nk_love_draw_rect_multi_color(r->x, r->y, r->w, r->h, r->left, r->top, r->right, r->bottom);
		} break;
		case NK_COMMAND_IMAGE: {
			const struct nk_command_image *i = (const struct nk_command_image *)cmd;
			nk_love_draw_image(i->x, i->y, i->w, i->h, i->img, i->col);
		} break;
		case NK_COMMAND_ARC: {
			const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;
			nk_love_draw_arc(a->cx, a->cy, a->r, a->line_thickness,
				a->a[0], a->a[1], a->color);
		} break;
		case NK_COMMAND_ARC_FILLED: {
			const struct nk_command_arc_filled *a = (const struct nk_command_arc_filled *)cmd;
			nk_love_draw_arc(a->cx, a->cy, a->r, -1, a->a[0], a->a[1], a->color);
		} break;
		default: break;
		}
	}

	lua_getfield(L, -1, "pop");
	lua_call(L, 0, 0);
	lua_pop(L, 2);
	nk_clear(&context);
	return 0;
}

static void nk_love_preserve(struct nk_style_item *item)
{
	if (item->type == NK_STYLE_ITEM_IMAGE) {
		lua_rawgeti(L, -1, item->data.image.handle.id);
		nk_love_toimage(&item->data.image);
	}
}

static void nk_love_preserve_all(void)
{
	nk_love_preserve(&context.style.button.normal);
	nk_love_preserve(&context.style.button.hover);
	nk_love_preserve(&context.style.button.active);
	
	nk_love_preserve(&context.style.contextual_button.normal);
	nk_love_preserve(&context.style.contextual_button.hover);
	nk_love_preserve(&context.style.contextual_button.active);
	
	nk_love_preserve(&context.style.menu_button.normal);
	nk_love_preserve(&context.style.menu_button.hover);
	nk_love_preserve(&context.style.menu_button.active);
	
	nk_love_preserve(&context.style.option.normal);
	nk_love_preserve(&context.style.option.hover);
	nk_love_preserve(&context.style.option.active);
	nk_love_preserve(&context.style.option.cursor_normal);
	nk_love_preserve(&context.style.option.cursor_hover);
	
	nk_love_preserve(&context.style.checkbox.normal);
	nk_love_preserve(&context.style.checkbox.hover);
	nk_love_preserve(&context.style.checkbox.active);
	nk_love_preserve(&context.style.checkbox.cursor_normal);
	nk_love_preserve(&context.style.checkbox.cursor_hover);
	
	nk_love_preserve(&context.style.selectable.normal);
	nk_love_preserve(&context.style.selectable.hover);
	nk_love_preserve(&context.style.selectable.pressed);
	nk_love_preserve(&context.style.selectable.normal_active);
	nk_love_preserve(&context.style.selectable.hover_active);
	nk_love_preserve(&context.style.selectable.pressed_active);
	
	nk_love_preserve(&context.style.slider.normal);
	nk_love_preserve(&context.style.slider.hover);
	nk_love_preserve(&context.style.slider.active);
	nk_love_preserve(&context.style.slider.cursor_normal);
	nk_love_preserve(&context.style.slider.cursor_hover);
	nk_love_preserve(&context.style.slider.cursor_active);
	
	nk_love_preserve(&context.style.progress.normal);
	nk_love_preserve(&context.style.progress.hover);
	nk_love_preserve(&context.style.progress.active);
	nk_love_preserve(&context.style.progress.cursor_normal);
	nk_love_preserve(&context.style.progress.cursor_hover);
	nk_love_preserve(&context.style.progress.cursor_active);
	
	nk_love_preserve(&context.style.property.normal);
	nk_love_preserve(&context.style.property.hover);
	nk_love_preserve(&context.style.property.active);
	nk_love_preserve(&context.style.property.edit.normal);
	nk_love_preserve(&context.style.property.edit.hover);
	nk_love_preserve(&context.style.property.edit.active);
	nk_love_preserve(&context.style.property.inc_button.normal);
	nk_love_preserve(&context.style.property.inc_button.hover);
	nk_love_preserve(&context.style.property.inc_button.active);
	nk_love_preserve(&context.style.property.dec_button.normal);
	nk_love_preserve(&context.style.property.dec_button.hover);
	nk_love_preserve(&context.style.property.dec_button.active);

	nk_love_preserve(&context.style.edit.normal);
	nk_love_preserve(&context.style.edit.hover);
	nk_love_preserve(&context.style.edit.active);
	nk_love_preserve(&context.style.edit.scrollbar.normal);
	nk_love_preserve(&context.style.edit.scrollbar.hover);
	nk_love_preserve(&context.style.edit.scrollbar.active);
	nk_love_preserve(&context.style.edit.scrollbar.cursor_normal);
	nk_love_preserve(&context.style.edit.scrollbar.cursor_hover);
	nk_love_preserve(&context.style.edit.scrollbar.cursor_active);

	nk_love_preserve(&context.style.chart.background);

	nk_love_preserve(&context.style.scrollh.normal);
	nk_love_preserve(&context.style.scrollh.hover);
	nk_love_preserve(&context.style.scrollh.active);
	nk_love_preserve(&context.style.scrollh.cursor_normal);
	nk_love_preserve(&context.style.scrollh.cursor_hover);
	nk_love_preserve(&context.style.scrollh.cursor_active);

	nk_love_preserve(&context.style.scrollv.normal);
	nk_love_preserve(&context.style.scrollv.hover);
	nk_love_preserve(&context.style.scrollv.active);
	nk_love_preserve(&context.style.scrollv.cursor_normal);
	nk_love_preserve(&context.style.scrollv.cursor_hover);
	nk_love_preserve(&context.style.scrollv.cursor_active);

	nk_love_preserve(&context.style.tab.background);
	nk_love_preserve(&context.style.tab.tab_maximize_button.normal);
	nk_love_preserve(&context.style.tab.tab_maximize_button.hover);
	nk_love_preserve(&context.style.tab.tab_maximize_button.active);
	nk_love_preserve(&context.style.tab.tab_minimize_button.normal);
	nk_love_preserve(&context.style.tab.tab_minimize_button.hover);
	nk_love_preserve(&context.style.tab.tab_minimize_button.active);
	nk_love_preserve(&context.style.tab.node_maximize_button.normal);
	nk_love_preserve(&context.style.tab.node_maximize_button.hover);
	nk_love_preserve(&context.style.tab.node_maximize_button.active);
	nk_love_preserve(&context.style.tab.node_minimize_button.normal);
	nk_love_preserve(&context.style.tab.node_minimize_button.hover);
	nk_love_preserve(&context.style.tab.node_minimize_button.active);

	nk_love_preserve(&context.style.combo.normal);
	nk_love_preserve(&context.style.combo.hover);
	nk_love_preserve(&context.style.combo.active);
	nk_love_preserve(&context.style.combo.button.normal);
	nk_love_preserve(&context.style.combo.button.hover);
	nk_love_preserve(&context.style.combo.button.active);

	nk_love_preserve(&context.style.window.fixed_background);
	nk_love_preserve(&context.style.window.scaler);
	nk_love_preserve(&context.style.window.header.normal);
	nk_love_preserve(&context.style.window.header.hover);
	nk_love_preserve(&context.style.window.header.active);
	nk_love_preserve(&context.style.window.header.close_button.normal);
	nk_love_preserve(&context.style.window.header.close_button.hover);
	nk_love_preserve(&context.style.window.header.close_button.active);
	nk_love_preserve(&context.style.window.header.minimize_button.normal);
	nk_love_preserve(&context.style.window.header.minimize_button.hover);
	nk_love_preserve(&context.style.window.header.minimize_button.active);
}

static int nk_love_frame_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.frame_begin: wrong number of arguments");
	nk_input_end(&context);
	lua_getglobal(L, "love");
	lua_getfield(L, -1, "timer");
	lua_getfield(L, -1, "getDelta");
	lua_call(L, 0, 1);
	float dt = lua_tonumber(L, -1);
	context.delta_time_seconds = dt;
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "image");
	lua_newtable(L);
	lua_setfield(L, -3, "image");
	nk_love_preserve_all();
	lua_pop(L, 1);
	lua_getfield(L, -1, "font");
	lua_newtable(L);
	lua_setfield(L, -3, "font");
	font_count = 0;
	lua_rawgeti(L, -1, context.style.font->userdata.id);
	nk_love_tofont(&fonts[font_count]);
	context.style.font = &fonts[font_count++];
	int i;
	for (i = 0; i < context.stacks.fonts.head; ++i) {
		struct nk_config_stack_user_font_element *element = &context.stacks.fonts.elements[i];
		lua_rawgeti(L, -1, element->old_value->userdata.id);
		nk_love_tofont(&fonts[font_count]);
		context.stacks.fonts.elements[i].old_value = &fonts[font_count++];
	}
	layout_ratio_count = 0;
	return 0;
}

static int nk_love_frame_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.frame_end: wrong number of arguments");
	nk_input_begin(&context);
	return 0;
}

static int nk_love_window_begin(lua_State *L)
{
	const char *name, *title;
	int bounds_begin;
	if (lua_type(L, 2) == LUA_TNUMBER) {
		nk_love_assert(lua_gettop(L) >= 5, "nk.window_begin: wrong number of arguments");
		nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_begin: arg 1 should be a string");
		nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.window_begin: arg 3 should be a number");
		nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.window_begin: arg 4 should be a number");
		nk_love_assert(lua_type(L, 5) == LUA_TNUMBER, "nk.window_begin: arg 5 should be a number");
		name = title = lua_tostring(L, 1);
		bounds_begin = 2;
	} else {
		nk_love_assert(lua_gettop(L) >= 6, "nk.window_begin: wrong number of arguments");
		nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_begin: arg 1 should be a string");
		nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.window_begin: arg 2 should be a string");
		nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.window_begin: arg 3 should be a number");
		nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.window_begin: arg 4 should be a number");
		nk_love_assert(lua_type(L, 5) == LUA_TNUMBER, "nk.window_begin: arg 5 should be a number");
		nk_love_assert(lua_type(L, 6) == LUA_TNUMBER, "nk.window_begin: arg 6 should be a number");
		name = lua_tostring(L, 1);
		title = lua_tostring(L, 2);
		bounds_begin = 3;
	}
	nk_flags flags = nk_love_parse_window_flags(bounds_begin + 4);
	float x = lua_tonumber(L, bounds_begin);
	float y = lua_tonumber(L, bounds_begin + 1);
	float width = lua_tonumber(L, bounds_begin + 2);
	float height = lua_tonumber(L, bounds_begin + 3);
	int open = nk_begin_titled(&context, name, title, nk_rect(x, y, width, height), flags);
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_window_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_end: wrong number of arguments");
	nk_end(&context);
	return 0;
}

static int nk_love_window_get_bounds(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_get_bounds: wrong number of arguments");
	struct nk_rect rect = nk_window_get_bounds(&context);
	lua_pushnumber(L, rect.x);
	lua_pushnumber(L, rect.y);
	lua_pushnumber(L, rect.w);
	lua_pushnumber(L, rect.h);
	return 4;
}

static int nk_love_window_get_position(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_get_position: wrong number of arguments");
	struct nk_vec2 pos = nk_window_get_position(&context);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	return 2;
}

static int nk_love_window_get_size(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_get_size: wrong number of arguments");
	struct nk_vec2 size = nk_window_get_size(&context);
	lua_pushnumber(L, size.x);
	lua_pushnumber(L, size.y);
	return 2;
}

static int nk_love_window_get_content_region(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_get_content_region: wrong number of arguments");
	struct nk_rect rect = nk_window_get_content_region(&context);
	lua_pushnumber(L, rect.x);
	lua_pushnumber(L, rect.y);
	lua_pushnumber(L, rect.w);
	lua_pushnumber(L, rect.h);
	return 4;
}

static int nk_love_window_has_focus(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_has_focus: wrong number of arguments");
	int has_focus = nk_window_has_focus(&context);
	lua_pushboolean(L, has_focus);
	return 1;
}

static int nk_love_window_is_collapsed(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_is_collapsed: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_is_collapsed: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	int is_collapsed = nk_window_is_collapsed(&context, name);
	lua_pushboolean(L, is_collapsed);
	return 1;
}

static int nk_love_window_is_hidden(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_is_hidden: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_is_hidden: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	int is_hidden = nk_window_is_hidden(&context, name);
	lua_pushboolean(L, is_hidden);
	return 1;
}

static int nk_love_window_is_active(lua_State *L) {
	nk_love_assert(lua_gettop(L) == 1, "nk.window_is_active: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_is_active: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	int is_active = nk_window_is_active(&context, name);
	lua_pushboolean(L, is_active);
	return 1;
}

static int nk_love_window_is_hovered(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_is_hovered: wrong number of arguments");
	int is_hovered = nk_window_is_hovered(&context);
	lua_pushboolean(L, is_hovered);
	return 1;
}

static int nk_love_window_is_any_hovered(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.window_is_any_hovered: wrong number of arguments");
	int is_any_hovered = nk_window_is_any_hovered(&context);
	lua_pushboolean(L, is_any_hovered);
	return 1;
}

static int nk_love_item_is_any_active(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.item_is_any_active: wrong number of arguments");
	lua_pushboolean(L, nk_love_is_active(&context));
	return 1;
}

static int nk_love_window_set_bounds(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.window_set_bounds: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.window_set_bounds: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.window_set_bounds: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.window_set_bounds: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.window_set_bounds: arg 4 should be a number");
	struct nk_rect bounds;
	bounds.x = lua_tonumber(L, 1);
	bounds.y = lua_tonumber(L, 2);
	bounds.w = lua_tonumber(L, 3);
	bounds.h = lua_tonumber(L, 4);
	nk_window_set_bounds(&context, bounds);
	return 0;
}

static int nk_love_window_set_position(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.window_set_position: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.window_set_position: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.window_set_position: arg 2 should be a number");
	struct nk_vec2 pos;
	pos.x = lua_tonumber(L, 1);
	pos.y = lua_tonumber(L, 2);
	nk_window_set_position(&context, pos);
	return 0;
}

static int nk_love_window_set_size(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.window_set_size: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.window_set_size: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.window_set_size: arg 2 should be a number");
	struct nk_vec2 size;
	size.x = lua_tonumber(L, 1);
	size.y = lua_tonumber(L, 2);
	nk_window_set_size(&context, size);
	return 0;
}

static int nk_love_window_set_focus(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_set_focus: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_set_focus: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	nk_window_set_focus(&context, name);
	return 0;
}

static int nk_love_window_close(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_close: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_close: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	nk_window_close(&context, name);
	return 0;
}

static int nk_love_window_collapse(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_collapse: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_collapse: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	nk_window_collapse(&context, name, NK_MINIMIZED);
	return 0;
}

static int nk_love_window_expand(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_expand: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_expand: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	nk_window_collapse(&context, name, NK_MAXIMIZED);
	return 0;
}

static int nk_love_window_show(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_show: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_show: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	nk_window_show(&context, name, NK_SHOWN);
	return 0;
}

static int nk_love_window_hide(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.window_hide: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.window_hide: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	nk_window_show(&context, name, NK_HIDDEN);
	return 0;
}

static int nk_love_layout_row(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 3 || argc == 4, "nk.layout_row: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.layout_row: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_row: arg 2 should be a number");
	const char *type = lua_tostring(L, 1);
	float height = lua_tonumber(L, 2);
	enum nk_layout_format format;
	int use_ratios = 0;
	if (!strcmp(type, "dynamic")) {
		nk_love_assert(argc == 3, "nk.layout_row: wrong number of arguments");
		if (lua_type(L, 3) == LUA_TNUMBER) {
			int cols = lua_tointeger(L, 3);
			nk_layout_row_dynamic(&context, height, cols);
		} else {
			nk_love_assert(lua_type(L, 3) == LUA_TTABLE, "nk.layout_row: arg 3 should be a number or table");
			format = NK_DYNAMIC;
			use_ratios = 1;
		}
	} else if (!strcmp(type, "static")) {
		if (argc == 4) {
			nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.layout_row: arg 3 should be a number");
			nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.layout_row: arg 4 should be a number");
			int item_width = lua_tointeger(L, 3);
			int cols = lua_tointeger(L, 4);
			nk_layout_row_static(&context, height, item_width, cols);
		} else {
			nk_love_assert(lua_type(L, 3) == LUA_TTABLE, "nk.layout_row: arg 3 should be a number or table");
			format = NK_STATIC;
			use_ratios = 1;
		}
	} else {
		nk_love_error("nk.layout_row: arg 1 should be 'dynamic' or 'static'");
	}
	if (use_ratios) {
		int cols = lua_objlen(L, -1);
		int i, j;
		for (i = 1, j = layout_ratio_count; i <= cols && j < NK_LOVE_MAX_RATIOS; ++i, ++j) {
			lua_rawgeti(L, -1, i);
			layout_ratios[j] = lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
		nk_layout_row(&context, format, height, cols, layout_ratios + layout_ratio_count);
		layout_ratio_count += cols;
	}
	return 0;
}

static int nk_love_layout_row_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 3, "nk.layout_row_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.layout_row_begin: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_row_begin: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.layout_row_begin: arg 3 should be a number");
	const char *type = lua_tostring(L, 1);
	float height = lua_tonumber(L, 2);
	int cols = lua_tointeger(L, 3);
	enum nk_layout_format format;
	if (!strcmp(type, "dynamic")) {
		format = NK_DYNAMIC;
	} else if (!strcmp(type, "static")) {
		format = NK_STATIC;
	} else {
		nk_love_error("nk.layout_row_begin: arg 1 should be 'dynamic' or 'static'");
	}
	nk_layout_row_begin(&context, format, height, cols);
	return 0;
}

static int nk_love_layout_row_push(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.layout_row_push: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_row_push: arg 1 should be a number");
	float value = lua_tonumber(L, 1);
	nk_layout_row_push(&context, value);
	return 0;
}

static int nk_love_layout_row_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.layout_row_end: wrong number of arguments");
	nk_layout_row_end(&context);
	return 0;
}

static int nk_love_layout_space_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 3, "nk.layout_space_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.layout_space_begin: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_space_begin: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.layout_space_begin: arg 3 should be a number");
	const char *type = lua_tostring(L, 1);
	float height = lua_tonumber(L, 2);
	int widget_count = lua_tointeger(L, 3);
	enum nk_layout_format format;
	if (!strcmp(type, "dynamic")) {
		format = NK_LAYOUT_DYNAMIC;
	} else if (!strcmp(type, "static")) {
		format = NK_LAYOUT_STATIC;
	} else {
		nk_love_error("nk.layout_space_begin: arg 1 should be 'dynamic' or 'static'");
	}
	nk_layout_space_begin(&context, format, height, widget_count);
	return 0;
}

static int nk_love_layout_space_push(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.layout_space_push: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_space_push: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_space_push: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.layout_space_push: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.layout_space_push: arg 4 should be a number");
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float width = lua_tonumber(L, 3);
	float height = lua_tonumber(L, 4);
	nk_layout_space_push(&context, nk_rect(x, y, width, height));
	return 0;
}

static int nk_love_layout_space_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.layout_space_end: wrong number of arguments");
	nk_layout_space_end(&context);
	return 0;
}

static int nk_love_layout_space_bounds(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.layout_space_bounds: wrong number of arguments");
	struct nk_rect bounds = nk_layout_space_bounds( &context);
	lua_pushnumber(L, bounds.x);
	lua_pushnumber(L, bounds.y);
	lua_pushnumber(L, bounds.w);
	lua_pushnumber(L, bounds.h);
	return 4;
}

static int nk_love_layout_space_to_screen(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.layout_space_to_screen: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_space_to_screen: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_space_to_screen: arg 2 should be a number");
	struct nk_vec2 local;
	local.x = lua_tonumber(L, 1);
	local.y = lua_tonumber(L, 2);
	struct nk_vec2 screen = nk_layout_space_to_screen(&context, local);
	lua_pushnumber(L, screen.x);
	lua_pushnumber(L, screen.y);
	return 2;
}

static int nk_love_layout_space_to_local(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.layout_space_to_local: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_space_to_local: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_space_to_local: arg 2 should be a number");
	struct nk_vec2 screen;
	screen.x = lua_tonumber(L, 1);
	screen.y = lua_tonumber(L, 2);
	struct nk_vec2 local = nk_layout_space_to_local(&context, screen);
	lua_pushnumber(L, local.x);
	lua_pushnumber(L, local.y);
	return 2;
}

static int nk_love_layout_space_rect_to_screen(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.layout_space_rect_to_screen: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_space_rect_to_screen: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_space_rect_to_screen: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.layout_space_rect_to_screen: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.layout_space_rect_to_screen: arg 4 should be a number");
	struct nk_rect local;
	local.x = lua_tonumber(L, 1);
	local.y = lua_tonumber(L, 2);
	local.w = lua_tonumber(L, 3);
	local.h = lua_tonumber(L, 4);
	struct nk_rect screen = nk_layout_space_rect_to_screen(&context, local);
	lua_pushnumber(L, screen.x);
	lua_pushnumber(L, screen.y);
	lua_pushnumber(L, screen.w);
	lua_pushnumber(L, screen.h);
	return 4;
}

static int nk_love_layout_space_rect_to_local(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.layout_space_rect_to_local: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_space_rect_to_local: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.layout_space_rect_to_local: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.layout_space_rect_to_local: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.layout_space_rect_to_local: arg 4 should be a number");
	struct nk_rect screen;
	screen.x = lua_tonumber(L, 1);
	screen.y = lua_tonumber(L, 2);
	screen.w = lua_tonumber(L, 3);
	screen.h = lua_tonumber(L, 4);
	struct nk_rect local = nk_layout_space_rect_to_screen(&context, screen);
	lua_pushnumber(L, local.x);
	lua_pushnumber(L, local.y);
	lua_pushnumber(L, local.w);
	lua_pushnumber(L, local.h);
	return 4;
}

static int nk_love_layout_ratio_from_pixel(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.layout_ratio_from_pixel: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.layout_ratio_from_pixel: arg 1 should be a number");
	float pixel_width = lua_tonumber(L, 1);
	float ratio = nk_layout_ratio_from_pixel(&context, pixel_width);
	lua_pushnumber(L, ratio);
	return 1;
}

static int nk_love_group_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) >= 1, "nk.group_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.group_begin: arg 1 should be a string");
	const char *title = lua_tostring(L, 1);
	nk_flags flags = nk_love_parse_window_flags(2);
	int open = nk_group_begin(&context, title, flags);
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_group_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.group_end: wrong number of arguments");
	nk_group_end(&context);
	return 0;
}

static int nk_love_tree_push(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 2 && argc <= 4, "nk.tree_push: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.tree_push: arg 1 should be a string");
	const char *type_string = lua_tostring(L, 1);
	enum nk_tree_type type;
	if (!strcmp(type_string, "node")) {
		type = NK_TREE_NODE;
	} else if (!strcmp(type_string, "tab")) {
		type = NK_TREE_TAB;
	} else {
		nk_love_error("nk.tree_push: arg 1 should be 'node' or 'tab'");
	}
	nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.tree_push: arg 2 should be a string");
	const char *title = lua_tostring(L, 2);
	struct nk_image image;
	int use_image = 0;
	if (argc >= 3 && lua_type(L, 3) != LUA_TNIL) {
		nk_love_assert_type(3, "Image", "nk.tree_push: arg 3 should be an image");
		lua_pushvalue(L, 3);
		nk_love_toimage(&image);
		use_image = 1;
	}
	const char *state_string = "collapsed";
	if (argc >= 4) {
		nk_love_assert(lua_type(L, 4) == LUA_TSTRING, "nk.tree_push: arg 4 should be a string");
		state_string = lua_tostring(L, 4);
	}
	enum nk_collapse_states state;
	if (!strcmp(state_string, "collapsed")) {
		state = NK_MINIMIZED;
	} else if (!strcmp(state_string, "expanded")) {
		state = NK_MAXIMIZED;
	} else {
		nk_love_error("nk.tree_push: arg 4 should be 'collapsed' or 'expanded'");
	}
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "l", &ar);
	int id = ar.currentline;
	int open = 0;
	if (use_image) {
		open = nk_tree_image_push_hashed(&context, type, image, title, state, title, strlen(title), id);
	} else {
		open = nk_tree_push_hashed(&context, type, title, state, title, strlen(title), id);
	}
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_tree_pop(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.tree_pop: wrong number of arguments");
	nk_tree_pop(&context);
	return 0;
}

static void nk_love_color(int r, int g, int b, int a, char *color_string)
{
	r = NK_CLAMP(0, r, 255);
	g = NK_CLAMP(0, g, 255);
	b = NK_CLAMP(0, b, 255);
	a = NK_CLAMP(0, a, 255);
	const char *format_string;
	if (a < 255) {
		format_string = "#%02x%02x%02x%02x";
	} else {
		format_string = "#%02x%02x%02x";
	}
	sprintf(color_string, format_string, r, g, b, a);
}

static int nk_love_color_rgba(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 3 || argc == 4, "nk.color_rgba: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.color_rgba: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.color_rgba: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.color_rgba: arg 3 should be a number");

	int r = lua_tointeger(L, 1);
	int g = lua_tointeger(L, 2);
	int b = lua_tointeger(L, 3);
	int a = 255;
	if (argc == 4) {
		nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.color_rgba: arg 4 should be a number");
		a = lua_tointeger(L, 4);
	}

	char color_string[10];
	nk_love_color(r, g, b, a, color_string);
	lua_pushstring(L, color_string);
	return 1;
}

static int nk_love_color_hsva(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 3 || argc == 4, "nk.color_hsva: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.color_hsva: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.color_hsva: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.color_hsva: arg 3 should be a number");

	int h = NK_CLAMP(0, lua_tointeger(L, 1), 255);
	int s = NK_CLAMP(0, lua_tointeger(L, 2), 255);
	int v = NK_CLAMP(0, lua_tointeger(L, 3), 255);
	int a = 255;
	if (argc == 4) {
		nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.color_hsva: arg 4 should be a number");
		a = NK_CLAMP(0, lua_tointeger(L, 4), 255);
	}
	
	struct nk_color rgba = nk_hsva(h, s, v, a);
	char color_string[10];
	nk_love_color(rgba.r, rgba.g, rgba.b, rgba.a, color_string);
	lua_pushstring(L, color_string);
	return 1;
}

static struct nk_color nk_love_color_parse(const char *color_string)
{
	int r, g, b, a = 255;
	sscanf(color_string, "#%02x%02x%02x", &r, &g, &b);
	if (strlen(color_string) == 9) {
		sscanf(color_string + 7, "%02x", &a);
	}
	struct nk_color color = {r, g, b, a};
	return color;
}


static int nk_love_color_parse_rgba(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.color_parse_rgba: wrong number of arguments");
	nk_love_assert_color(1, "nk.color_parse_rgba: arg 1 should be a color string");
	const char *color_string = lua_tostring(L, 1);
	struct nk_color rgba = nk_love_color_parse(color_string);
	lua_pushnumber(L, rgba.r);
	lua_pushnumber(L, rgba.g);
	lua_pushnumber(L, rgba.b);
	lua_pushnumber(L, rgba.a);
	return 4;
}

static int nk_love_color_parse_hsva(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.color_parse_hsva: wrong number of arguments");
	nk_love_assert_color(1, "nk.color_parse_hsva: arg 1 should be a color string");
	const char *color_string = lua_tostring(L, 1);
	struct nk_color rgba = nk_love_color_parse(color_string);
	int h, s, v, a2;
	nk_color_hsva_i(&h, &s, &v, &a2, rgba);
	lua_pushnumber(L, h);
	lua_pushnumber(L, s);
	lua_pushnumber(L, v);
	lua_pushnumber(L, a2);
	return 4;
}

static int nk_love_label(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 1 && argc <= 3, "nk.label: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING || lua_type(L, 1) == LUA_TNUMBER, "nk.label: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	const char *align_string = "left";
	const char *color_string = NULL;
	if (argc >= 2) {
		nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.label: arg 2 should be a string");
		align_string = lua_tostring(L, 2);
		if (argc >= 3) {
			nk_love_assert_color(3, "nk.label: arg 3 should be a color string");
			color_string = lua_tostring(L, 3);
		}
	}
	nk_flags align;
	int wrap = 0;
	if (!strcmp(align_string, "wrap")) {
		wrap = 1;
	} else if (!nk_love_parse_align(align_string, &align)) {
		nk_love_error("nk.label: arg 2 should be an alignment or 'wrap'");
	}

	struct nk_color color;
	if (color_string != NULL) {
		color = nk_love_color_parse(color_string);
		if (wrap) {
			nk_label_colored_wrap(&context, text, color);
		} else {
			nk_label_colored(&context, text, align, color);
		}
	} else {
		if (wrap) {
			nk_label_wrap(&context, text);
		} else {
			nk_label(&context, text, align);
		}
	}
	return 0;
}

static int nk_love_image(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.image: wrong number of arguments");
	nk_love_assert_type(1, "Image", "nk.image: arg 1 should be an image");
	struct nk_image image;
	nk_love_toimage(&image);
	nk_image(&context, image);
	return 0;
}

static int nk_love_button(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 1 || argc == 2, "nk.button: wrong number of arguments");
	const char *title = NULL;
	if (lua_type(L, 1) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.button: arg 1 should be a string");
		title = lua_tostring(L, 1);
	}
	int use_color = 0, use_image = 0;
	struct nk_color color;
	enum nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	if (argc >= 2 && lua_type(L, 2) != LUA_TNIL) {
		if (lua_type(L, 2) == LUA_TSTRING) {
			const char *s = lua_tostring(L, 2);
			if (!nk_love_parse_symbol(s, &symbol)) {
				nk_love_assert_color(2, "nk.button: arg 2 should be a color string, symbol type, or image");
				color = nk_love_color_parse(s);
				use_color = 1;
			}
		} else {
			nk_love_assert_type(2, "Image", "nk.button: arg 2 should be a color string, symbol type, or image");
			lua_pushvalue(L, 2);
			nk_love_toimage(&image);
			use_image = 1;
		}
	}
	nk_flags align = context.style.button.text_alignment;
	int activated = 0;
	if (title != NULL) {
		if (use_color) {
			nk_love_error("nk.button: color buttons can't have titles");
		} else if (symbol != NK_SYMBOL_NONE) {
			activated = nk_button_symbol_label(&context, symbol, title, align);
		} else if (use_image) {
			activated = nk_button_image_label(&context, image, title, align);
		} else {
			activated = nk_button_label(&context, title);
		}
	} else {
		if (use_color) {
			activated = nk_button_color(&context, color);
		} else if (symbol != NK_SYMBOL_NONE) {
			activated = nk_button_symbol(&context, symbol);
		} else if (use_image) {
			activated = nk_button_image(&context, image);
		} else {
			nk_love_error("nk.button: must specify a title, color, symbol, and/or image");
		}
	}
	lua_pushboolean(L, activated);
	return 1;
}

static int nk_love_button_set_behavior(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.button_set_behavior: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.button_set_behavior: arg 1 should be 'default' or 'repeater'");
	const char *behavior_string = lua_tostring(L, 1);
	enum nk_button_behavior behavior;
	if (!strcmp(behavior_string, "default")) {
		behavior = NK_BUTTON_DEFAULT;
	} else if (!strcmp(behavior_string, "repeater")) {
		behavior = NK_BUTTON_REPEATER;
	} else {
		nk_love_error("nk.button_set_behavior: arg 1 should be 'default' or 'repeater'");
	}
	nk_button_set_behavior(&context, behavior);
	return 0;
}

static int nk_love_button_push_behavior(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.button_push_behavior: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.button_push_behavior: arg 1 should be 'default' or 'repeater'");
	const char *behavior_string = lua_tostring(L, 1);
	enum nk_button_behavior behavior;
	if (!strcmp(behavior_string, "default")) {
		behavior = NK_BUTTON_DEFAULT;
	} else if (!strcmp(behavior_string, "repeater")) {
		behavior = NK_BUTTON_REPEATER;
	} else {
		nk_love_error("nk.button_push_behavior: arg 1 should be 'default' or 'repeater'");
	}
	nk_button_push_behavior(&context, behavior);
	return 0;
}

static int nk_love_button_pop_behavior(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.button_pop_behavior: wrong number of arguments");
	nk_button_pop_behavior(&context);
	return 0;
}

static int nk_love_checkbox(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.checkbox: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.checkbox: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	if (lua_type(L, 2) == LUA_TBOOLEAN) {
		int value = lua_toboolean(L, 2);
		value = nk_check_label(&context, text, value);
		lua_pushboolean(L, value);
	} else if (lua_type(L, 2) == LUA_TTABLE) {
		lua_getfield(L, 2, "value");
		int value = lua_toboolean(L, -1);
		int changed = nk_checkbox_label(&context, text, &value);
		if (changed) {
			lua_pushboolean(L, value);
			lua_setfield(L, 2, "value");
		}
		lua_pushboolean(L, changed);
	} else {
		nk_love_error("nk.checkbox: arg 2 should be a boolean or table");
	}
	return 1;
}

static int nk_love_radio(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 2 || argc == 3, "nk.radio: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.radio: arg 1 should be a string");
	const char *name = lua_tostring(L, 1);
	const char *text;
	if (argc == 3) {
		nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.radio: arg 2 should be a string");
		text = lua_tostring(L, 2);
	} else {
		text = lua_tostring(L, 1);
	}
	if (lua_type(L, -1) == LUA_TSTRING) {
		const char *value = lua_tostring(L, -1);
		int active = !strcmp(value, name);
		active = nk_option_label(&context, text, active);
		if (active) {
			lua_pushstring(L, name);
		} else {
			lua_pushstring(L, value);
		}
	} else if (lua_type(L, -1) == LUA_TTABLE) {
		lua_getfield(L, -1, "value");
		const char *value = lua_tostring(L, -1);
		int active = !strcmp(value, name);
		int changed = nk_radio_label(&context, text, &active);
		if (changed && active) {
			lua_pushstring(L, name);
			lua_setfield(L, -3, "value");
		}
		lua_pushboolean(L, changed);
	} else {
		if (argc == 2) {
			nk_love_error("nk.radio: arg 2 should be a boolean or table");
		} else {
			nk_love_error("nk.radio: arg 3 should be a boolean or table");
		}
	}
	return 1;
}

static int nk_love_selectable(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 2 && argc <= 4, "nk.selectable: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.selectable: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	struct nk_image image;
	int use_image = 0;
	if (argc >= 3 && lua_type(L, 2) != LUA_TNIL) {
		nk_love_assert_type(2, "Image", "nk.selectable: arg 2 should be an image");
		lua_pushvalue(L, 2);
		nk_love_toimage(&image);
		use_image = 1;
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 4) {
		nk_love_assert(lua_type(L, 3) == LUA_TSTRING, "nk.selectable: arg 3 should be a string");
		const char *align_text = lua_tostring(L, 3);
		if (!nk_love_parse_align(align_text, &align)) {
			nk_love_error("nk.selectable: arg 3 should be an alignment");
		}
	}
	if (lua_type(L, -1) == LUA_TBOOLEAN) {
		int value = lua_toboolean(L, -1);
		if (use_image) {
			value = nk_select_image_label(&context, image, text, align, value);
		} else {
			value = nk_select_label(&context, text, align, value);
		}
		lua_pushboolean(L, value);
	} else if (lua_type(L, -1) == LUA_TTABLE) {
		lua_getfield(L, -1, "value");
		int value = lua_toboolean(L, -1);
		int changed;
		if (use_image) {
			changed = nk_selectable_image_label(&context, image, text, align, &value);
		} else {
			changed = nk_selectable_label(&context, text, align, &value);
		}
		if (changed) {
			lua_pushboolean(L, value);
			lua_setfield(L, -3, "value");
		}
		lua_pushboolean(L, changed);
	} else {
		if (argc == 2) {
			nk_love_error("nk.selectable: arg 2 should be a boolean or table");
		} else if (argc == 3) {
			nk_love_error("nk.selectable: arg 3 should be a boolean or table");
		} else {
			nk_love_error("nk.selectable: arg 4 should be a boolean or table");
		}
	}
	return 1;
}

static int nk_love_slider(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 4, "nk.slider: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.slider: arg 1 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.slider: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.slider: arg 4 should be a number");
	float min = lua_tonumber(L, 1);
	float max = lua_tonumber(L, 3);
	float step = lua_tonumber(L, 4);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		float value = lua_tonumber(L, 2);
		value = nk_slide_float(&context, min, value, max, step);
		lua_pushnumber(L, value);
	} else if (lua_type(L, 2) == LUA_TTABLE) {
		lua_getfield(L, 2, "value");
		float value = lua_tonumber(L, -1);
		int changed = nk_slider_float(&context, min, &value, max, step);
		if (changed) {
			lua_pushnumber(L, value);
			lua_setfield(L, 2, "value");
		}
		lua_pushboolean(L, changed);
	} else {
		nk_love_error("nk.slider: arg 2 should be a number or table");
	}
	return 1;
}

static int nk_love_progress(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 2 || argc == 3, "nk.progress: wrong number of arguments");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.progress: arg 2 should be a number");
	nk_size max = lua_tonumber(L, 2);
	int modifiable = 0;
	if (argc == 3) {
		nk_love_assert(lua_type(L, 3) == LUA_TBOOLEAN || lua_type(L, 3) == LUA_TNIL, "nk.progress: arg 3 should be a boolean");
		modifiable = lua_toboolean(L, 3);
	}
	if (lua_type(L, 1) == LUA_TNUMBER) {
		nk_size value = lua_tonumber(L, 1);
		value = nk_prog(&context, value, max, modifiable);
		lua_pushnumber(L, value);
	} else if (lua_type(L, 1) == LUA_TTABLE) {
		lua_getfield(L, 1, "value");
		nk_size value = lua_tonumber(L, -1);
		int changed = nk_progress(&context, &value, max, modifiable);
		if (changed) {
			lua_pushnumber(L, value);
			lua_setfield(L, 1, "value");
		}
		lua_pushboolean(L, changed);
	} else {
		nk_love_error("nk.progress: arg 2 should be a number or table");
	}
	return 1;
}

static int nk_love_color_picker(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 1 || argc == 2, "nk.color_picker: wrong number of arguments");
	const char *format_string = "RGB";
	if (argc == 2) {
		nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.color_picker: arg 2 should be a string");
		format_string = lua_tostring(L, 2);
	}
	enum nk_color_format format;
	if (!strcmp(format_string, "RGB")) {
		format = NK_RGB;
	} else if (!strcmp(format_string, "RGBA")) {
		format = NK_RGBA;
	} else {
		nk_love_error("nk.color_picker: arg 2 should be 'RGB' or 'RGBA'");
	}
	if (lua_type(L, 1) == LUA_TSTRING) {
		nk_love_assert_color(1, "nk.color_picker: arg 1 should be a color string");
		const char *color_string = lua_tostring(L, 1);
		struct nk_color color = nk_love_color_parse(color_string);
		color = nk_color_picker(&context, color, format);
		char new_color_string[10];
		nk_love_color(color.r, color.g, color.b, color.a, new_color_string);
		lua_pushstring(L, new_color_string);
	} else if (lua_type(L, 1) == LUA_TTABLE) {
		lua_getfield(L, 1, "value");
		nk_love_assert_color(-1, "nk.color_picker: arg 1 should have a color string value");
		const char *color_string = lua_tostring(L, -1);
		struct nk_color color = nk_love_color_parse(color_string);
		int changed = nk_color_pick(&context, &color, format);
		if (changed) {
			char new_color_string[10];
			nk_love_color(color.r, color.g, color.b, color.a, new_color_string);
			lua_pushstring(L, new_color_string);
			lua_setfield(L, 1, "value");
		}
		lua_pushboolean(L, changed);
	}
	return 1;
}

static int nk_love_property(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 6, "nk.property: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.property: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.property: arg 2 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.property: arg 4 should be a number");
	nk_love_assert(lua_type(L, 5) == LUA_TNUMBER, "nk.property: arg 5 should be a number");
	nk_love_assert(lua_type(L, 6) == LUA_TNUMBER, "nk.property: arg 6 should be a number");
	const char *name = lua_tostring(L, 1);
	double min = lua_tonumber(L, 2);
	double max = lua_tonumber(L, 4);
	double step = lua_tonumber(L, 5);
	float inc_per_pixel = lua_tonumber(L, 6);
	if (lua_type(L, 3) == LUA_TNUMBER) {
		double value = lua_tonumber(L, 3);
		value = nk_propertyd(&context, name, min, value, max, step, inc_per_pixel);
		lua_pushnumber(L, value);
	} else if (lua_type(L, 3) == LUA_TTABLE) {
		lua_getfield(L, 3, "value");
		nk_love_assert(lua_type(L, -1) == LUA_TNUMBER, "nk.property: arg 3 should have a number value");
		double value = lua_tonumber(L, -1);
		double old = value;
		nk_property_double(&context, name, min, &value, max, step, inc_per_pixel);
		int changed = value != old;
		if (changed) {
			lua_pushnumber(L, value);
			lua_setfield(L, 3, "value");
		}
		lua_pushboolean(L, changed);
	} else {
		nk_love_error("nk.property: arg 3 should be a number or table");
	}
	return 1;
}

static int nk_love_edit(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 2, "nk.edit: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.edit: arg 1 should be a string");
	const char *type_string = lua_tostring(L, 1);
	nk_flags flags;
	if (!strcmp(type_string, "simple")) {
		flags = NK_EDIT_SIMPLE;
	} else if (!strcmp(type_string, "field")) {
		flags = NK_EDIT_FIELD;
	} else if (!strcmp(type_string, "box")) {
		flags = NK_EDIT_BOX;
	} else {
		nk_love_error("nk.edit: arg 1 must be an editor type");
	}
	nk_love_assert(lua_type(L, 2) == LUA_TTABLE, "nk.edit: arg 2 should be a table");
	lua_getfield(L, 2, "value");
	nk_love_assert(lua_type(L, -1) == LUA_TSTRING, "nk.edit: arg 2 should have a string value");
	const char *value = lua_tostring(L, -1);
	size_t len = NK_CLAMP(0, strlen(value), NK_LOVE_EDIT_BUFFER_LEN - 1);
	memcpy(edit_buffer, value, len);
	edit_buffer[len] = '\0';
	nk_flags event = nk_edit_string_zero_terminated(&context, flags, edit_buffer, NK_LOVE_EDIT_BUFFER_LEN - 1, nk_filter_default);
	lua_pushstring(L, edit_buffer);
	lua_pushvalue(L, -1);
	lua_setfield(L, 2, "value");
	int changed = !lua_equal(L, -1, -2);
	if (event & NK_EDIT_COMMITED) {
		lua_pushstring(L, "commited");
	} else if (event & NK_EDIT_ACTIVATED) {
		lua_pushstring(L, "activated");
	} else if (event & NK_EDIT_DEACTIVATED) {
		lua_pushstring(L, "deactivated");
	} else if (event & NK_EDIT_ACTIVE) {
		lua_pushstring(L, "active");
	} else if (event & NK_EDIT_INACTIVE) {
		lua_pushstring(L, "inactive");
	} else {
		lua_pushnil(L);
	}
	lua_pushboolean(L, changed);
	return 2;
}

static int nk_love_popup_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) >= 6, "nk.popup_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.popup_begin: arg 1 should be a string");
	nk_love_assert(lua_type(L, 2) == LUA_TSTRING, "nk.popup_begin: arg 2 should be a string");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.popup_begin: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.popup_begin: arg 4 should be a number");
	nk_love_assert(lua_type(L, 5) == LUA_TNUMBER, "nk.popup_begin: arg 5 should be a number");
	nk_love_assert(lua_type(L, 6) == LUA_TNUMBER, "nk.popup_begin: arg 6 should be a number");
	const char *type_string = lua_tostring(L, 1);
	enum nk_popup_type type;
	if (!strcmp(type_string, "dynamic")) {
		type = NK_POPUP_DYNAMIC;
	} else if (!strcmp(type_string, "static")) {
		type = NK_POPUP_STATIC;
	} else {
		nk_love_error("nk.popup_begin: arg 1 should be 'dynamic' or 'static'");
	}
	const char *title = lua_tostring(L, 2);
	struct nk_rect bounds;
	bounds.x = lua_tonumber(L, 3);
	bounds.y = lua_tonumber(L, 4);
	bounds.w = lua_tonumber(L, 5);
	bounds.h = lua_tonumber(L, 6);
	nk_flags flags = nk_love_parse_window_flags(7);
	int open = nk_popup_begin(&context, type, title, flags, bounds);
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_popup_close(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.popup_close: wrong number of arguments");
	nk_popup_close(&context);
	return 0;
}

static int nk_love_popup_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.popup_end: wrong number of arguments");
	nk_popup_end(&context);
	return 0;
}

static int nk_love_combobox(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 2 && argc <= 5, "nk.combobox: wrong number of arguments");
	nk_love_assert(lua_type(L, 2) == LUA_TTABLE, "nk.combobox: arg 2 should be a table");
	int i;
	for (i = 0; i < NK_LOVE_COMBOBOX_MAX_ITEMS && lua_checkstack(L, 4); ++i) {
		lua_rawgeti(L, 2, i + 1);
		if (lua_type(L, -1) == LUA_TSTRING) {
			combobox_items[i] = lua_tostring(L, -1);
		} else if (lua_type(L, -1) == LUA_TNIL) {
			break;
		} else {
			nk_love_error("nk.combobox: items must be strings");
		}
	}
	struct nk_rect bounds = nk_widget_bounds(&context);
	int item_height = bounds.h;
	if (argc >= 3 && lua_type(L, 3) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.combobox: arg 3 should be a number");
		item_height = lua_tointeger(L, 3);
	}
	struct nk_vec2 size = nk_vec2(bounds.w, item_height * 8);
	if (argc >= 4 && lua_type(L, 4) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.combobox: arg 4 should be a number");
		size.x = lua_tonumber(L, 4);
	}
	if (argc >= 5 && lua_type(L, 5) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 5) == LUA_TNUMBER, "nk.combobox: arg 5 should be a number");
		size.y = lua_tonumber(L, 5);
	}
	if (lua_type(L, 1) == LUA_TNUMBER) {
		int value = lua_tointeger(L, 1) - 1;
		value = nk_combo(&context, combobox_items, i, value, item_height, size);
		lua_pushnumber(L, value + 1);
	} else if (lua_type(L, 1) == LUA_TTABLE) {
		lua_getfield(L, 1, "value");
		nk_love_assert(lua_type(L, -1) == LUA_TNUMBER, "nk.combobox: arg 1 should have a number value");
		int value = lua_tointeger(L, -1) - 1;
		int old = value;
		nk_combobox(&context, combobox_items, i, &value, item_height, size);
		int changed = value != old;
		if (changed) {
			lua_pushnumber(L, value + 1);
			lua_setfield(L, 1, "value");
		}
		lua_pushboolean(L, changed); 
	} else {
		nk_love_error("nk.combobox: arg 1 should be a number or table");
	}
	return 1;
}

static int nk_love_combobox_begin(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 1 && argc <= 4, "nk.combobox_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING || lua_type(L, 1) == LUA_TNUMBER || lua_type(L, 1) == LUA_TNIL, "nk.combobox_begin: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	struct nk_color color;
	int use_color = 0;
	enum nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	int use_image = 0;
	if (argc >= 2 && lua_type(L, 2) != LUA_TNIL) {
		if (lua_type(L, 2) == LUA_TSTRING) {
			const char *s = lua_tostring(L, 2);
			if (!nk_love_parse_symbol(s, &symbol)) {
				nk_love_assert_color(2, "nk.combobox_begin: arg 1 should be a color string, symbol type, or image");
				color = nk_love_color_parse(s);
				use_color = 1;
			}
		} else {
			nk_love_assert_type(2, "Image", "nk.combobox_begin: arg 1 should be a color string, symbol type, or image");
			lua_pushvalue(L, 2);
			nk_love_toimage(&image);
			use_image = 1;
		}
	}
	struct nk_rect bounds = nk_widget_bounds(&context);
	struct nk_vec2 size = nk_vec2(bounds.w, bounds.h * 8);
	if (argc >= 3 && lua_type(L, 3) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.combobox_begin: arg 3 should be a number");
		size.x = lua_tonumber(L, 3);
	}
	if (argc >= 4 && lua_type(L, 4) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.combobox_begin: arg 4 should be a number");
		size.y = lua_tonumber(L, 4);
	}
	int open = 0;
	if (text != NULL) {
		if (use_color) {
			nk_love_error("nk.combobox_begin: color comboboxes can't have titles");
		} else if (symbol != NK_SYMBOL_NONE) {
			open = nk_combo_begin_symbol_label(&context, text, symbol, size);
		} else if (use_image) {
			open = nk_combo_begin_image_label(&context, text, image, size);
		} else {
			open = nk_combo_begin_label(&context, text, size);
		}
	} else {
		if (use_color) {
			open = nk_combo_begin_color(&context, color, size);
		} else if (symbol != NK_SYMBOL_NONE) {
			open = nk_combo_begin_symbol(&context, symbol, size);
		} else if (use_image) {
			open = nk_combo_begin_image(&context, image, size);
		} else {
			nk_love_error("nk.combobox_begin: must specify color, symbol, image, and/or title");
		}
	}
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_combobox_item(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 1 && argc <= 3, "nk.combobox_item: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING || lua_type(L, 1) == LUA_TNUMBER, "nk.combobox_item: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	enum nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	int use_image = 0;
	if (argc >= 2 && lua_type(L, 2) != LUA_TNIL) {
		if (lua_type(L, 2) == LUA_TSTRING) {
			const char *s = lua_tostring(L, 2);
			nk_love_assert(nk_love_parse_symbol(s, &symbol), "nk.combobox_item: arg 2 should be a symbol type or image");
		} else {
			nk_love_assert_type(2, "Image", "nk.combobox_item: arg 1 should be a symbol type or image");
			lua_pushvalue(L, 2);
			nk_love_toimage(&image);
			use_image = 1;
		}
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 3 && lua_type(L, 3) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 3) == LUA_TSTRING, "nk.combobox_item: arg 3 should be a string");
		const char *align_string = lua_tostring(L, 3);
		if (!nk_love_parse_align(align_string, &align)) {
			nk_love_error("nk.combobox_item: arg 3 should be an alignment");
		}
	}
	int activated = 0;
	if (symbol != NK_SYMBOL_NONE) {
		activated = nk_combo_item_symbol_label(&context, symbol, text, align);
	} else if (use_image) {
		activated = nk_combo_item_image_label(&context, image, text, align);
	} else {
		activated = nk_combo_item_label(&context, text, align);
	}
	lua_pushboolean(L, activated);
	return 1;
}

static int nk_love_combobox_close(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.combobox_close: wrong number of arguments");
	nk_combo_close(&context);
	return 0;
}

static int nk_love_combobox_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.combobox_end: wrong number of arguments");
	nk_combo_end(&context);
	return 0;
}

static int nk_love_contextual_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) >= 6, "nk.contextual_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.contextual_begin: arg 1 should be a number");
	nk_love_assert(lua_type(L, 2) == LUA_TNUMBER, "nk.contextual_begin: arg 2 should be a number");
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.contextual_begin: arg 3 should be a number");
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.contextual_begin: arg 4 should be a number");
	nk_love_assert(lua_type(L, 5) == LUA_TNUMBER, "nk.contextual_begin: arg 5 should be a number");
	nk_love_assert(lua_type(L, 6) == LUA_TNUMBER, "nk.contextual_begin: arg 6 should be a number");
	struct nk_vec2 size;
	size.x = lua_tonumber(L, 1);
	size.y = lua_tonumber(L, 2);
	struct nk_rect trigger;
	trigger.x = lua_tonumber(L, 3);
	trigger.y = lua_tonumber(L, 4);
	trigger.w = lua_tonumber(L, 5);
	trigger.h = lua_tonumber(L, 6);
	nk_flags flags = nk_love_parse_window_flags(7);
	int open = nk_contextual_begin(&context, flags, size, trigger);
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_contextual_item(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 1 && argc <= 3, "nk.contextual_item: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.contextual_item: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	enum nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	int use_image = 0;
	if (argc >= 2 && lua_type(L, 2) != LUA_TNIL) {
		if (lua_type(L, 2) == LUA_TSTRING) {
			const char *symbol_string = lua_tostring(L, 2);
			if (!nk_love_parse_symbol(symbol_string, &symbol)) {
				nk_love_error("nk.contextual_item: arg 1 should be a symbol type or image");
			}
		} else {
			nk_love_assert_type(2, "Image", "nk.contextual_item: arg 1 should be a symbol type or image");
			lua_pushvalue(L, 2);
			nk_love_toimage(&image);
			use_image = 1;
		}
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 3 && lua_type(L, 3) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 3) == LUA_TSTRING, "nk.contextual_item: arg 3 should be a string");
		const char *align_string = lua_tostring(L, 3);
		if (!nk_love_parse_align(align_string, &align)) {
			nk_love_error("nk.contextual_item: arg 3 should be an alignment");
		}
	}
	int activated;
	if (symbol != NK_SYMBOL_NONE) {
		activated = nk_contextual_item_symbol_label(&context, symbol, text, align);
	} else if (use_image) {
		activated = nk_contextual_item_image_label(&context, image, text, align);
	} else {
		activated = nk_contextual_item_label(&context, text, align);
	}
	lua_pushboolean(L, activated);
	return 1;
}

static int nk_love_contextual_close(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.contextual_close: wrong number of arguments");
	nk_contextual_close(&context);
	return 0;
}

static int nk_love_contextual_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.contextual_end: wrong number of arguments");
	nk_contextual_end(&context);
	return 0;
}

static int nk_love_tooltip(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.tooltip: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.tooltip: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	nk_tooltip(&context, text);
	return 0;
}

static int nk_love_tooltip_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.tooltip_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.tooltip_begin: arg 1 should be a number");
	float width = lua_tonumber(L, 1);
	int open = nk_tooltip_begin(&context, width);
	lua_pushnumber(L, open);
	return 1;
}

static int nk_love_tooltip_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.tooltip_end: wrong number of arguments");
	nk_tooltip_end(&context);
	return 0;
}

static int nk_love_menubar_begin(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.menubar_begin: wrong number of arguments");
	nk_menubar_begin(&context);
	return 0;
}

static int nk_love_menubar_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.menubar_end: wrong number of arguments");
	nk_menubar_end(&context);
	return 0;
}

static int nk_love_menu_begin(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 4 || argc == 5, "nk.menu_begin: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.menu_begin: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	enum nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	int use_image = 0;
	if (lua_type(L, 2) == LUA_TSTRING) {
		const char *symbol_string = lua_tostring(L, 2);
		if (!nk_love_parse_symbol(symbol_string, &symbol)) {
			nk_love_error("nk.menu_begin: arg 2 should be a symbol type or image");
		}
	} else if (lua_type(L, 2) != LUA_TNIL) {
		nk_love_assert_type(2, "Image", "nk.menu_begin: arg 2 should be a symbol type or image");
		lua_pushvalue(L, 2);
		nk_love_toimage(&image);
		use_image = 1;
	}
	struct nk_vec2 size;
	nk_love_assert(lua_type(L, 3) == LUA_TNUMBER, "nk.menu_begin: arg 3 should be a number");
	size.x = lua_tonumber(L, 3);
	nk_love_assert(lua_type(L, 4) == LUA_TNUMBER, "nk.menu_begin: arg 4 should be a number");
	size.y = lua_tonumber(L, 4);
	nk_flags align = NK_TEXT_LEFT;
	if (argc == 5 && lua_type(L, 5) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 5) == LUA_TSTRING, "nk.menu_begin: arg 5 should be a string");
		const char *align_string = lua_tostring(L, 5);
		if (!nk_love_parse_align(align_string, &align)) {
			nk_love_error("nk.menu_begin: arg 5 should be an alignment");
		}
	}
	int open;
	if (symbol != NK_SYMBOL_NONE) {
		open = nk_menu_begin_symbol_label(&context, text, align, symbol, size);
	} else if (use_image) {
		open = nk_menu_begin_image_label(&context, text, align, image, size);
	} else {
		open = nk_menu_begin_label(&context, text, align, size);
	}
	lua_pushboolean(L, open);
	return 1;
}

static int nk_love_menu_item(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 1 && argc <= 3, "nk.menu_item: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.menu_item: arg 1 should be a string");
	const char *text = lua_tostring(L, 1);
	enum nk_symbol_type symbol = NK_SYMBOL_NONE;
	struct nk_image image;
	int use_image = 0;
	if (argc >= 2 && lua_type(L, 2) != LUA_TNIL) {
		if (lua_type(L, 2) == LUA_TSTRING) {
			const char *symbol_string = lua_tostring(L, 2);
			if (!nk_love_parse_symbol(symbol_string, &symbol)) {
				nk_love_error("nk.menu_item: arg 2 should be a symbol type or image");
			}
		} else {
			nk_love_assert_type(2, "Image", "nk.menu_item: arg 2 should be a symbol type or image");
			lua_pushvalue(L, 2);
			nk_love_toimage(&image);
			use_image = 1;
		}
	}
	nk_flags align = NK_TEXT_LEFT;
	if (argc >= 3 && lua_type(L, 3) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 3) == LUA_TSTRING, "nk.menu_item: arg 3 should be a string");
		const char *align_string = lua_tostring(L, 3);
		if (!nk_love_parse_align(align_string, &align)) {
			nk_love_error("nk.menu_item: arg 3 should be an alignment");
		}
	}
	int activated;
	if (symbol != NK_SYMBOL_NONE) {
		activated = nk_menu_item_symbol_label(&context, symbol, text, align);
	} else if (use_image) {
		activated = nk_menu_item_image_label(&context, image, text, align);
	} else {
		activated = nk_menu_item_label(&context, text, align);
	}
	lua_pushboolean(L, activated);
	return 1;
}

static int nk_love_menu_close(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.menu_close: wrong number of arguments");
	nk_menu_close(&context);
	return 0;
}

static int nk_love_menu_end(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.menu_end: wrong number of arguments");
	nk_menu_end(&context);
	return 0;
}

static int nk_love_style_default(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.style_default: wrong number of arguments");
	nk_style_default(&context);
	return 0;
}

#define NK_LOVE_LOAD_COLOR(type) \
	lua_getfield(L, -1, (type)); \
	nk_love_assert_color(-1, "nk.style_load_colors: table missing color value for '" type "'"); \
	colors[index++] = nk_love_color_parse(lua_tostring(L, -1)); \
	lua_pop(L, 1);

static int nk_love_style_load_colors(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.style_load_colors: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TTABLE, "nk.style_load_colors: arg 1 should be a table");
	struct nk_color colors[NK_COLOR_COUNT];
	int index = 0;
	NK_LOVE_LOAD_COLOR("text");
	NK_LOVE_LOAD_COLOR("window");
	NK_LOVE_LOAD_COLOR("header");
	NK_LOVE_LOAD_COLOR("border");
	NK_LOVE_LOAD_COLOR("button");
	NK_LOVE_LOAD_COLOR("button hover");
	NK_LOVE_LOAD_COLOR("button active");
	NK_LOVE_LOAD_COLOR("toggle");
	NK_LOVE_LOAD_COLOR("toggle hover");
	NK_LOVE_LOAD_COLOR("toggle cursor");
	NK_LOVE_LOAD_COLOR("select");
	NK_LOVE_LOAD_COLOR("select active");
	NK_LOVE_LOAD_COLOR("slider");
	NK_LOVE_LOAD_COLOR("slider cursor");
	NK_LOVE_LOAD_COLOR("slider cursor hover");
	NK_LOVE_LOAD_COLOR("slider cursor active");
	NK_LOVE_LOAD_COLOR("property");
	NK_LOVE_LOAD_COLOR("edit");
	NK_LOVE_LOAD_COLOR("edit cursor");
	NK_LOVE_LOAD_COLOR("combo");
	NK_LOVE_LOAD_COLOR("chart");
	NK_LOVE_LOAD_COLOR("chart color");
	NK_LOVE_LOAD_COLOR("chart color highlight");
	NK_LOVE_LOAD_COLOR("scrollbar");
	NK_LOVE_LOAD_COLOR("scrollbar cursor");
	NK_LOVE_LOAD_COLOR("scrollbar cursor hover");
	NK_LOVE_LOAD_COLOR("scrollbar cursor active");
	NK_LOVE_LOAD_COLOR("tab header");
	nk_style_from_table(&context, colors);
	return 0;
}

static int nk_love_style_set_font(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.style_set_font: wrong number of arguments");
	nk_love_assert_type(1, "Font", "nk.style_set_font: arg 1 should be a font");
	nk_love_tofont(&fonts[font_count]);
	nk_style_set_font(&context, &fonts[font_count++]);
	return 0;
}

static int nk_love_style_push_color(struct nk_color *field)
{
	nk_love_assert_color(-1, "nk.style_push: color fields must be color strings");
	const char *color_string = lua_tostring(L, -1);
	struct nk_color color = nk_love_color_parse(color_string);
	int success = nk_style_push_color(&context, field, color);
	if (success) {
		lua_pushstring(L, "color");
		size_t stack_size = lua_objlen(L, 1);
		lua_rawseti(L, 1, stack_size + 1);
	}
	return success;
}

static int nk_love_style_push_vec2(struct nk_vec2 *field)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "nk.style_push: vec2 fields must have x and y components");
	lua_getfield(L, -1, "x");
	nk_love_assert(lua_type(L, -1) == LUA_TNUMBER, "nk.style_push: vec2 fields must have x and y components");
	lua_getfield(L, -2, "y");
	nk_love_assert(lua_type(L, -1) == LUA_TNUMBER, "nk.style_push: vec2 fields must have x and y components");
	struct nk_vec2 vec2;
	vec2.x = lua_tonumber(L, -2);
	vec2.y = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int success = nk_style_push_vec2(&context, field, vec2);
	if (success) {
		lua_pushstring(L, "vec2");
		size_t stack_size = lua_objlen(L, 1);
		lua_rawseti(L, 1, stack_size + 1);
	}
	return success;
}

static int nk_love_style_push_item(struct nk_style_item *field)
{
	struct nk_style_item item;
	if (lua_type(L, -1) == LUA_TSTRING) {
		nk_love_assert_color(-1, "nk.style_push: item fields must be color strings or images");
		const char *color_string = lua_tostring(L, -1);
		item.type = NK_STYLE_ITEM_COLOR;
		item.data.color = nk_love_color_parse(color_string);
	} else {
		nk_love_assert_type(-1, "Image", "nk.style_push: item fields must be color strings or images");
		lua_pushvalue(L, -1);
		item.type = NK_STYLE_ITEM_IMAGE;
		nk_love_toimage(&item.data.image);
	}
	int success = nk_style_push_style_item(&context, field, item);
	if (success) {
		lua_pushstring(L, "item");
		size_t stack_size = lua_objlen(L, 1);
		lua_rawseti(L, 1, stack_size + 1);
	}
	return success;
}

static int nk_love_style_push_align(nk_flags *field)
{
	nk_love_assert(lua_type(L, -1) == LUA_TSTRING, "nk.style_push: alignment fields must be alignments");
	const char *align_string = lua_tostring(L, -1);
	nk_flags align;
	if (!nk_love_parse_align(align_string, &align)) {
		nk_love_error("nk.style_push: alignment fields must be alignments");
	}
	int success = nk_style_push_flags(&context, field, align);
	if (success) {
		lua_pushstring(L, "flags");
		size_t stack_size = lua_objlen(L, 1);
		lua_rawseti(L, 1, stack_size + 1);
	}
	return success;
}

static int nk_love_style_push_float(float *field) {
	nk_love_assert(lua_type(L, -1) == LUA_TNUMBER, "nk.style_push: float fields must be numbers");
	float f = lua_tonumber(L, -1);
	int success = nk_style_push_float(&context, field, f);
	if (success) {
		lua_pushstring(L, "float");
		size_t stack_size = lua_objlen(L, 1);
		lua_rawseti(L, 1, stack_size + 1);
	}
	return success;
}

static int nk_love_style_push_font(const struct nk_user_font **field)
{
	nk_love_assert_type(-1, "Font", "nk.style_push: font fields must be fonts");
	lua_pushvalue(L, -1);
	nk_love_tofont(&fonts[font_count]);
	int success = nk_style_push_font(&context, &fonts[font_count++]);
	if (success) {
		lua_pushstring(L, "font");
		size_t stack_size = lua_objlen(L, 1);
		lua_rawseti(L, 1, stack_size + 1);
	}
	return success;
}

#define NK_LOVE_STYLE_PUSH(name, type, field) \
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "nk.style_push: " name " field must be a table"); \
	lua_getfield(L, -1, name); \
	if (lua_type(L, -1) != LUA_TNIL) \
		nk_love_style_push_##type(field); \
	lua_pop(L, 1);

static void nk_love_style_push_text(struct nk_style_text *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "text style must be a table");
	NK_LOVE_STYLE_PUSH("color", color, &style->color);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
}

static void nk_love_style_push_button(struct nk_style_button *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "button style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("text background", color, &style->text_background);
	NK_LOVE_STYLE_PUSH("text normal", color, &style->text_normal);
	NK_LOVE_STYLE_PUSH("text hover", color, &style->text_hover);
	NK_LOVE_STYLE_PUSH("text active", color, &style->text_active);
	NK_LOVE_STYLE_PUSH("text alignment", align, &style->text_alignment);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("image padding", vec2, &style->image_padding);
	NK_LOVE_STYLE_PUSH("touch padding", vec2, &style->touch_padding);
}

static void nk_love_style_push_scrollbar(struct nk_style_scrollbar *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "scrollbar style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	NK_LOVE_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	NK_LOVE_STYLE_PUSH("cursor active", item, &style->active);
	NK_LOVE_STYLE_PUSH("cursor border color", color, &style->cursor_border_color);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("border cursor", float, &style->border_cursor);
	NK_LOVE_STYLE_PUSH("rounding cursor", float, &style->rounding_cursor);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
}

static void nk_love_style_push_edit(struct nk_style_edit *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "edit style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("scrollbar", scrollbar, &style->scrollbar);
	NK_LOVE_STYLE_PUSH("cursor normal", color, &style->cursor_normal);
	NK_LOVE_STYLE_PUSH("cursor hover", color, &style->cursor_hover);
	NK_LOVE_STYLE_PUSH("cursor text normal", color, &style->cursor_text_normal);
	NK_LOVE_STYLE_PUSH("cursor text hover", color, &style->cursor_text_hover);
	NK_LOVE_STYLE_PUSH("text normal", color, &style->text_normal);
	NK_LOVE_STYLE_PUSH("text hover", color, &style->text_hover);
	NK_LOVE_STYLE_PUSH("text active", color, &style->text_active);
	NK_LOVE_STYLE_PUSH("selected normal", color, &style->selected_normal);
	NK_LOVE_STYLE_PUSH("selected hover", color, &style->selected_hover);
	NK_LOVE_STYLE_PUSH("selected text normal", color, &style->text_normal);
	NK_LOVE_STYLE_PUSH("selected text hover", color, &style->selected_text_hover);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("cursor size", float, &style->cursor_size);
	NK_LOVE_STYLE_PUSH("scrollbar size", vec2, &style->scrollbar_size);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("row padding", float, &style->row_padding);
}

static void nk_love_style_push_toggle(struct nk_style_toggle *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "toggle style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	NK_LOVE_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	NK_LOVE_STYLE_PUSH("text normal", color, &style->text_normal);
	NK_LOVE_STYLE_PUSH("text hover", color, &style->text_hover);
	NK_LOVE_STYLE_PUSH("text active", color, &style->text_active);
	NK_LOVE_STYLE_PUSH("text background", color, &style->text_background);
	NK_LOVE_STYLE_PUSH("text alignment", align, &style->text_alignment);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("touch padding", vec2, &style->touch_padding);
	NK_LOVE_STYLE_PUSH("spacing", float, &style->spacing);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
}

static void nk_love_style_push_selectable(struct nk_style_selectable *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "selectable style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("pressed", item, &style->pressed);
	NK_LOVE_STYLE_PUSH("normal active", item, &style->normal_active);
	NK_LOVE_STYLE_PUSH("hover active", item, &style->hover_active);
	NK_LOVE_STYLE_PUSH("pressed active", item, &style->pressed_active);
	NK_LOVE_STYLE_PUSH("text normal", color, &style->text_normal);
	NK_LOVE_STYLE_PUSH("text hover", color, &style->text_hover);
	NK_LOVE_STYLE_PUSH("text pressed", color, &style->text_pressed);
	NK_LOVE_STYLE_PUSH("text normal active", color, &style->text_normal_active);
	NK_LOVE_STYLE_PUSH("text hover active", color, &style->text_hover_active);
	NK_LOVE_STYLE_PUSH("text pressed active", color, &style->text_pressed_active);
	NK_LOVE_STYLE_PUSH("text background", color, &style->text_background);
	NK_LOVE_STYLE_PUSH("text alignment", align, &style->text_alignment);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("touch padding", vec2, &style->touch_padding);
	NK_LOVE_STYLE_PUSH("image padding", vec2, &style->image_padding);
}

static void nk_love_style_push_slider(struct nk_style_slider *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "slider style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("bar normal", color, &style->bar_normal);
	NK_LOVE_STYLE_PUSH("bar active", color, &style->bar_active);
	NK_LOVE_STYLE_PUSH("bar filled", color, &style->bar_filled);
	NK_LOVE_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	NK_LOVE_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	NK_LOVE_STYLE_PUSH("cursor active", item, &style->cursor_active);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("bar height", float, &style->bar_height);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("spacing", vec2, &style->spacing);
	NK_LOVE_STYLE_PUSH("cursor size", vec2, &style->cursor_size);
}

static void nk_love_style_push_progress(struct nk_style_progress *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "progress style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("cursor normal", item, &style->cursor_normal);
	NK_LOVE_STYLE_PUSH("cursor hover", item, &style->cursor_hover);
	NK_LOVE_STYLE_PUSH("cusor active", item, &style->cursor_active);
	NK_LOVE_STYLE_PUSH("cursor border color", color, &style->cursor_border_color);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("cursor border", float, &style->cursor_border);
	NK_LOVE_STYLE_PUSH("cursor rounding", float, &style->cursor_rounding);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
}

static void nk_love_style_push_property(struct nk_style_property *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "property style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("label normal", color, &style->label_normal);
	NK_LOVE_STYLE_PUSH("label hover", color, &style->label_hover);
	NK_LOVE_STYLE_PUSH("label active", color, &style->label_active);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("edit", edit, &style->edit);
	NK_LOVE_STYLE_PUSH("inc button", button, &style->inc_button);
	NK_LOVE_STYLE_PUSH("dec button", button, &style->dec_button);
}

static void nk_love_style_push_chart(struct nk_style_chart *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "chart style must be a table");
	NK_LOVE_STYLE_PUSH("background", item, &style->background);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("selected color", color, &style->selected_color);
	NK_LOVE_STYLE_PUSH("color", color, &style->color);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
}

static void nk_love_style_push_tab(struct nk_style_tab *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "tab style must be a table");
	NK_LOVE_STYLE_PUSH("background", item, &style->background);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("text", color, &style->text);
	NK_LOVE_STYLE_PUSH("tab maximize button", button, &style->tab_maximize_button);
	NK_LOVE_STYLE_PUSH("tab minimize button", button, &style->tab_minimize_button);
	NK_LOVE_STYLE_PUSH("node maximize button", button, &style->node_maximize_button);
	NK_LOVE_STYLE_PUSH("node minimize button", button, &style->node_minimize_button);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("indent", float, &style->indent);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("spacing", vec2, &style->spacing);
}

static void nk_love_style_push_combo(struct nk_style_combo *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "combo style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("label normal", color, &style->label_normal);
	NK_LOVE_STYLE_PUSH("label hover", color, &style->label_hover);
	NK_LOVE_STYLE_PUSH("label active", color, &style->label_active);
	NK_LOVE_STYLE_PUSH("symbol normal", color, &style->symbol_normal);
	NK_LOVE_STYLE_PUSH("symbol hover", color, &style->symbol_hover);
	NK_LOVE_STYLE_PUSH("symbol active", color, &style->symbol_active);
	NK_LOVE_STYLE_PUSH("button", button, &style->button);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("content padding", vec2, &style->content_padding);
	NK_LOVE_STYLE_PUSH("button padding", vec2, &style->button_padding);
	NK_LOVE_STYLE_PUSH("spacing", vec2, &style->spacing);
}

static void nk_love_style_push_window_header(struct nk_style_window_header *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "window header style must be a table");
	NK_LOVE_STYLE_PUSH("normal", item, &style->normal);
	NK_LOVE_STYLE_PUSH("hover", item, &style->hover);
	NK_LOVE_STYLE_PUSH("active", item, &style->active);
	NK_LOVE_STYLE_PUSH("close button", button, &style->close_button);
	NK_LOVE_STYLE_PUSH("minimize button", button, &style->minimize_button);
	NK_LOVE_STYLE_PUSH("label normal", color, &style->label_normal);
	NK_LOVE_STYLE_PUSH("label hover", color, &style->label_hover);
	NK_LOVE_STYLE_PUSH("label active", color, &style->label_active);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("label padding", vec2, &style->label_padding);
	NK_LOVE_STYLE_PUSH("spacing", vec2, &style->spacing);
}

static void nk_love_style_push_window(struct nk_style_window *style)
{
	nk_love_assert(lua_type(L, -1) == LUA_TTABLE, "window style must be a table");
	NK_LOVE_STYLE_PUSH("header", window_header, &style->header);
	NK_LOVE_STYLE_PUSH("fixed background", item, &style->fixed_background);
	NK_LOVE_STYLE_PUSH("background", color, &style->background);
	NK_LOVE_STYLE_PUSH("border color", color, &style->border_color);
	NK_LOVE_STYLE_PUSH("popup border color", color, &style->popup_border_color);
	NK_LOVE_STYLE_PUSH("combo border color", color, &style->combo_border_color);
	NK_LOVE_STYLE_PUSH("contextual border color", color, &style->contextual_border_color);
	NK_LOVE_STYLE_PUSH("menu border color", color, &style->menu_border_color);
	NK_LOVE_STYLE_PUSH("group border color", color, &style->group_border_color);
	NK_LOVE_STYLE_PUSH("tooltip border color", color, &style->tooltip_border_color);
	NK_LOVE_STYLE_PUSH("scaler", item, &style->scaler);
	NK_LOVE_STYLE_PUSH("border", float, &style->border);
	NK_LOVE_STYLE_PUSH("combo border", float, &style->combo_border);
	NK_LOVE_STYLE_PUSH("contextual border", float, &style->contextual_border);
	NK_LOVE_STYLE_PUSH("menu border", float, &style->menu_border);
	NK_LOVE_STYLE_PUSH("group border", float, &style->group_border);
	NK_LOVE_STYLE_PUSH("tooltip border", float, &style->tooltip_border);
	NK_LOVE_STYLE_PUSH("popup border", float, &style->popup_border);
	NK_LOVE_STYLE_PUSH("rounding", float, &style->rounding);
	NK_LOVE_STYLE_PUSH("spacing", vec2, &style->spacing);
	NK_LOVE_STYLE_PUSH("scrollbar size", vec2, &style->scrollbar_size);
	NK_LOVE_STYLE_PUSH("min size", vec2, &style->min_size);
	NK_LOVE_STYLE_PUSH("padding", vec2, &style->padding);
	NK_LOVE_STYLE_PUSH("group padding", vec2, &style->group_padding);
	NK_LOVE_STYLE_PUSH("popup padding", vec2, &style->popup_padding);
	NK_LOVE_STYLE_PUSH("combo padding", vec2, &style->combo_padding);
	NK_LOVE_STYLE_PUSH("contextual padding", vec2, &style->contextual_padding);
	NK_LOVE_STYLE_PUSH("menu padding", vec2, &style->menu_padding);
	NK_LOVE_STYLE_PUSH("tooltip padding", vec2, &style->tooltip_padding);
}

static int nk_love_style_push(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.style_push: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TTABLE, "nk.style_push: arg 1 should be a table");
	lua_newtable(L);
	lua_insert(L, 1);
	NK_LOVE_STYLE_PUSH("font", font, &context.style.font);
	NK_LOVE_STYLE_PUSH("text", text, &context.style.text);
	NK_LOVE_STYLE_PUSH("button", button, &context.style.button);
	NK_LOVE_STYLE_PUSH("contextual button", button, &context.style.contextual_button);
	NK_LOVE_STYLE_PUSH("menu button", button, &context.style.menu_button);
	NK_LOVE_STYLE_PUSH("option", toggle, &context.style.option);
	NK_LOVE_STYLE_PUSH("checkbox", toggle, &context.style.checkbox);
	NK_LOVE_STYLE_PUSH("selectable", selectable, &context.style.selectable);
	NK_LOVE_STYLE_PUSH("slider", slider, &context.style.slider);
	NK_LOVE_STYLE_PUSH("progress", progress, &context.style.progress);
	NK_LOVE_STYLE_PUSH("property", property, &context.style.property);
	NK_LOVE_STYLE_PUSH("edit", edit, &context.style.edit);
	NK_LOVE_STYLE_PUSH("chart", chart, &context.style.chart);
	NK_LOVE_STYLE_PUSH("scrollh", scrollbar, &context.style.scrollh);
	NK_LOVE_STYLE_PUSH("scrollv", scrollbar, &context.style.scrollv);
	NK_LOVE_STYLE_PUSH("tab", tab, &context.style.tab);
	NK_LOVE_STYLE_PUSH("combo", combo, &context.style.combo);
	NK_LOVE_STYLE_PUSH("window", window, &context.style.window);
	lua_pop(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "stack");
	size_t stack_size = lua_objlen(L, -1);
	lua_pushvalue(L, 1);
	lua_rawseti(L, -2, stack_size + 1);
	return 0;
}

static int nk_love_style_pop(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.style_pop: wrong number of arguments");
	lua_getfield(L, LUA_REGISTRYINDEX, "nuklear");
	lua_getfield(L, -1, "stack");
	size_t stack_size = lua_objlen(L, -1);
	lua_rawgeti(L, -1, stack_size);
	lua_pushnil(L);
	lua_rawseti(L, -3, stack_size);
	stack_size = lua_objlen(L, -1);
	size_t i;
	for (i = stack_size; i > 0; --i) {
		lua_rawgeti(L, -1, i);
		const char *type = lua_tostring(L, -1);
		if (!strcmp(type, "color")) {
			nk_style_pop_color(&context);
		} else if (!strcmp(type, "vec2")) {
			nk_style_pop_vec2(&context);
		} else if (!strcmp(type, "item")) {
			nk_style_pop_style_item(&context);
		} else if (!strcmp(type, "flags")) {
			nk_style_pop_flags(&context);
		} else if (!strcmp(type, "float")) {
			nk_style_pop_float(&context);
		} else if (!strcmp(type, "font")) {
			nk_style_pop_font(&context);
		} else {
			nk_love_error("nk.pop: bad style item type");
		}
		lua_pop(L, 1);
	}
	return 0;
}

static int nk_love_widget_bounds(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.widget_bounds: wrong number of arguments");
	struct nk_rect bounds = nk_widget_bounds(&context);
	lua_pushnumber(L, bounds.x);
	lua_pushnumber(L, bounds.y);
	lua_pushnumber(L, bounds.w);
	lua_pushnumber(L, bounds.h);
	return 4;
}

static int nk_love_widget_position(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.widget_position: wrong number of arguments");
	struct nk_vec2 pos = nk_widget_position(&context);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	return 2;
}

static int nk_love_widget_size(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.widget_size: wrong number of arguments");
	struct nk_vec2 pos = nk_widget_size(&context);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	return 2;
}

static int nk_love_widget_width(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.widget_width: wrong number of arguments");
	float width = nk_widget_width(&context);
	lua_pushnumber(L, width);
	return 1;
}

static int nk_love_widget_height(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.widget_height: wrong number of arguments");
	float height = nk_widget_height(&context);
	lua_pushnumber(L, height);
	return 1;
}

static int nk_love_widget_is_hovered(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 0, "nk.widget_is_hovered: wrong number of arguments");
	int hovered = nk_widget_is_hovered(&context);
	lua_pushboolean(L, hovered);
	return 1;
}

static int nk_love_widget_is_mouse_clicked(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc == 0 || argc == 1, "nk.widget_is_mouse_clicked: wrong number of arguments");
	enum nk_buttons button = NK_BUTTON_LEFT;
	if (argc == 1 && lua_type(L, 1) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.widget_is_mouse_clicked: arg 1 should be a string");
		const char *button_string = lua_tostring(L, 1);
		if (!nk_love_parse_button(button_string, &button)) {
			nk_love_error("nk.widget_is_mouse_clicked: arg 1 should be a button");
		}
	}
	int clicked = nk_widget_is_mouse_clicked(&context, button);
	lua_pushboolean(L, clicked);
	return 1;
}

static int nk_love_widget_has_mouse_click(lua_State *L)
{
	int argc = lua_gettop(L);
	nk_love_assert(argc >= 0 && argc <= 2, "nk.widget_has_mouse_click: wrong number of arguments");
	enum nk_buttons button = NK_BUTTON_LEFT;
	if (argc >= 1 && lua_type(L, 1) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 1) == LUA_TSTRING, "nk.widget_has_mouse_click: arg 1 should be a string");
		const char *button_string = lua_tostring(L, 1);
		if (!nk_love_parse_button(button_string, &button)) {
			nk_love_error("nk.widget_has_mouse_click: arg 1 should be a button");
		}
	}
	int down = 1;
	if (argc >= 2 && lua_type(L, 2) != LUA_TNIL) {
		nk_love_assert(lua_type(L, 2) == LUA_TBOOLEAN, "nk.widget_has_mouse_click: arg 2 should be a boolean");
		down = lua_toboolean(L, 2);
	}
	int has_click = nk_widget_has_mouse_click_down(&context, button, down);
	lua_pushboolean(L, has_click);
	return 1;
}

static int nk_love_spacing(lua_State *L)
{
	nk_love_assert(lua_gettop(L) == 1, "nk.spacing: wrong number of arguments");
	nk_love_assert(lua_type(L, 1) == LUA_TNUMBER, "nk.spacing: arg 1 should be a number");
	int cols = lua_tointeger(L, 1);
	nk_spacing(&context, cols);
	return 0;
}

LUALIB_API int luaopen_nuklear(lua_State *L)
{
	lua_newtable(L);
	
	lua_pushcfunction(L, nk_love_init);
	lua_setfield(L, -2, "init");
	lua_pushcfunction(L, nk_love_shutdown);
	lua_setfield(L, -2, "shutdown");

	lua_pushcfunction(L, nk_love_keypressed);
	lua_setfield(L, -2, "keypressed");
	lua_pushcfunction(L, nk_love_keyreleased);
	lua_setfield(L, -2, "keyreleased");
	lua_pushcfunction(L, nk_love_mousepressed);
	lua_setfield(L, -2, "mousepressed");
	lua_pushcfunction(L, nk_love_mousereleased);
	lua_setfield(L, -2, "mousereleased");
	lua_pushcfunction(L, nk_love_mousemoved);
	lua_setfield(L, -2, "mousemoved");
	lua_pushcfunction(L, nk_love_textinput);
	lua_setfield(L, -2, "textinput");
	lua_pushcfunction(L, nk_love_wheelmoved);
	lua_setfield(L, -2, "wheelmoved");

	lua_pushcfunction(L, nk_love_draw);
	lua_setfield(L, -2, "draw");

	lua_pushcfunction(L, nk_love_frame_begin);
	lua_setfield(L, -2, "frame_begin");
	lua_pushcfunction(L, nk_love_frame_end);
	lua_setfield(L, -2, "frame_end");

	lua_pushcfunction(L, nk_love_window_begin);
	lua_setfield(L, -2, "window_begin");
	lua_pushcfunction(L, nk_love_window_end);
	lua_setfield(L, -2, "window_end");
	lua_pushcfunction(L, nk_love_window_get_bounds);
	lua_setfield(L, -2, "window_get_bounds");
	lua_pushcfunction(L, nk_love_window_get_position);
	lua_setfield(L, -2, "window_get_position");
	lua_pushcfunction(L, nk_love_window_get_size);
	lua_setfield(L, -2, "window_get_size");
	lua_pushcfunction(L, nk_love_window_get_content_region);
	lua_setfield(L, -2, "window_get_content_region");
	lua_pushcfunction(L, nk_love_window_has_focus);
	lua_setfield(L, -2, "window_has_focus");
	lua_pushcfunction(L, nk_love_window_is_collapsed);
	lua_setfield(L, -2, "window_is_collapsed");
	lua_pushcfunction(L, nk_love_window_is_hidden);
	lua_setfield(L, -2, "window_is_hidden");
	lua_pushcfunction(L, nk_love_window_is_active);
	lua_setfield(L, -2, "window_is_active");
	lua_pushcfunction(L, nk_love_window_is_hovered);
	lua_setfield(L, -2, "window_is_hovered");
	lua_pushcfunction(L, nk_love_window_is_any_hovered);
	lua_setfield(L, -2, "window_is_any_hovered");
	lua_pushcfunction(L, nk_love_item_is_any_active);
	lua_setfield(L, -2, "item_is_any_active");
	lua_pushcfunction(L, nk_love_window_set_bounds);
	lua_setfield(L, -2, "window_set_bounds");
	lua_pushcfunction(L, nk_love_window_set_position);
	lua_setfield(L, -2, "window_set_position");
	lua_pushcfunction(L, nk_love_window_set_size);
	lua_setfield(L, -2, "window_set_size");
	lua_pushcfunction(L, nk_love_window_set_focus);
	lua_setfield(L, -2, "window_set_focus");
	lua_pushcfunction(L, nk_love_window_close);
	lua_setfield(L, -2, "window_close");
	lua_pushcfunction(L, nk_love_window_collapse);
	lua_setfield(L, -2, "window_collapse");
	lua_pushcfunction(L, nk_love_window_expand);
	lua_setfield(L, -2, "window_expand");
	lua_pushcfunction(L, nk_love_window_show);
	lua_setfield(L, -2, "window_show");
	lua_pushcfunction(L, nk_love_window_hide);
	lua_setfield(L, -2, "window_hide");
	
	lua_pushcfunction(L, nk_love_layout_row);
	lua_setfield(L, -2, "layout_row");
	lua_pushcfunction(L, nk_love_layout_row_begin);
	lua_setfield(L, -2, "layout_row_begin");
	lua_pushcfunction(L, nk_love_layout_row_push);
	lua_setfield(L, -2, "layout_row_push");
	lua_pushcfunction(L, nk_love_layout_row_end);
	lua_setfield(L, -2, "layout_row_end");
	lua_pushcfunction(L, nk_love_layout_space_begin);
	lua_setfield(L, -2, "layout_space_begin");
	lua_pushcfunction(L, nk_love_layout_space_push);
	lua_setfield(L, -2, "layout_space_push");
	lua_pushcfunction(L, nk_love_layout_space_end);
	lua_setfield(L, -2, "layout_space_end");
	lua_pushcfunction(L, nk_love_layout_space_bounds);
	lua_setfield(L, -2, "layout_space_bounds");
	lua_pushcfunction(L, nk_love_layout_space_to_screen);
	lua_setfield(L, -2, "layout_space_to_screen");
	lua_pushcfunction(L, nk_love_layout_space_to_local);
	lua_setfield(L, -2, "layout_space_to_local");
	lua_pushcfunction(L, nk_love_layout_space_rect_to_screen);
	lua_setfield(L, -2, "layout_space_rect_to_screen");
	lua_pushcfunction(L, nk_love_layout_space_rect_to_local);
	lua_setfield(L, -2, "layout_space_rect_to_local");
	lua_pushcfunction(L, nk_love_layout_ratio_from_pixel);
	lua_setfield(L, -2, "layout_ratio_from_pixel");

	lua_pushcfunction(L, nk_love_group_begin);
	lua_setfield(L, -2, "group_begin");
	lua_pushcfunction(L, nk_love_group_end);
	lua_setfield(L, -2, "group_end");

	lua_pushcfunction(L, nk_love_tree_push);
	lua_setfield(L, -2, "tree_push");
	lua_pushcfunction(L, nk_love_tree_pop);
	lua_setfield(L, -2, "tree_pop");

	lua_pushcfunction(L, nk_love_color_rgba);
	lua_setfield(L, -2, "color_rgba");
	lua_pushcfunction(L, nk_love_color_hsva);
	lua_setfield(L, -2, "color_hsva");
	lua_pushcfunction(L, nk_love_color_parse_rgba);
	lua_setfield(L, -2, "color_parse_rgba");
	lua_pushcfunction(L, nk_love_color_parse_hsva);
	lua_setfield(L, -2, "color_parse_hsva");

	lua_pushcfunction(L, nk_love_label);
	lua_setfield(L, -2, "label");
	lua_pushcfunction(L, nk_love_image);
	lua_setfield(L, -2, "image");
	lua_pushcfunction(L, nk_love_button);
	lua_setfield(L, -2, "button");
	lua_pushcfunction(L, nk_love_button_set_behavior);
	lua_setfield(L, -2, "button_set_behavior");
	lua_pushcfunction(L, nk_love_button_push_behavior);
	lua_setfield(L, -2, "button_push_behavior");
	lua_pushcfunction(L, nk_love_button_pop_behavior);
	lua_setfield(L, -2, "button_pop_behavior");
	lua_pushcfunction(L, nk_love_checkbox);
	lua_setfield(L, -2, "checkbox");
	lua_pushcfunction(L, nk_love_radio);
	lua_setfield(L, -2, "radio");
	lua_pushcfunction(L, nk_love_selectable);
	lua_setfield(L, -2, "selectable");
	lua_pushcfunction(L, nk_love_slider);
	lua_setfield(L, -2, "slider");
	lua_pushcfunction(L, nk_love_progress);
	lua_setfield(L, -2, "progress");
	lua_pushcfunction(L, nk_love_color_picker);
	lua_setfield(L, -2, "color_picker");
	lua_pushcfunction(L, nk_love_property);
	lua_setfield(L, -2, "property");
	lua_pushcfunction(L, nk_love_edit);
	lua_setfield(L, -2, "edit");
	lua_pushcfunction(L, nk_love_popup_begin);
	lua_setfield(L, -2, "popup_begin");
	lua_pushcfunction(L, nk_love_popup_close);
	lua_setfield(L, -2, "popup_close");
	lua_pushcfunction(L, nk_love_popup_end);
	lua_setfield(L, -2, "popup_end");
	lua_pushcfunction(L, nk_love_combobox);
	lua_setfield(L, -2, "combobox");
	lua_pushcfunction(L, nk_love_combobox_begin);
	lua_setfield(L, -2, "combobox_begin");
	lua_pushcfunction(L, nk_love_combobox_item);
	lua_setfield(L, -2, "combobox_item");
	lua_pushcfunction(L, nk_love_combobox_close);
	lua_setfield(L, -2, "combobox_close");
	lua_pushcfunction(L, nk_love_combobox_end);
	lua_setfield(L, -2, "combobox_end");
	lua_pushcfunction(L, nk_love_contextual_begin);
	lua_setfield(L, -2, "contextual_begin");
	lua_pushcfunction(L, nk_love_contextual_item);
	lua_setfield(L, -2, "contextual_item");
	lua_pushcfunction(L, nk_love_contextual_close);
	lua_setfield(L, -2, "contextual_close");
	lua_pushcfunction(L, nk_love_contextual_end);
	lua_setfield(L, -2, "contextual_end");
	lua_pushcfunction(L, nk_love_tooltip);
	lua_setfield(L, -2, "tooltip");
	lua_pushcfunction(L, nk_love_tooltip_begin);
	lua_setfield(L, -2, "tooltip_begin");
	lua_pushcfunction(L, nk_love_tooltip_end);
	lua_setfield(L, -2, "tooltip_end");
	lua_pushcfunction(L, nk_love_menubar_begin);
	lua_setfield(L, -2, "menubar_begin");
	lua_pushcfunction(L, nk_love_menubar_end);
	lua_setfield(L, -2, "menubar_end");
	lua_pushcfunction(L, nk_love_menu_begin);
	lua_setfield(L, -2, "menu_begin");
	lua_pushcfunction(L, nk_love_menu_item);
	lua_setfield(L, -2, "menu_item");
	lua_pushcfunction(L, nk_love_menu_close);
	lua_setfield(L, -2, "menu_close");
	lua_pushcfunction(L, nk_love_menu_end);
	lua_setfield(L, -2, "menu_end");

	lua_pushcfunction(L, nk_love_style_default);
	lua_setfield(L, -2, "style_default");
	lua_pushcfunction(L, nk_love_style_load_colors);
	lua_setfield(L, -2, "style_load_colors");
	lua_pushcfunction(L, nk_love_style_set_font);
	lua_setfield(L, -2, "style_set_font");
	lua_pushcfunction(L, nk_love_style_push);
	lua_setfield(L, -2, "style_push");
	lua_pushcfunction(L, nk_love_style_pop);
	lua_setfield(L, -2, "style_pop");

	lua_pushcfunction(L, nk_love_widget_bounds);
	lua_setfield(L, -2, "widget_bounds");
	lua_pushcfunction(L, nk_love_widget_position);
	lua_setfield(L, -2, "widget_position");
	lua_pushcfunction(L, nk_love_widget_size);
	lua_setfield(L, -2, "widget_size");
	lua_pushcfunction(L, nk_love_widget_width);
	lua_setfield(L, -2, "widget_width");
	lua_pushcfunction(L, nk_love_widget_height);
	lua_setfield(L, -2, "widget_height");
	lua_pushcfunction(L, nk_love_widget_is_hovered);
	lua_setfield(L, -2, "widget_is_hovered");
	lua_pushcfunction(L, nk_love_widget_is_mouse_clicked);
	lua_setfield(L, -2, "widget_is_mouse_clicked");
	lua_pushcfunction(L, nk_love_widget_has_mouse_click);
	lua_setfield(L, -2, "widget_has_mouse_click");
	lua_pushcfunction(L, nk_love_spacing);
	lua_setfield(L, -2, "spacing");

    return 1;
}

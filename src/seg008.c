/*
SDLPoP, a port/conversion of the DOS game Prince of Persia.
Copyright (C) 2013-2025  Dávid Nagy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

The authors of this program may be contacted at https://forum.princed.org
*/

#include "common.h"
#include "mac_palace_palette.h"

// data:27E0
add_table_type ptr_add_table = add_backtable;

// data:259C
const piece tile_table[31] = {
{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, // 0x00 empty
{  41,   1,   0,  42,   1,   2, 145,   0,  43,   0,   0,   0}, // 0x01 floor
{ 127,   1,   0, 133,   1,   2, 145,   0,  43,   0,   0,   0}, // 0x02 spike
{  92,   1,   0,  93,   1,   2,   0,  94,  43,  95,   1,   0}, // 0x03 pillar
{  46,   1,   0,  47,   1,   2,   0,  48,  43,  49,   3,   0}, // 0x04 door
{  41,   1,   1,  35,   1,   3, 145,   0,  36,   0,   0,   0}, // 0x05 stuck floor
{  41,   1,   0,  42,   1,   2, 145,   0,  96,   0,   0,   0}, // 0x06 close button
{  46,   1,   0,   0,   0,   2,   0,   0,  43,  49,   3,   0}, // 0x07 door top with floor
{  86,   1,   0,  87,   1,   2,   0,   0,  43,  88,   1,   0}, // 0x08 big pillar bottom
{   0,   0,   0,  89,   0,   3,   0,  90,   0,  91,   1,   3}, // 0x09 big pillar top
{  41,   1,   0,  42,   1,   2, 145,   0,  43,  12,   2,  -3}, // 0x0A potion
{   0,   1,   0,   0,   0,   0, 145,   0,   0,   0,   0,   0}, // 0x0B loose floor
{   0,   0,   0,   0,   0,   2,   0,   0,  85,  49,   3,   0}, // 0x0C door top
{  75,   1,   0,  42,   1,   2,   0,   0,  43,  77,   0,   0}, // 0x0D mirror
{  97,   1,   0,  98,   1,   2, 145,   0,  43, 100,   0,   0}, // 0x0E debris
{ 147,   1,   0,  42,   1,   1, 145,   0, 149,   0,   0,   0}, // 0x0F open button
{  41,   1,   0,  37,   0,   0,   0,  38,  43,   0,   0,   0}, // 0x10 leveldoor left
{   0,   0,   0,  39,   1,   2,   0,  40,  43,   0,   0,   0}, // 0x11 leveldoor right
{   0,   0,   0,  42,   1,   2, 145,   0,  43,   0,   0,   0}, // 0x12 chomper
{  41,   1,   0,  42,   1,   2,   0,   0,  43,   0,   0,   0}, // 0x13 torch
{   0,   0,   0,   1,   1,   2,   0,   2,   0,   0,   0,   0}, // 0x14 wall
{  30,   1,   0,  31,   1,   2,   0,   0,  43,   0,   0,   0}, // 0x15 skeleton
{  41,   1,   0,  42,   1,   2, 145,   0,  43,   0,   0,   0}, // 0x16 sword
{  41,   1,   0,  10,   0,   0,   0,  11,  43,   0,   0,   0}, // 0x17 balcony left
{   0,   0,   0,  12,   1,   2,   0,  13,  43,   0,   0,   0}, // 0x18 balcony right
{  92,   1,   0,  42,   1,   2, 145,   0,  43,  95,   1,   0}, // 0x19 lattice pillar
{   1,   0,   0,   0,   0,   0,   0,   0,   2,   9,   0, -53}, // 0x1A lattice down
{   3,   0, -10,   0,   0,   0,   0,   0,   0,   9,   0, -53}, // 0x1B lattice small
{   4,   0, -10,   0,   0,   0,   0,   0,   0,   9,   0, -53}, // 0x1C lattice left
{   5,   0, -10,   0,   0,   0,   0,   0,   0,   9,   0, -53}, // 0x1D lattice right
{  97,   1,   0,  98,   1,   2,   0,   0,  43, 100,   0,   0}, // 0x1E debris with torch
};

// data:4334
short drawn_row;
// data:4352
short draw_bottom_y;
// data:4326
short draw_main_y;
// data:34D0
short drawn_col;

// data:6592
byte tile_left;
// data:4CCC
byte modifier_left;


// -----------------------------------------------------------------------------
// Macintosh COLOR 640x400 compositor (v6 foundation; v11 floor states)
//
// The original Mac renderer places SHPD images at a logical anchor, then
// subtracts each image's SHPD origin in *native* pixels:
//   px = anchor_x * 2 - origin_x
//   py = anchor_y * 2 - origin_y
// The old SDLPoP path instead treated graphics as DOS top-left/bottom-aligned
// rectangles.  The two models are close enough to look plausible, but odd
// origins and tall palace pieces drift or overlap incorrectly.  The helpers
// below preserve SDLPoP gameplay/tile state while using the exact Mac formula.

#define MAC_CELL_TOP_ANCHOR    (-63)
#define MAC_FLOOR_TOP_ANCHOR   (-4)
#define MAC_FLOOR_BOTTOM_ANCHOR (-1)

/* v11.9: native climb redraws need the same midtable floor occlusion
 * semantics as the original renderer. */
static void mac_draw_floor_overlay_exact(void);
static void mac_draw_other_overlay_exact(void);
int mac_add_mid_anchor_exact(short chtab_id, int id,
		int anchor_x, int anchor_y, int blit, byte peel);
/* The generic airborne overlay reuses native components, on the MID layer. */
static int mac_back_to_mid = 0;
/* v11.13: Dynamic foreground redraws stay a separate compositor phase from
 * full-tile redraws, but native dungeon pillar foreground must remain enabled
 * during climb-up. The pillar face is the foreground occluder that hides the
 * Prince's lower body behind the ledge, matching the original Macintosh draw
 * order. Earlier v11.10-v11.12 suppression made his leg/foot show through it. */

/* v7: Moving actors still use the original gameplay/object coordinate system,
 * but native Macintosh actor and combat-sword images carry SHPD origins just
 * like the room art.  These chtabs are character/sword layers whose logical
 * anchor must remain untouched while the native image is positioned around it. */
static int mac_native_actor_chtab(short chtab_id) {
	if (!uses_mac_native_full()) return 0;
	return chtab_id == id_chtab_0_sword ||
	       chtab_id == id_chtab_2_kid ||
	       chtab_id == id_chtab_3_princessinstory ||
	       chtab_id == id_chtab_4_jaffarinstory_princessincutscenes ||
	       chtab_id == id_chtab_5_guard;
}

static int mac_add_back_or_fore_exact(int fore, short chtab_id, int id,
		int anchor_x, int anchor_y, int blit, int clip_bottom, int clip_top, int physical_dy) {
	if (id == 0) return 0;
	if (!fore && mac_back_to_mid) {
		return mac_add_mid_anchor_exact(chtab_id, id, anchor_x, anchor_y, blit, 0);
	}
	back_table_type* table = fore ? foretable : backtable;
	short* count_ptr = fore ? &foretable_count : &backtable_count;
	word index = *count_ptr;
	if (index >= (fore ? FORETABLE_MAX : 200)) {
		show_dialog(fore ? "ForeTable Overflow" : "BackTable Overflow");
		return 0;
	}
	image_type* image = get_image(chtab_id, id - 1);
	if (image == NULL) return 0;
	short ox = 0, oy = 0;
	bool has_origin = get_native_image_origin(image, &ox, &oy);

	back_table_type* item = &table[index];
	item->xh = (sbyte)(anchor_x / 8);
	item->xl = (sbyte)(anchor_x % 8);
	item->y = (short)(anchor_y - logical_image_height(image) + 1);
	item->chtab_id = (byte)chtab_id;
	item->id = (byte)(id - 1);
	item->blit = blit;
	item->physical_x = (short)(anchor_x * 2 - (has_origin ? ox : 0));
	item->physical_y = (short)(anchor_y * 2 - (has_origin ? oy : image->h) + physical_dy);
	item->physical_clip_bottom = clip_bottom;
	item->physical_clip_top = clip_top;
	item->use_physical = 1;
	if (draw_mode) draw_back_fore(fore ? 1 : 0, index);
	(*count_ptr)++;
	return 1;
}

static int mac_add_back_exact(short chtab_id, int id, int dx, int dy, int blit) {
	return mac_add_back_or_fore_exact(0, chtab_id, id,
		draw_xh * 8 + dx, draw_main_y + 1 + dy, blit, -1, -1, 0);
}

static int mac_add_fore_exact(short chtab_id, int id, int dx, int dy, int blit) {
	return mac_add_back_or_fore_exact(1, chtab_id, id,
		draw_xh * 8 + dx, draw_main_y + 1 + dy, blit, -1, -1, 0);
}

int mac_add_fore_anchor_exact(short chtab_id, int id, int anchor_x, int anchor_y, int blit) {
	return mac_add_back_or_fore_exact(1, chtab_id, id, anchor_x, anchor_y, blit, -1, -1, 0);
}

/* Original Mac CODE 2:$223E. Render-only state never touches random_seed. */
static word mac_palace_random(dword* seed, word maximum) {
	word upper = *seed >> 16;
	word old_low = (word)*seed;
	word low = (word)(*seed ^ 0x569A) + upper;
	*seed = (dword)low * 0x6A59;
	return (dword)(word)((word)*seed + old_low) * (maximum + 1) >> 16;
}

/* Original Mac CODE 3:$0B72: 3 rows, 4 courses, 11 colors/course,
 * palette 76..79 on even courses and 80..83 on odd, no repeated neighbors. */
static byte mac_palace_color(int row, int course, int column) {
	static byte colors[25][132], ready[25];
	int room = drawn_room <= 24 ? drawn_room : 0;
	if (!ready[room]) {
		dword seed = room;
		mac_palace_random(&seed, 1);
		for (int r = 0; r < 3; ++r) for (int c = 0; c < 4; ++c) {
			int previous = -1;
			for (int col = 0; col < 11; ++col) {
				int color;
				do { color = ((c & 1) ? 4 : 0) + mac_palace_random(&seed, 3); } while (color == previous);
				colors[room][r * 44 + c * 11 + col] = color;
				previous = color;
			}
		}
		ready[room] = 1;
	}
	return colors[room][row * 44 + course * 11 + column];
}

static void mac_add_wall_fill(int fore, int dx, int bottom, int width, int height, byte color) {
	int x = (draw_xh * 8 + dx) * 2;
	int y = (bottom + 1 - height) * 2;
	if (!fore && mac_back_to_mid) {
		if (midtable_count >= 50) { show_dialog("MidTable Overflow"); return; }
		midtable_type* item = &midtable[midtable_count];
		memset(item, 0, sizeof(*item));
		item->use_physical = native_entry_fill;
		item->physical_x = x; item->physical_y = y;
		item->physical_w = width * 2; item->physical_h = height * 2;
		item->fill_rgb = mac_palace_rgb[color];
		if (draw_mode) draw_mid(midtable_count);
		++midtable_count;
	} else {
		short* count = fore ? &foretable_count : &backtable_count;
		if (*count >= (fore ? FORETABLE_MAX : 200)) { show_dialog("Wall fill table overflow"); return; }
		back_table_type* item = &(fore ? foretable : backtable)[*count];
		memset(item, 0, sizeof(*item));
		item->use_physical = native_entry_fill;
		item->physical_x = x; item->physical_y = y;
		item->physical_w = width * 2; item->physical_h = height * 2;
		item->fill_rgb = mac_palace_rgb[color];
		if (draw_mode) draw_back_fore(fore, *count);
		++*count;
	}
}

static void mac_draw_palace_wall(int fore) {
	int row = drawn_row, col = drawn_col;
	if (row < 0 || row > 2 || col < 0 || col > 9) return;
	/* Native floor registration is three logical pixels above the DOS
	 * baselines. The recovered Mac rectangle dimensions remain unchanged. */
	int main_y = draw_main_y - 3, bottom_y = draw_bottom_y - 3;
	if (fore) {
		mac_add_wall_fill(1, 0, main_y - 40, 32, 20, mac_palace_color(row, 0, col));
		mac_add_wall_fill(1, 0, main_y - 19, 16, 21, mac_palace_color(row, 1, col));
		mac_add_wall_fill(1, 16, main_y - 19, 16, 21, mac_palace_color(row, 1, col + 1));
		mac_add_wall_fill(1, 0, main_y, 8, 19, mac_palace_color(row, 2, col));
		mac_add_wall_fill(1, 8, main_y, 24, 19, mac_palace_color(row, 2, col + 1));
	}
	mac_add_wall_fill(fore, 0, bottom_y, 32, 3, mac_palace_color(row, 3, col));
	dword seed = drawn_room + row * 10 + col;
	int bottom_id = 15 + mac_palace_random(&seed, 2);
	if (fore) {
		mac_add_fore_exact(id_chtab_7_environmentwall, bottom_id, 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
		const int ids[] = {3, 6, 9, 12};
		const int ys[] = {-53, -34, -13, 0};
		for (int i = 0; i < 4; ++i)
			mac_add_fore_exact(id_chtab_7_environmentwall, ids[i] + mac_palace_random(&seed, 2),
			                   i ? 0 : 24, MAC_FLOOR_TOP_ANCHOR + ys[i], blitters_10h_transp);
	} else {
		mac_add_back_exact(id_chtab_7_environmentwall, bottom_id, 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
	}
}

/* v11: Interactive floors are composed from Macintosh-native components with
 * explicit logical anchors.  Keeping the IDs and anchors together avoids
 * accidentally reusing a DOS-only Y correction for one state of the same
 * floor family.  IDs are the 1-based chtab IDs used by add_*table(). */
typedef struct mac_floor_state_type {
	byte base_id;
	sbyte base_anchor_y;
	byte right_id;
	sbyte right_anchor_y;
	byte front_id;
	sbyte front_anchor_y;
} mac_floor_state_type;

static int mac_get_floor_state(byte tile, byte modifier, mac_floor_state_type* out) {
	static const byte loose_base[11]  = {0x29,0x45,0x29,0x46,0x46,0x29,0x29,0x29,0x46,0x46,0x46};
	static const byte loose_right[11] = {0x2A,0x47,0x2A,0x48,0x48,0x2A,0x2A,0x2A,0x48,0x48,0x48};
	static const byte loose_front[11] = {0x2B,0x49,0x2B,0x4A,0x4A,0x2B,0x2B,0x2B,0x4A,0x4A,0x4A};

	if (out == NULL) return 0;
	memset(out, 0, sizeof(*out));
	switch (tile) {
		case tiles_1_floor:
			*out = (mac_floor_state_type){0x29, MAC_FLOOR_TOP_ANCHOR, 0x2A, MAC_FLOOR_TOP_ANCHOR + 2, 0x2B, MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		case tiles_2_spike:
			*out = (mac_floor_state_type){0x7F, MAC_FLOOR_TOP_ANCHOR, 0x85, MAC_FLOOR_TOP_ANCHOR + 2, 0x2B, MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		case tiles_5_stuck:
			*out = (mac_floor_state_type){0x29, MAC_FLOOR_TOP_ANCHOR + 1, 0x23, MAC_FLOOR_TOP_ANCHOR + 3, 0x24, MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		case tiles_6_closer:
			*out = (mac_floor_state_type){0x29, MAC_FLOOR_TOP_ANCHOR, 0x2A, MAC_FLOOR_TOP_ANCHOR + 2, 0x60, MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		case tiles_11_loose: {
			byte frame = MIN(get_loose_frame(modifier), 10);
			*out = (mac_floor_state_type){loose_base[frame], MAC_FLOOR_TOP_ANCHOR,
				loose_right[frame], MAC_FLOOR_BOTTOM_ANCHOR - 1,
				loose_front[frame], MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		}
		case tiles_14_debris:
		case tiles_30_torch_with_debris:
			*out = (mac_floor_state_type){0x61, MAC_FLOOR_TOP_ANCHOR, 0x62, MAC_FLOOR_TOP_ANCHOR + 2, 0x2B, MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		case tiles_15_opener:
			*out = (mac_floor_state_type){0x93, MAC_FLOOR_TOP_ANCHOR, 0x2A, MAC_FLOOR_TOP_ANCHOR + 1, 0x95, MAC_FLOOR_BOTTOM_ANCHOR};
			return 1;
		default:
			return 0;
	}
}

static int mac_floor_debug_enabled(void) {
	return getenv("SDLPOP_MAC_FLOOR_DEBUG") != NULL;
}

static void mac_debug_floor_piece(const char* role, byte tile, byte modifier,
		int id, int anchor_x, int anchor_y, image_type* image) {
	if (!mac_floor_debug_enabled() || id == 0 || image == NULL) return;
	short ox = 0, oy = 0;
	bool has_origin = get_native_image_origin(image, &ox, &oy);
	printf("MAC_FLOOR tile=%u mod=%u role=%s image=%d res=%d anchor_x=%d anchor_y=%d "
	       "origin_x=%d origin_y=%d physical_x=%d physical_y=%d native=%d\n",
	       tile, modifier, role, id, 200 + id, anchor_x, anchor_y,
	       has_origin ? ox : 0, has_origin ? oy : image->h,
	       anchor_x * 2 - (has_origin ? ox : 0),
	       anchor_y * 2 - (has_origin ? oy : image->h),
	       is_native_resolution_image(image) ? 1 : 0);
}

static int mac_add_floor_back_exact(const char* role, byte tile, byte modifier,
		int id, int dx, int anchor_y, int blit) {
	if (id == 0) return 0;
	image_type* image = get_image(id_chtab_6_environment, id - 1);
	int anchor_x = draw_xh * 8 + dx;
	int absolute_anchor_y = draw_main_y + 1 + anchor_y;
	mac_debug_floor_piece(role, tile, modifier, id, anchor_x, absolute_anchor_y, image);
	return mac_add_back_or_fore_exact(0, id_chtab_6_environment, id,
		anchor_x, absolute_anchor_y, blit, -1, -1, 0);
}

static int mac_add_floor_fore_exact(const char* role, byte tile, byte modifier,
		int id, int dx, int anchor_y, int blit) {
	if (id == 0) return 0;
	image_type* image = get_image(id_chtab_6_environment, id - 1);
	int anchor_x = draw_xh * 8 + dx;
	int absolute_anchor_y = draw_main_y + 1 + anchor_y;
	mac_debug_floor_piece(role, tile, modifier, id, anchor_x, absolute_anchor_y, image);
	return mac_add_back_or_fore_exact(1, id_chtab_6_environment, id,
		anchor_x, absolute_anchor_y, blit, -1, -1, 0);
}

/* Native moving-floor pieces live in the midtable, but their Y values are
 * Macintosh logical anchors, not DOS bottom-alignment coordinates.  v11 gives
 * midtable entries the same origin-aware placement used by the static room
 * compositor so a loose tile does not jump when it becomes a falling mob. */
int mac_add_mid_anchor_exact(short chtab_id, int id,
		int anchor_x, int anchor_y, int blit, byte peel) {
	if (id == 0) return 0;
	image_type* image = get_image(chtab_id, id - 1);
	if (image == NULL) return 0;
	if (!is_native_resolution_image(image)) {
		sbyte xh = (sbyte)(anchor_x / 8);
		sbyte xl = (sbyte)(anchor_x % 8);
		return add_midtable(chtab_id, id, xh, xl, anchor_y, blit, peel);
	}
	if (midtable_count >= 50) {
		show_dialog("MidTable Overflow");
		return 0;
	}
	short ox = 0, oy = 0;
	bool has_origin = get_native_image_origin(image, &ox, &oy);
	word index = midtable_count;
	midtable_type* item = &midtable[index];
	item->xh = (sbyte)(anchor_x / 8);
	item->xl = (sbyte)(anchor_x % 8);
	item->y = (short)(anchor_y - logical_image_height(image) + 1);
	item->chtab_id = (byte)chtab_id;
	item->id = (byte)(id - 1);
	item->blit = blit;
	item->physical_x = (short)(anchor_x * 2 - (has_origin ? ox : 0));
	item->physical_y = (short)(anchor_y * 2 - (has_origin ? oy : image->h));
	item->use_physical = 1;
	item->peel = peel;
	item->clip.left = obj_clip_left;
	item->clip.right = obj_clip_right;
	item->clip.top = obj_clip_top;
	item->clip.bottom = obj_clip_bottom;
	if (draw_mode) draw_mid(index);
	++midtable_count;
	return 1;
}

static int mac_is_palace(void) {
	return custom->tbl_level_type[current_level] != 0;
}

static void mac_draw_left_below_corner(void) {
	byte type = row_below_left_[drawn_col].tiletype;
	byte mod = row_below_left_[drawn_col].modifier;
	if ((type == tiles_7_doortop_with_floor || type == tiles_12_doortop) && mac_is_palace()) {
		static const byte tapestry_corner[4] = {0, 0x51, 0x53, 0};
		mac_add_back_exact(id_chtab_6_environment, tapestry_corner[mod & 3], 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
	} else if (type == tiles_20_wall) {
		/* Only a real wall in the current column below has a FORE row that
		 * occludes this corner. Synthetic left-level borders keep all pixels. */
		int clip_bottom = row_below_left_[drawn_col + 1].tiletype == tiles_20_wall ?
		                  draw_main_y * 2 + 2 : -1;
		mac_add_back_or_fore_exact(0, id_chtab_7_environmentwall, 2, draw_xh * 8,
		                          draw_main_y + 1 + MAC_FLOOR_BOTTOM_ANCHOR,
		                          blitters_10h_transp, clip_bottom, -1, 0);
	} else if (type < COUNT(tile_table)) {
		mac_add_back_exact(id_chtab_6_environment, tile_table[type].topright_id, 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
	}
}

static int mac_can_see_bottomleft(void) {
	return curr_tile == tiles_0_empty ||
	       curr_tile == tiles_9_bigpillar_top ||
	       curr_tile == tiles_12_doortop ||
	       curr_tile == tiles_26_lattice_down;
}

static void mac_draw_left_floor_overhang(void) {
	if (!mac_can_see_bottomleft()) return;

	/* v11.8: Always submit the below-left corner. v11.7 returned here for a
	 * depressed/stuck plate, which also dropped the corner piece. When a plate
	 * changes state, redraw_tile_height() wipes the plate cell AND the cell to
	 * its right to black and redraws only those two tiles, so the corner is the
	 * only thing that fills the area under the plate's diagonal edge. With the
	 * corner skipped, that area stayed wiped -> the reported black triangle. */
	mac_draw_left_below_corner();

	/* A depressed/stuck plate carries its complete exposed edge in native
	 * component 0x23, whose opaque pixels already cover everything above its
	 * diagonal. Do not add the generic 0x2A black silhouette: it is 2 rows
	 * higher than 0x23 and has a different diagonal (v11.4..v11.6 double shadow,
	 * v11.7 black band). mac_draw_left_other_overhang() still submits 0x23. */
	if (tile_left == tiles_5_stuck) return;

	/* v11.4: Preserve the original exposed-ledge occlusion order. The tile
	 * below-left can contribute a top-right corner into the gap. SDLPoP then
	 * paints the generic floor-right silhouette in BLACK before drawing the
	 * state-specific right edge. This is an occlusion pass only: 0x2A is drawn
	 * with the black blitter, then mac_draw_left_other_overhang() paints the
	 * real Mac edge on top. Do not replace this with a second colored floor
	 * component. */
	if (tile_left < COUNT(tile_table) && tile_table[tile_left].floor_right) {
		mac_add_floor_back_exact("right-mask", tiles_1_floor, 0, 0x2A, 0,
			MAC_FLOOR_TOP_ANCHOR + 2, blitters_9_black);
	}
}

static void mac_draw_below_gate_top(void) {
	byte below_type = row_below_left_[drawn_col].tiletype;
	byte below_mod = row_below_left_[drawn_col].modifier;
	if ((curr_tile == tiles_9_bigpillar_top || curr_tile == tiles_0_empty || curr_tile == tiles_12_doortop) &&
	    below_type == tiles_4_gate) {
		byte progress = MIN(below_mod, 188) / 4;
		mac_add_back_exact(id_chtab_6_environment, 0x44, 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
		mac_add_back_exact(id_chtab_6_environment, 0x3C + (progress % 8), 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
	}
}

static int mac_get_floor_right_piece(byte tile, byte modifier, byte neighbor,
		int* id, int* anchor_y) {
	mac_floor_state_type state;
	if (!mac_get_floor_state(tile, modifier, &state)) return 0;
	if (state.right_id == 0) return 0;

	/* v11.3: Keep the Macintosh right component that belongs to the tile
	 * state, including at an exposed edge. The DOS renderer substitutes the
	 * normal-floor right wedge for a stuck/depressed button beside empty space,
	 * but that rule is not valid for the native Mac art: 0x23 is the depressed
	 * plate edge, while 0x2A is the taller/broader normal-floor wedge. Swapping
	 * to 0x2A produces the characteristic double-width dark shadow at a gap. */
	(void)neighbor;

	*id = state.right_id;
	*anchor_y = state.right_anchor_y;
	return 1;
}

static void mac_draw_left_other_overhang(void) {
	if (curr_tile == tiles_20_wall) return;
	if (tile_left >= COUNT(tile_table)) return;
	const piece* pc = &tile_table[tile_left];
	/* Loose-floor right pieces are animation frames and keep their historical
	 * ordering in mac_draw_left_animated(). All other interactive floor states
	 * choose one state/context-specific right component here. */
	if (tile_left != tiles_1_floor && tile_left != tiles_11_loose) {
		int right_id = 0, right_anchor_y = 0;
		if (mac_get_floor_right_piece(tile_left, modifier_left, curr_tile, &right_id, &right_anchor_y)) {
			mac_add_floor_back_exact("right", tile_left, modifier_left, right_id, 0,
				right_anchor_y, blitters_10h_transp);
			if (mac_is_palace() && pc->stripe_id) {
				mac_add_back_exact(id_chtab_6_environment, pc->stripe_id, 0,
					MAC_FLOOR_TOP_ANCHOR - 0x1B, blitters_10h_transp);
			}
			if (tile_left == tiles_30_torch_with_debris) {
				mac_add_back_exact(id_chtab_6_environment, 0x92, 0,
					MAC_FLOOR_BOTTOM_ANCHOR - 0x1C, blitters_10h_transp);
			}
			return;
		}
	}
	switch (tile_left) {
		case tiles_0_empty: {
			static const byte ids[4] = {0, 0x7C, 0x7D, 0x7E};
			static const sbyte offs[4] = {0, -20, -20, 0};
			if (modifier_left < 4) mac_add_back_exact(id_chtab_6_environment, ids[modifier_left], 0, MAC_FLOOR_TOP_ANCHOR + offs[modifier_left], blitters_10h_transp);
		} break;
		case tiles_1_floor: {
			static const byte ids[4] = {0x2C, 0x7C, 0x2D, 0x2D};
			byte mod = modifier_left > 3 ? 0 : modifier_left;
			int right_id = 0, right_anchor_y = 0;
			if (mac_get_floor_right_piece(tile_left, modifier_left, curr_tile, &right_id, &right_anchor_y)) {
				mac_add_floor_back_exact("right", tile_left, modifier_left, right_id, 0,
					right_anchor_y, blitters_10h_transp);
			}
			if (!mac_is_palace()) {
				if (mod != 0) mac_add_back_exact(id_chtab_6_environment, ids[mod], 0, MAC_FLOOR_TOP_ANCHOR - 0x14, blitters_10h_transp);
			} else if (mod != 1) {
				mac_add_back_exact(id_chtab_6_environment, ids[mod], 0, MAC_FLOOR_TOP_ANCHOR + 2, blitters_10h_transp);
			}
		} break;
		case tiles_7_doortop_with_floor:
		case tiles_12_doortop:
			if (mac_is_palace()) {
				static const byte ids[4] = {0x4E, 0x50, 0x52, 0};
				mac_add_back_exact(id_chtab_6_environment, ids[modifier_left & 3], 0,
					MAC_FLOOR_TOP_ANCHOR + pc->right_y, blitters_10h_transp);
			}
			break;
		case tiles_20_wall:
			if (mac_is_palace() && !(modifier_left & 0x80)) {
				mac_add_back_exact(id_chtab_6_environment, 0x54, 0x18, -0x1B, blitters_10h_transp);
			}
			mac_add_back_exact(id_chtab_7_environmentwall, 1, 0, MAC_FLOOR_TOP_ANCHOR + pc->right_y, blitters_10h_transp);
			break;
		default:
			if (pc->right_id) mac_add_back_exact(id_chtab_6_environment, pc->right_id, 0, MAC_FLOOR_TOP_ANCHOR + pc->right_y, blitters_10h_transp);
			if (mac_is_palace() && pc->stripe_id) mac_add_back_exact(id_chtab_6_environment, pc->stripe_id, 0, MAC_FLOOR_TOP_ANCHOR - 0x1B, blitters_10h_transp);
			if (tile_left == tiles_19_torch || tile_left == tiles_30_torch_with_debris) {
				mac_add_back_exact(id_chtab_6_environment, 0x92, 0, MAC_FLOOR_BOTTOM_ANCHOR - 0x1C, blitters_10h_transp);
			}
			break;
	}
}

static void mac_draw_gate_parts(int fore) {
	byte progress = MIN(modifier_left, 188) / 4;
	int gate_anchor_y = MAC_FLOOR_TOP_ANCHOR - (progress + 1);
	#define MAC_GATE_ADD(id, dx, dy) do { \
		if (fore) mac_add_fore_exact(id_chtab_6_environment, (id), (dx), (dy), blitters_10h_transp); \
		else mac_add_back_exact(id_chtab_6_environment, (id), (dx), (dy), blitters_10h_transp); \
	} while (0)
	if (fore) {
		/* Match draw_gate_fore(): only moving bars belong over the actor.
		 * Repainting the baked floor wedge here leaves a dark diagonal strip. */
		MAC_GATE_ADD(0x33, 0, gate_anchor_y - 2);
	} else if ((gate_anchor_y + 12) < MAC_FLOOR_TOP_ANCHOR) {
		MAC_GATE_ADD(0x32, 0, gate_anchor_y);
	} else {
		MAC_GATE_ADD(tile_table[tiles_4_gate].right_id, 0, MAC_FLOOR_TOP_ANCHOR + tile_table[tiles_4_gate].right_y);
		MAC_GATE_ADD(0x33, 0, gate_anchor_y - 2);
	}
	int delta_y = gate_anchor_y - 12;
	for (; delta_y >= MAC_CELL_TOP_ANCHOR + 8; delta_y -= 8) MAC_GATE_ADD(0x34, 0, delta_y);
	int partial = delta_y - MAC_CELL_TOP_ANCHOR;
	if (!fore && partial >= 0 && partial < 8) MAC_GATE_ADD(0x3B - partial, 0, delta_y);
	#undef MAC_GATE_ADD
}

/* A full native panel replaces the DOS stack of four-pixel slices. Map the
 * existing 0..43 animation phase to its SHPD-registered height so closed meets
 * the lintel and fully open retracts completely. Gameplay keeps its modifiers. */
static int mac_leveldoor_geometry(int anchor_x, int anchor_y, byte modifier,
                                 rect_type* opening, int* lift) {
	image_type* door = get_image(id_chtab_6_environment, 0x21 - 1);
	short ox, oy;
	if (!door || !get_native_image_origin(door, &ox, &oy)) return 0;
	opening->left = anchor_x * 2 - ox;
	opening->top = anchor_y * 2 - oy;
	opening->right = opening->left + door->w;
	opening->bottom = opening->top + door->h;
	*lift = (MIN(modifier, 43) * door->h + 21) / 43;
	return 1;
}

int mac_get_leveldoor_clip(rect_type* clip) {
	if (!uses_mac_native_full() || !Char.room || Char.room > 24 || Char.curr_row < 0 || Char.curr_row > 2) return 0;
	/* Resolve the exit beside this actor, rather than using globals left by
	 * another door/row or by an earlier room. Do not mutate tile lookup state. */
	int best = -1, distance = 10000;
	for (int col = 0; col < 10; ++col) {
		int pos = (Char.room - 1) * 30 + Char.curr_row * 10 + col;
		if ((level.fg[pos] & 0x1F) != tiles_16_level_door_left) continue;
		int d = abs(Char.curr_col - col);
		if (d < distance) { best = pos; distance = d; }
	}
	if (best < 0) return 0;
	rect_type opening;
	int lift;
	if (!mac_leveldoor_geometry((best % 10 + 1) * 32 + 8,
	                           Char.curr_row * 63 + 63 + MAC_FLOOR_TOP_ANCHOR - 13,
	                           level.bg[best], &opening, &lift)) return 0;
	clip->top = (opening.bottom - lift + 1) / 2;
	clip->right = opening.right / 2;
	return 1;
}

static void mac_draw_left_animated(void) {
	switch (tile_left) {
		case tiles_2_spike: {
			static const byte ids[10] = {0,0x86,0x87,0x88,0x89,0x8A,0x89,0x87,0x86,0};
			byte frame = (modifier_left & 0x80) ? 5 : MIN(modifier_left, 9);
			mac_add_floor_back_exact("spike-right-anim", tile_left, modifier_left, ids[frame], 0,
				MAC_FLOOR_TOP_ANCHOR - 7, blitters_10h_transp);
		} break;
		case tiles_4_gate:
			mac_draw_gate_parts(0);
			break;
		case tiles_11_loose: {
			mac_floor_state_type state;
			if (mac_get_floor_state(tile_left, modifier_left, &state)) {
				mac_add_floor_back_exact("right", tile_left, modifier_left, state.right_id, 0,
					state.right_anchor_y, blitters_10h_transp);
			}
		} break;
		case tiles_16_level_door_left: {
			int passage_y = MAC_FLOOR_TOP_ANCHOR - 0x0D;
			rect_type opening;
			int lift;
			if (!mac_leveldoor_geometry(draw_xh * 8 + 8, draw_main_y + 1 + passage_y,
			                            modifier_left, &opening, &lift)) break;
			/* The closed panel's native top meets the carved lintel. CELL_TOP
			 * is 13/16 physical pixels lower and introduced a black gap. */
			int clip_top = opening.top;
			int clip_bottom = opening.bottom;
			leveldoor_ybottom = (opening.bottom - lift + 1) / 2 - 1;
			leveldoor_right = opening.right / 2;
			int saved_mid = mac_back_to_mid;
			mac_back_to_mid = 0; // Legacy draw_leveldoor() also keeps these pieces in BACK.
			/* Match the Mac layering: passage, moving door, optional stairs, then
			 * the door once more so it remains in front of the exit stair art. */
			mac_add_back_exact(id_chtab_6_environment, 0x63, 8, passage_y, blitters_10h_transp);
			mac_add_back_or_fore_exact(0, id_chtab_6_environment, 0x21,
			                          draw_xh * 8 + 8, draw_main_y + 1 + passage_y,
			                          blitters_10h_transp, clip_bottom, clip_top, -lift);
			if (level.start_room != drawn_room && modifier_left != 0) {
				mac_add_back_exact(id_chtab_6_environment, 0x90, 8, passage_y, blitters_10h_transp);
				mac_add_back_or_fore_exact(0, id_chtab_6_environment, 0x21,
				                          draw_xh * 8 + 8, draw_main_y + 1 + passage_y,
				                          blitters_10h_transp, clip_bottom, clip_top, -lift);
			}
			mac_back_to_mid = saved_mid;
		} break;
		case tiles_19_torch:
		case tiles_30_torch_with_debris:
			if (modifier_left < 9) {
				int y = MAC_FLOOR_TOP_ANCHOR - (mac_is_palace() ? 0x2A : 0x28);
				mac_add_back_exact(id_chtab_1_flameswordpotion, modifier_left + 1, 8 + mac_is_palace(), y, blitters_10h_transp);
			}
			break;
	}
}

static void mac_draw_current_floor_front(void) {
	mac_floor_state_type floor_state;
	if (mac_get_floor_state(curr_tile, curr_modifier, &floor_state)) {
		mac_add_floor_back_exact("front", curr_tile, curr_modifier, floor_state.front_id, 0,
			floor_state.front_anchor_y, blitters_10h_transp);
		return;
	}

	int id = 0;
	short chtab = id_chtab_6_environment;
	if (curr_tile == tiles_20_wall) {
		if (mac_is_palace()) { mac_draw_palace_wall(0); return; }
		static const byte dungeon_ids[4] = {7,9,5,3};
		chtab = id_chtab_7_environmentwall;
		id = mac_is_palace() ? 3 : dungeon_ids[curr_modifier & 3];
	} else if (curr_tile < COUNT(tile_table)) {
		id = tile_table[curr_tile].bottom_id;
	}
	mac_add_back_exact(chtab, id, 0, MAC_FLOOR_BOTTOM_ANCHOR, blitters_10h_transp);
}

static int mac_lattice_in_foreground(void) {
#ifdef USE_SUPER_HIGH_JUMP
	return fixes->enable_super_high_jump &&
	       ((curr_tile >= tiles_26_lattice_down && curr_tile <= tiles_29_lattice_right) ||
	        (tile_left == tiles_26_lattice_down && curr_tile == tiles_12_doortop));
#else
	return 0;
#endif
}

static void mac_draw_current_floor_top(void) {
	if (curr_tile >= COUNT(tile_table)) return;
	/* Preserve the existing high-jump occlusion rule with native origins. */
	if (mac_lattice_in_foreground()) return;
	mac_floor_state_type floor_state;
	if (mac_get_floor_state(curr_tile, curr_modifier, &floor_state)) {
		int id = floor_state.base_id;
		/* The open-button edge variant is a distinct Mac component when the
		 * neighboring tile is empty; keep the same state anchor. */
		if (curr_tile == tiles_15_opener && tile_left == tiles_0_empty) id = 0x94;
		mac_add_floor_back_exact("base", curr_tile, curr_modifier, id, 0,
			floor_state.base_anchor_y, blitters_10h_transp);
		return;
	}

	int id;
	int y = MAC_FLOOR_TOP_ANCHOR;
	if (tile_left == tiles_26_lattice_down && curr_tile == tiles_12_doortop) {
		id = 6; y += 3;
	} else {
		id = tile_table[curr_tile].base_id;
	}
	mac_add_back_exact(id_chtab_6_environment, id, 0, y + tile_table[curr_tile].base_y, blitters_10h_transp);
}

static int mac_potion_bubble_id(byte bg) {
	static const byte base_ids[8] = {0,0x10,0x11,0x12,0x13,0x14,0x15,0};
	/* Original Mac CODE 4:$42F8: empty/life have no bubbles, feather/invert
	 * are green, hurt/open blue. Translate SDLPoP's effect bits (3..7);
	 * reading the Mac $E0 field directly confuses effect and animation bits. */
	static const sbyte color_offsets[7] = {-1,0,-1,6,6,12,12};
	byte frame = bg & 7;
	byte type = bg >> 3;
	int id = base_ids[frame];
	if (type < COUNT(color_offsets)) {
		if (color_offsets[type] < 0) return 0;
		if (id) id += color_offsets[type];
	}
	return id;
}

static void mac_draw_current_animated(void) {
	switch (curr_tile) {
		case tiles_2_spike: {
			static const byte ids[10] = {0,0x80,0x81,0x82,0x83,0x84,0x83,0x81,0x80,0};
			byte frame = (curr_modifier & 0x80) ? 5 : MIN(curr_modifier, 9);
			mac_add_floor_back_exact("spike-back", curr_tile, curr_modifier, ids[frame], 0,
				MAC_FLOOR_TOP_ANCHOR - 2, blitters_10h_transp);
		} break;
		case tiles_10_potion:
			mac_add_back_exact(id_chtab_1_flameswordpotion, mac_potion_bubble_id(curr_modifier), 25, MAC_FLOOR_TOP_ANCHOR - 14, blitters_10h_transp);
			break;
		case tiles_18_chomper: {
			static const byte vf[7] = {3,2,0,1,4,3,3};
			static const byte bot[5] = {0x65,0x66,0x67,0x68,0x69};
			static const byte top[5] = {0,0,0x6F,0x70,0x71};
			static const byte top_y[5] = {0,0,0x25,0x2F,0x32};
			byte f = vf[MIN(curr_modifier & 0x7F, 6)];
			mac_add_back_exact(id_chtab_6_environment, bot[f], 0, MAC_FLOOR_TOP_ANCHOR, blitters_10h_transp);
			mac_add_back_exact(id_chtab_6_environment, top[f], 0, MAC_FLOOR_TOP_ANCHOR - top_y[f], blitters_10h_transp);
			if (curr_modifier & 0x80) mac_add_back_exact(id_chtab_6_environment, f + 0x72, 12, MAC_FLOOR_TOP_ANCHOR - 6, blitters_10h_transp);
		} break;
		case tiles_22_sword:
			mac_add_back_exact(id_chtab_1_flameswordpotion, 10 + (curr_modifier == 1), 0, MAC_FLOOR_TOP_ANCHOR - 3, blitters_10h_transp);
			break;
	}
}

static void mac_draw_current_foreground(void) {
	if (tile_left == tiles_4_gate && Kid.curr_row == drawn_row && Kid.curr_col == drawn_col - 1 && Kid.room != room_R) {
		mac_draw_gate_parts(1);
	}
	if (curr_tile >= COUNT(tile_table)) return;
	const piece* pc = &tile_table[curr_tile];
	switch (curr_tile) {
		case tiles_2_spike: {
			static const byte ids[10] = {0,0x8B,0x8C,0x8D,0x8E,0x8F,0x8E,0x8C,0x8B,0};
			byte frame = (curr_modifier & 0x80) ? 5 : MIN(curr_modifier, 9);
			mac_add_floor_fore_exact("spike-fore", curr_tile, curr_modifier, ids[frame], 0,
				MAC_FLOOR_TOP_ANCHOR - 2, blitters_10h_transp);
		} break;
		case tiles_10_potion: {
			/* CODE 4:$41BC selects the tall Mac bottle only for life. */
			int id = pc->fore_id + 2 * mac_is_palace() + ((curr_modifier >> 3) == 2);
			mac_add_fore_exact(id_chtab_1_flameswordpotion, id, pc->fore_x * 8 + 6, MAC_FLOOR_TOP_ANCHOR + pc->fore_y, blitters_10h_transp);
			mac_add_fore_exact(id_chtab_1_flameswordpotion, mac_potion_bubble_id(curr_modifier), 25, MAC_FLOOR_TOP_ANCHOR - 14, blitters_10h_transp);
		} break;
		case tiles_18_chomper: {
			static const byte vf[7] = {3,2,0,1,4,3,3};
			static const byte ids[5] = {0x6A,0x6B,0x6C,0x6D,0x6E};
			byte f = vf[MIN(curr_modifier & 0x7F, 6)];
			mac_add_fore_exact(id_chtab_6_environment, ids[f], 0, MAC_FLOOR_TOP_ANCHOR, blitters_10h_transp);
			if (curr_modifier & 0x80) mac_add_fore_exact(id_chtab_6_environment, f + 0x77, 12, MAC_FLOOR_TOP_ANCHOR - 6, blitters_10h_transp);
		} break;
		case tiles_3_pillar:
			/* v11.13: Keep the native pillar foreground in FORE during climb-up.
			 * Original Mac layering uses this face to hide the Prince's lower
			 * body behind the pillar/ledge while the dedicated climb MID overlay
			 * handles the thin floor lip. */
			if (pc->fore_id) mac_add_fore_exact(id_chtab_6_environment, pc->fore_id,
				pc->fore_x * 8, MAC_FLOOR_TOP_ANCHOR + pc->fore_y, blitters_10h_transp);
			break;
		case tiles_20_wall: {
			static const byte dungeon_ids[4] = {0x08,0x0A,0x06,0x04};
			if (mac_is_palace()) { mac_draw_palace_wall(1); break; }
			mac_add_fore_exact(id_chtab_7_environmentwall, dungeon_ids[curr_modifier & 3], 0, MAC_FLOOR_TOP_ANCHOR, blitters_10h_transp);
		} break;
		default:
			if (mac_lattice_in_foreground()) {
				int combined = tile_left == tiles_26_lattice_down && curr_tile == tiles_12_doortop;
				mac_add_fore_exact(id_chtab_6_environment, combined ? 6 : pc->base_id, 0,
				                   MAC_FLOOR_TOP_ANCHOR + pc->base_y + (combined ? 3 : 0), blitters_10h_transp);
			}
			if (pc->fore_id) mac_add_fore_exact(id_chtab_6_environment, pc->fore_id, pc->fore_x * 8, MAC_FLOOR_TOP_ANCHOR + pc->fore_y, blitters_10h_transp);
			break;
	}
}

static void mac_draw_tile_exact(void) {
	mac_draw_left_floor_overhang();
	mac_draw_below_gate_top();
	mac_draw_left_other_overhang();
	mac_draw_left_animated();
	mac_draw_current_floor_front();
	mac_draw_current_floor_top();
	mac_draw_current_animated();
	mac_draw_current_foreground();
}

/* v11.18: Match draw_tile2()/draw_other_overlay() without DOS placement or
 * a full-tile/foreground redraw. Airborne actors are submitted at an earlier
 * tile; these native pieces must follow them in MID to occlude the overlap. */
static void mac_draw_overlay_components(int mid) {
	mac_back_to_mid = mid;
	mac_draw_left_other_overhang();
	mac_draw_left_animated();
	mac_draw_current_floor_front();
	mac_draw_current_floor_top();
	mac_draw_current_animated();
	mac_back_to_mid = 0;
}

static void mac_draw_other_overlay_exact(void) {
	byte type, modifier;
	if (tile_left == tiles_0_empty) {
		mac_draw_overlay_components(1);
	} else if (curr_tile != tiles_0_empty && drawn_col > 0 &&
	           get_tile_to_draw(drawn_room, drawn_col - 2, drawn_row,
	                            &type, &modifier, tiles_0_empty) == tiles_0_empty) {
		mac_draw_overlay_components(1);
		mac_draw_overlay_components(0);
		tile_object_redraw[tbl_line[drawn_row] + drawn_col] = 0xFF;
	}
}

static void mac_draw_tile_above_exact(void) {
	/* Match SDLPoP's above-room partial draw: only pieces that can protrude into
	 * the visible room are submitted. Exact origins still allow odd-pixel edges. */
	mac_draw_left_floor_overhang();
	mac_draw_below_gate_top();
	mac_draw_left_other_overhang();
	mac_draw_current_floor_front();
	mac_draw_current_foreground();
}

// seg008:0006
void redraw_room() {
	free_peels();
	memset(table_counts, 0, sizeof(table_counts));
	reset_obj_clip();
	draw_room();
	clear_tile_wipes();
}

// seg008:0035
void load_room_links() {
	room_BR = 0;
	room_BL = 0;
	room_AR = 0;
	room_AL = 0;
	if (drawn_room) {
		get_room_address(drawn_room);
		room_L = level.roomlinks[drawn_room-1].left;
		room_R = level.roomlinks[drawn_room-1].right;
		room_A = level.roomlinks[drawn_room-1].up;
		room_B = level.roomlinks[drawn_room-1].down;
		if (room_A) {
			room_AL = level.roomlinks[room_A-1].left;
			room_AR = level.roomlinks[room_A-1].right;
		} else {
			if (room_L) {
				room_AL = level.roomlinks[room_L-1].up;
			}
			if (room_R) {
				room_AR = level.roomlinks[room_R-1].up;
			}
		}
		if (room_B) {
			room_BL = level.roomlinks[room_B-1].left;
			room_BR = level.roomlinks[room_B-1].right;
		} else {
			if (room_L) {
				room_BL = level.roomlinks[room_L-1].down;
			}
			if (room_R) {
				room_BR = level.roomlinks[room_R-1].down;
			}
		}
	} else {
		room_B = 0;
		room_A = 0;
		room_R = 0;
		room_L = 0;
	}
}

// seg008:0125
void draw_room() {
	load_leftroom();
	for (drawn_row = 3; drawn_row--; ) { /*2,1,0*/
		load_rowbelow();
		draw_bottom_y = 63 * drawn_row + 65;
		draw_main_y = draw_bottom_y - 3;
		for (drawn_col = 0; drawn_col < 10; drawn_col++) {
			load_curr_and_left_tile();
			draw_tile();
		}
	}
	word saved_room = drawn_room;
	drawn_room = room_A;
	load_room_links();
	load_leftroom();
	drawn_row = 2;
	load_rowbelow();
	for (drawn_col = 0; drawn_col < 10; ++drawn_col) {
		load_curr_and_left_tile();
		draw_main_y = -1;
		draw_bottom_y = 2;
		draw_tile_aboveroom();
	}
	drawn_room = saved_room;
	load_room_links();
}

// seg008:01C7
void draw_tile() {
	if (uses_mac_native_full()) {
		mac_draw_tile_exact();
		return;
	}
	draw_tile_floorright();
	draw_tile_anim_topright();
	draw_tile_right();
	draw_tile_anim_right();
	draw_tile_bottom(0);
	draw_loose(0);
	draw_tile_base();
	draw_tile_anim();
	draw_tile_fore();
}

// seg008:01F2
void draw_tile_aboveroom() {
	if (uses_mac_native_full()) {
		mac_draw_tile_above_exact();
		return;
	}
	draw_tile_floorright();
	draw_tile_anim_topright();
	draw_tile_right();
	draw_tile_bottom(0);
	draw_loose(0);
	draw_tile_fore();
}

// seg008:0211
void redraw_needed(short tilepos) {
	if (wipe_frames[tilepos]) {
		--wipe_frames[tilepos];
		draw_tile_wipe(wipe_heights[tilepos]);
	}
	if (uses_mac_native_full()) {
		int redraw_mac_tile = 0;
		int redraw_mac_floor_overlay = 0;
		int redraw_mac_other_overlay = 0;
		int redraw_mac_fore = 0;
		if (redraw_frames_full[tilepos]) { --redraw_frames_full[tilepos]; redraw_mac_tile = 1; }
		if (redraw_frames_anim[tilepos]) { --redraw_frames_anim[tilepos]; redraw_mac_tile = 1; }
		if (redraw_frames2[tilepos]) { --redraw_frames2[tilepos]; redraw_mac_other_overlay = 1; }
		if (redraw_frames_floor_overlay[tilepos]) {
			--redraw_frames_floor_overlay[tilepos];
			/* v11.10: match the legacy compositor exactly here. A floor-overlay
			 * request is NOT a full tile redraw. Only the dedicated MID-table
			 * climb overlay belongs to this flag. */
			redraw_mac_floor_overlay = 1;
		}
		if (redraw_frames_fore[tilepos]) {
			--redraw_frames_fore[tilepos];
			/* v11.11: foreground redraws are also NOT full-tile redraws. The
			 * legacy path calls draw_tile_fore() here after the actor/object pass.
			 * Keeping this as a distinct phase prevents floor/back pieces from
			 * being redundantly re-submitted while preserving pillar depth. */
			redraw_mac_fore = 1;
		}
		if (redraw_mac_tile) {
			mac_draw_tile_exact();
			/* Dungeon big-pillar-top FORE res291 begins one physical row above
			 * its logical cell (SHPD origin Y=125). A refresh of the floor above
			 * can erase that cap. Restore only its overlap in FORE, preserving the
			 * lower pillar's actor ordering and all unrelated cells. */
			byte below = row_below_left_[drawn_col + 1].tiletype;
			if (drawn_row < 2 && below == tiles_9_bigpillar_top) {
				const piece* pc = &tile_table[below];
				image_type* cap = get_image(id_chtab_6_environment, pc->fore_id - 1);
				short ox, oy;
				int anchor_y = draw_main_y + 1 + 63 + MAC_FLOOR_TOP_ANCHOR + pc->fore_y;
				int boundary = (draw_main_y + 1) * 2;
				if (cap && get_native_image_origin(cap, &ox, &oy) && anchor_y * 2 - oy < boundary)
					mac_add_back_or_fore_exact(1, id_chtab_6_environment, pc->fore_id,
					                          (draw_xh + pc->fore_x) * 8, anchor_y,
					                          blitters_10h_transp, boundary, -1, 0);
			}
			/* v11.19: A native wall side starts one physical row inside the
			 * corner from the tile above. Restore that higher-priority corner
			 * later in this bottom-to-top pass, as a full room draw does. This
			 * also preserves the original synthetic left-level border artwork. */
			if (drawn_row > 0 && tile_left == tiles_20_wall && curr_tile != tiles_20_wall) {
				set_redraw_full(get_tilepos(drawn_col, drawn_row - 1), 1);
			}
		}
		if (redraw_mac_other_overlay) mac_draw_other_overlay_exact();
		if (redraw_mac_floor_overlay) mac_draw_floor_overlay_exact();
		if (tile_object_redraw[tilepos]) {
			if (tile_object_redraw[tilepos] == 0xFF) draw_objtable_items_at_tile(tilepos - 1);
			draw_objtable_items_at_tile(tilepos);
			tile_object_redraw[tilepos] = 0;
		}
		if (redraw_mac_fore) {
			mac_draw_current_foreground();
		}
		return;
	}
	if (redraw_frames_full[tilepos]) {
		--redraw_frames_full[tilepos];
		draw_tile();
	} else {
		if (redraw_frames_anim[tilepos]) {
			--redraw_frames_anim[tilepos];
			draw_tile_anim_topright();
			draw_tile_anim_right();
			draw_tile_anim();
#ifdef FIX_ABOVE_GATE
			draw_tile_fore();
			draw_tile_bottom(0);
#endif
		}
	}
	if (redraw_frames2[tilepos]) {
		--redraw_frames2[tilepos];
		draw_other_overlay();
	} else {
		if (redraw_frames_floor_overlay[tilepos]) {
			--redraw_frames_floor_overlay[tilepos];
			draw_floor_overlay();
		}
	}
	if (tile_object_redraw[tilepos]) {
		if (tile_object_redraw[tilepos] == 0xFF) {
			draw_objtable_items_at_tile(tilepos - 1);
		}
		draw_objtable_items_at_tile(tilepos);
		tile_object_redraw[tilepos] = 0;
	}
	if (redraw_frames_fore[tilepos]) {
		--redraw_frames_fore[tilepos];
		draw_tile_fore();
	}
}

// seg008:02C1
void redraw_needed_above(int column) {
	if (redraw_frames_above[column] != 0) {
		--redraw_frames_above[column];
		if (uses_mac_native_full()) {
			mac_draw_tile_above_exact();
			return;
		}
#ifdef FIX_BIGPILLAR_JUMP_UP
		if (curr_tile != tiles_9_bigpillar_top)
#endif
		{
			draw_tile_wipe(3);
			draw_tile_floorright();
		}
		draw_tile_anim_topright();
		draw_tile_right();
		draw_tile_bottom(1);
		draw_loose(1);
		draw_tile_fore();
	}
}

// seg008:02FE
int get_tile_to_draw(int room, int column, int row, byte* ptr_tiletype, byte* ptr_modifier, byte tile_room0) {
	word tilepos = tbl_line[row] + column;
	if (column == -1) {
		*ptr_tiletype = leftroom_[row].tiletype;
		*ptr_modifier = leftroom_[row].modifier;
	} else if (room) {
		*ptr_tiletype = curr_room_tiles[tilepos] & 0x1F;
		*ptr_modifier = curr_room_modif[tilepos];
	} else {
		*ptr_modifier = 0;
		*ptr_tiletype = tile_room0;
	}
	// Is this a pressed button?
	byte tiletype = (*ptr_tiletype) & 0x1F;
	byte modifier = *ptr_modifier;
	if (tiletype == tiles_6_closer) {
		if (get_doorlink_timer(modifier) > 1) {
			*ptr_tiletype = tiles_5_stuck;
		}
	} else if (tiletype == tiles_15_opener) {
		if (get_doorlink_timer(modifier) > 1) {
			*ptr_modifier = 0;
			*ptr_tiletype = tiles_1_floor;
		}
	}
#ifdef USE_FAKE_TILES
	else if (tiletype == tiles_0_empty) {
		if (modifier == 4 || modifier == 12) {     // display a fake floor
			*ptr_tiletype = tiles_1_floor;
			*ptr_modifier = (modifier == 12) ? 1 : 0; // 12: noblue option
		}
		else if (modifier == 5 || modifier == 13) {   // display a fake wall
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = (modifier == 13) ? 0x80 : 0; // 13: noblue option
		}
		else if (modifier == 50) {   // display a fake wall (pattern: no walls left or right)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 0;
		}
		else if (modifier == 51) {   // display a fake wall (pattern: wall only to the right)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 1;
		}
		else if (modifier == 52) {   // display a fake wall (pattern: wall only to the left)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 2;
		}
		else if (modifier == 53) {   // display a fake wall (pattern: wall on both sides)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 3;
		}
	}
	else if (tiletype == tiles_1_floor) {
		if (modifier == 6 || modifier == 14) {   // display nothing (invisible floor)
			*ptr_tiletype = tiles_0_empty;
			*ptr_modifier = (modifier == 14) ? 1 : 0; // modifier should be '0' for noblue, instead of '1'
		}
		else if (modifier == 5 || modifier == 13) {   // display a fake wall
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = (modifier == 13) ? 0x80 : 0; // 13: noblue option
		}
		else if (modifier == 50) {   // display a fake wall (pattern: no walls left or right (noblue))
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 0;
		}
		else if (modifier == 51) {   // display a fake wall (pattern: wall only to the right)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 1;
		}
		else if (modifier == 52) {   // display a fake wall (pattern: wall only to the left)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 2;
		}
		else if (modifier == 53) {   // display a fake wall (pattern: wall on both sides)
			*ptr_tiletype = tiles_20_wall;
			*ptr_modifier = 3;
		}
	}
	else if (tiletype == tiles_20_wall) {
		// Walls are a bit strange, because a lot of modifier information is discarded in load_alter_mod() (seg008.c)
		// Also, the "noblue" info for the wall tile is moved to the --most significant-- modifier bit there.

		// load_alter_mod() has been tweaked to retain more information (now stored in the most significant 4 bytes)
		// Modifiers 2-7 are now accessible to define various fake tiles
		// Modifiers 9-15 'loop back' onto 2-7 (identical tiles), EXCEPT they also have the "noblue" bit set
		if (((modifier >> 4) & 7) == 4) {     // display a floor (invisible wall)
			*ptr_tiletype = tiles_1_floor;
			*ptr_modifier = (modifier >> 7); // modifier should be '1' for noblue option
		}
		else if (((modifier >> 4) & 7) == 6) {   // display empty tile (invisible wall)
			*ptr_tiletype = tiles_0_empty;
			*ptr_modifier = (modifier >> 7) ? 1 : 0; // modifier should be '0' for noblue, instead of '1'
		}
	}
#endif
#ifdef FIX_LOOSE_LEFT_OF_POTION
	else if (fixes->fix_loose_left_of_potion && tiletype == tiles_11_loose) {
		if ((*ptr_modifier & 0x7F) == 0) {
			*ptr_tiletype = tiles_1_floor;
		}
	}
#endif
	return *ptr_tiletype;
}

// data:24C6
const word col_xh[] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36};
// seg008:03BB
void load_curr_and_left_tile() {
	word tiletype = tiles_20_wall;
	if (drawn_row == 2) {
		tiletype = custom->drawn_tile_top_level_edge; // floor at top of level (default: tiles_1_floor)
	}
	get_tile_to_draw(drawn_room, drawn_col, drawn_row, &curr_tile, &curr_modifier, tiletype);
	get_tile_to_draw(drawn_room, drawn_col - 1, drawn_row, &tile_left, &modifier_left, tiletype);
	draw_xh = col_xh[drawn_col];
}

// seg008:041A
void load_leftroom() {
	get_room_address(room_L);
	for (word row = 0; row < 3; ++row) {
		// wall at left of level (drawn_tile_left_level_edge), default: tiles_20_wall
		get_tile_to_draw(room_L, 9, row, &leftroom_[row].tiletype, &leftroom_[row].modifier, custom->drawn_tile_left_level_edge);
	}
}

// seg008:0460
void load_rowbelow() {
	word row_below;
	word room;
	word room_left;
	if (drawn_row == 2) {
		room = room_B;
		room_left = room_BL;
		row_below = 0;
	} else {
		room = drawn_room;
		room_left = room_L;
		row_below = drawn_row + 1;
	}
	get_room_address(room);
	for (word column = 1; column <= 10; ++column) {
		get_tile_to_draw(room, column - 1, row_below, &row_below_left_[column].tiletype, &row_below_left_[column].modifier, tiles_0_empty);
	}
	get_room_address(room_left);
	// wall at left of level
	get_tile_to_draw(room_left, 9, row_below, &row_below_left_[0].tiletype, &row_below_left_[0].modifier, tiles_20_wall);
	get_room_address(drawn_room);
}

// seg008:04FA
void draw_tile_floorright() {
	if (can_see_bottomleft() == 0) return;
	draw_tile_topright();
	if (tile_table[tile_left].floor_right == 0) return;
	add_backtable(id_chtab_6_environment, 42 /*floor right part*/, draw_xh, 0, tile_table[tiles_1_floor].right_y + draw_main_y, blitters_9_black, 1);
}

// seg008:053A
int can_see_bottomleft() {
	return curr_tile == tiles_0_empty ||
		curr_tile == tiles_9_bigpillar_top ||
		curr_tile == tiles_12_doortop ||
		curr_tile == tiles_26_lattice_down;
}

const byte doortop_fram_top[] = {0, 81, 83, 0};

// seg008:055A
void draw_tile_topright() {
	byte tiletype = row_below_left_[drawn_col].tiletype;
	if (tiletype == tiles_7_doortop_with_floor || tiletype == tiles_12_doortop) {
		if (custom->tbl_level_type[current_level] == 0) return;
		add_backtable(id_chtab_6_environment, doortop_fram_top[row_below_left_[drawn_col].modifier], draw_xh, 0, draw_bottom_y, blitters_2_or, 0);
	} else if (tiletype == tiles_20_wall) {
		add_backtable(id_chtab_7_environmentwall, 2, draw_xh, 0, draw_bottom_y, blitters_2_or, 0);
	} else {
		int id = tile_table[tiletype].topright_id;
#ifdef USE_TELEPORTS
		// Use teleport graphics:
		// * for the left half of a balcony, if it has a non-zero modifier.
		// * for the right half of a balcony, if it has modifier == 1.
		if (
			(tiletype == tiles_23_balcony_left && row_below_left_[drawn_col].modifier != 0)
			|| (tiletype == tiles_24_balcony_right && row_below_left_[drawn_col].modifier == 1)
		) {
			id += 4;
		}
#endif
		add_backtable(id_chtab_6_environment, id, draw_xh, 0, draw_bottom_y, blitters_2_or, 0);
	}
}

const byte door_fram_top[] = {60, 61, 62, 63, 64, 65, 66, 67};

// seg008:05D1
void draw_tile_anim_topright() {
	if (	(curr_tile == tiles_0_empty ||
		curr_tile == tiles_9_bigpillar_top ||
		curr_tile == tiles_12_doortop)
		&& row_below_left_[drawn_col].tiletype == tiles_4_gate
	) {
		add_backtable(id_chtab_6_environment, 68 /*gate top mask*/, draw_xh, 0, draw_bottom_y, blitters_40h_mono, 0);
		word modifier = row_below_left_[drawn_col].modifier;
		if (modifier > 188) modifier = 188;
		add_backtable(id_chtab_6_environment, door_fram_top[(modifier>>2) % 8], draw_xh, 0, draw_bottom_y, blitters_2_or, 0);
	}
}

const byte blueline_fram1[] = {0, 124, 125, 126};
const sbyte blueline_fram_y[] = {0, -20, -20, 0};
const byte blueline_fram3[] = {44, 44, 45, 45};
const byte doortop_fram_bot[] = {78, 80, 82, 0};

// seg008:066A
void draw_tile_right() {
	byte id;
	byte blit;
	byte num;
	if (curr_tile == tiles_20_wall) return;
	switch (tile_left) {
		default:
			id = tile_table[tile_left].right_id;
#ifdef USE_TELEPORTS
			// Use teleport graphics:
			// * for the left half of a balcony, if it has a non-zero modifier.
			// * for the right half of a balcony, if it has modifier == 1.
			if (tile_left == tiles_23_balcony_left && modifier_left != 0) {
				id += 4;
			} else if (tile_left == tiles_24_balcony_right && modifier_left == 1) {
				id += 4;
			}
#endif
			if (id) {
				if (tile_left == tiles_5_stuck) {
					blit = blitters_10h_transp;
					if (curr_tile == tiles_0_empty || curr_tile == tiles_5_stuck || !tile_is_floor(curr_tile)) {
						id = 42; /*floor B*/
					}
				} else {
					blit = blitters_2_or;
				}
				int right_ybottom = tile_table[tile_left].right_y + draw_main_y;
				if (uses_mac_native_full() && id == 42) {
					/* Native Mac floor overhang 0x2A: Mac draws it from
					 * FLOOR_TOP_ANCHOR(-4)+2.  With its 31px/oy=28 shape,
					 * the DOS bottom anchor lands 4 physical pixels too low. */
					right_ybottom -= 2;
				}
				add_backtable(id_chtab_6_environment, id, draw_xh, 0, right_ybottom, blit, 0);
			}
			if (custom->tbl_level_type[current_level] != 0) {
				add_backtable(id_chtab_6_environment, tile_table[tile_left].stripe_id, draw_xh, 0, draw_main_y - 27, blitters_2_or, 0);
			}
			if (tile_left == tiles_19_torch || tile_left == tiles_30_torch_with_debris) {
				if (uses_mac_native_full()) {
					/* Macintosh COLOR SHPD 1200 image 0x92 has origin (-21,30).
					 * The Mac renderer anchors this image 28 anchor units above
					 * FLOOR_BOTTOM. In our 320x200 logical coordinate system,
					 * xh+1,xl=2 lands within one native pixel of the exact
					 * odd-pixel Mac X origin; y=-31 reproduces the native Y. */
					add_backtable(id_chtab_6_environment, 146 /* Mac torch sconce */, draw_xh + 1, 2, draw_bottom_y - 31, blitters_0_no_transp, 0);
				} else {
					add_backtable(id_chtab_6_environment, 146 /*torch base*/, draw_xh, 0, draw_bottom_y - 28, blitters_0_no_transp, 0);
				}
			}
			break;
		case tiles_0_empty:
			if (modifier_left > 3) return;
			add_backtable(id_chtab_6_environment, blueline_fram1[modifier_left], draw_xh, 0, blueline_fram_y[modifier_left] + draw_main_y, blitters_2_or, 0);
			break;
		case tiles_1_floor:
			ptr_add_table(id_chtab_6_environment, 42 /*floor B*/, draw_xh, 0, tile_table[tile_left].right_y + draw_main_y, blitters_10h_transp, 0);
			num = modifier_left;
			if (num > 3) num = 0;
			if (num == !!custom->tbl_level_type[current_level]) return;
			add_backtable(id_chtab_6_environment, blueline_fram3[num], draw_xh, 0, draw_main_y - 20, blitters_0_no_transp, 0);
			break;
		case tiles_7_doortop_with_floor:
		case tiles_12_doortop:
			if (custom->tbl_level_type[current_level] == 0) return;
			add_backtable(id_chtab_6_environment, doortop_fram_bot[modifier_left], draw_xh, 0, tile_table[tile_left].right_y + draw_main_y, blitters_2_or, 0);
			break;
		case tiles_20_wall:
			if (custom->tbl_level_type[current_level] && (modifier_left & 0x80) == 0) {
				add_backtable(id_chtab_6_environment, 84 /*wall stripe*/, draw_xh + 3, 0, draw_main_y - 27, blitters_0_no_transp, 0);
			}
			add_backtable(id_chtab_7_environmentwall, 1, draw_xh, 0, tile_table[tile_left].right_y + draw_main_y, blitters_2_or, 0);
			break;
	}
}

const byte spikes_fram_right[] = {0, 134, 135, 136, 137, 138, 137, 135, 134, 0};
const byte loose_fram_right[] = {42, 71, 42, 72, 72, 42, 42, 42, 72, 72, 72, 0};

// seg008:08A0
int get_spike_frame(byte modifier) {
	if (modifier & 0x80) {
		return 5;
	} else {
		return modifier;
	}
}

// seg008:08B5
void draw_tile_anim_right() {
	switch (tile_left) {
		case tiles_2_spike:
			add_backtable(id_chtab_6_environment, spikes_fram_right[get_spike_frame(modifier_left)], draw_xh, 0, draw_main_y - 7, blitters_10h_transp, 0);
		break;
		case tiles_4_gate:
			draw_gate_back();
		break;
		case tiles_11_loose:
			add_backtable(id_chtab_6_environment, loose_fram_right[get_loose_frame(modifier_left)], draw_xh, 0, draw_bottom_y - 1, blitters_2_or, 0);
		break;
		case tiles_16_level_door_left:
			draw_leveldoor();
		break;
		case tiles_19_torch:
		case tiles_30_torch_with_debris:
			if (modifier_left < 9) {
				int blit = blitters_0_no_transp;
#ifdef USE_COLORED_TORCHES
				int color;
				if (drawn_col == 0) {
					// Torch is in the rightmost column of the left-side room.
					color = torch_colors[room_L][drawn_row * 10 + 9];
				} else {
					color = torch_colors[drawn_room][drawn_row * 10 + drawn_col - 1];
				}
				if (color != 0) {
					blit = blitters_colored_flame + (color & 0x3F);
				}
#endif
				// images 1..9 are the flames
				if (uses_mac_native_full()) {
					/* Native Mac flames use origin_y=34 and are anchored three
					 * logical pixels higher than the DOS compositor. */
					add_backtable(id_chtab_1_flameswordpotion, modifier_left + 1, draw_xh + 1, 0, draw_main_y - 43, blit, 0);
				} else {
					add_backtable(id_chtab_1_flameswordpotion, modifier_left + 1, draw_xh + 1, 0, draw_main_y - 40, blit, 0);
				}
			}
		break;
	}
}

const byte wall_fram_bottom[] = {7, 9, 5, 3};

// seg008:0971
void draw_tile_bottom(word arg_0) {
	byte id = 0;
	byte blit = blitters_0_no_transp;
	word chtab_id = id_chtab_6_environment;
	switch (curr_tile) {
		case tiles_20_wall:
			if (custom->tbl_level_type[current_level] == 0 || custom->enable_wda_in_palace || graphics_mode != gmMcgaVga || strcasecmp(graphics_pack_name, "dos") != 0) {
				id = wall_fram_bottom[curr_modifier & 0x7F];
			}
			chtab_id = id_chtab_7_environmentwall;
			break;
		case tiles_12_doortop:
			blit = blitters_2_or;
			// fallthrough!
		default:
			id = tile_table[curr_tile].bottom_id;
			break;
	}
	int bottom_y = draw_bottom_y;
	if (uses_mac_native_full() &&
	    chtab_id == id_chtab_6_environment && id != 0) {
		/* Mac floor-front pieces are anchored at FLOOR_BOTTOM_ANCHOR=-1.
		 * The DOS compositor's baseline is 3 logical pixels lower. */
		bottom_y -= 3;
	}
	if (ptr_add_table(chtab_id, id, draw_xh, 0, bottom_y, blit, 0) && arg_0) {
		add_foretable(chtab_id, id, draw_xh, 0, bottom_y, blit, 0);
	}
	if (chtab_id == id_chtab_7_environmentwall && graphics_mode != gmCga && graphics_mode != gmHgaHerc &&
	    !uses_mac_native_full()) {
		/* The Macintosh wall SHPD contains complete wall faces/dividers.
		 * DOS wall_pattern() adds a second synthetic WDA layer and produces
		 * the gray rectangular intrusions seen with the native Mac pack. */
		wall_pattern(0, 0);
	}
}

const byte loose_fram_bottom[] = {43, 73, 43, 74, 74, 43, 43, 43, 74, 74, 74, 0};

// seg008:0A38
void draw_loose(int arg_0) {
	word id;
	if (curr_tile == tiles_11_loose) {
		id = loose_fram_bottom[get_loose_frame(curr_modifier)];
		int loose_bottom_y = draw_bottom_y;
		if (uses_mac_native_full()) loose_bottom_y -= 3;
		add_backtable(id_chtab_6_environment, id, draw_xh, 0, loose_bottom_y, blitters_0_no_transp, 0);
		add_foretable(id_chtab_6_environment, id, draw_xh, 0, loose_bottom_y, blitters_0_no_transp, 0);
	}
}

const byte loose_fram_left[] = {41, 69, 41, 70, 70, 41, 41, 41, 70, 70, 70, 0};

// seg008:0A8E
void draw_tile_base() {
	word id;
	word ybottom = draw_main_y;
	if (uses_mac_native_full()) {
		/* Native Mac floor-top pieces use FLOOR_TOP_ANCHOR=-4.  The DOS
		 * compositor baseline places these 6 physical pixels too low. */
		ybottom -= 3;
		/* v8: button / press-plate tops were lifted a little too aggressively by the
		 * generic floor correction, which widened their dark lip into a two-tile
		 * looking shadow.  Give button-floor tops one logical pixel back. */
		if (curr_tile == tiles_5_stuck || curr_tile == tiles_15_opener) {
			ybottom += 1;
		}
	}
#ifdef USE_SUPER_HIGH_JUMP
	// Latice tiles are drawn in the draw_tile_fore() method.
	if (fixes->enable_super_high_jump) {
		if ((curr_tile >= tiles_26_lattice_down && curr_tile <= tiles_29_lattice_right) ||
			(tile_left == tiles_26_lattice_down && curr_tile == tiles_12_doortop)) {
			return;
		}
	}
#endif
	if (tile_left == tiles_26_lattice_down && curr_tile == tiles_12_doortop) {
		id = 6; // Lattice + door A
		ybottom += 3;
	} else if (curr_tile == tiles_11_loose) {
		id = loose_fram_left[get_loose_frame(curr_modifier)];
	} else if (curr_tile == tiles_15_opener && tile_left == tiles_0_empty && custom->tbl_level_type[current_level] == 0) {
		id = 148; // left half of open button with no floor to the left
	} else {
		id = tile_table[curr_tile].base_id;
	}
	ptr_add_table(id_chtab_6_environment, id, draw_xh, 0, tile_table[curr_tile].base_y + ybottom, blitters_10h_transp, 0);
}

const byte spikes_fram_left[] = {0, 128, 129, 130, 131, 132, 131, 129, 128, 0};
const byte potion_fram_bubb[] = {0, 16, 17, 18, 19, 20, 21, 22};
const byte chomper_fram1[] = {3, 2, 0, 1, 4, 3, 3, 0};
const byte chomper_fram_bot[] = {101, 102, 103, 104, 105, 0};
const byte chomper_fram_top[] = {0, 0, 111, 112, 113, 0};
const byte chomper_fram_y[] = {0, 0, 0x25, 0x2F, 0x32};

// seg008:0B2B
void draw_tile_anim() {
	word chomper_num;
	word pot_size = 0;
	word color = 12; // red
	switch (curr_tile) {
		case tiles_2_spike:
			ptr_add_table(id_chtab_6_environment, spikes_fram_left[get_spike_frame(curr_modifier)], draw_xh, 0, draw_main_y - 2, blitters_10h_transp, 0);
			break;
		case tiles_10_potion:
			switch((curr_modifier & 0xF8) >> 3) {
				case 0: // empty
					return;
				case 5: // hurt
				case 6: // open
					color = 9; // blue
					break;
				case 3: // slow fall
				case 4: // upside down
					color = 10; // green
					// fallthrough!
				case 2: // life
					pot_size = 1;
					break;
			}
			add_backtable(id_chtab_1_flameswordpotion, 23 /*bubble mask*/, draw_xh + 3, 1, draw_main_y - (pot_size << 2) - 14, blitters_40h_mono, 0);
			add_foretable(id_chtab_1_flameswordpotion, potion_fram_bubb[curr_modifier & 0x7], draw_xh + 3, 1, draw_main_y - (pot_size << 2) - 14, color + blitters_40h_mono, 0);
			break;
		case tiles_22_sword:
			add_midtable(id_chtab_1_flameswordpotion, (curr_modifier == 1) + 10, draw_xh, 0, draw_main_y - 3, blitters_10h_transp, curr_modifier == 1);
			break;
		case tiles_18_chomper:
			chomper_num = chomper_fram1[MIN(curr_modifier & 0x7F, 6)];
			add_backtable(id_chtab_6_environment, chomper_fram_bot[chomper_num], draw_xh, 0, draw_main_y, blitters_10h_transp, 0);
			if (curr_modifier & 0x80) { // blood
				add_backtable(id_chtab_6_environment, chomper_num + 114, draw_xh + 1, 4, draw_main_y - 6, blitters_4Ch_mono_12, 0);
			}
			add_backtable(id_chtab_6_environment, chomper_fram_top[chomper_num], draw_xh, 0, draw_main_y - chomper_fram_y[chomper_num], blitters_10h_transp, 0);
			break;
	}
}

const byte spikes_fram_fore[] = {0, 139, 140, 141, 142, 143, 142, 140, 139, 0};
const byte chomper_fram_for[] = {106, 107, 108, 109, 110, 0};
const byte wall_fram_main[] = {8, 10, 6, 4};

// seg008:0D15
void draw_tile_fore() {
	word ybottom;
	byte xh;
	word potion_type;
	word id;
	word chomper_num;
	if (tile_left == tiles_4_gate && Kid.curr_row == drawn_row && Kid.curr_col == drawn_col - 1 && Kid.room != room_R) {
		draw_gate_fore();
	}
	switch (curr_tile) {
		case tiles_2_spike:
			add_foretable(id_chtab_6_environment, spikes_fram_fore[get_spike_frame(curr_modifier)], draw_xh, 0, draw_main_y - 2, blitters_10h_transp, 0);
			break;
		case tiles_18_chomper:
			chomper_num = chomper_fram1[MIN(curr_modifier & 0x7F, 6)];
			add_foretable(id_chtab_6_environment, chomper_fram_for[chomper_num], draw_xh, 0, draw_main_y, blitters_10h_transp, 0);
			if (curr_modifier & 0x80) {
				add_foretable(id_chtab_6_environment, chomper_num + 119, draw_xh + 1, 4, draw_main_y - 6, blitters_4Ch_mono_12, 0);
			}
			break;
		case tiles_20_wall:
			if (custom->tbl_level_type[current_level] == 0 || custom->enable_wda_in_palace || graphics_mode != gmMcgaVga || strcasecmp(graphics_pack_name, "dos") != 0) {
				add_foretable(id_chtab_7_environmentwall, wall_fram_main[curr_modifier & 0x7F], draw_xh, 0, draw_main_y, blitters_0_no_transp, 0);
			}
			if (graphics_mode != gmCga && graphics_mode != gmHgaHerc &&
			    !uses_mac_native_full()) {
				/* Do not overlay DOS procedural/WDA wall decoration on native
				 * Macintosh wall blocks. */
				wall_pattern(1, 1);
			}
			break;
#ifdef USE_SUPER_HIGH_JUMP
		// Draw front parts of lattice in the foretable
		// so prince is not in front of it while jumping.
		case tiles_26_lattice_down:
		case tiles_27_lattice_small:
		case tiles_28_lattice_left:
		case tiles_29_lattice_right:
			if (fixes->enable_super_high_jump) {
				add_foretable(id_chtab_6_environment, tile_table[curr_tile].base_id, draw_xh, 0, tile_table[curr_tile].base_y + draw_main_y, blitters_10h_transp, 0);
			}
			// fallthrough!
		default:
			if (fixes->enable_super_high_jump && tile_left == tiles_26_lattice_down && curr_tile == tiles_12_doortop) {
				add_foretable(id_chtab_6_environment, 6, draw_xh, 0, tile_table[curr_tile].base_y + draw_main_y + 3, blitters_10h_transp, 0);
			}
#else
		default:
#endif
			id = tile_table[curr_tile].fore_id;
			if (id == 0) return;
			if (curr_tile == tiles_10_potion) {
				// large pots are drawn for potion types 2, 3, 4
				potion_type = (curr_modifier & 0xF8) >> 3;
				if (potion_type < 5 && potion_type >= 2) id = 13; // small pot = 12, large pot = 13
			}
			xh = tile_table[curr_tile].fore_x + draw_xh;
			ybottom = tile_table[curr_tile].fore_y + draw_main_y;
			if (curr_tile == tiles_10_potion) {
				// potions look different in the dungeon and the palace
				if (custom->tbl_level_type[current_level] != 0) id += 2;
				add_foretable(id_chtab_1_flameswordpotion, id, xh, 6, ybottom, blitters_10h_transp, 0);
			} else {
				if ((curr_tile == tiles_3_pillar && custom->tbl_level_type[current_level] == 0) || (curr_tile >= tiles_27_lattice_small && curr_tile < tiles_30_torch_with_debris)) {
					add_foretable(id_chtab_6_environment, id, xh, 0, ybottom, blitters_0_no_transp, 0);
				} else {
					add_foretable(id_chtab_6_environment, id, xh, 0, ybottom, blitters_10h_transp, 0);
				}
			}
			break;
	}
}

// seg008:0FF6
int get_loose_frame(byte modifier) {
	// Don't display garbled tiles if the delay is greater than the default.
	if ((modifier & 0x80) || custom->loose_floor_delay > 11) {
		modifier &= 0x7F;
		if (modifier > 10) {
			return 1;
		}
	}
	return modifier;
}

// Get an image, with index and NULL checks.
image_type* get_image(short chtab_id, int id) {
	if (chtab_id < 0 || chtab_id > COUNT(chtab_addrs)) {
		printf("Tried to use chtab %d not in 0..%d\n", chtab_id, (int)COUNT(chtab_addrs));
		return NULL;
	}
	chtab_type* chtab = chtab_addrs[chtab_id];
	if (chtab == NULL) {
		printf("Tried to use null chtab %d\n", chtab_id);
		return NULL;
	}
	if (id < 0 || id >= chtab->n_images) {
		if (id != 255) printf("Tried to use image %d of chtab %d, not in 0..%d\n", id, chtab_id, chtab->n_images-1);
		return NULL;
	}
	return chtab->images[id];
}

// seg008:10A8
int add_backtable(short chtab_id, int id, sbyte xh, sbyte xl, int ybottom, int blit, byte peel) {
	if (id == 0) {
		return 0;
	}
	word index = backtable_count;
	if (index >= 200) {
		show_dialog("BackTable Overflow");
		return 0; // added
	}
	back_table_type* backtable_item = &backtable[index];
	backtable_item->xh = xh;
	backtable_item->xl = xl;
	backtable_item->chtab_id = chtab_id;
	backtable_item->id = id - 1;
	image_type* image = get_image(chtab_id, id - 1);
	if (image == NULL) {
		return 0;
	}
	backtable_item->y = ybottom - logical_image_height(image) + 1;
	backtable_item->blit = blit;
	backtable_item->use_physical = 0;
	backtable_item->physical_x = 0;
	backtable_item->physical_y = 0;
	backtable_item->physical_clip_bottom = -1;
	backtable_item->physical_clip_top = -1;
	if (draw_mode) {
		draw_back_fore(0, index);
	}
	++backtable_count;
	return 1;
}

// seg008:1017
/* v7: Add a native image at an exact 640x400 top-left position.
 * Used by cutscene props whose attachment point is baked into a native full-
 * screen background (for example the Princess-room wall sconces). */
int add_backtable_physical(short chtab_id, int id, int physical_x, int physical_y, int blit) {
	if (id == 0) return 0;
	word index = backtable_count;
	if (index >= 200) {
		show_dialog("BackTable Overflow");
		return 0;
	}
	image_type* image = get_image(chtab_id, id - 1);
	if (image == NULL) return 0;
	back_table_type* item = &backtable[index];
	item->xh = (sbyte)((physical_x / 2) / 8);
	item->xl = (sbyte)((physical_x / 2) % 8);
	item->y = (short)(physical_y / 2);
	item->chtab_id = (byte)chtab_id;
	item->id = (byte)(id - 1);
	item->blit = blit;
	item->physical_x = (short)physical_x;
	item->physical_y = (short)physical_y;
	item->physical_clip_bottom = -1;
	item->physical_clip_top = -1;
	item->use_physical = 1;
	if (draw_mode) draw_back_fore(0, index);
	++backtable_count;
	return 1;
}

int add_foretable(short chtab_id, int id, sbyte xh, sbyte xl, int ybottom, int blit, byte peel) {
	if (id == 0) return 0;
	word index = foretable_count;
	if (index >= FORETABLE_MAX) {
		show_dialog("ForeTable Overflow");
		return 0; // added
	}
	back_table_type* foretable_item = &foretable[index];
	foretable_item->xh = xh;
	foretable_item->xl = xl;
	foretable_item->chtab_id = chtab_id;
	foretable_item->id = id - 1;
	image_type* image = get_image(chtab_id, id - 1);
	if (image == NULL) {
		return 0;
	}
	foretable_item->y = ybottom - logical_image_height(image) + 1;
	foretable_item->blit = blit;
	foretable_item->use_physical = 0;
	foretable_item->physical_x = 0;
	foretable_item->physical_y = 0;
	foretable_item->physical_clip_bottom = -1;
	foretable_item->physical_clip_top = -1;
	if (draw_mode) {
		draw_back_fore(1, index);
	}
	++foretable_count;
	return 1;
}

// seg008:113A
int add_midtable(short chtab_id, int id, sbyte xh, sbyte xl, int ybottom, int blit, byte peel) {
	if (id == 0) {
		return 0;
	}
	word index = midtable_count;
	if (index >= 50) {
		show_dialog("MidTable Overflow");
		return 0; // added
	}
	midtable_type* midtable_item = &midtable[index];
	midtable_item->xh = xh;
	midtable_item->xl = xl;
	midtable_item->chtab_id = chtab_id;
	midtable_item->id = id - 1;
	image_type* image = get_image(chtab_id, id - 1);
	if (image == NULL) {
		return 0;
	}
	midtable_item->y = ybottom - logical_image_height(image) + 1;
	if (obj_direction == dir_0_right && chtab_flip_clip[chtab_id] != 0) {
		blit += 0x80;
	}
	midtable_item->blit = blit;
	midtable_item->use_physical = 0;
	midtable_item->physical_x = 0;
	midtable_item->physical_y = 0;
	midtable_item->peel = peel;
	midtable_item->clip.left = obj_clip_left;
	midtable_item->clip.right = obj_clip_right;
	midtable_item->clip.top = obj_clip_top;
	midtable_item->clip.bottom = obj_clip_bottom;
	if (draw_mode) {
		draw_mid(index);
	}
	++midtable_count;
	return 1;
}

// seg008:1208
void add_peel(int left,int right,int top,int height) {
	rect_type rect;
	if (peels_count >= 50) {
		show_dialog("Peels OverFlow");
		return /*0*/; // added
	}
	rect.left = left;
	rect.right = right;
	rect.top = top;
	rect.bottom = top + height;
	peels_table[peels_count] = read_peel_from_screen(&rect);
	peels_count++;
}

// seg008:1254
void add_wipetable(sbyte layer,short left,short bottom,sbyte height,short width,sbyte color) {
	word index = wipetable_count;
	if (index >= 300) {
		show_dialog("WipeTable Overflow");
		return /*0*/; // added
	}
	wipetable_type* wipetable_item = &wipetable[index];
	wipetable_item->left = left;
	wipetable_item->bottom = bottom + 1;
	wipetable_item->height = height;
	wipetable_item->width = width;
	wipetable_item->color = color;
	wipetable_item->layer = layer;
	if (draw_mode) {
		draw_wipe(index);
	}
	++wipetable_count;
}

// seg008:12BB
void draw_table(int which_table) {
	short count = table_counts[which_table];
	for (short index = 0; index < count; ++index) {
		if (which_table == 3) {
			draw_mid(index);
		} else {
			draw_back_fore(which_table, index);
		}
	}
}

// seg008:12FE
void draw_wipes(int which) {
	word count = wipetable_count;
	for (word index = 0; index < count; ++index) {
		if (which == wipetable[index].layer) {
			draw_wipe(index);
		}
	}
}

// seg008:133B
void draw_back_fore(int which_table,int index) {
	image_type* image;
	image_type* mask;
	back_table_type* table_entry;
	if (which_table == 0) {
		table_entry = &backtable[index];
	} else /* if (which_table == 1) this is always true */ {
		table_entry = &foretable[index];
	}
	if (table_entry->use_physical == native_entry_fill) {
		draw_native_rect_physical(table_entry->physical_x, table_entry->physical_y,
		                          table_entry->physical_w, table_entry->physical_h, table_entry->fill_rgb);
		return;
	}
	image = mask = get_image(table_entry->chtab_id, table_entry->id);
	/*
	if ((graphics_mode == gmCga || graphics_mode == gmHgaHerc) &&
		chtab_shift[table_entry->chtab_id] == 0) {
		chtab_type* chtab = chtab_addrs[table_entry->chtab_id];
		mask = chtab->images[chtab->n_images / 2 + table_entry->id];
	}
	*/
	if (table_entry->use_physical && is_native_resolution_image(image)) {
		/* v11.19: The dungeon wall's below-left corner (res362) extends one
		 * native row past the floor front. A full room draw covers that row
		 * with the wall FORE below, but an upper-tile redraw does not repaint
		 * the lower wall. Keep the corner above that boundary so spike/button/
		 * debris updates cannot overpaint its top edge. Preserve SHPD placement
		 * and the caller's clip; palace corners already end at this boundary. */
		SDL_Rect saved_clip;
		bool clip_native = table_entry->physical_clip_bottom >= 0 || table_entry->physical_clip_top >= 0;
		if (clip_native) {
			SDL_GetClipRect(current_target_surface, &saved_clip);
			SDL_Rect clip = saved_clip;
			int bottom = table_entry->physical_clip_bottom;
			if (bottom >= 0 && clip.y + clip.h > bottom) clip.h = MAX(0, bottom - clip.y);
			if (table_entry->physical_clip_top >= 0 && clip.y < table_entry->physical_clip_top) {
				int top = table_entry->physical_clip_top;
				clip.h = MAX(0, clip.y + clip.h - top);
				clip.y = top;
			}
			SDL_SetClipRect(current_target_surface, &clip);
		}
		draw_native_image_physical_blit(image, table_entry->physical_x, table_entry->physical_y, table_entry->blit);
		if (clip_native) SDL_SetClipRect(current_target_surface, &saved_clip);
	} else {
		draw_image(image, mask, table_entry->xh * 8 + table_entry->xl, table_entry->y, table_entry->blit);
	}
}


SDL_Surface* hflip(SDL_Surface* input) {
	int width = input->w;
	int height = input->h;

	// The simplest way to create a surface with same format as input:
	SDL_Surface* output = SDL_ConvertSurface(input, input->format, 0);
	SDL_SetSurfacePalette(output, input->format->palette);
	output->userdata = input->userdata; // preserve native-resolution tag
	// The copied image will be overwritten anyway.
	if (output == NULL) {
		sdlperror("hflip: SDL_ConvertSurface");
		quit(1);
	}

	SDL_SetSurfaceBlendMode(input, SDL_BLENDMODE_NONE);
	// Temporarily turn off alpha and colorkey on input. So we overwrite the output image.
	SDL_SetColorKey(input, SDL_FALSE, 0);
	SDL_SetColorKey(output, SDL_FALSE, 0);
	SDL_SetSurfaceAlphaMod(input, 255);

	for (int source_x = 0, target_x = width-1; source_x < width; ++source_x, --target_x) {
		SDL_Rect srcrect = {source_x, 0, 1, height};
		SDL_Rect dstrect = {target_x, 0, 1, height};
		if (SDL_BlitSurface(input/*32*/, &srcrect, output, &dstrect) != 0) {
			sdlperror("hflip: SDL_BlitSurface");
			quit(1);
		}
	}

	return output;
}

/* v11.16: Native climb masks 222..224 end one physical column before the
 * DOS-positioned body's outline. Restore that column from the actual floor
 * top, in the same MID pass as the mask. Copying the registered floor pixels
 * preserves its texture; moving the actor or stretching the mask would not.
 * v11.17 adds mask 221's tail below the elbow and mask 227's shoe-outline dot.
 * Keep the repair limited to these cutouts and retain the current viewport. */
static void mac_draw_climb_mask_edge(const midtable_type* item, image_type* mask) {
	if (!uses_mac_native_full() || item->chtab_id != id_chtab_6_environment) return;
	int first_row = 0;
	switch (item->id) {
		case 0x14: // res221, frame 138: retain the four rows touching the elbow.
			first_row = 4;
			break;
		case 0x15: case 0x16: case 0x17: // res222..224, frames 139..141.
		case 0x1A: // res227, frame 144: cover the last shoe-outline pixel.
			break;
		default:
			return;
	}
	image_type* base = get_image(id_chtab_6_environment, 0x29 - 1);
	short mx, my, bx, by;
	if (base == NULL || !is_native_resolution_image(base) ||
	    !get_native_image_origin(mask, &mx, &my) ||
	    !get_native_image_origin(base, &bx, &by)) return;

	SDL_Rect saved_clip, edge_clip;
	SDL_Rect edge = {item->physical_x + mask->w, item->physical_y + first_row,
	                 1, mask->h - first_row};
	SDL_GetClipRect(current_target_surface, &saved_clip);
	if (SDL_IntersectRect(&saved_clip, &edge, &edge_clip)) {
		SDL_SetClipRect(current_target_surface, &edge_clip);
		draw_native_image_physical_blit(base, item->physical_x + mx - bx,
		                               item->physical_y + my - by, blitters_10h_transp);
		SDL_SetClipRect(current_target_surface, &saved_clip);
	}
}


// seg008:140C
void draw_mid(int index) {
	image_type*mask;
//	word image_flipped;

	word blit_flip = 0;
	word need_free_image = 0;
	word need_free_mask = 0;
	midtable_type* midtable_entry = &midtable[index];
	if (midtable_entry->use_physical == native_entry_fill) {
		draw_native_rect_physical(midtable_entry->physical_x, midtable_entry->physical_y,
		                          midtable_entry->physical_w, midtable_entry->physical_h, midtable_entry->fill_rgb);
		return;
	}
	word image_id = midtable_entry->id;
	word chtab_id = midtable_entry->chtab_id;
	image_type* image = mask = get_image(chtab_id, image_id);
	/*
	if ((graphics_mode == gmCga || graphics_mode == gmHgaHerc) && chtab_shift[chtab_id]) {
		mask = chtab_addrs[chtab_id]->images[image_id + chtab_addrs[chtab_id]->n_images / 2];
	}
	*/
	short xpos = midtable_entry->xh * 8 + midtable_entry->xl;
	short ypos = midtable_entry->y;
	word blit = midtable_entry->blit;
	if (blit & 0x80) {
		blit_flip = 0x8000;
		blit &= 0x7F;
	}

	/* v7 actor anchor: y in the midtable is a top-left coordinate derived from
	 * the old DOS bottom-alignment rule. Reconstruct that original logical
	 * bottom/anchor before replacing the image placement with SHPD origins. */
	const int native_actor = mac_native_actor_chtab((short)chtab_id) &&
	                         is_native_resolution_image(image);
	const int actor_anchor_y = ypos + logical_image_height(image) - 1;
	short native_ox = 0, native_oy = 0;
	const bool actor_has_origin = native_actor &&
	                              get_native_image_origin(image, &native_ox, &native_oy);

	if (midtable_entry->use_physical && is_native_resolution_image(image) && !blit_flip) {
		if (midtable_entry->peel) {
			const int physical_x = midtable_entry->physical_x;
			const int physical_y = midtable_entry->physical_y;
			const int left = physical_x >= 0 ? physical_x / 2 : -((-physical_x + 1) / 2);
			const int top = physical_y >= 0 ? physical_y / 2 : -((-physical_y + 1) / 2);
			const int right = (physical_x + image->w + 1) / 2;
			const int bottom = (physical_y + image->h + 1) / 2;
			add_peel(round_xpos_to_byte(left, 0), round_xpos_to_byte(right, 1), top, bottom - top);
		}
		draw_native_image_physical_blit(image, midtable_entry->physical_x, midtable_entry->physical_y, blit);
		mac_draw_climb_mask_edge(midtable_entry, image);
		return;
	}

	if (chtab_flip_clip[chtab_id]) {
		/* Gameplay uses 192 logical rows above the HUD. Cutscenes own all 200. */
		rect_type effective_clip = midtable_entry->clip;
		if (uses_mac_native_full()) {
			if (effective_clip.top < rect_top.top) effective_clip.top = rect_top.top;
			if (effective_clip.left < rect_top.left) effective_clip.left = rect_top.left;
			int bottom = is_cutscene ? screen_rect.bottom : rect_top.bottom;
			if (effective_clip.bottom > bottom) effective_clip.bottom = bottom;
			if (effective_clip.right > rect_top.right) effective_clip.right = rect_top.right;
		}
		set_clip_rect(&effective_clip);
		if (chtab_id != id_chtab_0_sword) {
			xpos = calc_screen_x_coord(xpos);
		}
	}

	/* At this point xpos is the same logical horizontal anchor used by the DOS
	 * actor path. Preserve it before the legacy flip code subtracts image width. */
	const int actor_anchor_x = xpos;
	if (blit_flip) {
		xpos -= logical_image_width(image);
		need_free_image = 1;
		image = hflip(image);
	}

	if (native_actor && actor_has_origin) {
		/* The SHPD origin is measured in physical Macintosh pixels. When an actor
		 * is mirrored, mirror the origin around the image as well; otherwise the
		 * anchor would jump by the sprite width when the Prince turns around. */
		int ox = native_ox;
		if (blit_flip) ox = image->w - 1 - native_ox;
		const int physical_x = actor_anchor_x * 2 - ox;
		int physical_y = actor_anchor_y * 2 - native_oy;
		/* v11.15: The supplied KID553 Mac reference places this pose about
		 * six native pixels above the current DOS-derived body anchor.
		 * Align the post-row-change climb phase (141..144, KID552..555,
		 * image indices 151..154) with the native floor-top baseline using
		 * the same three-logical-pixel bridge as the masks. Without it the
		 * trousers enter the top plane and the foot can emerge below the
		 * front lip. Keep this render-
		 * only correction out of Char/Kid, clipping and collision coordinates.
		 * Earlier hanging poses and the standing/crouching poses keep their
		 * existing anchors. Applying it by body resource also covers mirroring. */
		if (chtab_id == id_chtab_2_kid && image_id >= 151 && image_id <= 154) {
			physical_y += (1 + MAC_FLOOR_TOP_ANCHOR) * 2;
		}

		if (midtable_entry->peel) {
			const int left = physical_x >= 0 ? physical_x / 2 : -((-physical_x + 1) / 2);
			const int top = physical_y >= 0 ? physical_y / 2 : -((-physical_y + 1) / 2);
			const int right = (physical_x + image->w + 1) / 2;
			const int bottom = (physical_y + image->h + 1) / 2;
			add_peel(round_xpos_to_byte(left, 0), round_xpos_to_byte(right, 1), top, bottom - top);
		}
		draw_native_image_physical_blit(image, physical_x, physical_y, blit);
	} else {
		if (midtable_entry->peel) {
			add_peel(round_xpos_to_byte(xpos, 0), round_xpos_to_byte(logical_image_width(image) + xpos, 1), ypos, logical_image_height(image));
		}
		draw_image(image, mask, xpos, ypos, blit);
	}

	if (chtab_flip_clip[chtab_id]) {
		reset_clip_rect();
		/* draw_tables() maintains a gameplay-area clip in native-full mode; an
		 * actor-specific clip temporarily replaces it, so restore it here. */
		if (uses_mac_native_full()) set_clip_rect(is_cutscene ? &screen_rect : &rect_top);
	}
	if (need_free_image) {
		SDL_FreeSurface(image);
	}
	if (need_free_mask) {
		free(mask);
	}
}

// seg008:167B
void draw_image(image_type* image,image_type* mask,int xpos,int ypos,int blit) {
	rect_type rect;
	switch (blit) {
		case blitters_10h_transp:
			draw_image_transp(image, mask, xpos, ypos);
		break;
		case blitters_9_black:
			method_6_blit_img_to_scr(mask, xpos, ypos, blitters_9_black);
		break;
		case blitters_0_no_transp:
		case blitters_2_or:
		case blitters_3_xor:
			method_6_blit_img_to_scr(image, xpos, ypos, blit);
		break;
		default:
			if (blit >= 0x100) {
				method_6_blit_img_to_scr(mask, xpos, ypos, blit);
			} else {
				method_3_blit_mono(image, xpos, ypos, 0, blit & 0xBF);
			}
		break;
	}
	if (need_drects) {
		rect.left = rect.right = xpos;
		rect.right += logical_image_width(image);
		rect.top = rect.bottom = ypos;
		rect.bottom += logical_image_height(image);
		add_drect(&rect);
	}
}

// seg008:1730
void draw_wipe(int index) {
	rect_type rect;
	wipetable_type* ptr = &wipetable[index];
	rect.left = rect.right = ptr->left;
	rect.right += ptr->width;
	rect.bottom = rect.top = ptr->bottom;
	rect.top -= ptr->height;
	draw_rect(&rect, ptr->color);
	if (need_drects) {
		add_drect(&rect);
	}
}

// data:4E8C
word gate_top_y;
// data:4CB6
word gate_openness;
// data:436C
word gate_bottom_y;

// seg008:178E
void calc_gate_pos() {
	gate_top_y = draw_bottom_y - 62;
	gate_openness = (MIN(modifier_left, 188) >> 2) + 1;
	gate_bottom_y = draw_main_y - gate_openness;
}

// data:2785
const byte door_fram_slice[] = {67, 59, 58, 57, 56, 55, 54, 53, 52};
// seg008:17B7
void draw_gate_back() {
	calc_gate_pos();
	if (gate_bottom_y + 12 < draw_main_y) {
		add_backtable(id_chtab_6_environment, 50 /*gate bottom with B*/, draw_xh, 0, gate_bottom_y, blitters_0_no_transp, 0);
	} else {
		// The following line (erroneously) erases the top-right of the tile below-left (because it is drawn non-transparently).
		// -- But it draws something that was already drawn! (in draw_tile_right()).
		add_backtable(id_chtab_6_environment, tile_table[tiles_4_gate].right_id, draw_xh, 0,
		              tile_table[tiles_4_gate].right_y + draw_main_y, blitters_0_no_transp, 0);
		// And this line tries to fix it. But it fails if it was a gate or a pillar.
		if (can_see_bottomleft()) draw_tile_topright();
#ifdef FIX_GATE_DRAWING_BUG
		if (fixes->fix_gate_drawing_bug) {
			draw_tile_anim_topright(); // redraw the erased top-right section of the gate below-left
		}
#endif
		// The following 3 lines draw things that are drawn after this anyway.
		draw_tile_bottom(0);
		draw_loose(0);
		draw_tile_base();
		add_backtable(id_chtab_6_environment, 51 /*gate bottom*/, draw_xh, 0, gate_bottom_y - 2, blitters_10h_transp, 0);
	}
	short ybottom = gate_bottom_y - 12;
	if (ybottom < 192) {
		for (; ybottom >= 0 && ybottom > 7 && ybottom - 7 > gate_top_y; ybottom -= 8) {
			add_backtable(id_chtab_6_environment, 52 /*gate slice 8px*/, draw_xh, 0, ybottom, blitters_0_no_transp, 0);
		}
	}
	word gate_frame = ybottom - gate_top_y + 1;
	if (gate_frame > 0 && gate_frame < 9) {
		add_backtable(id_chtab_6_environment, door_fram_slice[gate_frame], draw_xh, 0, ybottom, blitters_0_no_transp, 0);
	}
}

// seg008:18BE
void draw_gate_fore() {
	calc_gate_pos();
	add_foretable(id_chtab_6_environment, 51 /*gate bottom*/, draw_xh, 0, gate_bottom_y - 2, blitters_10h_transp, 0);
	short ybottom = gate_bottom_y - 12;
	if (ybottom < 192) {
		for (; ybottom >= 0 && ybottom > 7 && ybottom - 7 > gate_top_y; ybottom -= 8) {
			add_foretable(id_chtab_6_environment, 52 /*gate slice 8px*/, draw_xh, 0, ybottom, blitters_10h_transp, 0);
		}
	}
}

// seg008:1937
void alter_mods_allrm() {

#ifdef USE_COLORED_TORCHES
	memset(torch_colors, 0, sizeof(torch_colors));
#endif

	// level.used_rooms is 25 on some levels. Limit it to the actual number of rooms.
	if (level.used_rooms > 24) level.used_rooms = 24;

	for (word room = 1; room <= level.used_rooms; room++) {
		get_room_address(room);
		room_L = level.roomlinks[room-1].left;
		room_R = level.roomlinks[room-1].right;
		for (word tilepos = 0; tilepos < 30; tilepos++) {
			load_alter_mod(tilepos);
		}
	}
}

// seg008:198E
void load_alter_mod(int tilepos) {
	word wall_to_right;
	word wall_to_left;
	byte* curr_tile_modif = tilepos + curr_room_modif;
	word tiletype = curr_room_tiles[tilepos] & 0x1F;
	switch (tiletype) {
		case tiles_4_gate:
			if (*curr_tile_modif == 1) {
				*curr_tile_modif = 188;
			} else {
				*curr_tile_modif = 0;
			}
			break;
		case tiles_11_loose:
			*curr_tile_modif = 0;
			break;
		case tiles_10_potion:
			*curr_tile_modif <<= 3;
#ifdef USE_COPYPROT
			if (current_level == 15) {
				// Copy protection
				if (copyprot_room[copyprot_plac] == loaded_room &&
					copyprot_tile[copyprot_plac] == tilepos
				) {
					*curr_tile_modif = 6 << 3; // place open potion
				}
			}
#endif
			break;


		case tiles_20_wall:
		{
			byte stored_modif = *curr_tile_modif;
			//*curr_tile_modif <<= 7; // original: "no blue" mod becomes most significant bit
			if (stored_modif == 1) *curr_tile_modif = 0x80;
			else *curr_tile_modif = (stored_modif << 4);
			// retain three bits more information:
			// most significant bit:       1 ==> "no blue"
			// next 3 bits:                for displaying various fake tiles (invisible walls)
			// ..
			// least significant 2 bits:   wall to left/right?
		}
			// fallthrough: not done yet, just moving to another scope
			goto label_wall_continued;

#ifdef USE_FAKE_TILES
		// Need to deal with the possibility that certain tiles can impersonate walls
		case tiles_0_empty:
		case tiles_1_floor:
			if ((*curr_tile_modif & 7) != 5) break;
			// if tile is a fake wall, fall through
#endif

			label_wall_continued:
		{
			// Here, ahead of time, we define the condition for a neighbouring tile to be seen as a wall connection
			#ifndef USE_FAKE_TILES
			#define read_adj_tile_modif_in_curr_room()      // nop; access to modifier of adjacent tiles is not needed
			#define read_adj_tile_modif_in_external_room()  // nop
			#define WALL_CONNECTION_CONDITION (adj_tile == tiles_20_wall)

			#else // #ifdef USE_FAKE_TILES
			// When determining wall connections for fake walls, we need access to the tile modifier of adjacent tiles
			#define read_adj_tile_modif_in_curr_room() \
			int adj_tile_modif = curr_room_modif[adj_tile_index];
			#define read_adj_tile_modif_in_external_room() \
			int adj_tile_modif = level.bg[adj_tile_index];
			// Now redefine the condition for what tiletype / modifier combination counts as a valid "wall"
			#define WALL_CONNECTION_CONDITION (                                                                       \
				(adj_tile == tiles_20_wall && adj_tile_modif != 4 && (adj_tile_modif >> 4) != 4 &&                    \
				             adj_tile_modif != 6 && (adj_tile_modif >> 4) != 6) ||                                    \
				(adj_tile == tiles_0_empty && (adj_tile_modif == 5 || adj_tile_modif == 13 ||                         \
				             (adj_tile_modif >= 50 && adj_tile_modif <= 53))) ||                                      \
				(adj_tile == tiles_1_floor && (adj_tile_modif == 5 || adj_tile_modif == 13 ||                         \
				             (adj_tile_modif >= 50 && adj_tile_modif <= 53))))
			#endif

			if (graphics_mode != gmCga && graphics_mode != gmHgaHerc) {
				wall_to_right = 1;
				wall_to_left = 1;
				int adj_tile_index, adj_tile;
				if (tilepos % 10 == 0) {
					if (room_L) {
						adj_tile_index = 30*(room_L-1)+tilepos+9;
						adj_tile = (level.fg[adj_tile_index] & 0x1F);
						read_adj_tile_modif_in_external_room(); // only executed when fake tiles are enabled
						wall_to_left = WALL_CONNECTION_CONDITION;
					}
				} else {
					adj_tile_index = tilepos-1;
					adj_tile = (curr_room_tiles[adj_tile_index] & 0x1F);
					read_adj_tile_modif_in_curr_room();
					wall_to_left = WALL_CONNECTION_CONDITION;
				}
				if (tilepos % 10 == 9) {
					if (room_R) {
						adj_tile_index = 30*(room_R-1)+tilepos-9;
						adj_tile = (level.fg[adj_tile_index] & 0x1F);
						read_adj_tile_modif_in_external_room();
						wall_to_right = WALL_CONNECTION_CONDITION;
					}
				} else {
					adj_tile_index = tilepos+1;
					adj_tile = (curr_room_tiles[adj_tile_index] & 0x1F);
					read_adj_tile_modif_in_curr_room();
					wall_to_right = WALL_CONNECTION_CONDITION;
				}

				#ifdef USE_FAKE_TILES
				if (tiletype == tiles_1_floor || tiletype == tiles_0_empty) {
					if (wall_to_left && wall_to_right) {
						*curr_tile_modif = 53;
					} else if (wall_to_left) {
						*curr_tile_modif = 52;
					} else if (wall_to_right) {
						*curr_tile_modif = 51;
					}
					break;
				}
				#endif

				if (wall_to_left && wall_to_right) {
					*curr_tile_modif |= 3;
				} else if (wall_to_left) {
					*curr_tile_modif |= 2;
				} else if (wall_to_right) {
					*curr_tile_modif |= 1;
				}
			} else {
				*curr_tile_modif = 3;
			}
		}
			break;

#ifdef USE_COLORED_TORCHES
		case tiles_19_torch:
		case tiles_30_torch_with_debris:
			torch_colors[loaded_room][tilepos] = *curr_tile_modif;
			*curr_tile_modif = 0;
		break;
#endif
	}
}

// seg008:1AF8
void draw_moving() {
	draw_mobs();
	draw_people();
	redraw_needed_tiles();
}

// seg008:1B06
static void mac_expand_background_redraws(void) {
	if (!uses_mac_native_full()) return;
	/* The wall-side stripe starts 24 logical pixels into its cell and is
	 * 32 pixels wide. Its last 24 pixels share the following cell, whose
	 * decoration is drawn later. Refresh both in the original BACK order:
	 * either a side redraw can overwrite that decoration, or a wipe in the
	 * following cell can erase the stripe. FORE/actor-only requests stay local. */
	for (int col = 0; col < 9; ++col) {
		byte left, modifier, tile, unused;
		get_tile_to_draw(drawn_room, col - 1, drawn_row, &left, &modifier, tiles_20_wall);
		get_tile_to_draw(drawn_room, col, drawn_row, &tile, &unused, tiles_20_wall);
		int pos = tbl_line[drawn_row] + col;
		if (mac_is_palace() && left == tiles_20_wall && !(modifier & 0x80) && tile != tiles_20_wall &&
		    (wipe_frames[pos] || redraw_frames_full[pos] || redraw_frames_anim[pos] ||
		     wipe_frames[pos + 1] || redraw_frames_full[pos + 1] || redraw_frames_anim[pos + 1])) {
			redraw_frames_full[pos] = MAX(redraw_frames_full[pos], 1);
			redraw_frames_full[pos + 1] = MAX(redraw_frames_full[pos + 1], 1);
		}
		/* The native floor sword and skeleton also reach one physical column
		 * into the next cell. Restore its later-drawn floor/overhang afterward. */
		if ((tile == tiles_22_sword || tile == tiles_21_skeleton) &&
		    (wipe_frames[pos] || redraw_frames_full[pos] || redraw_frames_anim[pos]))
			redraw_frames_full[pos + 1] = MAX(redraw_frames_full[pos + 1], 1);
		/* Palace exit-left res237 has SHPD origin X=1. Its next-cell right
		 * piece reaches one physical column backwards into this cell. A torch
		 * refresh here can repaint that border; restore the later-drawn piece.
		 * Resolve native metadata rather than shifting or clipping the art. */
		if (tile == tiles_16_level_door_left &&
		    (wipe_frames[pos] || redraw_frames_full[pos] || redraw_frames_anim[pos])) {
			image_type* edge = get_image(id_chtab_6_environment, tile_table[tile].right_id - 1);
			short ox, oy;
			if (edge && get_native_image_origin(edge, &ox, &oy) && ox > 0)
				redraw_frames_full[pos + 1] = MAX(redraw_frames_full[pos + 1], 1);
		}
	}
}

void redraw_needed_tiles() {
	load_leftroom();
	draw_objtable_items_at_tile(30);
	for (drawn_row = 3; drawn_row--; ) {
		load_rowbelow();
		mac_expand_background_redraws();
		draw_bottom_y = 63 * drawn_row + 65;
		draw_main_y = draw_bottom_y - 3;
		for (drawn_col = 0; drawn_col < 10; ++drawn_col) {
			load_curr_and_left_tile();
			redraw_needed(tbl_line[drawn_row] + drawn_col);
		}
	}
	word saved_drawn_room = drawn_room;
	drawn_room = room_A;
	load_room_links();
	load_leftroom();
	drawn_row = 2;
	load_rowbelow();
	for (drawn_col = 0; drawn_col < 10; ++drawn_col) {
		load_curr_and_left_tile();
		draw_main_y = -1;
		draw_bottom_y = 2;
		redraw_needed_above(drawn_col);
	}
	drawn_room = saved_drawn_room;
	load_room_links();
	draw_objtable_items_at_tile(-1);
}

// seg008:1BCB
void draw_tile_wipe(byte height) {
	short wipe_bottom = draw_bottom_y;
	if (uses_mac_native_full() && height > 3) {
		/* v11.8: The DOS wipe reaches draw_bottom_y, but the Macintosh floor
		 * front (e.g. 0x2B/0x24/0x60) ends at draw_main_y (physical row 2*M+1).
		 * The extra 3 logical rows (6 native rows) lie on the top of the wall /
		 * tile in the row below, which is NOT redrawn when only this tile (and
		 * its right neighbour) is refreshed. Wiping them left a permanent black
		 * strip under every pressure plate that changed state. Keep the wipe's
		 * top edge unchanged and stop it at the bottom of the Mac front. */
		byte trim = (byte)(draw_bottom_y - draw_main_y);
		wipe_bottom = draw_main_y;
		height = (height > trim) ? (byte)(height - trim) : 0;
	}
	add_wipetable(0, draw_xh*8, wipe_bottom, height, 4*8, 0);
}

// seg008:1BEB
void draw_tables() {
	drects_count = 0;
	current_target_surface = offscreen_surface;
	/* Native origins can extend beyond a tile. Gameplay stops above the HUD;
	 * cutscenes use the full 320x200 logical / 640x400 physical screen. */
	if (uses_mac_native_full()) set_clip_rect(is_cutscene ? &screen_rect : &rect_top);
	if (is_blind_mode) {
		draw_rect(&rect_top, color_0_black);
	}
	restore_peels();
	draw_wipes(0);
	draw_table(0); // backtable
	//printf("midtable_count = %d\n", midtable_count); // debug
#ifdef FIX_BLACK_RECT // I'm not sure if this is the best way to fix this.
	draw_wipes(1);
#endif
	draw_table(3); // midtable
	draw_wipes(1);
	draw_table(1); // foretable
	if (uses_mac_native_full()) reset_clip_rect();
	current_target_surface = onscreen_surface_;
	show_copyprot(1);
}

// seg008:1C4E
void restore_peels() {
	peel_type* peel;
	while (peels_count) {
		peels_count--;
		peel = peels_table[peels_count];
		if (need_drects) {
			add_drect(&peel->rect); // ?
		}
		restore_peel(peel);
	}
	peels_count = 0;
}

// seg008:1C8F
void add_drect(rect_type *source) {
	for (short index = 0; index < drects_count; ++index) {
		rect_type target_rect; // Dummy output argument, we care only about whether the intersection is non-empty.
		if (intersect_rect(&target_rect, shrink2_rect(&target_rect, source, -1, -1), &drects[index])) {
			rect_type* current_drect = &drects[index];
			union_rect(current_drect, current_drect, source);
			return;
		}
	}
	if (drects_count >= 30){
		show_dialog("DRects Overflow");
		return /*0*/; // added
	}
	drects[drects_count] = *source;
	drects_count++;
}

// seg008:1D29
void draw_leveldoor() {
	short ybottom = draw_main_y - 13;
	leveldoor_right = (draw_xh<<3)+48;
	if (custom->tbl_level_type[current_level]) leveldoor_right += 8;
	add_backtable(id_chtab_6_environment, 99 /*leveldoor stairs bottom*/, draw_xh + 1, 0, ybottom, blitters_0_no_transp, 0);
	if (modifier_left) {
		if (level.start_room != drawn_room) {
			add_backtable(id_chtab_6_environment, 144 /*level door stairs*/, draw_xh + 1, 0, ybottom - 4, blitters_0_no_transp, 0);
		}
		else {
			short leveldoor_width = (custom->tbl_level_type[current_level] == 0) ? 39 : 48;
			sbyte x_low = (custom->tbl_level_type[current_level] == 0) ? 2 : 0; // dungeon level doors are shifted 2px to the right
			add_wipetable(0, 8*(draw_xh + 1) + x_low, ybottom - 4, 45, leveldoor_width, 0);
		}
	}
	leveldoor_ybottom = ybottom - (modifier_left & 3) - 48;
	short y = ybottom - modifier_left;
	do { // runs at least once
		add_backtable(id_chtab_6_environment, 33 /*level door bottom*/, draw_xh + 1, 0, leveldoor_ybottom, blitters_0_no_transp, 0);
		if (y > leveldoor_ybottom) leveldoor_ybottom += 4;
		else break;
	} while (true);
	add_backtable(id_chtab_6_environment, 34 /*level door top*/, draw_xh + 1, 0, draw_main_y - 64, blitters_0_no_transp, 0);
}

// seg008:1E0C
void get_room_address(int room) {
	//if (room < 0 || room > 24) printf("Tried to access room %d, not in 0..24.\n", room);
	loaded_room = (word) room;
	if (room) {
		curr_room_tiles = &level.fg[(room-1)*30];
		curr_room_modif = &level.bg[(room-1)*30];
	}
}

// data:286A
const word floor_left_overlay[] = {32, 151, 151, 150, 150, 151, 32, 32};

/* v11.9: The legacy climb-up path does more than redraw the environment.
 * Frames 137..144 put a small floor-left wedge and the floor front into the
 * MID table so those pixels occlude the Prince while he pulls himself up.
 *
 * mac_native_full previously collapsed redraw_frames_floor_overlay into a
 * normal mac_draw_tile_exact() call. That repainted the room, but it lost the
 * midtable occluder completely, allowing the actor to show through floor /
 * pillar edges during the climb. Keep gameplay and actor placement untouched;
 * restore only the missing layer semantics here.
 */
static void mac_draw_floor_overlay_exact(void) {
#ifdef FIX_BIGPILLAR_CLIMB
	if (tile_left != tiles_0_empty &&
	    (!fixes->fix_bigpillar_climb || tile_left != tiles_9_bigpillar_top)) {
		return;
	}
#else
	if (tile_left != tiles_0_empty) return;
#endif

	if (curr_tile != tiles_1_floor &&
	    curr_tile != tiles_3_pillar &&
	    curr_tile != tiles_5_stuck &&
	    curr_tile != tiles_19_torch) {
		return;
	}

	if (Kid.frame < frame_137_climbing_3 || Kid.frame > frame_144_climbing_10) {
		return;
	}

	/* v11.14: Macintosh has eight native, frame-specific climb occluders in
	 * VDUNGEON/VPALACE resources 220..227 (chtab IDs 0x14..0x1B), one for
	 * each frame 137..144.  The legacy floor_left_overlay[] table points to
	 * DOS resources 232/350/351; using those in native-full produced the
	 * coarse grey wedge seen across the Prince's trousers during climb-down.
	 * Use the real Mac SHPD pieces and their origins instead. */
	static const byte mac_floor_left_overlay[8] = {
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B
	};
	const int overlay_id = mac_floor_left_overlay[Kid.frame - frame_137_climbing_3];
	/* v11.15: These masks are literal cutouts of the native floor TOP, so
	 * their anchor must be the same as its base component. The DOS overlay
	 * baseline (draw_main_y) placed them 3 logical / 6 native pixels too low,
	 * leaving the Prince visible inside the sloping surface. A stuck plate
	 * lowers both the base and its mask by one logical pixel. */
	const int overlay_anchor_y = draw_main_y + 1 + MAC_FLOOR_TOP_ANCHOR +
	                             (curr_tile == tiles_5_stuck);
	mac_debug_floor_piece("climb-overlay", curr_tile, curr_modifier, overlay_id,
	                      draw_xh * 8, overlay_anchor_y,
	                      get_image(id_chtab_6_environment, overlay_id - 1));
	mac_add_mid_anchor_exact(id_chtab_6_environment, overlay_id,
	                         draw_xh * 8,
	                         overlay_anchor_y,
	                         blitters_10h_transp, 0);

	/* Duplicate only the floor/front component into the MID table, just as
	 * draw_floor_overlay() does in the legacy compositor. Use the v11 state
	 * table so a depressed plate gets 0x24 while ordinary floor/pillar/torch
	 * gets the normal 0x2B front, all at the SHPD-aware Mac anchor. */
	int front_id = 0x2B;
	int front_anchor_y = MAC_FLOOR_BOTTOM_ANCHOR;
	mac_floor_state_type state;
	if (mac_get_floor_state(curr_tile, curr_modifier, &state) && state.front_id) {
		front_id = state.front_id;
		front_anchor_y = state.front_anchor_y;
	}

	mac_add_mid_anchor_exact(id_chtab_6_environment, front_id,
	                         draw_xh * 8,
	                         draw_main_y + 1 + front_anchor_y,
	                         blitters_10h_transp, 0);
}

// seg008:1E3A
void draw_floor_overlay() {
#ifdef FIX_BIGPILLAR_CLIMB
	if (tile_left != tiles_0_empty) {
		// Bug: When climbing up to a floor with a big pillar top behind, turned right, Kid sees through floor.
		// The bigpillar_top tile should be treated similarly to an empty tile here.
		if (!fixes->fix_bigpillar_climb || (tile_left != tiles_9_bigpillar_top)) {
			return;
		}
	}
#else
	if (tile_left != tiles_0_empty) return;
#endif
	if (curr_tile == tiles_1_floor ||
		curr_tile == tiles_3_pillar ||
		curr_tile == tiles_5_stuck ||
		curr_tile == tiles_19_torch
	) {
		// frames 137..144: climb
		// index overflow here?
		if (Kid.frame >= frame_137_climbing_3 && Kid.frame <= frame_144_climbing_10) {
			add_midtable(id_chtab_6_environment, floor_left_overlay[Kid.frame - 137], draw_xh, 0, (curr_tile == tiles_5_stuck) + draw_main_y, blitters_10h_transp, 0);
		} else {
			// triggered by 02-random-broken
			printf("draw_floor_overlay: attempted to draw floor overlay with frame %d not in 137..144\n", Kid.frame);
			//quit(1);
		}
		ptr_add_table = &add_midtable;
		draw_tile_bottom(0);
		ptr_add_table = &add_backtable;
	} else {
		draw_other_overlay();
	}
}

// seg008:1EB5
void draw_other_overlay() {
	byte tiletype;
	byte modifier;
	if (tile_left == tiles_0_empty) {
		ptr_add_table = &add_midtable;
		draw_tile2();
	} else if (curr_tile != tiles_0_empty && drawn_col > 0 &&
		get_tile_to_draw(drawn_room, drawn_col - 2, drawn_row, &tiletype, &modifier, tiles_0_empty) == tiles_0_empty
	) {
		ptr_add_table = &add_midtable;
		draw_tile2();
		ptr_add_table = &add_backtable;
		draw_tile2();
		tile_object_redraw[tbl_line[drawn_row] + drawn_col] = 0xFF;
	}
	ptr_add_table = &add_backtable;
}

// seg008:1F48
void draw_tile2() {
	draw_tile_right();
	draw_tile_anim_right();
	draw_tile_base();
	draw_tile_anim();
	draw_tile_bottom(0);
	draw_loose(0);
}

// seg008:1F67
void draw_objtable_items_at_tile(byte tilepos) {
	//printf("draw_objtable_items_at_tile(%d)\n",tilepos); // debug
	short obj_count = objtable_count;
	if (obj_count) {
		n_curr_objs = 0;
		for (short obj_index = obj_count - 1; obj_index >= 0; --obj_index) {
			if (objtable[obj_index].tilepos == tilepos) {
				curr_objs[n_curr_objs] = obj_index;
				n_curr_objs++;
			}
		}
		if (n_curr_objs) {
			sort_curr_objs();
			for (short obj_index = 0; obj_index < n_curr_objs; ++obj_index) {
				draw_objtable_item(curr_objs[obj_index]);
			}
		}
	}
}

// seg008:1FDE
void sort_curr_objs() {
	short swapped;
	// bubble sort
	short last = n_curr_objs - 1;
	do {
		swapped = 0;
		for (short index = 0; index < last; ++index) {
			if (compare_curr_objs(index, index + 1)) {
				short temp = curr_objs[index];
				curr_objs[index] = curr_objs[index + 1];
				curr_objs[index + 1] = temp;
				swapped = 1;
			}
		}
		// --last ?
	} while (swapped);
}

// seg008:203C
int compare_curr_objs(int index1,int index2) {
	short obj_index1 = curr_objs[index1];
	if (objtable[obj_index1].obj_type == 1) return 1;
	short obj_index2 = curr_objs[index2];
	if (objtable[obj_index2].obj_type == 1) return 0;
	if (objtable[obj_index1].obj_type == 0x80 &&
		objtable[obj_index2].obj_type == 0x80
	) {
		return (objtable[obj_index1].y < objtable[obj_index2].y);
	} else {
		return (objtable[obj_index1].y > objtable[obj_index2].y);
	}
	return 1;
}

// seg008:20CA
void draw_objtable_item(int index) {
	switch (load_obj_from_objtable(index)) {
		case 0: // Kid
		case 4: // mirror image
			//printf("index = %d, obj_id = %d\n", index, obj_id); // debug
			if (obj_id == 0xFF) return;
			// the Kid blinks a bit after uniting with shadow
			if (united_with_shadow && (united_with_shadow % 2) == 0) goto shadow;
			// fallthrough!
		case 2: // Guard
		case 3: // sword
		case 5: // hurt splash
			add_midtable(obj_chtab, obj_id + 1, obj_xh, obj_xl, obj_y, blitters_10h_transp, 1);
		break;
		case 1: // shadow
		shadow:
			if (united_with_shadow == 2) {
				play_sound(sound_41_end_level_music); // united with shadow
			}
			add_midtable(obj_chtab, obj_id + 1, obj_xh, obj_xl, obj_y, blitters_2_or, 1);
			add_midtable(obj_chtab, obj_id + 1, obj_xh, obj_xl + 1, obj_y, blitters_3_xor, 1);
		break;
		case 0x80: // loose floor
			obj_direction = dir_FF_left;
			if (uses_mac_native_full()) {
				mac_floor_state_type state;
				if (mac_get_floor_state(tiles_11_loose, obj_id, &state)) {
					const int left_x = obj_xh * 8 + obj_xl;
					const int right_x = (obj_x + 4) * 8 + obj_xl;
					/* curmob.y is the legacy floor-bottom baseline (65/128/191 at
					 * rest). The static Mac compositor's logical anchor origin is two
					 * pixels above that baseline: draw_main_y + 1 == draw_bottom_y - 2.
					 * Reusing that origin makes the last shaking frame and first falling
					 * frame land on the same native pixels instead of jumping 3px down. */
					const int anchor_origin_y = obj_y - 2;
					const int base_y = anchor_origin_y + state.base_anchor_y;
					const int front_y = anchor_origin_y + state.front_anchor_y;
					const int right_y = anchor_origin_y + state.right_anchor_y;
					mac_debug_floor_piece("fall-base", tiles_11_loose, obj_id, state.base_id, left_x, base_y,
						get_image(obj_chtab, state.base_id - 1));
					mac_add_mid_anchor_exact(obj_chtab, state.base_id, left_x, base_y, blitters_10h_transp, 1);
					mac_debug_floor_piece("fall-front", tiles_11_loose, obj_id, state.front_id, left_x, front_y,
						get_image(obj_chtab, state.front_id - 1));
					mac_add_mid_anchor_exact(obj_chtab, state.front_id, left_x, front_y, 0, 1);
					mac_debug_floor_piece("fall-right", tiles_11_loose, obj_id, state.right_id, right_x, right_y,
						get_image(obj_chtab, state.right_id - 1));
					mac_add_mid_anchor_exact(obj_chtab, state.right_id, right_x, right_y, blitters_10h_transp, 1);
				}
			} else {
				add_midtable(obj_chtab, loose_fram_left[obj_id], obj_xh, obj_xl, obj_y - 3, blitters_10h_transp, 1);
				add_midtable(obj_chtab, loose_fram_bottom[obj_id], obj_xh, obj_xl, obj_y, 0, 1);
				add_midtable(obj_chtab, loose_fram_right[obj_id], obj_x + 4, obj_xl, obj_y - 1, blitters_10h_transp, 1);
			}
		break;
	}
}

// seg008:2228
int load_obj_from_objtable(int index) {
	objtable_type* curr_obj = &objtable[index];
	obj_xh = obj_x = curr_obj->xh;
	obj_xl = curr_obj->xl;
	obj_y = curr_obj->y;
	obj_id = curr_obj->id;
	obj_chtab = curr_obj->chtab_id;
	obj_direction = curr_obj->direction;
	obj_clip_top = curr_obj->clip.top;
	obj_clip_bottom = curr_obj->clip.bottom;
	obj_clip_left = curr_obj->clip.left;
	obj_clip_right = curr_obj->clip.right;
	return curr_obj->obj_type;
}

// seg008:228A
void draw_people() {
	check_mirror();
	draw_kid();
	draw_guard();
	reset_obj_clip();
	draw_hp();
}

// seg008:22A2
void draw_kid() {
	if (Kid.room != 0 && Kid.room == drawn_room) {
		add_kid_to_objtable();
		if (hitp_delta < 0) {
			draw_hurt_splash();
		}
		add_sword_to_objtable();
	}
}

// seg008:22C9
void draw_guard() {
	if (Guard.direction != dir_56_none && Guard.room == drawn_room) {
		add_guard_to_objtable();
		if (guardhp_delta < 0) {
			draw_hurt_splash();
		}
		add_sword_to_objtable();
	}
}

// seg008:22F0
void add_kid_to_objtable() {
	//printf("add_kid_to_objtable\n");
	loadkid();
	load_fram_det_col();
	load_frame_to_obj();
	stuck_lower();
	set_char_collision();
	set_objtile_at_char();
	redraw_at_char();
	redraw_at_char2();
	clip_char();
	add_objtable(0); // Kid
}

// seg008:2324
void add_guard_to_objtable() {
	word obj_type;
	loadshad();
	load_fram_det_col();
	load_frame_to_obj();
	stuck_lower();
	set_char_collision();
	set_objtile_at_char();
	redraw_at_char();
	redraw_at_char2();
	clip_char();
	if (Char.charid == charid_1_shadow) {
		// Special event: shadow is clipped: may appear only right from the mirror
		if (current_level == /*4*/ custom->mirror_level && Char.room == /*4*/ custom->mirror_room) {
			obj_clip_left = 137;
			obj_clip_left += (custom->mirror_column - 4) * 32; // added
		}
		obj_type = 1; // shadow
	} else {
		obj_type = 2; // Guard
	}
	add_objtable(obj_type);
}

// seg008:2388
void add_objtable(byte obj_type) {
	//printf("in add_objtable: objtable_count = %d\n",objtable_count); // debug
	word index = objtable_count++;
	//printf("in add_objtable: objtable_count = %d\n",objtable_count); // debug
	if (index >= 50) {
		show_dialog("ObjTable Overflow");
		return /*0*/; // added
	}
	objtable_type* entry_addr = &objtable[index];
	entry_addr->obj_type = obj_type;
	x_to_xh_and_xl(obj_x, &entry_addr->xh, &entry_addr->xl);
	entry_addr->y = obj_y;
	entry_addr->clip.top = obj_clip_top;
	entry_addr->clip.bottom = obj_clip_bottom;
	entry_addr->clip.left = obj_clip_left;
	entry_addr->clip.right = obj_clip_right;
	entry_addr->chtab_id = obj_chtab;
	entry_addr->id = obj_id;
	entry_addr->direction = obj_direction;
	mark_obj_tile_redraw(index);
}

// seg008:2423
void mark_obj_tile_redraw(int index) {
	//printf("mark_obj_tile_redraw: obj_tile = %d\n", obj_tile); // debug
	objtable[index].tilepos = obj_tilepos;
	if (obj_tilepos < 30) {
		tile_object_redraw[obj_tilepos] = 1;
	}
}

// seg008:2448
void load_frame_to_obj() {
	word chtab_base = id_chtab_2_kid;
	reset_obj_clip();
	load_frame();
	obj_direction = Char.direction;
	obj_id = cur_frame.image;
	// top 6 bits of sword are the chtab
	obj_chtab = chtab_base + (cur_frame.sword >> 6);
	obj_x = (char_dx_forward(cur_frame.dx) << 1) - 116;
	obj_y = cur_frame.dy + Char.y;
	if ((sbyte)(cur_frame.flags ^ obj_direction) >= 0) {
		// 0x80: even/odd pixel
		++obj_x;
	}
}

// seg008:24A8
void show_time() {
	char sprintf_temp[40];
	word rem_sec;

#ifdef FIX_ONE_HP_STOPS_BLINKING
	global_blink_state = !global_blink_state;
#endif

	if (Kid.alive < 0 &&
		#ifdef FREEZE_TIME_DURING_END_MUSIC
		(!(fixes->enable_freeze_time_during_end_music && next_level != current_level)) &&
		#endif
		#ifdef ALLOW_INFINITE_TIME
		// prevent overflow
		(!(rem_min == INT16_MIN && rem_tick == 1)) &&
		#endif
		rem_min != 0 &&
		(current_level < /*13*/ custom->victory_stops_time_level || (current_level == /*13*/ custom->victory_stops_time_level && leveldoor_open == 0)) &&
		current_level < 15
	) {
		// Time passes
		--rem_tick;
		if (rem_tick == 0) {
			rem_tick = 719; // 720=12*60 ticks = 1 minute
			--rem_min;
#ifndef ALLOW_INFINITE_TIME
			if (rem_min != 0 && (rem_min <= 5 || rem_min % 5 == 0)) {
				is_show_time = 1;
			}
#else
			if (rem_min > 0 && (rem_min <= 5 || rem_min % 5 == 0)) {
				is_show_time = 1;
			} else if (rem_min < 0) {
				is_show_time = ((~rem_min) % 5 == 0 ) ? 1 : 0;
			}
#endif
		} else {
			if (rem_min == 1 && rem_tick % 12 == 0) {
				is_show_time = 1;
				text_time_remaining = 0;
			}
		}
	}
	if (is_show_time && text_time_remaining == 0) {
		text_time_remaining = text_time_total = 24;
		if (rem_min > 0) {
			if (rem_min == 1) {
				rem_sec = (rem_tick + 1) / 12;
				if (rem_sec == 1) {
					strncpy(sprintf_temp, "1 SECOND LEFT", sizeof(sprintf_temp));
					text_time_remaining = text_time_total = 12;
				} else {
					snprintf(sprintf_temp, sizeof(sprintf_temp), "%d SECONDS LEFT", rem_sec);
				}
			} else {
				snprintf(sprintf_temp, sizeof(sprintf_temp), "%d MINUTES LEFT", rem_min);
			}
			display_text_bottom(sprintf_temp);
		} else {

#ifdef ALLOW_INFINITE_TIME
			if (rem_min < 0) {
				if (~rem_min == 0) {
					// don't display time elapsed in the first minute
					text_time_remaining = 0;
					text_time_total = 0;
					sprintf_temp[0] = '\0';
				}
				else if (~rem_min == 1) {
					snprintf(sprintf_temp, sizeof(sprintf_temp), "1 MINUTE PASSED");
				} else {
					snprintf(sprintf_temp, sizeof(sprintf_temp), "%d MINUTES PASSED", ~rem_min);
				}
				display_text_bottom(sprintf_temp);
			}

			else if (rem_min == 0) // may also be negative, don't report "expired" in that case!
#endif

			display_text_bottom("TIME HAS EXPIRED!");
		}
		is_show_time = 0;
	}
}

// seg008:25A8
void show_level() {
#ifdef FIX_LEVEL_14_RESTARTING
	text_time_remaining = text_time_total = 0;
#endif
	char sprintf_temp[32];
	byte disp_level = current_level;
	if (disp_level != 0 && disp_level < /*14*/ custom->hide_level_number_from_level && seamless == 0) {
		if (disp_level == 13) {
			disp_level = /*12*/ custom->level_13_level_number;
		}
		text_time_remaining = text_time_total = 24;
		snprintf(sprintf_temp, sizeof(sprintf_temp), "LEVEL %d", disp_level);
		display_text_bottom(sprintf_temp);
		is_show_time = 1;
	}
	seamless = 0;
}

// seg008:2602
short calc_screen_x_coord(short logical_x) {
	return logical_x*320/280;
}

// seg008:2627
void free_peels() {
	while (peels_count > 0) {
		--peels_count;
		free_peel(peels_table[peels_count]);
	}
}


// seg008:2644
void display_text_bottom(const char* text) {
	draw_rect(&rect_bottom_text, color_0_black);
	show_text(&rect_bottom_text, halign_center, valign_bottom, text);
#ifndef USE_TEXT
	SDL_SetWindowTitle(window_, text);
#endif
}

// seg008:266D
void erase_bottom_text(int arg_0) {
	draw_rect(&rect_bottom_text, color_0_black);
	if (arg_0) {
		text_time_total = 0;
		text_time_remaining = 0;
	}
#ifndef USE_TEXT
	SDL_SetWindowTitle(window_, WINDOW_TITLE);
#endif
}

// Dungeon wall drawing algorithm by HTamas

#define RSET_WALL              7

#define RES_WALL_FACE_MAIN     1    //(face stack main.bmp)
#define RES_WALL_FACE_TOP      2    //(face stack top.bmp)
#define RES_WALL_CENTRE_BASE   3    //(centre stack base.bmp)
#define RES_WALL_CENTRE_MAIN   4    //(centre stack main.bmp)
#define RES_WALL_RIGHT_BASE    5    //(right stack base.bmp)
#define RES_WALL_RIGHT_MAIN    6    //(right stack main.bmp)
#define RES_WALL_SINGLE_BASE   7    //(single stack base.bmp)
#define RES_WALL_SINGLE_MAIN   8    //(single stack main.bmp)
#define RES_WALL_LEFT_BASE     9    //(left stack base.bmp)
#define RES_WALL_LEFT_MAIN    10    //(left stack main.bmp)
#define RES_WALL_DIVIDER1     11    //(divider01.bmp, the broad divider)
#define RES_WALL_DIVIDER2     12    //(divider02.bmp, the narrow divider)
#define RES_WALL_RNDBLOCK     13    //(random block.bmp)
#define RES_WALL_MARK_TL      14    //(mark01.bmp, top left mark)
#define RES_WALL_MARK_BL      15    //(mark02.bmp, bottom left mark)
#define RES_WALL_MARK_TR      16    //(mark03.bmp, top right mark)
#define RES_WALL_MARK_BR      17    //(mark04.bmp, bottom right mark)

#define BLIT_NO_TRANS          0
#define BLIT_OR                2
#define BLIT_XOR               3
#define BLIT_BLACK             9
#define BLIT_TRANS            16

#define GRAPHICS_CGA          1
#define GRAPHICS_HERCULES     2
#define GRAPHICS_EGA          3
#define GRAPHICS_TANDY        4
#define GRAPHICS_VGA          5

#define DESIGN_DUNGEON        0
#define DESIGN_PALACE         1

#define WALL_MODIFIER_SWS     0
#define WALL_MODIFIER_SWW     1
#define WALL_MODIFIER_WWS     2
#define WALL_MODIFIER_WWW     3


// seg008:268F
void wall_pattern(int which_part,int which_table) {
	word bottom_divider; word middle_divider; byte bottom_divider_offset; byte middle_divider_offset;
	// save the value for the sprite insertion method, so that it can be restored
	add_table_type saved_sim = ptr_add_table;
	// set the sprite insertion method based on the arguments
	if (which_table == 0) {
		ptr_add_table = &add_backtable;
	} else {
		ptr_add_table = &add_foretable;
	}
	// save the state of the pseudorandom number generator
	dword saved_prng_state = random_seed;
	// set the new seed
	random_seed = drawn_room + tbl_line[drawn_row] + drawn_col;
	prandom(1); // fetch a random number and discard it
	word is_dungeon = (custom->tbl_level_type[current_level] < DESIGN_PALACE) || custom->enable_wda_in_palace;
	if ( (!is_dungeon) && (graphics_mode== GRAPHICS_VGA) &&
		 strcasecmp(graphics_pack_name, "dos") == 0 ) {
		// The DOS palace wall set uses small divider decals here while the wall
		// faces themselves are generated procedurally below. Alternate graphics
		// packs (SNES/Mac) store complete wall-face stacks in RSET_WALL, so they
		// must use the generic WDA path in the else branch instead.
		// I haven't traced the palace WDA
		//[...]
		if (which_part) {
			// Draw solid colours for the bricks
			add_wipetable(which_table, 8*(draw_xh)    , draw_main_y - 40, 20, 4*8, palace_wall_colors[44 * drawn_row +      drawn_col]);
			add_wipetable(which_table, 8*(draw_xh)    , draw_main_y - 19, 21, 2*8, palace_wall_colors[44 * drawn_row + 11 + drawn_col]);
			add_wipetable(which_table, 8*(draw_xh + 2), draw_main_y - 19, 21, 2*8, palace_wall_colors[44 * drawn_row + 12 + drawn_col]);
			add_wipetable(which_table, 8*(draw_xh)    , draw_main_y     , 19, 1*8, palace_wall_colors[44 * drawn_row + 22 + drawn_col]);
			add_wipetable(which_table, 8*(draw_xh + 1), draw_main_y     , 19, 3*8, palace_wall_colors[44 * drawn_row + 23 + drawn_col]);
			
			// Draw the dividing decals between bricks
			ptr_add_table(id_chtab_7_environmentwall, prandom(2) +  3, draw_xh + 3, 0, draw_main_y - 53, blitters_46h_mono_6, 0);
			ptr_add_table(id_chtab_7_environmentwall, prandom(2) +  6, draw_xh    , 0, draw_main_y - 34, blitters_46h_mono_6, 0);
			ptr_add_table(id_chtab_7_environmentwall, prandom(2) +  9, draw_xh    , 0, draw_main_y - 13, blitters_46h_mono_6, 0);
			ptr_add_table(id_chtab_7_environmentwall, prandom(2) + 12, draw_xh    , 0, draw_main_y     , blitters_46h_mono_6, 0);
		}
		add_wipetable(which_table, 8*draw_xh    , draw_bottom_y   ,  3, 4*8, palace_wall_colors[44 * drawn_row + 33 + drawn_col]);
		ptr_add_table(id_chtab_7_environmentwall, prandom(2) + 15, draw_xh    , 0, draw_bottom_y   , blitters_46h_mono_6, 0);
	} else {
		middle_divider = prandom(1); // 0 = thick middle brick divider, 1 = thin middle brick divider
		middle_divider_offset = prandom(4); // Horizontal render offset for middle brick divider
		
		// Same as above but for bottom brick dividers
		bottom_divider = prandom(1);
		bottom_divider_offset = prandom(4);

		// store the background modifier for the current tile in a local variable
		// apparently, for walls, the modifier stores whether there are adjacent walls
		byte bg_modifier = curr_modifier & 0x7F;
		switch (bg_modifier) {
			case WALL_MODIFIER_WWW: // left and right tiles are walls
				if (which_part != 0) {
					if (prandom(4) == 0) {
						ptr_add_table(RSET_WALL, RES_WALL_RNDBLOCK, draw_xh, 0, draw_bottom_y - 42, BLIT_NO_TRANS, 0);
					}
					ptr_add_table(RSET_WALL, RES_WALL_DIVIDER1 + middle_divider, draw_xh + 1, middle_divider_offset, draw_bottom_y - 21, BLIT_TRANS, 0);
				}
				ptr_add_table(RSET_WALL, RES_WALL_DIVIDER1 + bottom_divider, draw_xh, bottom_divider_offset, draw_bottom_y, BLIT_TRANS, 0);
				if (which_part != 0) {
					if (is_dungeon) {
						if (prandom(4) == 0) {
							draw_right_mark(prandom(3), middle_divider_offset);
						}
						if (prandom(4) == 0) {
							draw_left_mark(prandom(4), middle_divider_offset - middle_divider, bottom_divider_offset - bottom_divider);
						}
					}
				}
				break;
			case WALL_MODIFIER_SWS: // left and right tiles are not walls
				if (is_dungeon) {
					if (which_part != 0) {
						if (prandom(6) == 0) {
							draw_left_mark(prandom(1), middle_divider_offset - middle_divider, bottom_divider_offset - bottom_divider);
						}
					}
				}
				break;
			case WALL_MODIFIER_SWW: // right tile is a wall, but not left
				if (which_part != 0) {
					if (prandom(4) == 0) {
						ptr_add_table(RSET_WALL, RES_WALL_RNDBLOCK, draw_xh, 0, draw_bottom_y - 42, BLIT_NO_TRANS, 0);
					}
					ptr_add_table(RSET_WALL, RES_WALL_DIVIDER1 + middle_divider, draw_xh + 1 /*fix*/ , middle_divider_offset, draw_bottom_y - 21, BLIT_TRANS, 0);
					if (is_dungeon) {
						if (prandom(4) == 0) {
							draw_right_mark(prandom(3), middle_divider_offset);
						}
						if (prandom(4) == 0) {
							draw_left_mark(prandom(3), middle_divider_offset - middle_divider, bottom_divider_offset - bottom_divider);
						}
					}
				}
				break;
			case WALL_MODIFIER_WWS: // left tile is a wall, but not right
				if (which_part != 0) {
					ptr_add_table(RSET_WALL, RES_WALL_DIVIDER1 + middle_divider, draw_xh + 1, middle_divider_offset, draw_bottom_y - 21, BLIT_TRANS, 0);
				}
				ptr_add_table(RSET_WALL, RES_WALL_DIVIDER1 + bottom_divider, draw_xh, bottom_divider_offset, draw_bottom_y, BLIT_TRANS, 0);
				if (which_part != 0) {
					if (is_dungeon) {
						if (prandom(4) == 0) {
							draw_right_mark(prandom(1) + 2, middle_divider_offset);
						}
						if (prandom(4) == 0) {
							draw_left_mark(prandom(4), middle_divider_offset - middle_divider, bottom_divider_offset - bottom_divider);
						}
					}
				}
				break;
		}
	}
	random_seed = saved_prng_state;
	ptr_add_table = saved_sim;
}

void draw_left_mark (word decal_variant, word arg2, word arg1) {
	static const word LPOS[] = {58, 41, 37, 20, 16}; // Vertical render offset for all brick decal variants (last entry seems to be unused)
	word image_id = RES_WALL_MARK_TL;
	word lv2 = 0;
	if (decal_variant % 2) { // Variants alternate between bottomleft and topleft decals
		image_id = RES_WALL_MARK_BL;
	}
	if (decal_variant > 3) {
		lv2 = arg1 + 6;
	} else if (decal_variant > 1) {
		lv2 = arg2 + 6;
	}
	ptr_add_table(RSET_WALL, image_id, draw_xh + (decal_variant == 2 || decal_variant == 3), lv2, draw_bottom_y - LPOS[decal_variant], BLIT_TRANS, 0);
}

void draw_right_mark (word decal_variant, word arg1) {
	static const word RPOS[] = {52, 42, 31, 21}; // Vertical render offset for all brick decal variants (last entry seems to be unused)
	word image_id = RES_WALL_MARK_TR;
	if (decal_variant % 2) { // Variants alternate between bottomright and topright decals
		image_id = RES_WALL_MARK_BR;
	}
	if (decal_variant < 2) {
		arg1 = 24;
	} else {
		arg1 -= 3;
	}
	ptr_add_table(RSET_WALL, image_id, draw_xh + (decal_variant > 1), arg1, draw_bottom_y - RPOS[decal_variant], BLIT_TRANS, 0);
}

/**************************************************************************/
/*  test_tile_map_layer.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/os/memory.h"
#include "core/variant/typed_array.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/resources/2d/tile_set.h"
#include "tests/test_macros.h"

namespace TestTileMapLayer {

TEST_CASE("[SceneTree][TileMapLayer] Constructor") {
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);

	CHECK(tile_map_layer->is_enabled());
	CHECK(tile_map_layer->get_tile_set().is_null());
	CHECK(tile_map_layer->is_occlusion_enabled());
	CHECK(tile_map_layer->get_y_sort_origin() == 0);
	CHECK_FALSE(tile_map_layer->is_x_draw_order_reversed());
	CHECK(tile_map_layer->get_rendering_quadrant_size() == 16);
	CHECK(tile_map_layer->is_collision_enabled());
	CHECK_FALSE(tile_map_layer->is_using_kinematic_bodies());
	CHECK(tile_map_layer->get_collision_visibility_mode() == TileMapLayer::DEBUG_VISIBILITY_MODE_DEFAULT);
	CHECK(tile_map_layer->get_physics_quadrant_size() == 16);
	CHECK(tile_map_layer->is_navigation_enabled());
	CHECK(tile_map_layer->get_navigation_map().is_null());
	CHECK(tile_map_layer->get_navigation_visibility_mode() == TileMapLayer::DEBUG_VISIBILITY_MODE_DEFAULT);

	memdelete(tile_map_layer);
}

TEST_CASE("[SceneTree][TileMapLayer] TileSet") {
	Ref<TileSet> tile_set = memnew(TileSet);
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);

	tile_map_layer->set_tile_set(tile_set);
	CHECK_FALSE(tile_map_layer->get_tile_set().is_null());

	memdelete(tile_map_layer);
}

/**
 * Creates a generic TileSet with a generated single color texture_size
 * and fills it with a TileSetAtlasSource.
 *
 * @param tile_shape A valid TileShape (square, isometric, hexagonal,
 *                   half-offset square, ... as defined by the TileSet enum)
 * @param texture_size The size of the generated texture, in pixels. The
 *                     texture will be a white square of that side.
 * @param tile_size The size of the tile, in pixels.
 *
 * @return A reference to the generated TileSet.
 *
*/
inline Ref<TileSet> create_generic_tile_set(TileSet::TileShape tile_shape,
											const unsigned int texture_size = 256,
											const unsigned int tile_size = 16) {

	Ref<Image> image = memnew(Image(texture_size,
									texture_size,
									false,
									Image::FORMAT_RGBA8));
	image->fill(Color::named("white"));
	Ref<Texture2D> image_texture = ImageTexture::create_from_image(image);
	Ref<TileSetAtlasSource> tile_set_atlas_source = memnew(TileSetAtlasSource);
	tile_set_atlas_source->set_texture(image_texture);  // source_id 0
	unsigned int tile_length = texture_size / tile_size;
	for (unsigned int i = 0; i < tile_length; i++) {
		for (unsigned int j = 0; j < tile_length; j++) {
			tile_set_atlas_source->create_tile(Vector2i(i, j));
		}
	}
	Ref<TileSet> tile_set = memnew(TileSet);
	tile_set->set_tile_shape(tile_shape);
	tile_set->set_tile_size(Size2i(tile_size, tile_size));
	tile_set->add_source(tile_set_atlas_source);
	return tile_set;
}

TEST_CASE("[SceneTree][TileMapLayer] Cells manipulation") {
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);
	const TileMapCell default_cell = TileMapCell(TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, TileSetAtlasSource::INVALID_TILE_ALTERNATIVE);
	const Vector2i coord1 = Vector2i(10, -7);
	const Vector2i coord2 = Vector2i(-1, -1);
	const Vector2i coord3 = Vector2i(0, 0);
	Ref<TileSet> tile_set = create_generic_tile_set(
		TileSet::TILE_SHAPE_SQUARE, 256, 16
	);
	tile_map_layer->set_tile_set(tile_set);

	SUBCASE("Basic Set & get") {
		CHECK_EQ(tile_map_layer->get_cell(coord1), default_cell);
		tile_map_layer->set_cell(coord1, 0, Vector2i(10, 2), 0);
		CHECK_EQ(tile_map_layer->get_cell(coord1), TileMapCell(0, Vector2i(10, 2), 0));
		CHECK(tile_map_layer->get_cell_source_id(coord1) == 0);
		CHECK_EQ(tile_map_layer->get_cell_atlas_coords(coord1), Vector2i(10, 2));
		CHECK(tile_map_layer->get_cell_alternative_tile(coord1) == 0);
		tile_map_layer->set_cell(coord1, 0, Vector2i(-5, -2), 0);
		CHECK_EQ(tile_map_layer->get_cell(coord1), TileMapCell(0, Vector2i(-5, -2), 0));
	}

	// Requires a TileSet
	SUBCASE("TileData & custom data") {
		tile_map_layer->set_cell(coord1, 0, Vector2i(3, 2), 0);
		TileData *tile_data = tile_map_layer->get_cell_tile_data(coord1);
		ERR_FAIL_COND_MSG(tile_data == nullptr, "Set cell tile data is null");
		tile_set->add_custom_data_layer(0);
		tile_set->set_custom_data_layer_name(0, "custom_data");
		tile_set->set_custom_data_layer_type(0, Variant::FLOAT);
		tile_data->set_custom_data("custom_data", 1.1);
		CHECK(tile_data->has_custom_data("custom_data"));
		CHECK(Math::is_equal_approx((float)tile_data->get_custom_data("custom_data"), (float)1.1));
	}

	SUBCASE("Erase") {
		tile_map_layer->set_cell(coord2, 0, Vector2i(5, 10), 0);
		CHECK_EQ(tile_map_layer->get_cell(coord2), TileMapCell(0, Vector2i(5, 10), 0));
		tile_map_layer->erase_cell(coord2);
		CHECK_EQ(tile_map_layer->get_cell(coord2), default_cell);
	}

	SUBCASE("Clear") {
		tile_map_layer->set_cell(coord1, 0, Vector2i(1, 10), 0);
		tile_map_layer->set_cell(coord2, 0, Vector2i(0, 0), 0);
		CHECK_EQ(tile_map_layer->get_cell(coord1), TileMapCell(0, Vector2i(1, 10), 0));
		CHECK_EQ(tile_map_layer->get_cell(coord2), TileMapCell(0, Vector2i(0, 0), 0));
		tile_map_layer->clear();
		CHECK_EQ(tile_map_layer->get_cell(coord1), default_cell);
		CHECK_EQ(tile_map_layer->get_cell(coord2), default_cell);
	}

	SUBCASE("Get used cells") {
		tile_map_layer->set_cell(coord1, 0, Vector2i(1, 10), 0);
		tile_map_layer->set_cell(coord2, 1, Vector2i(-5, -10), 0);

		TypedArray<Vector2i> used_cells = tile_map_layer->get_used_cells();
		CHECK(used_cells.size() == 2);
		CHECK(used_cells.has(coord1));
		CHECK(used_cells.has(coord2));
		CHECK_FALSE(used_cells.has(coord3));

		used_cells = tile_map_layer->get_used_cells_by_id(0);
		CHECK(used_cells.size() == 1);
		CHECK(used_cells.has(coord1));
		CHECK_FALSE(used_cells.has(coord2));
		CHECK_EQ(tile_map_layer->get_used_rect(), Rect2i(-1, -7, 12, 7));

		tile_map_layer->clear();

		used_cells = tile_map_layer->get_used_cells();
		CHECK(used_cells.size() == 0);
		CHECK_FALSE(used_cells.has(coord1));
		CHECK_FALSE(used_cells.has(coord2));
		CHECK_FALSE(used_cells.has(coord3));

		used_cells = tile_map_layer->get_used_cells_by_id(3);
		CHECK(used_cells.size() == 0);
		CHECK_EQ(tile_map_layer->get_used_rect(), Rect2i());
	}

	memdelete(tile_map_layer);
}


TEST_CASE("[SceneTree][TileMapLayer] Changed signal") {
	Array empty_args = { {} };
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);
	Ref<TileSet> tile_set = create_generic_tile_set(
		TileSet::TILE_SHAPE_SQUARE, 256, 16
	);

	SIGNAL_WATCH(tile_map_layer, "changed");
	tile_map_layer->set_tile_set(tile_set);
	SIGNAL_CHECK("changed", empty_args);
	SIGNAL_DISCARD("changed")

	tile_map_layer->set_cell(Vector2i(100, 2000), 0, Vector2i(0, 0), 0);
	MessageQueue::get_singleton()->flush();  // signal for cells is deferred
	SIGNAL_CHECK("changed", empty_args);
	SIGNAL_DISCARD("changed")

	tile_map_layer->set_navigation_enabled(false);
	SIGNAL_CHECK("changed", empty_args);
	SIGNAL_DISCARD("changed")

	tile_set->set_tile_size(Size2i(10, 10));
	SIGNAL_CHECK("changed", empty_args);
	SIGNAL_DISCARD("changed")

	SIGNAL_UNWATCH(tile_map_layer, "changed");

	memdelete(tile_map_layer);
}

inline TileSet::CellNeighbor get_bit(
	long x,
	long y,
	TileSet::TerrainMode terrain_mode = TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES
) {
	Array bits = {};
	if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
		bits.append({TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER, TileSet::CELL_NEIGHBOR_TOP_SIDE, TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER});
		bits.append({TileSet::CELL_NEIGHBOR_LEFT_SIDE, -1, TileSet::CELL_NEIGHBOR_RIGHT_SIDE});
		bits.append({TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER});
	}
	else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
		bits.append({TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER, -1, TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER});
		bits.append({-1, -1, -1});
		bits.append({TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER, -1, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER});
	}
	else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
		bits.append({-1, TileSet::CELL_NEIGHBOR_TOP_SIDE, -1});
		bits.append({TileSet::CELL_NEIGHBOR_LEFT_SIDE, -1, TileSet::CELL_NEIGHBOR_RIGHT_SIDE});
		bits.append({-1, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, -1});
	}
	return (TileSet::CellNeighbor)((Array)bits[y])[x];
}

/**
 * Adds a terrain to a generic TileSet
 *
 * @param tile_set A reference to an existing TileSet
 * @param peering_bits An array of arrays containing the peering bits.
 * @param terrain_set_id The id of the Terrain set to add. Default: 0
 * @param terrain_id The id of the Terrain to add. Default: 0
 * @param terrain_mode The TerrainMode value to use (Match corners and sides,
 *                     only corners, etc.)
 *
*/
inline void create_terrain(
	Ref<TileSet> tile_set,
	Array peering_bits,
	int terrain_set_id = 0,
	int terrain_id = 0,
	TileSet::TerrainMode terrain_mode = TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES
) {
	const char BIT_ON = 'X';
	Ref<TileSetAtlasSource> tile_set_atlas_source = (Ref<TileSetAtlasSource>)
	                                                tile_set->get_source(0);

	if (tile_set->get_terrain_sets_count() <= terrain_set_id) {
		tile_set->add_terrain_set(terrain_set_id);
		tile_set->set_terrain_set_mode(terrain_set_id, terrain_mode);
	}
	tile_set->add_terrain(terrain_set_id, terrain_id);
	tile_set->set_terrain_name(terrain_set_id, terrain_id, "test_terrain");

	TileData *tile_data;
	for (int tile_number = 0; tile_number < peering_bits.size(); tile_number++) {
		Array tile = peering_bits[tile_number];
		ERR_FAIL_COND_MSG(tile.size() != 3, "Each tile must have exactly 3 rows.");
		tile_data = tile_set_atlas_source->get_tile_data(Vector2i(terrain_set_id, tile_number), 0);
		tile_data->set_terrain_set(terrain_set_id);
		tile_data->set_terrain(terrain_id);

		for (int string_row = 0; string_row < tile.size(); string_row++) {
			String row = tile[string_row];
			ERR_FAIL_COND_MSG(row.length() != 3, "Each row must have exactly 3 characters.");
			for (int string_column = 0; string_column < row.length(); string_column++) {
				if ((int)get_bit(string_column, string_row) == -1) {
					continue;
				}
				if (row[string_column] == BIT_ON) {
					tile_data->set_terrain_peering_bit(
						get_bit(string_column, string_row),
						terrain_id
					);
				}
			}
		}
	}
}

/**
 * Creates a test case for a tileset with a given terrain bit peering setup,
 * and checks it against an expected distribution of tiles.
 *
 * @param drawing An array of arrays containing a list of tiles, each tile is
 *                a list of strings representing in ascii art with an 'X' where
 *                the terrain is being used.
 * @param expected_result A similar-sized array or arrays containing the
 *                        cell expected from the atlas (as a character from '0'
 *                        to '9') or an underscore '_' is no mapped cell is
 *                        expected.
*/
inline void check_terrain(
	TileMapLayer *tile_map_layer,
	Array drawing,
	Array expected_result
) {
	String obtained_tile_distribution = String::chr('\n');
	String expected_tile_distribution = String::chr('\n');
	const char UNSET_CELL = '_';

	// Set the terrain on the tile map
	TypedArray<Vector2i> cells = {};
	for (int i = 0; i < drawing.size(); i++) {
		String row = drawing[i];
		for (int j = 0; j < row.length(); j++) {
			char cell = row[j];
			if (cell >= '0' and cell <= '9') {
				cells.append(Vector2i(j, i));
			}
		}
	}
	tile_map_layer->set_cells_terrain_connect(cells, 0, 0, false);

	// Walk the result and print the differences in nice ascii art
	for (int i = 0; i < expected_result.size(); i++) {
		String expected_string_row = expected_result[i];
		for (int j = 0; j < expected_string_row.length(); j++) {
			int terrain = tile_map_layer->get_cell(Vector2i(j, i)).get_atlas_coords().y;
			if (terrain == TileSetSource::INVALID_ATLAS_COORDS.y) {
				obtained_tile_distribution += UNSET_CELL;
			}
			else {
				obtained_tile_distribution += '0' + terrain;
			}
			expected_tile_distribution += expected_string_row[j];
		}
		obtained_tile_distribution += String::chr('\n');
		expected_tile_distribution += String::chr('\n');
	}
	CHECK_EQ(obtained_tile_distribution, expected_tile_distribution);
}

TEST_CASE("[SceneTree][TileMapLayer] Square terrain match corners and sides") {
	Ref<TileSet> tile_set = create_generic_tile_set(
		TileSet::TILE_SHAPE_SQUARE, 256, 16
	);
	create_terrain(tile_set,
		{{"___",
		  "_X_",
		  "___"},
		 {"X_X",
		  "_X_",
		  "X_X"}},
		 0,
		 0,
		 TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES
	);
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);
	tile_map_layer->set_tile_set(tile_set);

	SUBCASE("2 side by side") {
		check_terrain(
			tile_map_layer,
			{"00"},
			{"00"}
		);
	}

	SUBCASE("2 diagonal accounting spaces") {
		check_terrain(
			tile_map_layer,
			{"0_",
			 "_0"},
			{"0_",
			 "_0"}
		);
	}

	SUBCASE("3 diagonal accounting spaces") {
		check_terrain(
			tile_map_layer,
			{"0__",
			 "_0_",
			 "__0"},
			{"0__",
			 "_0_",
			 "__0"}
		);
	}

	SUBCASE("4 diagonal accounting spaces") {
		check_terrain(
			tile_map_layer,
			{"0_0",
			 "_0_",
			 "__0"},
			{"0_0",
			 "_1_",
			 "__0"}
		);
	}

	SUBCASE("5 diagonal accounting spaces") {
		check_terrain(
			tile_map_layer,
			{"0_0",
			 "_0_",
			 "0_0"},
			{"0_0",
			 "_1_",
			 "0_0"}
		);
	}

	SUBCASE("3x3 square") {
		check_terrain(
			tile_map_layer,
			{"000",
			 "000",
			 "000"},
			{"000",
			 "010",
			 "000"}
		);
	}

	memdelete(tile_map_layer);
}

TEST_CASE("[SceneTree][TileMapLayer] Square terrain match sides only") {
	Ref<TileSet> tile_set = create_generic_tile_set(
		TileSet::TILE_SHAPE_SQUARE, 256, 16
	);
	create_terrain(tile_set,
		{{"___",
		  "_X_",
		  "___"},
		 {"___",
		  "XX_",
		  "___"}},
		 0,
		 0,
		 TileSet::TERRAIN_MODE_MATCH_SIDES
	);
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);
	tile_map_layer->set_tile_set(tile_set);

	SUBCASE("2 side by side") {
		check_terrain(
			tile_map_layer,
			{"____",
			 "_00_",
			 "____"},
			{"____",
			 "_01_",
			 "____"}
		);
	}

	memdelete(tile_map_layer);
}

TEST_CASE("[SceneTree][TileMapLayer] Square terrain match corners only") {
	Ref<TileSet> tile_set = create_generic_tile_set(
		TileSet::TILE_SHAPE_SQUARE, 256, 16
	);
	create_terrain(tile_set,
		{{"___",
		  "_X_",
		  "___"},
		 {"X__",
		  "_X_",
		  "___"}},
		 0,
		 0,
		 TileSet::TERRAIN_MODE_MATCH_CORNERS
	);
	TileMapLayer *tile_map_layer = memnew(TileMapLayer);
	tile_map_layer->set_tile_set(tile_set);

	SUBCASE("2 diagonal") {
		check_terrain(
			tile_map_layer,
			{"____",
			 "_0__",
			 "__0_",
			 "____"},
			{"____",
			 "_0__",
			 "__1_",
			 "____"}
		);
	}

	memdelete(tile_map_layer);
}

} // namespace TestTileMapLayer

#include <flame/universe/world.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/tile_map.h>

#include "../renderpath/canvas/canvas.h"

#include <flame/reflect_macros.h>

namespace flame
{
	struct cTileMapPrivate : cTileMap
	{
		std::vector<uint> tiles;
		std::vector<int> cells;

		void* draw_cmd;

		cTileMapPrivate()
		{
			element = nullptr;

			size_ = Vec2u(0);
			cell_size = Vec2f(0.f);

			draw_cmd = nullptr;
		}

		~cTileMapPrivate()
		{
			if (!entity->dying_)
				element->cmds.remove(draw_cmd);
		}

		void on_component_added(Component* c) override
		{
			if (c->name_hash == FLAME_CHASH("cElement"))
			{
				element = (cElement*)c;
				draw_cmd = element->cmds.add([](void* c, graphics::Canvas* canvas) {
					(*(cTileMapPrivate**)c)->draw(canvas);
				}, new_mail_p(this));
			}
		}

		void draw(graphics::Canvas* canvas)
		{
			if (!element->cliped)
			{
				auto padding = element->inner_padding_ * element->global_scale;
				auto pos = element->global_pos + Vec2f(padding[0], padding[1]);
				for (auto y = 0; y < size_.y(); y++)
				{
					for (auto x = 0; x < size_.x(); x++)
					{
						auto idx = cells[y * size_.x() + x];
						if (idx != -1)
							canvas->add_image(pos + Vec2f(x * cell_size.x(), y * cell_size.y()), cell_size, tiles[idx], Vec2f(0.f), Vec2f(1.f));
					}
				}
			}
		}
	};

	uint cTileMap::tile_count() const
	{
		return ((cTileMapPrivate*)this)->tiles.size();
	}

	uint cTileMap::tile(uint idx) const
	{
		return ((cTileMapPrivate*)this)->tiles[idx];
	}

	void cTileMap::add_tile(uint id)
	{
		auto& tiles = ((cTileMapPrivate*)this)->tiles;
		for (auto t : tiles)
		{
			if (t == id)
				return;
		}
		tiles.push_back(id);
	}

	void cTileMap::remove_tile(uint id)
	{
		auto thiz = (cTileMapPrivate*)this;
		auto& tiles = thiz->tiles;
		for (auto it = tiles.begin(); it != tiles.end(); it++)
		{
			if (*it == id)
			{
				tiles.erase(it);
				for (auto& c : thiz->cells)
					c = -1;
				return;
			}
		}
	}

	void cTileMap::set_size(const Vec2u& s)
	{
		if (size_ == s)
			return;
		auto& cells = ((cTileMapPrivate*)this)->cells;
		std::vector<int> new_cells(s.x() * s.y(), -1);
		auto mx = min(s.x(), size_.x());
		auto my = min(s.y(), size_.y());
		for (auto y = 0; y < my; y++)
		{
			for (auto x = 0; x < mx; x++)
				new_cells[y * s.x() + x] = cells[y * size_.x() + x];
		}
		size_ = s;
		cells = new_cells;
	}

	int cTileMap::cell(const Vec2i& idx) const
	{
		if (idx.x() < 0 || idx.x() >= size_.x() || idx.y() < 0 || idx.y() >= size_.y())
			return -2;
		return ((cTileMapPrivate*)this)->cells[idx.y() * size_.x() + idx.x()];
	}

	void cTileMap::set_cell(const Vec2u& idx, int tile_idx)
	{
		auto thiz = (cTileMapPrivate*)this;
		if (idx.x() >= size_.x() || idx.y() >= size_.y())
			return;
		if (tile_idx != -1 && tile_idx >= thiz->tiles.size())
			return;
		thiz->cells[idx.y() * size_.x() + idx.x()] = tile_idx;
	}

	void cTileMap::clear_cells()
	{
		auto thiz = (cTileMapPrivate*)this;
		for (auto y = 0; y < size_.y(); y++)
		{
			for (auto x = 0; x < size_.x(); x++)
				thiz->cells[y * size_.x() + x] = -1;
		}
	}

	cTileMap* cTileMap::create()
	{
		return new cTileMapPrivate();
	}

	struct R(Serializer_cTileMap, flame,)
	{
		Vec2u size;
		Vec2f cell_size;
		Array<ulonglong> tiles;
		Array<int> cells;

		FLAME_UNIVERSE_EXPORTS RF(Serializer_cTileMap)()
		{
			size = Vec2u(0);
			cell_size = Vec2f(0.f);
		}

		FLAME_UNIVERSE_EXPORTS Component* RF(create)(World* w)
		{
			auto c = new cTileMapPrivate();

			c->size_ = size;
			c->cell_size = cell_size;
			c->tiles.resize(tiles.s);
			for (auto i = 0; i < tiles.s; i++)
			{
				auto id = tiles[i];
				auto atlas = (graphics::Atlas*)w->find_object(FLAME_CHASH("Atlas"), id >> 32);
				c->tiles[i] = (atlas->canvas_slot_ << 16) + atlas->find_tile(id & 0xffffffff);
			}
			c->cells.resize(cells.s);
			for (auto i = 0; i < cells.s; i++)
				c->cells[i ]= cells[i];

			return c;
		}

		FLAME_UNIVERSE_EXPORTS void RF(serialize)(Component* _c, int offset)
		{
			auto c = (cTileMapPrivate*)_c;

			if (offset == -1)
			{
				size = c->size_;
				cell_size = c->cell_size;

			}
		}

		FLAME_UNIVERSE_EXPORTS void  RF(unserialize)(Component* _c, int offset)
		{
			auto c = (cTileMapPrivate*)_c;

			if (offset == -1)
			{
				c->size_ = size;
				c->cell_size = cell_size;
			}
		}
	};
}

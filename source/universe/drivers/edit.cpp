#include "../../graphics/font.h"
#include "../world_private.h"
#include "../components/text_private.h"
#include "../components/receiver_private.h"
#include "edit_private.h"
#include "../systems/dispatcher_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	void dEditPrivate::mark_changed()
	{
		if (trigger_changed_on_lost_focus)
			changed = true;
		else
			text->mark_text_changed();
	}

	void dEditPrivate::flash_cursor(int mode)
	{
		if (element)
			element->mark_drawing_dirty();
		if (mode == 1)
			show_cursor = false;
		else if (mode == 2)
		{
			if (flash_event)
				looper().reset_event(flash_event);
			show_cursor = true;
		}
		else
			show_cursor = !show_cursor;
	}

	int dEditPrivate::locate_cursor(const vec2& mpos)
	{
		const auto& str = text->text;
		auto atlas = text->atlas;
		auto font_size = text->font_size;

		element->update_transform();

		auto pp = element->axes_inv * (mpos - element->points[4]);

		int n = str.size();
		float base_y = 0, prev_x;
		int i = 0, k;

		auto r_size = vec2(0.f);
		auto r_num_chars = 0U;

		while (i < n)
		{
			text->row_layout(i, r_size, r_num_chars);
			if (r_num_chars <= 0)
				return n;

			if (i == 0 && pp.y < base_y)
				return 0;

			if (pp.y < base_y + r_size.y)
				break;

			i += r_num_chars;
			base_y += r_size.y;
		}

		if (i >= n)
			return n;

		if (pp.x < 0.f)
			return i;

		if (pp.x < r_size.x)
		{
			prev_x = 0.f;
			for (k = 0; k < r_num_chars; ++k)
			{
				auto w = atlas->get_glyph(str[i], font_size).advance;
				if (pp.x < prev_x + w)
				{
					if (pp.x < prev_x + w / 2)
						return k + i;
					else
						return k + i + 1;
				}
				prev_x += w;
			}
		}

		if (str[i + r_num_chars - 1] == L'\n')
			return i + r_num_chars - 1;
		else
			return i + r_num_chars;
	}

	void dEditPrivate::on_load_finished()
	{
		element = entity->get_component_i<cElementPrivate>(0);
		fassert(element);

		receiver = entity->get_component_t<cReceiverPrivate>();
		fassert(receiver);

		text = entity->get_component_t<cTextPrivate>();
		fassert(text);

		entity->add_message_listener([](Capture& c, uint msg, void* parm1, void* parm2) {
			auto thiz = c.thiz<dEditPrivate>();
			switch (msg)
			{
			case S<"visibility_changed"_h>:
				if (thiz->flash_event && !parm1)
				{
					looper().remove_event(thiz->flash_event);
					thiz->flash_event = nullptr;
				}
				break;
			case S<"state_changed"_h>:
				thiz->receiver->dispatcher->window->set_cursor(((int)parm1 & StateHovering) != 0 ? CursorIBeam : CursorArrow);
				if (((int)parm1 & StateFocusing) != 0)
				{
					if (!thiz->flash_event)
					{
						thiz->flash_event = looper().add_event([](Capture& c) {
							c.thiz<dEditPrivate>()->flash_cursor(0);
							c._current = nullptr;
						}, Capture().set_thiz(thiz), 0.5f);
					}
					//if (thiz->select_all_on_focus)
					//	thiz->set_select(0, thiz->text->text.s);
				}
				else
				{
					if (thiz->flash_event)
					{
						looper().remove_event(thiz->flash_event);
						thiz->flash_event = nullptr;
					}
					thiz->select_start = thiz->select_end = 0;
					thiz->flash_cursor(1);
					if (thiz->trigger_changed_on_lost_focus && thiz->changed)
					{
						thiz->text->mark_text_changed();
						thiz->changed = false;
					}
				}
				break;
			case S<"left_world"_h>:
				if (thiz->flash_event)
				{
					looper().remove_event(thiz->flash_event);
					thiz->flash_event = nullptr;
				}
				break;
			}
		}, Capture().set_thiz(this));

		element->add_drawer([](Capture& c, uint layer, sRendererPtr renderer) {
			auto thiz = c.thiz<dEditPrivate>();
			return thiz->draw(layer, renderer);
		}, Capture().set_thiz(this));

		receiver->add_key_down_listener([](Capture& c, KeyboardKey key) {
			auto thiz = c.thiz<dEditPrivate>();
			auto& str = thiz->text->text;
			auto& select_start = thiz->select_start;
			auto& select_end = thiz->select_end;
			auto low = min(select_start, select_end);
			auto high = max(select_start, select_end);
			auto ed = thiz->receiver->dispatcher;

			auto line_start = [&](int p) {
				p--;
				do
				{
					if (p < 0)
						return 0;
					if (str[p] == '\n')
						return p + 1;
					p--;
				} while (p >= 0);
				return 0;
			};
			auto line_end = [&](int p) {
				while (p < str.size())
				{
					if (str[p] == '\n')
						return p;
					p++;
				}
				return (int)str.size();
			};

			switch (key)
			{
			case Keyboard_Enter:
				if (thiz->enter_to_throw_focus)
				{
					auto dp = thiz->receiver->dispatcher;
					dp->next_focusing = dp->world->root->get_component_t<cReceiverPrivate>();
				}
				break;
			case Keyboard_Left:
				if (ed->kbtns[Keyboard_Shift].first)
				{
					if (select_end > 0)
						select_end--;
				}
				else
				{
					if (low > 0)
						low--;
					select_start = select_end = low;
				}
				break;
			case Keyboard_Right:
				if (ed->kbtns[Keyboard_Shift].first)
				{
					if (select_end < str.size())
						select_end++;
				}
				else
				{
					if (high < str.size())
						high++;
					select_start = select_end = high;
				}
				break;
			case Keyboard_Up:
			{
				if (ed->kbtns[Keyboard_Shift].first)
				{
					auto end_s = line_start(select_end);
					auto up_s = line_start(end_s - 1);
					if (end_s != up_s)
						select_end = up_s + min((int)select_end - end_s, max(0, end_s - up_s - 1));
				}
				else
				{
					auto end_s = line_start(select_end);
					auto low_s = low == select_end ? end_s : line_start(low);
					auto up_s = line_start(low_s - 1);
					if (low_s != up_s)
						select_end = select_start = up_s + min((int)select_end - end_s, max(0, low_s - up_s - 1));
					else
						select_start = select_end;
				}
			}
				break;
			case Keyboard_Down:
			{
				if (ed->kbtns[Keyboard_Shift].first)
				{
					auto end_s = line_start(select_end);
					auto end_e = line_end(select_end);
					auto down_l = line_end(end_e + 1) - end_e - 1;
					if (down_l >= 0)
						select_end = end_e + 1 + min(down_l, (int)select_end - end_s);
				}
				else
				{
					auto end_s = line_start(select_end);
					auto high_e = line_end(high);
					auto down_l = line_end(high_e + 1) - high_e - 1;
					if (down_l >= 0)
						select_end = select_start = high_e + 1 + min(down_l, (int)select_end - end_s);
					else
						select_start = select_end;
				}
			}
				break;
			case Keyboard_Home:
				select_end = (ed->kbtns[Keyboard_Ctrl].first) ? 0 : line_start(select_end);
				if (!(ed->kbtns[Keyboard_Shift].first))
					select_start = select_end;
				break;
			case Keyboard_End:
				select_end = (ed->kbtns[Keyboard_Ctrl].first) ? str.size() : line_end(select_end);
				if (!(ed->kbtns[Keyboard_Shift].first))
					select_start = select_end;
				break;
			case Keyboard_Del:
				if (low == high)
				{
					if (low < str.size())
					{
						str.erase(str.begin() + low);
						thiz->mark_changed();
					}
				}
				else
				{
					str = str.substr(0, low) + str.substr(high);
					thiz->mark_changed();
					select_start = select_end = low;
				}
				break;
			}

			thiz->flash_cursor(2);
		}, Capture().set_thiz(this));

		receiver->add_char_listener([](Capture& c, wchar_t ch) {
			auto thiz = c.thiz<dEditPrivate>();
			auto& str = thiz->text->text;
			auto& select_start = thiz->select_start;
			auto& select_end = thiz->select_end;
			auto low = min(select_start, select_end);
			auto high = max(select_start, select_end);

			switch (ch)
			{
			case 24: // Ctrl+X
			case 3: // Ctrl+C
				if (low == high)
					break;
				set_clipboard(str.substr(low, high - low).c_str());
				if (ch == 3)
					break;
			case L'\b':
				if (low == high)
				{
					if (low > 0)
					{
						low--;
						str.erase(str.begin() + low);
						thiz->mark_changed();
						select_start = select_end = low;
					}
				}
				else
				{
					str = str.substr(0, low) + str.substr(high);
					thiz->mark_changed();
					select_start = select_end = low;
				}
				break;
			case 22: // Ctrl+V
			{
				std::wstring cb;
				get_clipboard(&cb, [](void* _str, uint size) {
					auto& str = *(std::wstring*)_str;
					str.resize(size);
					return str.data();
				});
				cb.erase(std::remove(cb.begin(), cb.end(), '\r'), cb.end());
				if (!cb.empty())
				{
					str = str.substr(0, low) + cb + str.substr(high);
					thiz->mark_changed();
					select_start = select_end = high + cb.size() - (high - low);
				}
			}
				break;
			case 27:
				break;
			case '\r':
				if (thiz->enter_to_throw_focus)
					break;
				ch = '\n';
			default:
				str = str.substr(0, low) + ch + str.substr(high);
				thiz->mark_changed();
				select_start = select_end = high + 1 - (high - low);
			}

			thiz->flash_cursor(2);
		}, Capture().set_thiz(this));

		receiver->add_mouse_left_down_listener([](Capture& c, const ivec2& pos) {
			auto thiz = c.thiz<dEditPrivate>();
			thiz->select_start = thiz->select_end = thiz->locate_cursor((vec2)pos);
			thiz->flash_cursor(2);
		}, Capture().set_thiz(this));

		receiver->add_mouse_move_listener([](Capture& c, const ivec2& disp, const ivec2& pos) {
			auto thiz = c.thiz<dEditPrivate>();
			if (thiz->receiver->dispatcher->active == thiz->receiver)
			{
				thiz->select_end = thiz->locate_cursor((vec2)pos);
				thiz->flash_cursor(2);
			}
		}, Capture().set_thiz(this));

		receiver->add_mouse_dbclick_listener([](Capture& c) {
			auto thiz = c.thiz<dEditPrivate>();
			//	thiz->select_start = 0;
			//	thiz->select_end = thiz->text->text.size();
			//	if (thiz->element)
			//		thiz->element->mark_drawing_dirty();
		}, Capture().set_thiz(this));
	}

	uint dEditPrivate::draw(uint layer, sRendererPtr renderer)
	{
		layer++;

		const auto& str = text->text;
		auto res_id = text->res_id;
		auto atlas = text->atlas;
		auto font_size = text->font_size;
		element->update_transform();

		if (select_start != select_end)
		{
			auto low = min(select_start, select_end);
			auto high = max(select_start, select_end);
			while (low < high)
			{
				auto left = low;
				auto right = left;
				while (right < high)
				{
					if (str[right] == '\n')
						break;
					right++;
				}
				low = right + 1;

				auto sb = str.c_str() + left, se = str.c_str() + right;
				auto p1 = vec2(atlas->text_offset(font_size, str.c_str(), sb));
				auto p2 = vec2(atlas->text_offset(font_size, str.c_str(), se));
				if (right < high && str[right] == '\n')
					p2.x += 4.f;
				p2.y += font_size;
				renderer->fill_rect(layer, element, element->padding.xy() + p1, p2 - p1, cvec4(128, 128, 255, 255));
				renderer->draw_text(layer, element, element->padding.xy() + p1, font_size, res_id, sb, se, text->font_color);
			}
		}

		if (show_cursor)
		{
			auto off = vec2(atlas->text_offset(font_size, str.c_str(), str.c_str() + select_end)) + element->padding.xy();
			off.x += 0.5f;
			vec2 pts[] = {
				off,
				vec2(off.x, off.y + font_size)
			};
			renderer->stroke(layer, element, _countof(pts), pts, max(1.f, round(font_size / 14.f)), text->font_color);
		}

		return layer;
	}

	dEdit* dEdit::create(void* parms)
	{
		return f_new<dEditPrivate>();
	}
}

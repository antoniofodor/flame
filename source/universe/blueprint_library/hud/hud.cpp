#include "../../../foundation/blueprint.h"
#include "../../../foundation/typeinfo.h"
#include "../../systems/renderer_private.h"

namespace flame
{
	void add_hud_node_templates(BlueprintNodeLibraryPtr library)
	{
		library->add_template("Hud", "",
			{
				{
					.name = "Pos",
					.allowed_types = { TypeInfo::get<vec2>() }
				},
				{
					.name = "Size",
					.allowed_types = { TypeInfo::get<vec2>() }
				},
				{
					.name = "Col",
					.allowed_types = { TypeInfo::get<cvec4>() },
					.default_value = "200,200,200,255"
				},
				{
					.name = "Pivot",
					.allowed_types = { TypeInfo::get<vec2>() }
				},
				{
					.name = "Item Spacing",
					.allowed_types = { TypeInfo::get<vec2>() },
					.default_value = "2,2"
				}
			},
			{
			},
			true,
			[](BlueprintAttribute* inputs, BlueprintAttribute* outputs, uint* max_execute_times) {
				auto pos = *(vec2*)inputs[1].data;
				auto size = *(vec2*)inputs[2].data;
				auto col = *(cvec4*)inputs[3].data;
				auto pivot = *(vec2*)inputs[4].data;
				auto item_spacing = *(vec2*)inputs[5].data;

				sRenderer::instance()->begin_hud(pos, size, col, pivot, item_spacing);

				*max_execute_times = 1;
			},
			[](BlueprintAttribute* inputs, BlueprintAttribute* outputs) {
				sRenderer::instance()->end_hud();
			}
		);

		library->add_template("Hud Rect", "",
			{
				{
					.name = "Pos",
					.allowed_types = { TypeInfo::get<vec2>() }
				},
				{
					.name = "Size",
					.allowed_types = { TypeInfo::get<vec2>() }
				},
				{
					.name = "Col",
					.allowed_types = { TypeInfo::get<cvec4>() },
					.default_value = "255,255,255,255"
				}
			},
			{
			},
			[](BlueprintAttribute* inputs, BlueprintAttribute* outputs) {
				auto pos = *(vec2*)inputs[0].data;
				auto size = *(vec2*)inputs[1].data;
				auto col = *(cvec4*)inputs[2].data;

				sRenderer::instance()->hud_rect(pos, size, col);
			}
		);

		library->add_template("Hud Text", "",
			{
				{
					.name = "Text",
					.allowed_types = { TypeInfo::get<std::wstring>() }
				},
				{
					.name = "Col",
					.allowed_types = { TypeInfo::get<cvec4>() },
					.default_value = "255,255,255,255"
				}
			},
			{
			},
			[](BlueprintAttribute* inputs, BlueprintAttribute* outputs) {
				auto& text = *(std::wstring*)inputs[0].data;
				auto col = *(cvec4*)inputs[1].data;

				sRenderer::instance()->hud_text(text, col);
			}
		);

		library->add_template("Hud Button", "",
			{
				{
					.name = "Label",
					.allowed_types = { TypeInfo::get<std::wstring>() }
				}
			},
			{
			},
			true,
			[](BlueprintAttribute* inputs, BlueprintAttribute* outputs, uint* max_execute_times) {
				auto& label = *(std::wstring*)inputs[1].data;
				auto clicked = sRenderer::instance()->hud_button(label);

				*max_execute_times = clicked ? 1 : 0;
			}
		);
	}
}

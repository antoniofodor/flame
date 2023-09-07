#include "../../../foundation/blueprint.h"
#include "../../../foundation/typeinfo.h"
#include "../../systems/input_private.h"

namespace flame
{
	void add_input_node_templates(BlueprintNodeLibraryPtr library)
	{
		library->add_template("Mouse Pos", "",
			{
			},
			{
				{
					.name = "Pos",
					.allowed_types = { TypeInfo::get<vec2>() }
				}
			},
			[](BlueprintAttribute* inputs, BlueprintAttribute* outputs) {
				*(vec2*)outputs[0].data = sInput::instance()->mpos;
			},
			nullptr,
			nullptr,
			nullptr,
			nullptr
		);
	}
}

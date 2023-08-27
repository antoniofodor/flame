#include "blueprint.h"

namespace flame
{
	struct BlueprintSlotPrivate : BlueprintSlot
	{
		std::vector<BlueprintSlotPtr> linked_slots;
	};

	struct BlueprintNodePrivate : BlueprintNode
	{

	};

	struct BlueprintLinkPrivate : BlueprintLink
	{

	};

	struct BlueprintBlockPrivate : BlueprintBlock
	{

	};

	struct BlueprintGroupPrivate : BlueprintGroup
	{

	};

	struct BlueprintPrivate : Blueprint
	{
		uint next_object_id = 1;

		BlueprintPrivate();

		BlueprintNodePtr		add_node(BlueprintGroupPtr group, BlueprintBlockPtr block, const std::string& name,
			const std::vector<BlueprintSlotDesc>& inputs = {}, const std::vector<BlueprintSlotDesc>& outputs = {},
			BlueprintNodeFunction function = nullptr, BlueprintNodeConstructor constructor = nullptr, BlueprintNodeDestructor destructor = nullptr,
			BlueprintNodeInputSlotChangedCallback input_slot_changed_callback = nullptr, BlueprintNodePreviewProvider preview_provider = nullptr) override;
		BlueprintNodePtr		add_input_node(BlueprintGroupPtr group, BlueprintBlockPtr block, uint name) override;
		BlueprintNodePtr		add_variable_node(BlueprintGroupPtr group, BlueprintBlockPtr block, uint variable_group_name) override;
		void					remove_node(BlueprintNodePtr node) override;
		BlueprintLinkPtr		add_link(BlueprintSlotPtr from_slot, BlueprintSlotPtr to_slot) override;
		void					remove_link(BlueprintLinkPtr link) override;
		BlueprintBlockPtr		add_block(BlueprintGroupPtr group, BlueprintBlockPtr parent) override;
		void					remove_block(BlueprintBlockPtr block) override;
		BlueprintGroupPtr		add_group(const std::string& name) override;
		void					remove_group(BlueprintGroupPtr group) override;
		BlueprintSlotPtr		add_group_input(BlueprintGroupPtr group, const std::string& name, TypeInfo* type) override;
		void					remove_group_input(BlueprintGroupPtr group, BlueprintSlotPtr slot) override;
		BlueprintSlotPtr		add_group_output(BlueprintGroupPtr group, const std::string& name, TypeInfo* type) override;
		void					remove_group_output(BlueprintGroupPtr group, BlueprintSlotPtr slot) override;
		void					save(const std::filesystem::path& path) override;
	};

	struct BlueprintNodeLibraryPrivate : BlueprintNodeLibrary
	{
		void add_template(const std::string& name, const std::vector<BlueprintSlotDesc>& inputs = {}, const std::vector<BlueprintSlotDesc>& outputs = {},
			BlueprintNodeFunction function = nullptr, BlueprintNodeConstructor constructor = nullptr, BlueprintNodeDestructor destructor = nullptr,
			BlueprintNodeInputSlotChangedCallback input_slot_changed_callback = nullptr, BlueprintNodePreviewProvider preview_provider = nullptr) override;
	};

	struct BlueprintInstancePrivate : BlueprintInstance
	{
		BlueprintInstancePrivate(BlueprintPtr blueprint);
		~BlueprintInstancePrivate();

		void build() override;
		void prepare_executing(Group* group) override;
		void run() override;
		void step() override;
		void stop() override;
	};

	struct BlueprintDebuggerPrivate : BlueprintDebugger
	{
		void add_break_node(BlueprintNodePtr node) override;
		void remove_break_node(BlueprintNodePtr node) override;
	};
}

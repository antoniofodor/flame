#include "blueprint_window.h"
#include "project_window.h"

#include <flame/foundation/sheet.h>
#include <flame/foundation/blueprint.h>
#include <flame/graphics/model.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/mesh.h>

static ImColor color_from_depth(uint depth)
{
	depth -= 1;
	auto shift = depth / 7;
	auto h = (depth % 7) / 7.f * 360.f + shift * 10.f;
	auto color = rgbColor(vec3(h, 0.5f, 1.f));
	return ImColor(color.r, color.g, color.b);
}

static ImColor color_from_type(TypeInfo* type)
{
	auto h = 0.f;
	auto s = 0.f;
	auto v = 1.f;
	if (type->tag == TagD)
	{
		auto ti = (TypeInfo_Data*)type;
		switch (ti->data_type)
		{
		case DataBool:
			h = 300.f;
			s = 0.5f;
			break;
		case DataFloat:
			h = 120.f;
			s = 0.5f + ti->vec_size * 0.1f;
			break;
		case DataInt:
			if (ti->is_signed)
			{
				h = 60.f;
				s = 0.5f + ti->vec_size * 0.1f;
			}
			else
			{
				h = 0.f;
				s = 0.5f + ti->vec_size * 0.1f;
			}
			break;
		}
	}
	else
	{
		if (type == TypeInfo::get<EntityPtr>())
		{
			h = 240.f;
			s = 0.5f;
		}
	}
	auto color = rgbColor(vec3(h, s, v));
	return ImColor(color.r, color.g, color.b);
}

static void set_offset_recurisely(BlueprintNodePtr n, const vec2& offset)
{
	n->position += offset;
	ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);
	for (auto c : n->children)
		set_offset_recurisely(c, offset);
};

static BlueprintNodeLibraryPtr standard_library;
static BlueprintNodeLibraryPtr extern_library;
static BlueprintNodeLibraryPtr noise_library;
static BlueprintNodeLibraryPtr texture_library;
static BlueprintNodeLibraryPtr geometry_library;
static BlueprintNodeLibraryPtr entity_library;
static BlueprintNodeLibraryPtr camera_library;
static BlueprintNodeLibraryPtr procedural_library;
static BlueprintNodeLibraryPtr navigation_library;
static BlueprintNodeLibraryPtr colliding_library;
static BlueprintNodeLibraryPtr input_library;
static BlueprintNodeLibraryPtr primitive_library;
static BlueprintNodeLibraryPtr hud_library;
static BlueprintNodeLibraryPtr audio_library;
static BlueprintNodeLibraryPtr resource_library;

struct CopiedSlot
{
	TypeInfo* type = nullptr;
	std::string value;
};

struct CopiedNode
{
	uint object_id;
	uint name;
	std::string template_string;
	uint parent;
	std::map<uint, CopiedSlot> input_datas;
	vec2 position;
	Rect rect;
};

struct CopiedLink
{
	uint from_node;
	uint from_slot;
	uint to_node;
	uint to_slot;
};

static BlueprintGroupPtr copy_src_group = nullptr;
static std::vector<CopiedNode> copied_nodes;
static std::vector<CopiedLink> copied_links;

static bool if_any_contains(const std::vector<BlueprintNodePtr>& list, BlueprintNodePtr node)
{
	for (auto n : list)
	{
		if (n == node || n->contains(node))
			return true;
	}
	return false;
};

static std::vector<BlueprintNodePtr> get_selected_nodes()
{
	std::vector<BlueprintNodePtr> nodes;
	if (auto n = ax::NodeEditor::GetSelectedObjectCount(); n > 0)
	{
		std::vector<ax::NodeEditor::NodeId> node_ids(n);
		n = ax::NodeEditor::GetSelectedNodes(node_ids.data(), n);
		if (n > 0)
		{
			nodes.resize(n);
			for (auto i = 0; i < n; i++)
				nodes[i] = (BlueprintNodePtr)(uint64)node_ids[i];
		}
	}
	return nodes;
}

static std::vector<BlueprintLinkPtr> get_selected_links()
{
	std::vector<BlueprintLinkPtr> links;
	if (auto n = ax::NodeEditor::GetSelectedObjectCount(); n > 0)
	{
		std::vector<ax::NodeEditor::LinkId> link_ids(n);
		n = ax::NodeEditor::GetSelectedLinks(link_ids.data(), n);
		if (n > 0)
		{
			links.resize(n);
			for (auto i = 0; i < n; i++)
				links[i] = (BlueprintLinkPtr)(uint64)link_ids[i];
		}
	}
	return links;
}

static void set_input_type(BlueprintSlotPtr slot, TypeInfo* type)
{
	auto n = slot->node;
	auto g = n->group;
	std::vector<TypeInfo*> new_input_types;
	new_input_types.resize(n->inputs.size());
	for (auto i = 0; i < n->inputs.size(); i++)
		new_input_types[i] = n->inputs[i].get() == slot ? type : n->inputs[i]->type;
	g->blueprint->change_node_structure(n, "", new_input_types);
}

static void post_process_new_node(BlueprintNodePtr n)
{
	auto group = n->group;
	auto blueprint = group->blueprint;
	auto n_slots = 0;
	for (auto& o : n->outputs)
	{
		if (o->type == TypeInfo::get<BlueprintSignal>())
		{
			if (o->get_linked_count() == 0)
			{
				auto n_block = blueprint->add_block(group, n->parent);
				n_block->position = n->position + vec2(0.f, (n_slots + 1) * 100.f);
				ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n_block, n_block->position);
				blueprint->add_link(o.get(), n_block->find_input("Execute"_h));
			}
			n_slots++;
		}
	}
}

static BlueprintNodePtr add_variable_node_unifily(BlueprintGroupPtr g, uint var_name, uint var_location)
{
	for (auto& n : g->nodes)
	{
		if (n->depth == 1 && n->name_hash == "Variable"_h)
		{
			auto node_var_name = *(uint*)n->inputs[0]->data;
			auto node_var_location = *(uint*)n->inputs[1]->data;
			if (var_name == node_var_name && var_location == node_var_location)
				return n.get();
		}
	}
	auto y = 0.f;
	for (auto& n : g->nodes)
	{
		if (n->depth == 1 && n->name_hash == "Variable"_h)
			y += 100.f;
	}
	auto new_n = g->blueprint->add_variable_node(g, g->nodes.front().get(), var_name, "Variable"_h, var_location);
	new_n->position = vec2(0.f, y);
	ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)new_n, new_n->position);
	return new_n;

}

static float get_node_posy(BlueprintNodePtr n)
{
	auto ret = n->position.y;
	for (auto s : n->group->splits)
	{
		if (n->position.x > s)
			ret += 10000.f;
	}
	return ret;
}

static bool compare_nodes_posy(const BlueprintNodePtr a, const BlueprintNodePtr b)
{
	return get_node_posy(a) < get_node_posy(b);
}

static void compute_node_orders(BlueprintGroupPtr g)
{
	std::function<void(BlueprintNodePtr)> compute_order;
	compute_order = [&](BlueprintNodePtr n) {
		for (auto c : n->children)
			compute_order(c);
		std::sort(n->children.begin(), n->children.end(), compare_nodes_posy);
	};
	compute_order(g->nodes.front().get());

	auto frame = frames;
	g->structure_changed_frame = frame;
	g->blueprint->dirty_frame = frame;
}

static BlueprintNodePtr				f9_bound_breakpoint = nullptr;
static BlueprintBreakpointOption	f9_bound_breakpoint_option;

BlueprintWindow blueprint_window;

BlueprintView::BlueprintView() :
	BlueprintView(blueprint_window.views.empty() ? "Blueprint" : "Blueprint##" + str(rand()))
{
}

BlueprintView::BlueprintView(const std::string& name) :
	View(&blueprint_window, name)
{
	auto sp = SUS::split(name, '#');
	if (sp.size() > 1)
	{
		blueprint_path = sp[0];
		if (sp.size() > 2)
		{
			group_name = sp[1];
			group_name_hash = sh(group_name.c_str());
			View::name = std::string(sp[0]) + "##Blueprint";
		}
	}

	ax::NodeEditor::Config ax_config;
	ax_config.UserPointer = this;
	ax_config.SettingsFile = "";
	ax_config.NavigateButtonIndex = 2;
	ax_config.SaveNodeSettings = [](ax::NodeEditor::NodeId node_id, const char* data, size_t size, ax::NodeEditor::SaveReasonFlags reason, void* user_data) {
		auto& view = *(BlueprintView*)user_data;
		if (frames == view.load_frame)
			return true;
		if (blueprint_window.debugger->debugging &&
			blueprint_window.debugger->debugging->instance->blueprint == view.blueprint &&
			blueprint_window.debugger->debugging->name == view.group_name_hash)
			return true;
		auto node = (BlueprintNodePtr)(uint64)node_id;
		if ((reason & ax::NodeEditor::SaveReasonFlags::AddNode) != ax::NodeEditor::SaveReasonFlags::None ||
			(reason & ax::NodeEditor::SaveReasonFlags::RemoveNode) != ax::NodeEditor::SaveReasonFlags::None ||
			(reason & ax::NodeEditor::SaveReasonFlags::Position) != ax::NodeEditor::SaveReasonFlags::None)
		{
			auto& siblings = node->parent->children;
			std::sort(siblings.begin(), siblings.end(), compare_nodes_posy);
			auto frame = frames;
			node->group->structure_changed_frame = frame;
			view.blueprint->dirty_frame = frame;
		}
		view.unsaved = true;
		return true;
	};
	ax_editor = (ax::NodeEditor::Detail::EditorContext*)ax::NodeEditor::CreateEditor(&ax_config);
}

struct BpNodePreview
{
	ModelPreviewer model_previewer;

	~BpNodePreview()
	{
		model_previewer.destroy();
	}
};
static std::map<BlueprintNodePtr, BpNodePreview> previews;

BlueprintView::~BlueprintView()
{
	if (app_exiting)
		return;
	if (blueprint)
	{
		previews.clear();
		Blueprint::release(blueprint);
	}
	if (blueprint_instance)
		delete blueprint_instance;
	if (ax_editor)
		ax::NodeEditor::DestroyEditor((ax::NodeEditor::EditorContext*)ax_editor);
}

void BlueprintView::copy_nodes(BlueprintGroupPtr g)
{
	if (auto selected_nodes = get_selected_nodes(); !selected_nodes.empty())
	{
		copy_src_group = g;
		copied_nodes.clear();
		copied_links.clear();
		std::vector<BlueprintNodePtr> nodes;
		for (auto n : selected_nodes)
			blueprint_form_top_list(nodes, n);

		std::function<void(BlueprintNodePtr)> add_node_recursively;
		add_node_recursively = [&](BlueprintNodePtr src_n) {
			auto& n = copied_nodes.emplace_back();
			n.object_id = src_n->object_id;
			n.name = src_n->name_hash;
			n.template_string = src_n->template_string;
			n.parent = src_n->parent->object_id;
			for (auto& i : src_n->inputs)
			{
				if (i->get_linked_count() == 0)
				{
					CopiedSlot s;
					if ((i->type && !i->allowed_types.empty() && i->type != i->allowed_types.front()))
						s.type = i->type;
					if (i->type->tag != TagU)
					{
						if (auto value_str = i->type->serialize(i->data); value_str != i->default_value)
							s.value = value_str;
					}
					if (s.type || !s.value.empty())
						n.input_datas.emplace(i->name_hash, s);
				}
			}
			n.position = src_n->position;
			for (auto c : src_n->children)
				add_node_recursively(c);
		};
		for (auto n : nodes)
			add_node_recursively(n);

		std::vector<BlueprintLinkPtr> relevant_links;
		for (auto& src_l : g->links)
		{
			if (if_any_contains(nodes, src_l->from_slot->node) || if_any_contains(nodes, src_l->to_slot->node))
				relevant_links.push_back(src_l.get());
		}
		std::sort(relevant_links.begin(), relevant_links.end(), [](const auto a, const auto b) {
			return a->from_slot->node->degree < b->from_slot->node->degree;
		});
		for (auto src_l : relevant_links)
		{
			auto& l = copied_links.emplace_back();
			l.from_node = src_l->from_slot->node->object_id;
			l.from_slot = src_l->from_slot->name_hash;
			l.to_node = src_l->to_slot->node->object_id; 
			l.to_slot = src_l->to_slot->name_hash;
		}
	}

	app.last_status = std::format("Copied: {} nodes, {} links", (int)copied_nodes.size(), (int)copied_links.size());
}

void BlueprintView::paste_nodes(BlueprintGroupPtr g, const vec2& pos)
{
	if (copied_nodes.empty())
		return;

	auto paste_nodes_count = 0;
	auto paste_links_count = 0;

	std::map<uint, BlueprintNodePtr> node_map; // the original id to the new node, use for linking
	auto base_pos = vec2(+10000.f);
	for (auto& src_n : copied_nodes)
		base_pos = min(base_pos, src_n.position);
	for (auto& src_n : copied_nodes)
	{
		BlueprintNodePtr n = nullptr;
		auto parent = g->find_node_by_id(last_block);
		if (auto it = node_map.find(src_n.parent); it != node_map.end())
			parent = it->second;
		if (blueprint_is_variable_node(src_n.name))
		{
			uint name = 0;
			uint location = 0;
			if (auto it = src_n.input_datas.find("Name"_h); it != src_n.input_datas.end())
				name = s2t<uint>(it->second.value);
			if (auto it = src_n.input_datas.find("Location"_h); it != src_n.input_datas.end())
				location = s2t<uint>(it->second.value);
			n = blueprint->add_variable_node(g, parent, name, src_n.name, location);
		}
		else if (src_n.name == "Call"_h)
		{
			uint name = 0;
			uint location = 0;
			if (auto it = src_n.input_datas.find("Name"_h); it != src_n.input_datas.end())
				name = s2t<uint>(it->second.value);
			if (auto it = src_n.input_datas.find("Location"_h); it != src_n.input_datas.end())
				location = s2t<uint>(it->second.value);
			n = blueprint->add_call_node(g, parent, name, location);
		}
		else if (src_n.name == "Block"_h)
		{
			n = blueprint->add_block(g, parent);
		}
		else
		{
			for (auto library : blueprint_window.node_libraries)
			{
				for (auto& t : library->node_templates)
				{
					if (t.name_hash == src_n.name)
					{
						n = t.create_node(blueprint, g, parent);
						break;
					}
				}
				if (n)
					break;
			}
		}
		if (n)
		{
			if (!src_n.template_string.empty())
				blueprint->change_node_structure(n, src_n.template_string, {});

			for (auto& src_i : src_n.input_datas)
			{
				if (blueprint_is_variable_node(n->name_hash))
				{
					if (src_i.first == "Name"_h || src_i.first == "Location"_h)
						continue;
				}
				auto i = n->find_input(src_i.first);
				if (src_i.second.type && i->type != src_i.second.type)
					set_input_type(i, src_i.second.type);
				i->type->unserialize(src_i.second.value, i->data);
			}

			n->position = pos + src_n.position - base_pos;
			ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

			node_map[src_n.object_id] = n;
			paste_nodes_count++;
		}
	}
	for (auto& src_l : copied_links)
	{
		BlueprintNodePtr from_node = nullptr;
		BlueprintNodePtr to_node = nullptr;
		if (auto it = node_map.find(src_l.from_node); it != node_map.end())
			from_node = it->second;
		else
		{
			if (copy_src_group == g)
			{
				if (auto n = g->find_node_by_id(src_l.from_node); n)
					from_node = n;
			}
		}
		if (auto it = node_map.find(src_l.to_node); it != node_map.end())
			to_node = it->second;
		if (from_node && to_node)
		{
			blueprint->add_link(from_node->find_output(src_l.from_slot), to_node->find_input(src_l.to_slot));
			paste_links_count++;
		}
	}

	if (paste_nodes_count || paste_links_count)
		unsaved = true;
	app.last_status = std::format("Pasted: {} nodes, {} links", paste_nodes_count, paste_links_count);
}

void BlueprintView::set_parent_to_hovered_node()
{
	auto n = (BlueprintNodePtr)(uint64)ax::NodeEditor::GetHoveredNode();
	if (n)
	{
		if (!n->is_block)
			n = n->parent;
	}

	auto nodes = get_selected_nodes();
	if (nodes.empty())
		return;

	auto block = n ? n : nodes.front()->group->nodes.front().get();
	blueprint->set_nodes_parent(nodes, block);
	last_block = block->object_id;
}

void BlueprintView::navigate_to_node(BlueprintNodePtr n)
{
	if (n->group->name_hash != group_name_hash)
	{
		group_name = n->group->name;
		group_name_hash = n->group->name_hash;
		add_event([this, n]() { 
			navigate_to_node(n);
			return false;
		});
		return;
	}

	auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)n);
	if (ax_node)
		ax_editor->NavigateTo(ax_node->GetBounds(), true, 0.f);
}

static void expand_polygon(std::vector<vec2>& polygon, float r)
{
	vec2 mid(0.f);
	for (auto& v : polygon)
		mid += v;
	mid /= polygon.size();
	for (auto& v : polygon)
		v += normalize(v - mid) * r;
}

namespace GrahamScan
{
	// Returns true if a is lexicographically before b.
	bool isLeftOf(const vec2& a, const vec2& b) {
		return (a.x < b.x || (a.x == b.x && a.y < b.y));
	}

	// The z-value of the cross product of segments 
	// (a, b) and (a, c). Positive means c is ccw
	// from (a, b), negative cw. Zero means its collinear.
	float ccw(const vec2& a, const vec2& b, const vec2& c) {
		return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	}

	// Used to sort points in ccw order about a pivot.
	struct ccwSorter {
		const vec2& pivot;

		ccwSorter(const vec2& inPivot) : pivot(inPivot) { }

		bool operator()(const vec2& a, const vec2& b) {
			return ccw(pivot, a, b) < 0;
		}
	};

	static std::vector<vec2> compute(std::vector<vec2>& v)
	{
		if (v.size() < 3)
			return std::vector<vec2>();

		// Put our leftmost point at index 0
		std::swap(v[0], *min_element(v.begin(), v.end(), isLeftOf));

		// Sort the rest of the points in counter-clockwise order
		// from our leftmost point.
		sort(v.begin() + 1, v.end(), ccwSorter(v[0]));

		// Add our first three points to the hull.
		std::vector<vec2> hull;
		auto it = v.begin();
		hull.push_back(*it++);
		hull.push_back(*it++);
		hull.push_back(*it++);

		while (it != v.end()) {
			// Pop off any points that make a convex angle with *it
			while (hull.size() > 1 && ccw(*(hull.rbegin() + 1), *(hull.rbegin()), *it) >= 0)
				hull.pop_back();
			hull.push_back(*it++);
		}

		return hull;
	}
}

void BlueprintView::build_node_block_verts(BlueprintNodePtr n)
{
	if (!n->is_block)
		return;
	auto& verts = block_verts[n->object_id];
	verts.clear();
	for (auto c : n->children)
		build_node_block_verts(c);
	if (n != n->group->nodes.front().get())
	{
		std::vector<vec2> points;
		if (auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)n); ax_node)
		{
			auto im_rect = ax_node->GetBounds();
			auto rect = Rect(im_rect.Min, im_rect.Max);
			if (rect.a == rect.b)
				points.push_back(rect.a);
			else
			{
				auto pts = rect.get_points();
				pts[0] = pts[1] - vec2(0.f, 8.f);
				pts[3] = pts[2] - vec2(0.f, 8.f);
				pts[1].y += 8.f;
				pts[2].y += 8.f;
				points.insert(points.end(), pts.begin(), pts.end());
			}
		}
		for (auto c : n->children)
		{
			if (auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)c); ax_node)
			{
				auto im_rect = ax_node->GetBounds();
				auto rect = Rect(im_rect.Min, im_rect.Max);
				if (rect.a == rect.b)
					points.push_back(rect.a);
				else
				{
					auto pts = rect.get_points();
					points.insert(points.end(), pts.begin(), pts.end());
				}
			}
			if (auto it = block_verts.find(c->object_id); it != block_verts.end())
				points.insert(points.end(), it->second.begin(), it->second.end());
		}
		verts = GrahamScan::compute(points);
		expand_polygon(verts, 8.f);
	}
}

void BlueprintView::build_all_block_verts(BlueprintGroupPtr g)
{
	block_verts.clear();
	build_node_block_verts(g->nodes.front().get());
}

void BlueprintView::draw_block_verts(ImDrawList* dl, BlueprintNodePtr n)
{
	if (auto it = block_verts.find(n->object_id); it != block_verts.end())
	{
		if (!it->second.empty())
			dl->AddConvexPolyFilled((ImVec2*)it->second.data(), it->second.size(), color_from_depth(n->depth + 1));
		for (auto c : n->children)
			draw_block_verts(dl, c);
	}
}

static BlueprintInstanceNode* step(BlueprintInstanceGroup* debugging_group)
{
	blueprint_window.debugger->debugging = nullptr;

	BlueprintNodePtr breakpoint = nullptr;
	BlueprintBreakpointOption breakpoint_option;
	if (auto n = debugging_group->executing_node(); n)
	{
		if (blueprint_window.debugger->has_break_node(n->original, &breakpoint_option))
		{
			breakpoint = n->original;
			blueprint_window.debugger->remove_break_node(breakpoint);
		}
	}
	auto next_node = debugging_group->instance->step(debugging_group);
	if (breakpoint)
		blueprint_window.debugger->add_break_node(breakpoint, breakpoint_option);
	return next_node;
};

void BlueprintView::run_blueprint(BlueprintInstanceGroup* debugging_group)
{
	if (!debugging_group)
	{
		auto g = blueprint_instance->find_group(group_name_hash);
		blueprint_instance->prepare_executing(g);
		blueprint_instance->run(g);
	}
	else
	{
		step(debugging_group);
		debugging_group->instance->run(debugging_group);
	}
}

void BlueprintView::step_blueprint(BlueprintInstanceGroup* debugging_group)
{
	if (!debugging_group)
	{
		auto g = blueprint_instance->find_group(group_name_hash);
		blueprint_instance->prepare_executing(g);
		blueprint_window.debugger->debugging = g;
	}
	else
	{
		auto next_node = step(debugging_group);
		if (!debugging_group->executing_stack.empty())
			blueprint_window.debugger->debugging = debugging_group;

		if (next_node)
		{
			auto ax_node = ax_editor->GetNode((ax::NodeEditor::NodeId)next_node->original);
			ax_editor->NavigateTo(ax_node->GetBounds(), false, 0.f);
		}
	}
}

void BlueprintView::stop_blueprint(BlueprintInstanceGroup* debugging_group)
{
	if (debugging_group)
	{
		blueprint_window.debugger->debugging = nullptr;
		debugging_group->instance->stop(debugging_group);
	}
}

void BlueprintView::save_blueprint()
{
	if (!blueprint)
		return;
	if (!unsaved)
		return;

	blueprint->save();
	unsaved = false;
}

std::string BlueprintView::get_save_name()
{
	auto sp = SUS::split(name, '#');
	if (sp.size() == 2)
		return std::string(sp[0]) + '#' + group_name + "##" + "Blueprint";
	return name;
}

void BlueprintView::on_draw()
{
	bool opened = true;
	ImGui::SetNextWindowSize(vec2(400, 400), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(name.c_str(), &opened, unsaved ? ImGuiWindowFlags_UnsavedDocument : 0))
	{
		ImGui::End();
		return;
	}
	imgui_window = ImGui::GetCurrentWindow();

	ax::NodeEditor::SetCurrentEditor((ax::NodeEditor::EditorContext*)ax_editor);

	auto frame = frames;
	if (!blueprint)
	{
		blueprint = Blueprint::get(blueprint_path);
		if (blueprint)
		{
			blueprint_instance = BlueprintInstance::create(blueprint);
			load_frame = frame;
		}
	}
	if (blueprint)
	{
		if (blueprint_instance->built_frame < blueprint->dirty_frame)
			blueprint_instance->build();

		ImGui::Checkbox("Show Misc", &show_misc);
		ImGui::SameLine();
		if (ImGui::Button("Save"))
			save_blueprint();
		ImGui::SameLine();
		if (ImGui::Button("Zoom To Content"))
		{
			if (get_selected_nodes().empty() && get_selected_links().empty())
				ax::NodeEditor::NavigateToContent(0.f);
			else
				ax::NodeEditor::NavigateToSelection(true, 0.f);
		}
		ImGui::SameLine();
		if (ImGui::ToolButton("Hide Variable Links", hide_var_links))
			hide_var_links = !hide_var_links;

		auto group = blueprint->find_group(group_name_hash);
		if (!group)
		{
			group = blueprint->groups[0].get();
			group_name = group->name;
			group_name_hash = group->name_hash;
		}

		ImGui::SameLine();
		if (ImGui::BeginCombo("##group_dropdown", "", ImGuiComboFlags_NoPreview))
		{
			for (auto& g : blueprint->groups)
			{
				if (ImGui::Selectable(g->name.c_str()))
				{
					group = g.get();
					group_name = group->name;
					group_name_hash = group->name_hash;
					ax_editor->ClearSelection();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.f);
		ImGui::InputText("##group", &group_name);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			blueprint->alter_group(group_name_hash, group_name);
			group_name_hash = group->name_hash;
			ax_editor->ClearSelection();
			unsaved = true;

			if (blueprint_instance->built_frame < blueprint->dirty_frame)
				blueprint_instance->build();
		}
		ImGui::SameLine();

		if (ImGui::Button(graphics::font_icon_str("xmark"_h).c_str()))
		{
			blueprint->remove_group(group);
			if (!blueprint->groups.empty())
			{
				group = blueprint->groups.back().get();
				group_name = group->name;
				group_name_hash = group->name_hash;
			}
			else
			{
				group_name = "";
				group_name_hash = 0;
			}
			ax_editor->ClearSelection();
			unsaved = true;

			if (blueprint_instance->built_frame < blueprint->dirty_frame)
				blueprint_instance->build();

		}
		ImGui::SameLine();
		if (ImGui::Button("New Group"))
		{
			auto name = get_unique_name("new_group", [&](const std::string& name) {
				for (auto& g : blueprint->groups)
				{
					if (g->name == name)
						return true;
				}
				return false;
			});
			group = blueprint->add_group(name);
			group_name = group->name;
			group_name_hash = group->name_hash;
			ax_editor->ClearSelection();
			unsaved = true;

			if (blueprint_instance->built_frame < blueprint->dirty_frame)
				blueprint_instance->build();
		}

		ImGui::SameLine(0.f, 50);
		ImGui::SetNextItemWidth(100.f);
		if (ImGui::InputText("Trigger Message", &group->trigger_message))
		{
			group->structure_changed_frame = frame;
			blueprint->dirty_frame = frame;
			unsaved = true;

			if (blueprint_instance->built_frame < blueprint->dirty_frame)
				blueprint_instance->build();
		}

		auto debugging_group = blueprint_window.debugger->debugging &&
			blueprint_window.debugger->debugging->instance->blueprint == blueprint &&
			blueprint_window.debugger->debugging->name == group_name_hash ?
			blueprint_window.debugger->debugging : nullptr;
		auto& instance_group = debugging_group ? *debugging_group : blueprint_instance->groups[group_name_hash];

		if (ImGui::BeginTable("bp_editor", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::BeginChild("side_panel", ImVec2(0, -2));

			auto manipulate_value = [](TypeInfo* type, void* data, uint name = 0, uint* popup_name = nullptr) {
				auto changed = false;
				switch (type->tag)
				{
				case TagE:
				{
					auto ti = (TypeInfo_Enum*)type;
					auto ei = ti->ei;
					if (!ei->is_flags)
					{
						auto value = *(int*)data;
						auto curr_item = ei->find_item_by_value(value);

						ImGui::SetNextItemWidth(100.f);
						if (popup_name)
						{
							if (ImGui::Button(curr_item ? curr_item->name.c_str() : "-"))
								*popup_name = name;
						}
						else
						{
							if (ImGui::BeginCombo("", curr_item ? curr_item->name.c_str() : "-"))
							{
								for (auto& ii : ei->items)
								{
									if (ImGui::Selectable(ii.name.c_str()))
									{
										if (value != ii.value)
										{
											value = ii.value;
											changed = true;
										}
									}
								}
								ImGui::EndCombo();
							}
							if (changed)
								*(int*)data = value;
						}
					}
				}
					break;
				case TagD:
				{
					auto ti = (TypeInfo_Data*)type;
					switch (ti->data_type)
					{
					case DataBool:
						changed |= ImGui::Checkbox("", (bool*)data);
						break;
					case DataFloat:
						for (int i = 0; i < ti->vec_size; i++)
						{
							ImGui::PushID(i);
							auto& v = ((float*)data)[i];
							ImGui::SetNextItemWidth(min(60.f, ImGui::CalcTextSize(str(v).c_str()).x + 6.f));
							ImGui::DragScalar("", ImGuiDataType_Float, &v, 0.01f);
							changed |= ImGui::IsItemDeactivatedAfterEdit();
							ImGui::PopID();
						}
						break;
					case DataInt:
						for (int i = 0; i < ti->vec_size; i++)
						{
							ImGui::PushID(i);
							auto& v = ((int*)data)[i];
							ImGui::SetNextItemWidth(min(60.f, ImGui::CalcTextSize(str(v).c_str()).x + 6.f));
							ImGui::DragScalar("", ImGuiDataType_S32, &v);
							changed |= ImGui::IsItemDeactivatedAfterEdit();
							ImGui::PopID();
						}
						break;
					case DataChar:
						if (ti->vec_size == 4)
						{
							vec4 color = *(cvec4*)data;
							color /= 255.f;
							ImGui::SetNextItemWidth(160.f);
							changed |= ImGui::ColorEdit4("", &color[0]);
							if (changed)
								*(cvec4*)data = color * 255.f;
						}
						break;
					case DataString:
					{
						auto& s = *(std::string*)data;
						ImGui::SetNextItemWidth(min(100.f, ImGui::CalcTextSize(s.c_str()).x + 6.f));
						ImGui::InputText("", &s);
						changed |= ImGui::IsItemDeactivatedAfterEdit();
					}
						break;
					case DataWString:
					{
						auto s = w2s(*(std::wstring*)data);
						ImGui::SetNextItemWidth(min(100.f, ImGui::CalcTextSize(s.c_str()).x + 6.f));
						ImGui::InputText("", &s);
						changed |= ImGui::IsItemDeactivatedAfterEdit();
						if (changed)
							*(std::wstring*)data = s2w(s);
					}
						break;
					case DataPath:
					{
						auto& path = *(std::filesystem::path*)data;
						auto s = path.string();
						ImGui::SetNextItemWidth(100.f);
						ImGui::InputText("", s.data(), ImGuiInputTextFlags_ReadOnly);
						if (ImGui::BeginDragDropTarget())
						{
							if (auto payload = ImGui::AcceptDragDropPayload("File"); payload)
							{
								path = Path::reverse(std::wstring((wchar_t*)payload->Data));
								changed = true;
							}
							ImGui::EndDragDropTarget();
						}
						ImGui::SameLine();
						if (ImGui::Button(graphics::font_icon_str("location-crosshairs"_h).c_str()))
							project_window.ping(Path::get(path));
						ImGui::SameLine();
						if (ImGui::Button(graphics::font_icon_str("xmark"_h).c_str()))
						{
							path = L"";
							changed = true;
						}
					}
						break;
					}
				}
					break;
				}
				return changed;
			};

			if (ImGui::CollapsingHeader("Bp Enums:"))
			{
				ImGui::PushID("bp_enums");
				static int selected_enum = -1;
				if (ImGui::BeginListBox("##enums"))
				{
					for (auto i = 0; i < blueprint->enums.size(); i++)
					{
						if (ImGui::Selectable(blueprint->enums[i].name.c_str(), selected_enum == i))
							selected_enum = i;
					}
					ImGui::EndListBox();
				}
				selected_enum = min(selected_enum, (int)blueprint->enums.size() - 1);

				if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
				{
					auto name = get_unique_name("new_enum", [&](const std::string& name) {
						for (auto& v : blueprint->enums)
						{
							if (v.name == name)
								return true;
						}
						return false;
					});
					blueprint->add_enum(name, {});
					selected_enum = blueprint->enums.size() - 1;
					unsaved = true;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
				{
					if (selected_enum != -1)
					{
						blueprint->remove_enum(blueprint->enums[selected_enum].name_hash);
						if (blueprint->enums.empty() || selected_enum >= blueprint->enums.size())
							selected_enum = -1;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
				{
					if (selected_enum > 0)
					{
						std::swap(blueprint->enums[selected_enum], blueprint->enums[selected_enum - 1]);
						selected_enum--;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
				{
					if (selected_enum != -1 && selected_enum < blueprint->enums.size() - 1)
					{
						std::swap(blueprint->enums[selected_enum], blueprint->enums[selected_enum + 1]);
						selected_enum++;
						unsaved = true;
					}
				}

				if (selected_enum != -1)
				{
					auto& e = blueprint->enums[selected_enum];
					auto items = e.items;

					auto name = e.name;
					auto old_name_hash = e.name_hash;
					ImGui::InputText("Enum Name", &name);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						blueprint->alter_enum(e.name_hash, name, e.items);
						if (blueprint->is_static)
							app.change_bp_references(0, old_name_hash, 0, 0, sh(name.c_str()), 0);
						unsaved = true;
					}

					ImGui::PushID("bp_enum_items");

					static int selected_enum_item = -1;
					if (ImGui::BeginListBox("##enum_items"))
					{
						for (auto i = 0; i < e.items.size(); i++)
						{
							if (ImGui::Selectable(e.items[i].name.c_str(), selected_enum_item == i))
								selected_enum_item = i;
						}
						ImGui::EndListBox();
					}
					selected_enum_item = min(selected_enum_item, (int)e.items.size() - 1);

					if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
					{
						auto name = get_unique_name("new_item", [&](const std::string& name) {
							for (auto& v : e.items)
							{
								if (v.name == name)
									return true;
							}
							return false;
						});
						items.push_back({ name, sh(name.c_str()), 0});
						blueprint->alter_enum(e.name_hash, e.name, items);
						selected_enum_item = items.size() - 1;
						unsaved = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
					{
						if (selected_enum_item != -1)
						{
							auto old_item_hash = items[selected_enum_item].name_hash;
							items.erase(items.begin() + selected_enum_item);
							blueprint->alter_enum(e.name_hash, e.name, items);
							if (blueprint->is_static)
								app.change_bp_references(old_item_hash, e.name_hash, 0, 0, e.name_hash, 0);
							if (items.empty() || selected_enum_item >= items.size())
								selected_enum_item = -1;
							unsaved = true;
						}
					}
					ImGui::SameLine();
					if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
					{
						if (selected_enum_item > 0)
						{
							std::swap(items[selected_enum_item], items[selected_enum_item - 1]);
							e.items = items;
							selected_enum_item--;
							unsaved = true;
						}
					}
					ImGui::SameLine();
					if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
					{
						if (selected_enum_item != -1 && selected_enum_item < items.size() - 1)
						{
							std::swap(items[selected_enum_item], items[selected_enum_item + 1]);
							e.items = items;
							selected_enum_item++;
							unsaved = true;
						}
					}

					if (selected_enum_item != -1)
					{
						auto& item = items[selected_enum_item];
						auto old_item_hash = item.name_hash;

						auto name = item.name;
						ImGui::InputText("Item Name", &name);
						if (ImGui::IsItemDeactivatedAfterEdit())
						{
							item.name = name;
							blueprint->alter_enum(e.name_hash, e.name, items);
							if (blueprint->is_static)
								app.change_bp_references(old_item_hash, e.name_hash, 0, sh(name.c_str()), e.name_hash, 0);
							unsaved = true;
						}
						ImGui::InputInt("Item Value", &item.value);
						if (ImGui::IsItemDeactivatedAfterEdit())
						{
							blueprint->alter_enum(e.name_hash, e.name, items);
							if (blueprint->is_static)
								app.change_bp_references(old_item_hash, e.name_hash, 0, sh(name.c_str()), e.name_hash, 0);
							unsaved = true;
						}
					}

					ImGui::PopID();
				}

				ImGui::PopID();

				if (blueprint_instance->built_frame < blueprint->dirty_frame)
					blueprint_instance->build();
			}
			if (ImGui::CollapsingHeader("Bp Structs:"))
			{
				ImGui::PushID("bp_structs");
				static int selected_struct = -1;
				if (ImGui::BeginListBox("##structs"))
				{
					for (auto i = 0; i < blueprint->structs.size(); i++)
					{
						if (ImGui::Selectable(blueprint->structs[i].name.c_str(), selected_struct == i))
							selected_struct = i;
					}
					ImGui::EndListBox();
				}
				selected_struct = min(selected_struct, (int)blueprint->structs.size() - 1);

				if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
				{
					auto name = get_unique_name("new_struct", [&](const std::string& name) {
						for (auto& v : blueprint->structs)
						{
							if (v.name == name)
								return true;
						}
						return false;
					});
					blueprint->add_struct(name, {});
					selected_struct = blueprint->structs.size() - 1;
					unsaved = true;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
				{
					if (selected_struct != -1)
					{
						blueprint->remove_struct(blueprint->structs[selected_struct].name_hash);
						if (blueprint->structs.empty() || selected_struct >= blueprint->structs.size())
							selected_struct = -1;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
				{
					if (selected_struct > 0)
					{
						std::swap(blueprint->structs[selected_struct], blueprint->structs[selected_struct - 1]);
						selected_struct--;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
				{
					if (selected_struct != -1 && selected_struct < blueprint->structs.size() - 1)
					{
						std::swap(blueprint->structs[selected_struct], blueprint->structs[selected_struct + 1]);
						selected_struct++;
						unsaved = true;
					}
				}

				if (selected_struct != -1)
				{
					auto& s = blueprint->structs[selected_struct];
					auto variables = s.variables;

					auto name = s.name;
					auto old_name_hash = s.name_hash;
					ImGui::InputText("Struct Name", &name);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						blueprint->alter_struct(s.name_hash, name, s.variables);
						if (blueprint->is_static)
							app.change_bp_references(0, old_name_hash, 0, 0, sh(name.c_str()), 0);
						unsaved = true;
					}

					ImGui::PushID("bp_struct_variables");

					static int selected_struct_variable = -1;
					if (ImGui::BeginListBox("##struct_variables"))
					{
						for (auto i = 0; i < s.variables.size(); i++)
						{
							if (ImGui::Selectable(s.variables[i].name.c_str(), selected_struct_variable == i))
								selected_struct_variable = i;
						}
						ImGui::EndListBox();
					}
					selected_struct_variable = min(selected_struct_variable, (int)s.variables.size() - 1);

					if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
					{
						auto name = get_unique_name("new_variable", [&](const std::string& name) {
							for (auto& v : s.variables)
							{
								if (v.name == name)
									return true;
							}
							return false;
						});
						variables.push_back({ name, sh(name.c_str()), TypeInfo::get<float>() });
						blueprint->alter_struct(s.name_hash, s.name, variables);
						selected_struct_variable = variables.size() - 1;
						unsaved = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
					{
						if (selected_struct_variable != -1)
						{
							auto old_item_hash = variables[selected_struct_variable].name_hash;
							variables.erase(variables.begin() + selected_struct_variable);
							blueprint->alter_struct(s.name_hash, s.name, variables);
							if (blueprint->is_static)
								app.change_bp_references(old_item_hash, s.name_hash, 0, 0, s.name_hash, 0);
							if (variables.empty() || selected_struct_variable >= variables.size())
								selected_struct_variable = -1;
							unsaved = true;
						}
					}
					ImGui::SameLine();
					if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
					{
						if (selected_struct_variable > 0)
						{
							std::swap(variables[selected_struct_variable], variables[selected_struct_variable - 1]);
							s.variables = variables;
							selected_struct_variable--;
							unsaved = true;
						}
					}
					ImGui::SameLine();
					if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
					{
						if (selected_struct_variable != -1 && selected_struct_variable < variables.size() - 1)
						{
							std::swap(variables[selected_struct_variable], variables[selected_struct_variable + 1]);
							s.variables = variables;
							selected_struct_variable++;
							unsaved = true;
						}
					}

					if (selected_struct_variable != -1)
					{
						auto& variable = variables[selected_struct_variable];
						auto old_variable_hash = variable.name_hash;

						auto name = variable.name;
						ImGui::InputText("Variable Name", &name);
						if (ImGui::IsItemDeactivatedAfterEdit())
						{
							variable.name = name;
							blueprint->alter_struct(s.name_hash, s.name, variables);
							if (blueprint->is_static)
								app.change_bp_references(old_variable_hash, s.name_hash, 0, sh(name.c_str()), s.name_hash, 0);
							unsaved = true;
						}
						if (ImGui::BeginCombo("Variable Type", ti_str(variable.type).c_str()))
						{
							if (auto type = show_types_menu(); type)
							{
								variable.type = type;
								blueprint->alter_struct(s.name_hash, s.name, variables);
								if (blueprint->is_static)
									app.change_bp_references(old_variable_hash, s.name_hash, 0, sh(name.c_str()), s.name_hash, 0);
								unsaved = true;
							}

							ImGui::EndCombo();
						}
					}

					ImGui::PopID();
				}

				ImGui::PopID();

				if (blueprint_instance->built_frame < blueprint->dirty_frame)
					blueprint_instance->build();
			}
			if (ImGui::CollapsingHeader("Bp Variables:"))
			{
				ImGui::PushID("bp_variables");
				static int selected_variable = -1;
				ImGui::SetNextItemWidth(150.f);
				if (ImGui::BeginListBox("##variables"))
				{
					for (auto i = 0; i < blueprint->variables.size(); i++)
					{
						if (ImGui::Selectable(blueprint->variables[i].name.c_str(), selected_variable == i))
							selected_variable = i;
					}
					ImGui::EndListBox();
				}
				selected_variable = min(selected_variable, (int)blueprint->variables.size() - 1);

				if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
				{
					auto name = get_unique_name("new_variable", [&](const std::string& name) {
						for (auto& v : blueprint->variables)
						{
							if (v.name == name)
								return true;
						}
						return false;
					});
					blueprint->add_variable(nullptr, name, TypeInfo::get<float>());
					selected_variable = blueprint->variables.size() - 1;
					unsaved = true;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
				{
					if (selected_variable != -1)
					{
						blueprint->remove_variable(nullptr, blueprint->variables[selected_variable].name_hash);
						if (blueprint->variables.empty() || selected_variable >= blueprint->variables.size())
							selected_variable = -1;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
				{
					if (selected_variable > 0)
					{
						std::swap(blueprint->variables[selected_variable], blueprint->variables[selected_variable - 1]);
						selected_variable--;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
				{
					if (selected_variable != -1 && selected_variable < blueprint->variables.size() - 1)
					{
						std::swap(blueprint->variables[selected_variable], blueprint->variables[selected_variable + 1]);
						selected_variable++;
						unsaved = true;
					}
				}
				if (selected_variable != -1)
				{
					auto& var = blueprint->variables[selected_variable];

					auto name = var.name;
					auto old_name_hash = var.name_hash;
					ImGui::InputText("Name", &name);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						blueprint->alter_variable(nullptr, var.name_hash, name, var.type);
						if (blueprint->is_static)
							app.change_bp_references(old_name_hash, blueprint->name_hash, 0, sh(name.c_str()), blueprint->name_hash, 0);
						unsaved = true;
					}
					if (ImGui::BeginCombo("Type", ti_str(var.type).c_str()))
					{
						if (auto type = show_types_menu(); type)
						{
							blueprint->alter_variable(nullptr, var.name_hash, "", type);
							if (blueprint->is_static)
								app.change_bp_references(old_name_hash, blueprint->name_hash, 0, old_name_hash, blueprint->name_hash, 0);
							unsaved = true;
						}

						ImGui::EndCombo();
					}

					ImGui::TextUnformatted("Value");
					if (debugging_group)
					{
						auto it = debugging_group->instance->variables.find(var.name_hash);
						if (it != debugging_group->instance->variables.end())
						{
							ImGui::SameLine();
							ImGui::TextUnformatted(get_value_str(it->second.type, it->second.data).c_str());
						}
					}
					else
					{
						if (manipulate_value(var.type, var.data))
							unsaved = true;
					}
				}
				ImGui::PopID();

				if (blueprint_instance->built_frame < blueprint->dirty_frame)
					blueprint_instance->build();
			}

			if (ImGui::CollapsingHeader("Group Variables:"))
			{
				ImGui::PushID("group_variables");
				static int selected_variable = -1;
				ImGui::SetNextItemWidth(150.f);
				if (ImGui::BeginListBox("##variables"))
				{
					for (auto i = 0; i < group->variables.size(); i++)
					{
						if (ImGui::Selectable(group->variables[i].name.c_str(), selected_variable == i))
							selected_variable = i;
					}
					ImGui::EndListBox();
				}
				selected_variable = min(selected_variable, (int)group->variables.size() - 1);

				if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
				{
					auto name = get_unique_name("new_variable", [&](const std::string& name) {
						for (auto& v : group->variables)
						{
							if (v.name == name)
								return true;
						}
						return false;
					});
					blueprint->add_variable(group, name, TypeInfo::get<float>());
					selected_variable = group->variables.size() - 1;
					unsaved = true;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
				{
					if (selected_variable != -1)
					{
						blueprint->remove_variable(group, group->variables[selected_variable].name_hash);
						if (group->variables.empty() || selected_variable >= (int)group->variables.size())
							selected_variable = -1;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
				{
					if (selected_variable > 0)
					{
						std::swap(group->variables[selected_variable], group->variables[selected_variable - 1]);
						selected_variable--;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
				{
					if (selected_variable != -1 && selected_variable < group->variables.size() - 1)
					{
						std::swap(group->variables[selected_variable], group->variables[selected_variable + 1]);
						selected_variable++;
						unsaved = true;
					}
				}
				if (selected_variable != -1)
				{
					auto& var = group->variables[selected_variable];

					auto name = var.name;
					auto old_name_hash = var.name_hash;
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputText("Name", &name);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						blueprint->alter_variable(group, var.name_hash, name, var.type);
						unsaved = true;
					}
					ImGui::SetNextItemWidth(200.f);
					if (ImGui::BeginCombo("Type", ti_str(var.type).c_str()))
					{
						if (auto type = show_types_menu(); type)
						{
							blueprint->alter_variable(group, var.name_hash, "", type);
							unsaved = true;
						}

						ImGui::EndCombo();
					}

					ImGui::TextUnformatted("Value");
					if (debugging_group)
					{
						auto it = debugging_group->variables.find(var.name_hash);
						if (it != debugging_group->variables.end())
						{
							ImGui::SameLine();
							ImGui::TextUnformatted(get_value_str(it->second.type, it->second.data).c_str());
						}
					}
					else
					{
						if (manipulate_value(var.type, var.data))
							unsaved = true;
					}
				}
				ImGui::PopID();

				if (blueprint_instance->built_frame < blueprint->dirty_frame)
					blueprint_instance->build();
			}

			if (ImGui::CollapsingHeader("Group Inputs:"))
			{
				ImGui::PushID("group_inputs");
				static int selected_input = -1;
				ImGui::SetNextItemWidth(150.f);
				if (ImGui::BeginListBox("##inputs"))
				{
					for (auto i = 0; i < group->inputs.size(); i++)
					{
						if (ImGui::Selectable(group->inputs[i].name.c_str(), selected_input == i))
							selected_input = i;
					}
					ImGui::EndListBox();
				}
				selected_input = min(selected_input, (int)group->inputs.size() - 1);

				if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
				{
					auto name = get_unique_name("new_input", [&](const std::string& name) {
						for (auto& i : group->inputs)
						{
							if (i.name == name)
								return true;
						}
						return false;
					});
					blueprint->add_group_input(group, name, TypeInfo::get<float>());
					selected_input = group->inputs.size() - 1;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
				{
					if (selected_input != -1)
					{
						blueprint->remove_group_input(group, group->inputs[selected_input].name_hash);
						if (group->inputs.empty() || selected_input >= (int)group->inputs.size())
							selected_input = -1;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
				{
					if (selected_input > 0)
					{
						std::swap(group->inputs[selected_input], group->inputs[selected_input - 1]);
						selected_input--;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
				{
					if (selected_input != -1 && selected_input < group->inputs.size() - 1)
					{
						std::swap(group->inputs[selected_input], group->inputs[selected_input + 1]);
						selected_input++;
						unsaved = true;
					}
				}
				if (selected_input != -1)
				{
					auto& var = group->inputs[selected_input];

					auto name = var.name;
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputText("Name", &name);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						blueprint->alter_group_input(group, var.name_hash, name, var.type);
						unsaved = true;
					}
					ImGui::SetNextItemWidth(200.f);
					if (ImGui::BeginCombo("Type", ti_str(var.type).c_str()))
					{
						if (auto type = show_types_menu(); type)
						{
							blueprint->alter_group_input(group, var.name_hash, "", type);
							unsaved = true;
						}

						ImGui::EndCombo();
					}
				}
				ImGui::PopID();

				if (blueprint_instance->built_frame < blueprint->dirty_frame)
					blueprint_instance->build();
			}
			if (ImGui::CollapsingHeader("Group Outputs:"))
			{
				ImGui::PushID("group_outputs");
				static int selected_output = -1;
				ImGui::SetNextItemWidth(150.f);
				if (ImGui::BeginListBox("##outputs"))
				{
					for (auto i = 0; i < group->outputs.size(); i++)
					{
						if (ImGui::Selectable(group->outputs[i].name.c_str(), selected_output == i))
							selected_output = i;
					}
					ImGui::EndListBox();
				}
				selected_output = min(selected_output, (int)group->outputs.size() - 1);

				if (ImGui::SmallButton(graphics::font_icon_str("plus"_h).c_str()))
				{
					auto name = get_unique_name("new_output", [&](const std::string& name) {
						for (auto& o : group->outputs)
						{
							if (o.name == name)
								return true;
						}
						return false;
					});
					blueprint->add_group_output(group, name, TypeInfo::get<float>());
					selected_output = group->outputs.size() - 1;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("minus"_h).c_str()))
				{
					if (selected_output != -1)
					{
						blueprint->remove_group_output(group, group->outputs[selected_output].name_hash);
						if (group->outputs.empty() || selected_output >= (int)group->outputs.size())
							selected_output = -1;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-up"_h).c_str()))
				{
					if (selected_output > 0)
					{
						std::swap(group->outputs[selected_output], group->outputs[selected_output - 1]);
						selected_output--;
						unsaved = true;
					}
				}
				ImGui::SameLine();
				if (ImGui::SmallButton(graphics::font_icon_str("arrow-down"_h).c_str()))
				{
					if (selected_output != -1 && selected_output < group->outputs.size() - 1)
					{
						std::swap(group->outputs[selected_output], group->outputs[selected_output + 1]);
						selected_output++;
						unsaved = true;
					}
				}
				if (selected_output != -1)
				{
					auto& var = group->outputs[selected_output];

					auto name = var.name;
					ImGui::SetNextItemWidth(200.f);
					ImGui::InputText("Name", &name);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						blueprint->alter_group_output(group, var.name_hash, name, var.type);
						unsaved = true;
					}
					ImGui::SetNextItemWidth(200.f);
					if (ImGui::BeginCombo("Type", ti_str(var.type).c_str()))
					{
						if (auto type = show_types_menu(); type)
						{
							blueprint->alter_group_output(group, var.name_hash, "", type);
							unsaved = true;
						}

						ImGui::EndCombo();
					}
				}
				ImGui::PopID();

				if (blueprint_instance->built_frame < blueprint->dirty_frame)
					blueprint_instance->build();
			}
			if (ImGui::CollapsingHeader("Selections:", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto selected_nodes = get_selected_nodes();
				if (selected_nodes.size() == 1)
				{
					auto n = selected_nodes.front();
					ImGui::Text("Node: %s", n->name.c_str());
					ImGui::Text("ID: %d", n->object_id);
					ImGui::Text("Position: %.2f, %.2f", n->position.x, n->position.y);
					if (blueprint_is_variable_node(n->name_hash) || n->name_hash == "Call"_h)
					{
						auto name = *(uint*)n->inputs[0]->data;
						auto location = *(uint*)n->inputs[1]->data;
						if (location == 0)
						{
							if (auto var = group->find_variable(name); var)
								ImGui::Text("Variable: %s", var->name.c_str());
							else
							{
								if (auto var = blueprint->find_variable(name); var)
									ImGui::Text("Variable: %s", var->name.c_str());
							}
						}
						else
						{
							auto bp = Blueprint::get(location);
							if (bp)
							{
								if (auto var = bp->find_variable(name); var)
								{
									ImGui::Text("Variable: %s", var->name.c_str());
									ImGui::Text("From Blueprint: %s", bp->name.c_str());
								}
							}
							else if (auto ei = find_enum(location); ei)
							{
								if (auto ii = ei->find_item(name); ii)
								{
									ImGui::Text("Enum Value: %s %d", ii->name.c_str(), ii->value);
									ImGui::Text("From Enum: %s", ei->name.c_str());
								}
							}
						}
					}

				}
				else if (selected_nodes.size() > 1)
					ImGui::Text("%d Nodes Selected", (int)selected_nodes.size());

				auto selected_links = get_selected_links();
				if (selected_links.size() == 1)
				{
					auto l = selected_links.front();
					ImGui::TextUnformatted("Link: ");
					ImGui::Text("ID: %d", l->object_id);
					ImGui::Text("From: '%s' of '%s'", l->from_slot->name.c_str(), l->from_slot->node->name.c_str());
					ImGui::Text("To: '%s' of '%s'", l->to_slot->name.c_str(), l->to_slot->node->name.c_str());
				}
				else if (selected_links.size() > 1)
					ImGui::Text("%d Links Selected", (int)selected_links.size());

				if (selected_nodes.empty() && selected_links.empty())
				{
					ImGui::TextUnformatted("Input ID To Select");
					static int input_id = 0;
					if (ImGui::InputInt("##id", &input_id, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						auto n = group->find_node_by_id(input_id);
						if (n)
						{
							ax::NodeEditor::SelectNode((ax::NodeEditor::NodeId)n, false);
							ax::NodeEditor::NavigateToSelection(true, 0.f);
						}
						else
							ax::NodeEditor::ClearSelection();
					}
				}

			}
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(1); 
			ImGui::BeginChild("main_area", ImVec2(0, -2));
			{
				auto update_block_verts = false;
				if (group != last_group || group->structure_changed_frame > load_frame)
				{
					for (auto& n : group->nodes)
						ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n.get(), n->position);
					update_block_verts = true;
					last_group = group;
					load_frame = frame;
				}

				if (ImGui::Button(graphics::font_icon_str("play"_h).c_str()))
					run_blueprint(debugging_group);
				ImGui::SameLine();
				if (ImGui::Button(graphics::font_icon_str("circle-play"_h).c_str()))
					step_blueprint(debugging_group);
				ImGui::SameLine();
				if (ImGui::Button(graphics::font_icon_str("stop"_h).c_str()))
					stop_blueprint(debugging_group);

				auto& io = ImGui::GetIO();
				auto& style = ImGui::GetStyle();
				auto dl = ImGui::GetWindowDrawList();
				std::string tooltip; vec2 tooltip_pos;

				ImGui::SetNextItemAllowOverlap();
				ImGui::InvisibleButton("splits", ImVec2(ImGui::GetContentRegionAvail().x, 10.f));
				{
					auto next_cursor = ImGui::GetCursorScreenPos();
					vec2 p0 = ImGui::GetItemRectMin();
					vec2 p1 = ImGui::GetItemRectMax();
					dl->AddRectFilled(p0 + vec2(0.f, 2.f), p1 - vec2(0.f, 2.f), ImColor(0.6f, 0.6f, 0.6f, 1.f));

					static vec2 mpos;
					auto bar_id = ImGui::GetItemID();
					if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
					{
						mpos = io.MousePos;
						ImGui::OpenPopupEx(bar_id);
					}

					auto changed = false;
					for (auto i = 0; i < group->splits.size(); i++)
					{
						auto x = ax::NodeEditor::CanvasToScreen(ImVec2(group->splits[i], 0.f)).x;
						if (x > p0.x + 4.f && x < p1.x - 4.f)
						{
							ImGui::PushID(i);
							auto _p0 = vec2(x - 4.f, p0.y);
							auto _p1 = vec2(x + 4.f, p1.y);
							ImGui::SetCursorScreenPos(_p0);
							ImGui::InvisibleButton("", _p1 - _p0);
							dl->AddRectFilled(_p0, _p1, ImColor(0.4f, 0.5f, 0.6f, ImGui::IsItemHovered() ? 0.8f : 1.f));

							if (ImGui::IsItemActive())
								group->splits[i] += io.MouseDelta.x / ax_editor->GetView().Scale;
							if (ImGui::IsItemDeactivated())
								changed = true;

							if (ImGui::BeginPopupContextItem())
							{
								if (ImGui::MenuItem("Remove Split"))
								{
									group->splits.erase(group->splits.begin() + i);
									i = group->splits.size() + 1;
								}
								ImGui::EndPopup();
							}

							ImGui::PopID();
						}
					}

					if (ImGui::BeginPopupEx(bar_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
					{
						if (ImGui::MenuItem("Add Split"))
						{
							group->splits.push_back(ax::NodeEditor::ScreenToCanvas(ImVec2(mpos)).x);
							changed = true;
						}
						ImGui::EndPopup();
					}

					if (changed)
					{
						std::sort(group->splits.begin(), group->splits.end());
						compute_node_orders(group);
						unsaved = true;
					}

					ImGui::SetCursorScreenPos(next_cursor);
				}

				if (debugging_group)
					ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_Bg, ImColor(100, 80, 60, 200));
				ax::NodeEditor::Begin("node_editor");

				for (auto s : group->splits)
					dl->AddLine(vec2(s, -10000.f), vec2(s, +10000.f), ImColor(0.4f, 0.5f, 0.6f, 1.f));
				draw_block_verts(dl, group->nodes.front().get());

				auto executing_node = debugging_group ? debugging_group->executing_node() : nullptr;

				static uint combo_popup_name = 0;
				static BlueprintNodePtr combo_popup_node = nullptr;
				for (auto& nn : group->nodes)
				{
					auto n = nn.get();
					if (n == group->nodes.front().get()) // skip root block
						continue;
					if (n->flags & BlueprintNodeFlagWidget)
						continue;
					if (hide_var_links)
					{
						if (n->name_hash == "Variable"_h)
							continue;
					}

					ImGui::PushID(n->object_id);

					auto instance_node = instance_group.node_map[n->object_id];

					auto bg_color = ax::NodeEditor::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBg];
					auto border_color = color_from_depth(n->depth);
					if (blueprint_window.debugger->has_break_node(n))
					{
						if (executing_node && executing_node->original == n)
							bg_color = ImColor(204, 116, 45, 200);
						else
							bg_color = ImColor(197, 81, 89, 200);
					}
					else if (executing_node && executing_node->original == n)
						bg_color = ImColor(211, 151, 0, 200);
					ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, bg_color);
					ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBorder, border_color);

					ax::NodeEditor::BeginNode((uint64)n);

					if (executing_node && executing_node->original == n)
					{
						vec2 pos = ImGui::GetCursorPos();
						pos.x -= 20.f;
						pos.y -= ImGui::GetTextLineHeight();
						pos.y -= style.FramePadding.y * 2;
						dl->AddText(pos, ImColor(1.f, 1.f, 1.f), "=>");
					}
					if (show_misc)
					{
						vec2 pos = ImGui::GetCursorPos();
						pos.y -= ImGui::GetTextLineHeight();
						pos.y -= style.FramePadding.y * 2;
						auto text = std::format("{}", instance_node->order);
						dl->AddText(pos, ImColor(1.f, 1.f, 1.f), text.c_str());
					}

					if (n->name_hash == "Block"_h)
					{
						ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.f);
						ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.f, +1.f));
						ax::NodeEditor::BeginPin((uint64)n->inputs.front().get(), ax::NodeEditor::PinKind::Input);
						ImGui::InvisibleButton("", ImVec2(4.f, 14.f));
						ax::NodeEditor::EndPin();
						ax::NodeEditor::PopStyleVar(2);
						ImGui::SameLine(5.f);
						ImGui::TextUnformatted((n->display_name.empty() ? n->name : n->display_name).c_str());
					}
					else
					{
						ImGui::TextUnformatted((n->display_name.empty() ? n->name : n->display_name).c_str());
						if (n->flags & BlueprintNodeFlagEnableTemplate)
						{
							ImGui::SameLine();
							ImGui::SetNextItemWidth(min(100.f, ImGui::CalcTextSize(n->template_string.c_str()).x + 6.f));
							ImGui::InputText("T", &n->template_string);
							if (ImGui::IsItemDeactivatedAfterEdit())
							{
								blueprint->change_node_structure(n, n->template_string, {});
								post_process_new_node(n);
							}
						}
					}

					if (n->name_hash != "Block"_h)
					{
						ImGui::BeginGroup();
						auto n_slots = 0;
						for (auto i = 0; i < n->inputs.size(); i++)
						{
							auto input = n->inputs[i].get();
							if (input->flags & BlueprintSlotFlagHideInUI)
								continue;

							if ((n->flags & BlueprintNodeFlagHorizontalInputs) && n_slots > 0)
								ImGui::SameLine();
							ImGui::BeginGroup(); // slot

							auto n_push_styles = 0;
							if (n->flags & BlueprintNodeFlagHorizontalInputs)
							{
								ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.f);
								ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2(0.f, +1.f));
								n_push_styles += 2;
							}
							ax::NodeEditor::BeginPin((uint64)input, ax::NodeEditor::PinKind::Input);
							auto display_name = input->name;
							SUS::strip_tail_if(display_name, "_hash");
							ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)color_from_type(input->type));
							ImGui::TextUnformatted(display_name.c_str());
							ImGui::PopStyleColor();
							ax::NodeEditor::EndPin();
							if (n_push_styles)
								ax::NodeEditor::PopStyleVar(n_push_styles);

							if (ImGui::IsItemHovered())
							{
								if (debugging_group)
								{
									auto id = input->get_linked_count() > 0 ? input->get_linked(0)->object_id : input->object_id;
									if (auto it = instance_group.slot_datas.find(id); it != instance_group.slot_datas.end())
									{
										auto& arg = it->second.attribute;
										tooltip = std::format("Value: {}\nType: {})", get_value_str(arg.type, arg.data), ti_str(arg.type));
									}
								}
								else
									tooltip = std::format("({})", ti_str(input->type));
								ax::NodeEditor::Suspend();
								tooltip_pos = io.MousePos;
								ax::NodeEditor::Resume();

								if (!input->get_linked_count() > 0)
								{
									if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_F))
									{
										if (input->type->tag == TagD)
										{
											auto ti = (TypeInfo_Data*)input->type;
											auto type = TypeInfo::get<float>();
											switch (ti->vec_size)
											{
											case 2: type = TypeInfo::get<vec2>(); break;
											case 3: type = TypeInfo::get<vec3>(); break;
											case 4: type = TypeInfo::get<vec4>(); break;
											}
											if (blueprint_allow_type(input->allowed_types, type))
												set_input_type(input, type);
										}
									}
									if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_I))
									{
										if (input->type->tag == TagD)
										{
											auto ti = (TypeInfo_Data*)input->type;
											auto type = TypeInfo::get<int>();
											switch (ti->vec_size)
											{
											case 2: type = TypeInfo::get<ivec2>(); break;
											case 3: type = TypeInfo::get<ivec3>(); break;
											case 4: type = TypeInfo::get<ivec4>(); break;
											}
											if (blueprint_allow_type(input->allowed_types, type))
												set_input_type(input, type);
										}
									}
									if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_U))
									{
										if (input->type->tag == TagD)
										{
											auto ti = (TypeInfo_Data*)input->type;
											auto type = TypeInfo::get<uint>();
											switch (ti->vec_size)
											{
											case 2: type = TypeInfo::get<uvec2>(); break;
											case 3: type = TypeInfo::get<uvec3>(); break;
											case 4: type = TypeInfo::get<uvec4>(); break;
											}
											if (blueprint_allow_type(input->allowed_types, type))
												set_input_type(input, type);
										}
									}
									if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_D))
									{
										if (input->type->tag == TagD)
										{
											auto ti = (TypeInfo_Data*)input->type;
											if (ti->vec_size > 1)
											{
												if (ti->data_type == DataFloat || ti->data_type == DataInt)
												{
													auto name = std::format("Vec{}", ti->vec_size);
													auto new_node = blueprint->add_node(group, n->parent, sh(name.c_str()));
													new_node->position = n->position + vec2(-144.f, 0.f);
													blueprint->add_link(new_node->find_output("V"_h), input);
												}
											}
										}
									}
								}

								auto swap_inputs = [&](BlueprintSlotPtr slot0, BlueprintSlotPtr slot1) {
									BlueprintSlotPtr link_output0 = nullptr;
									BlueprintSlotPtr link_output1 = nullptr;
									TypeInfo* type0 = nullptr;
									TypeInfo* type1 = nullptr;
									std::string value0;
									std::string value1;

									if (slot0->get_linked_count() > 0)
										link_output0 = slot0->get_linked(0);
									else
									{
										type0 = slot0->type;
										value0 = type0->serialize(slot0->data);
									}

									if (slot1->get_linked_count() > 0)
										link_output1 = slot1->get_linked(0);
									else
									{
										type1 = slot1->type;
										value1 = type1->serialize(slot1->data);
									}

									if (link_output0)
									{
										if (auto l = group->find_link(link_output0, slot0); l)
											blueprint->remove_link(l);
										blueprint->add_link(link_output0, slot1);
									}
									else
									{
										set_input_type(slot1, type0);
										type0->unserialize(value0, slot1->data);
									}
									if (link_output1)
									{
										if (auto l = group->find_link(link_output1, slot1); l)
											blueprint->remove_link(l);
										blueprint->add_link(link_output1, slot0);
									}
									else
									{
										set_input_type(slot0, type1);
										type1->unserialize(value1, slot0->data);
									}
									};
								if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Shift) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Z))
								{
									if (i > 0)
									{
										auto prev_input = n->inputs[i - 1].get();
										if (blueprint_allow_type(input->allowed_types, prev_input->type) &&
											blueprint_allow_type(prev_input->allowed_types, input->type))
										{
											swap_inputs(input, prev_input);
											unsaved = true;
										}
									}
								}
								if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Shift) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_X))
								{
									if (i < n->inputs.size() - 1)
									{
										auto next_input = n->inputs[i + 1].get();
										if (blueprint_allow_type(input->allowed_types, next_input->type) &&
											blueprint_allow_type(next_input->allowed_types, input->type))
										{
											swap_inputs(input, next_input);
											unsaved = true;
										}
									}
								}
							}

							if (!input->get_linked_count() > 0)
							{
								if (debugging_group)
									ImGui::BeginDisabled();
								ImGui::PushID(input);
								auto no_combo_popup = combo_popup_name == 0;
								if (manipulate_value(input->type, input->data, input->name_hash, &combo_popup_name))
								{
									input->data_changed_frame = frame;
									group->data_changed_frame = frame;
									blueprint->dirty_frame = frame;
									unsaved = true;
								}
								if (combo_popup_name && no_combo_popup)
								{
									combo_popup_node = n;
									ax::NodeEditor::Suspend();
									ImGui::OpenPopup("combo"_h);
									ax::NodeEditor::Resume();
								}
								ImGui::PopID();
								if (debugging_group)
									ImGui::EndDisabled();
							}
							else
							{
								if (hide_var_links)
								{
									auto from_slot = input->get_linked(0);
									if (from_slot->node->name_hash == "Variable"_h)
									{
										ImGui::PushID(input->object_id);
										auto name = from_slot->node->display_name;
										if (name.contains('.'))
										{
											auto sp = SUS::to_string_vector(SUS::split(name, '.'));
											name = sp.back();
										}
										ImGui::SetNextItemWidth(min(100.f, ImGui::CalcTextSize(name.c_str()).x + 6.f));
										ImGui::InputText("", &name, ImGuiInputTextFlags_ReadOnly);
										if (ImGui::IsItemHovered())
											tooltip = std::format("Variable: {} ({})", from_slot->node->display_name, ti_str(from_slot->type));
										ImGui::SameLine();
										if (ImGui::SmallButton(graphics::font_icon_str("xmark"_h).c_str()))
										{
											ax::NodeEditor::ClearSelection();
											blueprint->remove_link(group->find_link(from_slot, input));
											unsaved = true;
										}
										ImGui::PopID();
									}
								}
							}

							ImGui::EndGroup(); // slot
							n_slots++;
						}
						ImGui::EndGroup();
						if (!(n->flags & BlueprintNodeFlagHorizontalInputs) && !(n->flags & BlueprintNodeFlagHorizontalOutputs))
							ImGui::SameLine(0.f, 16.f);
						ImGui::BeginGroup();
						n_slots = 0;
						for (auto i = 0; i < n->outputs.size(); i++)
						{
							auto output = n->outputs[i].get();
							if (!output->type || (output->flags & BlueprintSlotFlagHideInUI) || output->type == TypeInfo::get<BlueprintSignal>())
								continue;

							if ((n->flags & BlueprintNodeFlagHorizontalOutputs) && n_slots > 0)
								ImGui::SameLine();
							ImGui::BeginGroup(); // slot

							auto n_push_styles = 0;
							if (n->flags & BlueprintNodeFlagHorizontalOutputs)
							{
								ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.f);
								ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.f, -1.f));
								n_push_styles += 2;
							}
							ax::NodeEditor::BeginPin((uint64)output, ax::NodeEditor::PinKind::Output);
							auto display_name = output->name;
							SUS::strip_tail_if(display_name, "_hash");
							ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)color_from_type(output->type));
							ImGui::TextUnformatted(display_name.c_str());
							ImGui::PopStyleColor();
							ax::NodeEditor::EndPin();
							if (n_push_styles)
								ax::NodeEditor::PopStyleVar(n_push_styles);

							if (ImGui::IsItemHovered())
							{
								if (debugging_group)
								{
									if (auto it = instance_group.slot_datas.find(output->object_id); it != instance_group.slot_datas.end())
									{
										auto& arg = it->second.attribute;
										tooltip = std::format("Value: {}\nType: {}", get_value_str(arg.type, arg.data), ti_str(arg.type));
									}
								}
								else
									tooltip = std::format("({})", ti_str(output->type));
								ax::NodeEditor::Suspend();
								tooltip_pos = io.MousePos;
								ax::NodeEditor::Resume();

								if (!output->get_linked_count() > 0)
								{
									if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_D))
									{
										auto ti = (TypeInfo_Data*)output->type;
										if (ti->vec_size > 1)
										{
											if (ti->data_type == DataFloat || ti->data_type == DataInt)
											{
												auto new_node = blueprint->add_node(group, n->parent, "Decompose"_h);
												new_node->position = n->position + vec2(ax::NodeEditor::GetNodeSize((ax::NodeEditor::NodeId)n).x + 16.f, 0.f);
												blueprint->add_link(output, new_node->find_input("V"_h));
											}
										}
									}
								}
							}

							ImGui::EndGroup(); // slot
							n_slots++;
						}
						n_slots = 0;
						for (auto i = 0; i < n->outputs.size(); i++)
						{
							auto output = n->outputs[i].get();
							if (output->type != TypeInfo::get<BlueprintSignal>())
								continue;

							if (n_slots > 0)
								ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
							ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 0.f);
							ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2(0.f, -1.f));
							ax::NodeEditor::BeginPin((uint64)output, ax::NodeEditor::PinKind::Output);
							ImGui::InvisibleButton(output->name.c_str(), ImVec2(8.f, 8.f));
							ax::NodeEditor::EndPin();
							ax::NodeEditor::PopStyleVar(2);

							n_slots++;
						}
						ImGui::EndGroup();
					}

					if (n->preview_provider)
					{
						BpNodePreview* preview = nullptr;
						if (auto it = previews.find(n); it != previews.end())
							preview = &it->second;
						else
						{
							preview = &previews.emplace(n, BpNodePreview()).first->second;
							preview->model_previewer.init();
						}

						BlueprintNodePreview data;
						n->preview_provider(instance_node->inputs.size(), instance_node->inputs.data(), instance_node->outputs.size(), instance_node->outputs.data(), &data);
						switch (data.type)
						{
						case "image"_h:
						{
							auto image = (graphics::ImagePtr)data.data;
							if (image)
								ImGui::Image(image, vec2(256));
						}
							break;
						case "mesh"_h:
						{
							auto pmesh = (graphics::MeshPtr)data.data;
							if (preview->model_previewer.updated_frame < instance_node->updated_frame)
							{
								auto mesh = preview->model_previewer.model->get_component<cMesh>();
								if (mesh->mesh_res_id != -1)
								{
									app.renderer->release_mesh_res(mesh->mesh_res_id);
									mesh->mesh_res_id = -1;
								}
								mesh->mesh = pmesh;
								mesh->mesh_res_id = app.renderer->get_mesh_res(pmesh);
								mesh->node->mark_transform_dirty();
							}

							preview->model_previewer.update(instance_node->updated_frame);
						}
							break;
						}
					}

					auto ax_node = ax_editor->GetNodeBuilder().m_CurrentNode;
					vec2 new_pos = ax_node->m_Bounds.Min;
					if (n->position != new_pos)
					{
						if (!ImGui::IsKeyDown((ImGuiKey)Keyboard_Alt))
						{
							if (n->is_block)
							{
								auto offset = new_pos - n->position;
								set_offset_recurisely(n, offset);
							}
						}
						n->position = new_pos;
					}

					if (n->is_block)
					{
						auto col = color_from_depth(n->depth + 1);
						ImGui::InvisibleButton("block", ImVec2(20, 4));
						auto p0 = ImGui::GetItemRectMin();
						auto p1 = ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, col);
					}
					ax::NodeEditor::EndNode();

					ax::NodeEditor::PopStyleColor(2);

					ImGui::PopID(); // node
				}

				if (combo_popup_name)
				{
					if (auto input = combo_popup_node->find_input(combo_popup_name); input)
					{
						if (input->type->tag == TagE)
						{
							auto ti = (TypeInfo_Enum*)input->type;
							auto ei = ti->ei;
							if (!ei->is_flags)
							{
								auto value = *(int*)input->data;
								ax::NodeEditor::Suspend();
								if (ImGui::BeginPopupEx("combo"_h, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
								{
									for (auto& ii : ei->items)
									{
										if (ImGui::Selectable(ii.name.c_str()))
										{
											if (value != ii.value)
											{
												*(int*)input->data = ii.value;

												input->data_changed_frame = frame;
												group->data_changed_frame = frame;
												blueprint->dirty_frame = frame;
												unsaved = true;
											}
										}
									}
									ImGui::EndPopup();
								}
								else
									combo_popup_name = 0;
								ax::NodeEditor::Resume();
							}
						}
					}
				}

				for (auto& l : group->links)
				{
					if (hide_var_links)
					{
						if (l->from_slot->node->name_hash == "Variable"_h)
							continue;
					}
					ax::NodeEditor::Link((uint64)l.get(), (uint64)l->from_slot, (uint64)l->to_slot, color_from_type(l->from_slot->type));
				}

				for (auto& l : group->invalid_links)
				{

				}

				auto mouse_pos = ImGui::GetMousePos();
				static vec2				open_popup_pos;
				static BlueprintSlotPtr	new_node_link_slot = nullptr;

				if (ax::NodeEditor::BeginCreate())
				{
					if (BlueprintSlotPtr from_slot;
						ax::NodeEditor::QueryNewNode((ax::NodeEditor::PinId*)&from_slot))
					{
						if (ax::NodeEditor::AcceptNewItem())
						{
							open_popup_pos = floor((vec2)mouse_pos);
							new_node_link_slot = from_slot;
							ImGui::OpenPopup("add_node_context_menu");
						}
					}
					if (BlueprintSlotPtr from_slot, to_slot;
						ax::NodeEditor::QueryNewLink((ax::NodeEditor::PinId*)&from_slot, (ax::NodeEditor::PinId*)&to_slot))
					{
						if (from_slot && to_slot && from_slot != to_slot)
						{
							if ((from_slot->flags & BlueprintSlotFlagInput) && (to_slot->flags & BlueprintSlotFlagOutput))
								std::swap(from_slot, to_slot);
							if ((from_slot->flags & BlueprintSlotFlagInput) && (to_slot->flags & BlueprintSlotFlagInput) && from_slot->get_linked_count() == 0 &&
								blueprint_allow_type(from_slot->allowed_types, to_slot->type))
							{
								if (ax::NodeEditor::AcceptNewItem())
								{
									if (to_slot->get_linked_count() > 0)
									{
										blueprint->add_link(to_slot->get_linked(0), from_slot);
										unsaved = true;
									}
								}
							}
							else if ((from_slot->flags & BlueprintSlotFlagOutput) && (to_slot->flags & BlueprintSlotFlagInput))
							{
								if (from_slot->node->parent->contains(to_slot->node))
								{
									if (to_slot->allowed_types.size() == 1 && to_slot->allowed_types.front() != from_slot->type)
									{
										auto from_type = from_slot->type;
										auto to_type = to_slot->allowed_types.front();
										if ((from_type == TypeInfo::get<float>() &&
											(to_type == TypeInfo::get<int>() || to_type == TypeInfo::get<uint>())) ||
											((from_type == TypeInfo::get<int>() || from_type == TypeInfo::get<uint>()) &&
											(to_type == TypeInfo::get<float>())))
										{
											if (ax::NodeEditor::AcceptNewItem())
											{
												BlueprintNodeLibrary::NodeTemplate* t = nullptr;
												if (to_type == TypeInfo::get<float>())
													t = standard_library->find_node_template("To Float"_h);
												else if (to_type == TypeInfo::get<int>())
													t = standard_library->find_node_template("To Int"_h);
												else if (to_type == TypeInfo::get<uint>())
													t = standard_library->find_node_template("To Uint"_h);
												if (t)
												{
													auto convert_node = t->create_node(blueprint, group, to_slot->node->parent);
													convert_node->position = to_slot->node->position + vec2(-96.f, 0.f);
													blueprint->add_link(from_slot, convert_node->find_input("V"_h));
													blueprint->add_link(convert_node->find_output("V"_h), to_slot);
													unsaved = true;
												}
											}
										}
									}
									if (blueprint_allow_type(to_slot->allowed_types, from_slot->type))
									{
										if (ax::NodeEditor::AcceptNewItem())
										{
											blueprint->add_link(from_slot, to_slot);
											unsaved = true;
										}
									}
								}
							}
						}
					}
				}
				ax::NodeEditor::EndCreate();

				if (ax::NodeEditor::BeginDelete())
				{
					BlueprintLinkPtr link;
					BlueprintNodePtr node;
					std::vector<BlueprintLinkPtr> to_remove_links;
					std::vector<BlueprintNodePtr> to_remove_nodes;
					while (ax::NodeEditor::QueryDeletedLink((ax::NodeEditor::LinkId*)&link))
					{
						if (link->to_slot->node->name_hash != "Block"_h)
						{
							if (ax::NodeEditor::AcceptDeletedItem())
								to_remove_links.push_back(link);
						}
					}
					if (!to_remove_links.empty())
					{
						for (auto l : to_remove_links)
							blueprint->remove_link(l);
						unsaved = true;
					}
					while (ax::NodeEditor::QueryDeletedNode((ax::NodeEditor::NodeId*)&node))
					{
						if (node->name_hash == "Block"_h)
						{
							if (node->inputs.front()->get_linked_count() > 0)
								continue;
						}
						if (ax::NodeEditor::AcceptDeletedItem())
							blueprint_form_top_list(to_remove_nodes, node);
					}
					if (!to_remove_nodes.empty())
					{
						std::function<void(BlueprintNodePtr)> remove_preview;
						remove_preview = [&](BlueprintNodePtr node) {
							if (auto it = previews.find(node); it != previews.end())
								previews.erase(it);
							for (auto c : node->children)
								remove_preview(c);
						};
						for (auto n : to_remove_nodes)
						{
							remove_preview(n);
							for (auto& o : n->outputs)
							{
								if (o->type == TypeInfo::get<BlueprintSignal>())
								{
									if (o->get_linked_count() > 0)
										blueprint->remove_node(o->get_linked(0)->node, true);
								}
							}
							blueprint->remove_node(n, true);
						}
						unsaved = true;
					}
				}
				ax::NodeEditor::EndDelete();

				ax::NodeEditor::NodeId	context_node_id;
				static BlueprintNodePtr	context_node;
				static BlueprintSlotPtr	context_slot = nullptr;
				static BlueprintLinkPtr	context_link = nullptr;

				ax::NodeEditor::Suspend();
				if (ax::NodeEditor::ShowNodeContextMenu(&context_node_id))
				{
					context_node = (BlueprintNodePtr)(uint64)context_node_id;
					open_popup_pos = floor((vec2)mouse_pos);
					ImGui::OpenPopup("node_context_menu");
				}
				else if (ax::NodeEditor::ShowPinContextMenu((ax::NodeEditor::PinId*)&context_slot))
				{
					open_popup_pos = floor((vec2)mouse_pos);
					ImGui::OpenPopup("pin_context_menu");
				}
				else if (ax::NodeEditor::ShowLinkContextMenu((ax::NodeEditor::LinkId*)&context_link))
				{
					open_popup_pos = floor((vec2)mouse_pos);
					ImGui::OpenPopup("link_context_menu");
				}
				else if (ax::NodeEditor::ShowBackgroundContextMenu())
				{
					open_popup_pos = floor((vec2)mouse_pos);
					ImGui::OpenPopup("add_node_context_menu");
				}
				ax::NodeEditor::Resume();

				ax::NodeEditor::Suspend();
				if (ImGui::BeginPopup("node_context_menu"))
				{
					if (ImGui::Selectable("Copy"))
						copy_nodes(group);
					if (context_node->is_block)
					{
						if (ImGui::Selectable("Unblock"))
						{
							ax::NodeEditor::ClearSelection();
							blueprint->remove_node(context_node, false);
							compute_node_orders(group);
							context_node = nullptr;
						}
					}
					if (auto n = ax::NodeEditor::GetSelectedObjectCount(); n >= 2)
					{
						ax::NodeEditor::NodeId node_ids[2];
						if (ax::NodeEditor::GetSelectedNodes(node_ids, 2) == 2)
						{
							if (ImGui::Selectable("Set Parent To Hovered Node"))
								set_parent_to_hovered_node();
						}
					}
					BlueprintBreakpointOption breakpoint_option;
					if (!blueprint_window.debugger->has_break_node(context_node, &breakpoint_option))
					{
						if (ImGui::BeginMenu("Breakpoint"))
						{
							if (ImGui::Selectable("Set Normal"))
								blueprint_window.debugger->add_break_node(context_node);
							if (ImGui::Selectable("Set Once"))
								blueprint_window.debugger->add_break_node(context_node, BlueprintBreakpointTriggerOnce);
							if (ImGui::Selectable("Set Break In Code"))
								blueprint_window.debugger->add_break_node(context_node, BlueprintBreakpointBreakInCode);
							if (ImGui::BeginMenu("Bind To F9"))
							{
								if (ImGui::Selectable("Set Normal"))
								{
									f9_bound_breakpoint = context_node;
									f9_bound_breakpoint_option = BlueprintBreakpointNormal;
								}
								if (ImGui::Selectable("Set Once"))
								{
									f9_bound_breakpoint = context_node;
									f9_bound_breakpoint_option = BlueprintBreakpointTriggerOnce;
								}
								if (ImGui::Selectable("Set Break In Code"))
								{
									f9_bound_breakpoint = context_node;
									f9_bound_breakpoint_option = BlueprintBreakpointBreakInCode;
								}
								ImGui::EndMenu();
							}
							ImGui::EndMenu();
						}
					}
					else
					{
						if (ImGui::Selectable("Unset Breakpoint"))
							blueprint_window.debugger->remove_break_node(context_node);
					}
					ImGui::EndPopup();
				}
				if (ImGui::BeginPopup("pin_context_menu"))
				{
					if (ImGui::Selectable("Break Links"))
						;
					if (ImGui::Selectable("Reset"))
						;
					if (context_slot->flags & BlueprintSlotFlagInput)
					{
						if (ImGui::BeginMenu("Type"))
						{
							for (auto t : context_slot->allowed_types)
							{
								if (ImGui::Selectable(ti_str(t).c_str()))
									set_input_type(context_slot, t);
							}
							ImGui::EndMenu();
						}
					}
					ImGui::EndPopup();
				}
				if (ImGui::BeginPopup("link_context_menu"))
				{
					if (ImGui::Selectable("Delete"))
						;
					ImGui::EndPopup();
				}

				static std::string add_node_filter = "";
				static bool add_node_filter_match_case = false;
				static bool add_node_filter_match_whole_word = false;
				if (ImGui::BeginPopup("add_node_context_menu"))
				{
					auto new_node_block = new_node_link_slot ? new_node_link_slot->node->parent : nullptr;
					if (!new_node_block)
						new_node_block = group->find_node_by_id(last_block);

					ImGui::InputText("##Filter", &add_node_filter);
					ImGui::SameLine();
					ImGui::ToolButton("C", &add_node_filter_match_case);
					ImGui::SameLine();
					ImGui::ToolButton("W", &add_node_filter_match_whole_word);

					std::string header = "";

					auto show_node_template = [&](const std::string& name, const std::vector<BlueprintSlotDesc>& inputs, const std::vector<BlueprintSlotDesc>& outputs, uint& slot_name) {
						if (!add_node_filter.empty())
						{
							if (!filter_name(name, add_node_filter, add_node_filter_match_case, add_node_filter_match_whole_word))
								return false;
						}

						slot_name = 0;
						if (new_node_link_slot)
						{
							auto& slots = (new_node_link_slot->flags & BlueprintSlotFlagOutput) ? inputs : outputs;
							for (auto& s : slots)
							{
								if (blueprint_allow_any_type(s.allowed_types, new_node_link_slot->allowed_types))
								{
									slot_name = s.name_hash;
									break;
								}
							}
							if (!slot_name)
								return false;
						}

						if (!add_node_filter.empty() && !header.empty())
						{
							ImGui::TextDisabled(header.c_str());
							header = "";
						}

						return true;
					};
					auto show_node_library_template = [&](BlueprintNodeLibrary::NodeTemplate& t) {
						uint slot_name = 0;
						if (show_node_template(t.name, t.inputs, t.outputs, slot_name))
						{
							if (ImGui::Selectable(t.name.c_str()))
							{
								auto n = t.create_node(blueprint, group, new_node_block);
								n->position = open_popup_pos;
								ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

								if (new_node_link_slot)
								{
									if (new_node_link_slot->flags & BlueprintSlotFlagOutput)
										blueprint->add_link(new_node_link_slot, n->find_input(slot_name));
									else
										blueprint->add_link(n->find_output(slot_name), new_node_link_slot);
								}

								post_process_new_node(n);

								if (n->is_block)
									last_block = n->object_id;

								unsaved = true;
							}
						}
					};

					if (!copied_nodes.empty())
					{
						if (ImGui::MenuItem("Paste"))
							paste_nodes(group, open_popup_pos);
						if (ImGui::MenuItem("Clear Copies"))
						{
							copied_nodes.clear();
							copied_links.clear();
						}
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Variables"))
					{
						header = "Variables";

						auto show_variable = [&](const std::string& name, uint name_hash, TypeInfo* type, uint location_name) {
							uint slot_name = 0;
							std::vector<std::pair<std::string, std::function<void()>>> actions;
							if (show_node_template(name, {}, { BlueprintSlotDesc{.name = "V", .name_hash = "V"_h, .flags = BlueprintSlotFlagOutput, .allowed_types = {type}} }, slot_name))
							{
								actions.emplace_back("Get", [&, slot_name]() {
									BlueprintNodePtr n;
									if (hide_var_links)
										n = add_variable_node_unifily(group, name_hash, location_name);
									else
									{
										n = blueprint->add_variable_node(group, new_node_block, name_hash, "Variable"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);
									}

									if (new_node_link_slot)
										blueprint->add_link(n->find_output(slot_name), new_node_link_slot);

									unsaved = true;
								});
							}

							if (type->tag == TagU)
							{
								// TODO
							}

							if (is_vector(type->tag))
							{
								if (show_node_template(name, {}, { BlueprintSlotDesc{.name = "V", .name_hash = "V"_h, .flags = BlueprintSlotFlagOutput, .allowed_types = {TypeInfo::get<uint>()}} }, slot_name))
								{
									actions.emplace_back("Size", [&, slot_name]() {
										auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Size"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

										if (new_node_link_slot)
											blueprint->add_link(n->find_output(slot_name), new_node_link_slot);

										unsaved = true;
									});
								}
								if (show_node_template(name,
									{ BlueprintSlotDesc{.name = "Index", .name_hash = "Index"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {TypeInfo::get<uint>()}} },
									{ BlueprintSlotDesc{.name = "V", .name_hash = "V"_h, .flags = BlueprintSlotFlagOutput, .allowed_types = {type->get_wrapped()}} }, slot_name))
								{
									actions.emplace_back("Get Item", [&, slot_name]() {
										auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Get Item"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

										if (new_node_link_slot)
										{
											if (new_node_link_slot->flags & BlueprintSlotFlagOutput)
												blueprint->add_link(new_node_link_slot, n->find_input(slot_name));
											else
												blueprint->add_link(n->find_output(slot_name), new_node_link_slot);
										}

										unsaved = true;
									});
								}
								if (show_node_template(name,
									{ BlueprintSlotDesc{.name = "Index", .name_hash = "Index"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {TypeInfo::get<uint>()}},
									  BlueprintSlotDesc{.name = "V", .name_hash = "V"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {type->get_wrapped()}} },
									{}, slot_name))
								{
									actions.emplace_back("Set Item", [&, slot_name]() {
										auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Set Item"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

										if (new_node_link_slot)
											blueprint->add_link(new_node_link_slot, n->find_input(slot_name));

										unsaved = true;
									});
								}
								if (show_node_template(name, { BlueprintSlotDesc{.name = "V", .name_hash = "V"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {type->get_wrapped()}} }, {}, slot_name))
								{
									actions.emplace_back("Add Item", [&, slot_name]() {
										auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Add Item"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

										if (new_node_link_slot)
											blueprint->add_link(new_node_link_slot, n->find_input(slot_name));

										unsaved = true;
									});
								}
								if (type->tag == TagVU)
								{
									auto ui = type->get_wrapped()->retrive_ui();

									for (auto& vi : ui->variables)
									{
										if (show_node_template(name,
											{ BlueprintSlotDesc{.name = "Index", .name_hash = "Index"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {TypeInfo::get<uint>()}} },
											{ BlueprintSlotDesc{.name = vi.name, .name_hash = vi.name_hash, .flags = BlueprintSlotFlagOutput, .allowed_types = {vi.type}} }, slot_name))
										{
											actions.emplace_back("Get Item " + vi.name, [&, slot_name]() {
												auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Get Item Property"_h, location_name, vi.name_hash);
												n->position = open_popup_pos;
												ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

												if (new_node_link_slot)
												{
													if (new_node_link_slot->flags & BlueprintSlotFlagOutput)
														blueprint->add_link(new_node_link_slot, n->find_input(slot_name));
													else
														blueprint->add_link(n->find_output(slot_name), new_node_link_slot);
												}

												unsaved = true;
											});
										}

										if (show_node_template(name,
											{ BlueprintSlotDesc{.name = "Index", .name_hash = "Index"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {TypeInfo::get<uint>()}},
											  BlueprintSlotDesc{.name = vi.name, .name_hash = vi.name_hash, .flags = BlueprintSlotFlagInput, .allowed_types = {vi.type}} },
											{}, slot_name))
										{
											actions.emplace_back("Set Item " + vi.name, [&, slot_name]() {
												auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Set Item Property"_h, location_name, vi.name_hash);
												n->position = open_popup_pos;
												ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

												if (new_node_link_slot)
													blueprint->add_link(new_node_link_slot, n->find_input(slot_name));

												unsaved = true;
											});
										}
									}

									{
										std::vector<BlueprintSlotDesc> outputs;
										for (auto& vi : ui->variables)
											outputs.push_back(BlueprintSlotDesc{ .name = vi.name, .name_hash = vi.name_hash, .flags = BlueprintSlotFlagOutput, .allowed_types = {vi.type} });
										if (show_node_template(name, { BlueprintSlotDesc{.name = "Index", .name_hash = "Index"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {TypeInfo::get<uint>()}} }, 
											outputs, slot_name))
										{
											actions.emplace_back("Get Item Properties", [&, slot_name]() {
												auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Get Item Properties"_h, location_name);
												n->position = open_popup_pos;
												ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

												if (new_node_link_slot)
													blueprint->add_link(new_node_link_slot, n->find_input(slot_name));

												unsaved = true;
											});
										}
									}

									{
										std::vector<BlueprintSlotDesc> inputs;
										for (auto& vi : ui->variables)
											inputs.push_back(BlueprintSlotDesc{ .name = vi.name, .name_hash = vi.name_hash, .flags = BlueprintSlotFlagInput, .allowed_types = {vi.type} });
										if (show_node_template(name, inputs, {}, slot_name))
										{
											actions.emplace_back("Emplace Item", [&, slot_name]() {
												auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Emplace Item"_h, location_name);
												n->position = open_popup_pos;
												ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

												if (new_node_link_slot)
													blueprint->add_link(new_node_link_slot, n->find_input(slot_name));

												unsaved = true;
											});
										}
									}
								}
								if (show_node_template(name, {}, {}, slot_name))
								{
									actions.emplace_back("Clear", [&, slot_name]() {
										auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Array Clear"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

										unsaved = true;
									});
								}
							}
							else
							{
								if (show_node_template(name, { BlueprintSlotDesc{.name = "V", .name_hash = "V"_h, .flags = BlueprintSlotFlagInput, .allowed_types = {type}} }, {}, slot_name))
								{
									actions.emplace_back("Set", [&, slot_name]() {
										auto n = blueprint->add_variable_node(group, new_node_block, name_hash, "Set Variable"_h, location_name);
										n->position = open_popup_pos;
										ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

										if (new_node_link_slot)
											blueprint->add_link(new_node_link_slot, n->find_input(slot_name));

										unsaved = true;
									});
								}
							}
							if (!actions.empty())
							{
								if (ImGui::BeginMenu(name.c_str()))
								{
									for (auto& item : actions)
									{
										if (ImGui::Selectable(item.first.c_str()))
											item.second();
									}
									ImGui::EndMenu();
								}
							}
						};
						for (auto& v : blueprint->variables)
							show_variable(v.name, v.name_hash, v.type, 0);
						for (auto& v : group->variables)
							show_variable(v.name, v.name_hash, v.type, 0);

						if (!add_node_filter.empty() || ImGui::BeginMenu("Other Blueprints"))
						{
							for (auto bp : app.project_static_blueprints)
							{
								if (bp == blueprint)
									continue;
								if (!add_node_filter.empty() || ImGui::BeginMenu(bp->name.c_str()))
								{
									header = "Blueprint: " + bp->name;
									for (auto& v : bp->variables)
										show_variable(bp->name + '.' + v.name, v.name_hash, v.type, bp->name_hash);
									if (add_node_filter.empty())
										ImGui::EndMenu();
								}
							}
							if (add_node_filter.empty())
								ImGui::EndMenu();
						}

						if (!add_node_filter.empty() || ImGui::BeginMenu("Enums"))
						{
							for (auto& ei : tidb.enums)
							{
								if (!add_node_filter.empty() || ImGui::BeginMenu(ei.second.name.c_str()))
								{
									header = "Enum: " + ei.second.name;
									for (auto& ii : ei.second.items)
										show_variable(ei.second.name + '.' + ii.name, ii.name_hash, TypeInfo::get<int>(), ei.second.name_hash);
									if (add_node_filter.empty())
										ImGui::EndMenu();
								}
							}

							if (add_node_filter.empty())
								ImGui::EndMenu();
						}

						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Call"))
					{
						header = "Call";
						for (auto& g : blueprint->groups)
						{
							uint slot_name = 0;
							if (g.get() == group || 
								g->name_hash == "start"_h || 
								g->name_hash == "update"_h)
								continue;
							if (show_node_template(g->name, {}, {}, slot_name))
							{
								if (ImGui::Selectable(g->name.c_str()))
								{
									auto n = blueprint->add_call_node(group, new_node_block, g->name_hash);
									n->position = open_popup_pos;
									ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

									unsaved = true;
								}
							}
						}

						if (!add_node_filter.empty() || ImGui::BeginMenu("Other Blueprints"))
						{
							for (auto bp : app.project_static_blueprints)
							{
								if (bp == blueprint)
									continue;
								if (!add_node_filter.empty() || ImGui::BeginMenu(bp->name.c_str()))
								{
									header = "Call Blueprint: " + bp->name;
									for (auto& g : bp->groups)
									{
										uint slot_name = 0;
										if (g->name_hash == "start"_h ||
											g->name_hash == "update"_h)
											continue;
										if (show_node_template(bp->name + '.' + g->name, {}, {}, slot_name))
										{
											if (ImGui::Selectable(g->name.c_str()))
											{
												auto n = blueprint->add_call_node(group, new_node_block, g->name_hash, bp->name_hash);
												n->position = open_popup_pos;
												ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position);

												unsaved = true;
											}
										}
									}
									if (add_node_filter.empty())
										ImGui::EndMenu();
								}
							}
							if (add_node_filter.empty())
								ImGui::EndMenu();
						}

						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Standard"))
					{
						header = "Standard";
						for (auto& t : standard_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Extern"))
					{
						header = "Extern";
						for (auto& t : extern_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Noise"))
					{
						header = "Noise";
						for (auto& t : noise_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Texture"))
					{
						header = "Texture";
						for (auto& t : texture_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Geometry"))
					{
						header = "Geometry";
						for (auto& t : geometry_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Entity"))
					{
						header = "Entity";
						for (auto& t : entity_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Camera"))
					{
						header = "Camera";
						for (auto& t : camera_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Navigation"))
					{
						header = "Navigation";
						for (auto& t : navigation_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Colliding"))
					{
						header = "Colliding";
						for (auto& t : colliding_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Procedural"))
					{
						header = "Procedural";
						for (auto& t : procedural_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Input"))
					{
						header = "Input";
						for (auto& t : input_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("primitive"))
					{
						header = "primitive";
						for (auto& t : primitive_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("HUD"))
					{
						header = "HUD";
						for (auto& t : hud_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Audio"))
					{
						header = "audio";
						for (auto& t : audio_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}
					if (!add_node_filter.empty() || ImGui::BeginMenu("Resource"))
					{
						header = "resource";
						for (auto& t : resource_library->node_templates)
							show_node_library_template(t);
						if (add_node_filter.empty())
							ImGui::EndMenu();
					}

					ImGui::EndPopup();
				}
				else
				{
					add_node_filter = "";
					new_node_link_slot = nullptr;
				}

				ax::NodeEditor::Resume();

				if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
				{
					if (!io.WantCaptureKeyboard)
					{
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_C))
							copy_nodes(group);
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_V))
							paste_nodes(group, mouse_pos);
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Shift) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Left))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
							{
								if (n->is_block)
								{
									auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)n);
									ax_node->m_GroupBounds.Max.x -= 10.f;
									ax_node->m_Bounds.Max.x -= 10.f;
								}
							}
						}
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Shift) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Right))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
							{
								if (n->is_block)
								{
									auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)n);
									ax_node->m_GroupBounds.Max.x += 10.f;
									ax_node->m_Bounds.Max.x += 10.f;
								}
							}
						}
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Shift) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Up))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
							{
								if (n->is_block)
								{
									auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)n);
									ax_node->m_GroupBounds.Max.y -= 10.f;
									ax_node->m_Bounds.Max.y -= 10.f;
								}
							}
						}
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Shift) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Down))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
							{
								if (n->is_block)
								{
									auto ax_node = ax_editor->FindNode((ax::NodeEditor::NodeId)n);
									ax_node->m_GroupBounds.Max.y += 10.f;
									ax_node->m_Bounds.Max.y += 10.f;
								}
							}
						}
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Left))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
								ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position + vec2(-10.f, 0.f));
						}
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Right))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
								ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position + vec2(+10.f, 0.f));
						}
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Up))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
								ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position + vec2(0.f, -10.f));
						}
						if (ImGui::IsKeyDown((ImGuiKey)(ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_Down))
						{
							auto nodes = get_selected_nodes();
							for (auto n : nodes)
								ax::NodeEditor::SetNodePosition((ax::NodeEditor::NodeId)n, n->position + vec2(0.f, +10.f));
						}
						if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_P))
							set_parent_to_hovered_node();
						if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_F10))
							step_blueprint(debugging_group);
						if (ImGui::IsKeyDown((ImGuiKey)Keyboard_Ctrl) && ImGui::IsKeyPressed((ImGuiKey)Keyboard_S))
							save_blueprint();
					}
				}

				ax::NodeEditor::End();
				if (debugging_group)
					ax::NodeEditor::PopStyleColor();

				if (update_block_verts)
					build_all_block_verts(group);

				if (!tooltip.empty())
				{
					auto mpos = io.MousePos;
					io.MousePos = tooltip_pos;
					ImGui::SetTooltip(tooltip.c_str());
					io.MousePos = mpos;
				}
			}
			ImGui::EndChild();

			ImGui::EndTable();
		}
	}

	ax::NodeEditor::SetCurrentEditor(nullptr);

	ImGui::End();
	if (!opened)
		delete this;
}

void BlueprintView::on_global_shortcuts()
{
	if (ImGui::IsKeyPressed((ImGuiKey)Keyboard_F9))
	{
		if (f9_bound_breakpoint)
		{
			blueprint_window.debugger->add_break_node(f9_bound_breakpoint, f9_bound_breakpoint_option);
			f9_bound_breakpoint = nullptr;
		}
	}
}

BlueprintWindow::BlueprintWindow() :
	Window("Blueprint")
{
	debugger = BlueprintDebugger::create();
	debugger->callbacks.add([this](uint msg, void* parm1, void* parm2) {
		if (msg == "breakpoint_triggered"_h)
		{
			auto node = (BlueprintNodePtr)parm1;
			auto blueprint = node->group->blueprint;
			for (auto& v : views)
			{
				auto view = (BlueprintView*)v.get();
				if (blueprint == view->blueprint)
				{
					auto ax_node = view->ax_editor->GetNode((ax::NodeEditor::NodeId)node);
					view->ax_editor->NavigateTo(ax_node->GetBounds(), false, 0.f);
					break;
				}
			}
		}
	});
	BlueprintDebugger::set_current(debugger);
}

void BlueprintWindow::init()
{
	if (node_libraries.empty())
	{
		standard_library = BlueprintNodeLibrary::get(L"standard");
		extern_library = BlueprintNodeLibrary::get(L"extern");
		noise_library = BlueprintNodeLibrary::get(L"graphics::noise");
		texture_library = BlueprintNodeLibrary::get(L"graphics::texture");
		geometry_library = BlueprintNodeLibrary::get(L"graphics::geometry");
		entity_library = BlueprintNodeLibrary::get(L"universe::entity");
		camera_library = BlueprintNodeLibrary::get(L"universe::camera");
		navigation_library = BlueprintNodeLibrary::get(L"universe::navigation");
		colliding_library = BlueprintNodeLibrary::get(L"universe::colliding");
		procedural_library = BlueprintNodeLibrary::get(L"universe::procedural");
		input_library = BlueprintNodeLibrary::get(L"universe::input");
		primitive_library = BlueprintNodeLibrary::get(L"universe::primitive");
		hud_library = BlueprintNodeLibrary::get(L"universe::HUD");
		audio_library = BlueprintNodeLibrary::get(L"universe::audio");
		resource_library = BlueprintNodeLibrary::get(L"universe::resource");
		node_libraries.push_back(standard_library);
		node_libraries.push_back(extern_library);
		node_libraries.push_back(noise_library);
		node_libraries.push_back(texture_library);
		node_libraries.push_back(geometry_library);
		node_libraries.push_back(entity_library);
		node_libraries.push_back(camera_library);
		node_libraries.push_back(navigation_library);
		node_libraries.push_back(colliding_library);
		node_libraries.push_back(procedural_library);
		node_libraries.push_back(input_library);
		node_libraries.push_back(primitive_library);
		node_libraries.push_back(hud_library);
		node_libraries.push_back(audio_library);
		node_libraries.push_back(resource_library);
	}
}

View* BlueprintWindow::open_view(bool new_instance)
{
	init();
	if (new_instance || views.empty())
		return new BlueprintView;
	return nullptr;
}

View* BlueprintWindow::open_view(const std::string& name)
{
	init();
	return new BlueprintView(name);
}

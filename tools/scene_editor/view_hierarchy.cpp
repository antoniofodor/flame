#include "selection.h"
#include "view_hierarchy.h"
#include "view_scene.h"

View_Hierarchy view_hierarchy;

View_Hierarchy::View_Hierarchy() :
	View("Hierarchy")
{
}

void View_Hierarchy::on_draw()
{
	std::function<void(EntityPtr)> draw_entity;
	draw_entity = [&](EntityPtr e) {
		std::function<bool(EntityPtr, EntityPtr)> is_ancestor;
		is_ancestor = [&](EntityPtr t, EntityPtr e) {
			if (!e->parent)
				return false;
			if (e->parent == t)
				return true;
			return false;
		};

		auto flags = selection.selecting(e) ? ImGuiTreeNodeFlags_Selected : 0;
		if (e->children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		else
			flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
		ImGui::PushID(e);
		auto opened = ImGui::TreeNodeEx(("[] " + e->name).c_str(), flags) && !(flags & ImGuiTreeNodeFlags_Leaf);
		ImGui::PopID();
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("Entity", &e, sizeof(void*));
			ImGui::Text("Entity");
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			auto payload = ImGui::AcceptDragDropPayload("Entity");
			if (payload)
			{
				auto e_src = *(EntityPtr*)payload->Data;
				if (!is_ancestor(e_src, e))
				{
					e_src->parent->remove_child(e_src, false);
					e->add_child(e_src);
				}
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered())
		{
			selection.select(e);
			_just_selected = true;
		}
		if (opened)
		{
			auto gap_item = [&](int i) {
				ImGui::PushID(e); ImGui::PushID(i);
				ImGui::InvisibleButton("gap", ImVec2(-1, 4));
				ImGui::PopID(); ImGui::PopID();
				if (ImGui::BeginDragDropTarget())
				{
					auto payload = ImGui::AcceptDragDropPayload("Entity");
					if (payload)
					{
						auto e_src = *(EntityPtr*)payload->Data;
						if (!is_ancestor(e_src, e))
						{
							auto idx = i;
							if (e_src->parent == e && e_src->index < i) idx--;
							e_src->parent->remove_child(e_src, false);
							e->add_child(e_src, idx);
						}
					}
					ImGui::EndDragDropTarget();
				}
			};
			auto i = 0;
			for (; i < e->children.size(); i++)
			{
				gap_item(i);
				draw_entity(e->children[i].get());
			}
			gap_item(i);
			ImGui::TreePop();
		}
	};

	_just_selected = false;

	if (view_scene.e_prefab)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.f));
		draw_entity(view_scene.e_prefab);
		ImGui::PopStyleVar(1);
	}

	if (ImGui::IsMouseReleased(0) && ImGui::IsWindowFocused() && !_just_selected)
		selection.clear();
}

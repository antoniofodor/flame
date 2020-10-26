#pragma once

#include <flame/universe/components/layout.h>

namespace flame
{
	struct cElementPrivate;
	struct cAlignerPrivate;

	struct sLayoutSystemPrivate;

	struct cLayoutPrivate : cLayout // R ~ on_*
	{
		LayoutType type = LayoutFree;
		float gap = 0.f;
		bool auto_width = true;
		bool auto_height = true;

		float scrollx = 0.f;
		float scrolly = 0.f;

		cElementPrivate* element = nullptr; // R ref
		sLayoutSystemPrivate* layout_system = nullptr; // R ref

		bool pending_layouting = false;
		bool updating = false;

		LayoutType get_type() const override { return type; }
		void set_type(LayoutType t) override;

		float get_gap() const override { return gap; }
		void set_gap(float g) override;

		bool get_auto_width() const override { return auto_width; }
		void set_auto_width(bool a) override;
		bool get_auto_height() const override { return auto_height; }
		void set_auto_height(bool a) override;

		float get_scrollx() const override { return scrollx; }
		void set_scrollx(float s) override;
		float get_scrolly() const override { return scrolly; }
		void set_scrolly(float s) override;

		void apply_basic_h(cElementPrivate* e, cAlignerPrivate* a, bool free);
		void apply_basic_v(cElementPrivate* e, cAlignerPrivate* a, bool free);
		void judge_width(float w);
		void judge_height(float h);
		void update();

		void on_gain_layout_system();
		void on_lost_layout_system();

		void mark_layout_dirty();

		void on_local_message(Message msg, void* p) override;
		void on_child_message(Message msg, void* p) override;
		void on_local_data_changed(Component* c, uint64 hash) override;
		void on_child_data_changed(Component* c, uint64 hash) override;
	};
}

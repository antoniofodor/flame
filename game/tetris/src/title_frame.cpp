#include "share.h"
#include "title_frame.h"
#include "game_frame.h"

#include <flame/graphics/image.h>
#include <flame/UI/canvas.h>
#include <flame/UI/widgets/image.h>
#include <flame/UI/dialogs/yesno_dialog.h>
#include <flame/UI/styles/button_style_color.h>

using namespace flame;

struct Rain : UI::Widget
{
	Rain(UI::Instance *ui) :
		UI::Widget(ui)
	{
	}

	virtual void on_draw(UI::Canvas *c, const Vec2f &off, float scl) override
	{
		
	}
};

Rain *w_rain;
UI::Layout *w_title_bg;
UI::Text *w_title_text;
UI::Text *w_title_start_btn;
UI::Text *w_title_option_btn;
UI::Text *w_title_exit_btn;

UI::Button *create_round_button(const wchar_t *text)
{
	auto btn = new UI::Button(ui);
	btn->align = UI::AlignLittleEnd;
	btn->sdf_scale = 7.f;
	btn->set_text_and_size(text);
	btn->inner_padding[0] = 20.f;
	btn->background_round_radius = btn->size.y() * 0.4f;
	btn->background_round_flags = Rect::SideNE | Rect::SideSE;
	btn->background_offset = Vec4f(-15.f, 0.f,
		btn->background_round_radius, 0.f);
	btn->background_col = Vec4c(255, 255, 255, 0);
	btn->background_frame_thickness = 7.f;
	btn->background_frame_col = Vec4c(0, 0, 0, 255);
	btn->text_col = Vec4c(0, 0, 0, 255);
	btn->add_style_T<UI::ButtonStyleColor>(0, Vec3f(0.f, 0.f, 0.7f));

	return btn;
}

void create_title_frame()
{
	w_title_bg = new UI::Layout(ui);
	w_title_bg->align = UI::AlignFloatLeftNoPadding;
	w_title_bg->layout_type = UI::LayoutVertical;
	w_title_bg->item_padding = 28.f;

	w_title_text = new UI::Text(ui);
	w_title_text->align = UI::AlignLittleEnd;
	w_title_text->layout_padding = 16.f;
	w_title_text->sdf_scale = 10.f;
	w_title_text->set_text_and_size(L"TETRIS");
	w_title_text->text_col = Vec4c(0, 0, 0, 255);
	w_title_bg->add_widget(-1, w_title_text, true);

	w_title_start_btn = create_round_button(L"Start");
	w_title_bg->add_widget(-1, w_title_start_btn, true);

	w_title_start_btn->add_clicked_listener([]() {
		snd_src_ok->stop();
		snd_src_ok->play();

		destroy_title_frame();
		create_game_frame();
	});

	w_title_start_btn->add_mouseenter_listener([]() {
		snd_src_select->stop();
		snd_src_select->play();
	});

	w_title_option_btn = create_round_button(L"Option");
	w_title_bg->add_widget(-1, w_title_option_btn, true);

	w_title_option_btn->add_mouseenter_listener([]() {
		snd_src_select->stop();
		snd_src_select->play();
	});

	w_title_exit_btn = create_round_button(L"Exit");
	w_title_bg->add_widget(-1, w_title_exit_btn, true);

	w_title_exit_btn->add_clicked_listener([]() {
		snd_src_ok->stop();
		snd_src_ok->play();

		auto d = new UI::YesNoDialog(ui, L"Sure to exit?", 3.f, L"", L"Exit", L"Cancel", [](bool ok) {
			if (ok)
			{

			}
		});

		d->w_yes->add_mouseenter_listener([]() {
			snd_src_select->stop();
			snd_src_select->play();
		});

		d->w_no->add_mouseenter_listener([]() {
			snd_src_select->stop();
			snd_src_select->play();
		});

		d->w_no->add_clicked_listener([]() {
			snd_src_back->stop();
			snd_src_back->play();
		});
	});

	w_title_exit_btn->add_mouseenter_listener([]() {
		snd_src_select->stop();
		snd_src_select->play();
	});

	ui->root()->add_widget(-1, w_title_bg, true);
}

void destroy_title_frame()
{
	ui->root()->remove_widget(w_title_bg, true);

	w_title_bg = nullptr;
	w_title_text = nullptr;
	w_title_start_btn = nullptr;
	w_title_option_btn = nullptr;
	w_title_exit_btn = nullptr;
}

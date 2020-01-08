#include <flame/serialize.h>
#include <flame/foundation/blueprint.h>
#include <flame/graphics/device.h>
#include <flame/graphics/synchronize.h>
#include <flame/graphics/swapchain.h>
#include <flame/graphics/commandbuffer.h>
#include <flame/graphics/image.h>
#include <flame/graphics/font.h>
#include <flame/universe/default_style.h>
#include <flame/universe/topmost.h>
#include <flame/universe/world.h>
#include <flame/universe/systems/layout_management.h>
#include <flame/universe/systems/event_dispatcher.h>
#include <flame/universe/systems/2d_renderer.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/edit.h>
#include <flame/universe/components/image.h>
#include <flame/universe/components/event_receiver.h>
#include <flame/universe/components/aligner.h>
#include <flame/universe/components/layout.h>
#include <flame/universe/components/style.h>
#include <flame/universe/components/checkbox.h>
#include <flame/universe/components/combobox.h>
#include <flame/universe/components/tree.h>
#include <flame/universe/components/scrollbar.h>
#include <flame/universe/components/window.h>

#include "../renderpath/canvas/canvas.h"

#include "app.h"
#include "window/resource_explorer.h"

void App::create()
{
	w = Window::create("Editor", Vec2u(300, 200), WindowFrame | WindowResizable);
	w->set_maximized(true);
	d = Device::create(true);
	scr = SwapchainResizable::create(d, w);
	fence = Fence::create(d);
	sc_cbs.resize(scr->sc()->image_count());
	for (auto i = 0; i < sc_cbs.s; i++)
		sc_cbs.v[i] = Commandbuffer::create(d->gcp);
	render_finished = Semaphore::create(d);

	wchar_t* fonts[] = { 
		L"c:/windows/fonts/msyh.ttc", 
		L"../art/font_awesome.ttf"
	};
	font_atlas_pixel = FontAtlas::create(d, FontDrawPixel, 2, fonts);
	default_style.set_to_light();

	app.u = Universe::create();
	app.u->add_object(app.w);

	auto w = World::create(app.u);
	w->add_system(sLayoutManagement::create());
	w->add_system(sEventDispatcher::create());

	s_2d_renderer = s2DRenderer::create(L"../renderpath/canvas/bp", scr, cH("SwapchainResizable"), &sc_cbs);
	w->add_system(s_2d_renderer);
	{
		auto canvas = s_2d_renderer->canvas;
		canvas->add_font(app.font_atlas_pixel);
		canvas->set_clear_color(Vec4c(100, 100, 100, 255));
	}

	root = w->root();
	{
		c_element_root = cElement::create();
		root->add_component(c_element_root);

		root->add_component(cLayout::create(LayoutFree));
	}

	auto e_fps = Entity::create();
	root->add_child(e_fps);
	{
		e_fps->add_component(cElement::create());

		auto c_text = cText::create(font_atlas_pixel);
		e_fps->add_component(c_text);

		auto c_aligner = cAligner::create();
		c_aligner->x_align_ = AlignxLeft;
		c_aligner->y_align_ = AlignyBottom;
		e_fps->add_component(c_aligner);

		add_fps_listener([](void* c, uint fps) {
			(*(cText**)c)->set_text(std::to_wstring(fps).c_str());
		}, new_mail_p(c_text));
	}

	dbs.push_back(TypeinfoDatabase::load(dbs.size(), dbs.data(), L"flame_foundation.typeinfo"));
	dbs.push_back(TypeinfoDatabase::load(dbs.size(), dbs.data(), L"flame_graphics.typeinfo"));
	dbs.push_back(TypeinfoDatabase::load(dbs.size(), dbs.data(), L"flame_universe.typeinfo"));

	open_resource_explorer(L"..", Vec2f(5, 724.f));
}

void App::run()
{
	auto sc = scr->sc();

	if (sc)
		sc->acquire_image();

	fence->wait();
	looper().process_events();

	c_element_root->set_size(Vec2f(w->size));
	u->update();

	if (sc)
	{
		std::vector<Commandbuffer*> cbs;
		cbs.push_back(sc_cbs.v[sc->image_index()]);
		cbs.insert(cbs.begin(), extra_cbs.begin(), extra_cbs.end());
		extra_cbs.clear();
		d->gq->submit(cbs.size(), cbs.data(), sc->image_avalible(), render_finished, fence);
		d->gq->present(sc, render_finished);
	}
}

App app;

Entity* create_drag_edit(FontAtlas* font_atlas, float font_size_scale, bool is_float)
{
	auto e_layout = Entity::create();
	{
		e_layout->add_component(cElement::create());

		auto c_layout = cLayout::create(LayoutVertical);
		c_layout->fence = 1;
		e_layout->add_component(c_layout);
	}

	auto e_edit = create_standard_edit(50.f, font_atlas, font_size_scale);
	e_layout->add_child(e_edit);
	e_edit->set_visibility(false);

	auto e_drag = Entity::create();
	e_layout->add_child(e_drag);
	{
		auto c_element = cElement::create();
		c_element->inner_padding_ = Vec4f(4.f, 2.f, 4.f, 2.f);
		c_element->size_.x() = 58.f;
		e_drag->add_component(c_element);

		auto c_text = cText::create(font_atlas);
		c_text->font_size_ = default_style.font_size * font_size_scale;
		c_text->auto_width_ = false;
		e_drag->add_component(c_text);

		e_drag->add_component(cEventReceiver::create());

		e_drag->add_component(cStyleColor::create(default_style.button_color_normal, default_style.button_color_hovering, default_style.button_color_active));
	}

	struct Capture
	{
		Entity* e;
		cText* e_t;
		cEdit* e_e;
		cEventReceiver* e_er;
		Entity* d;
		cEventReceiver* d_er;
		bool is_float;
	}capture;
	capture.e = e_edit;
	capture.e_t = e_edit->get_component(cText);
	capture.e_e = e_edit->get_component(cEdit);
	capture.e_er = e_edit->get_component(cEventReceiver);
	capture.d = e_drag;
	capture.d_er = e_drag->get_component(cEventReceiver);
	capture.is_float = is_float;

	capture.e_er->data_changed_listeners.add([](void* c, Component* er, uint hash, void*) {
		auto& capture = *(Capture*)c;
		if (hash == cH("focusing") && ((cEventReceiver*)er)->focusing == false)
		{
			capture.e->set_visibility(false);
			capture.d->set_visibility(true);
		}
	}, new_mail(&capture));

	capture.d_er->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
		auto& capture = *(Capture*)c;
		if (is_mouse_clicked(action, key) && pos == 0)
		{
			capture.e->set_visibility(true);
			capture.d->set_visibility(false);
			auto dp = capture.d_er->dispatcher;
			dp->next_focusing = capture.e_er;
			dp->pending_update = true;
		}
		else if (capture.d_er->active && is_mouse_move(action, key))
		{
			if (capture.is_float)
			{
				auto v = std::stof(capture.e_t->text());
				v += pos.x() * 0.05f;
				capture.e_t->set_text(to_wstring(v, 2).c_str());
			}
			else
			{
				auto v = std::stoi(capture.e_t->text());
				v += pos.x();
				capture.e_t->set_text(std::to_wstring(v).c_str());
			}
		}
	}, new_mail(&capture));

	return e_layout;
}

void create_enum_combobox(EnumInfo* info, float width, FontAtlas* font_atlas, float font_size_scale, Entity* parent)
{
	std::vector<std::wstring> _items;
	for (auto i = 0; i < info->item_count(); i++)
		_items.push_back(s2w(info->item(i)->name()));
	std::vector<const wchar_t*> items(_items.size());
	for (auto i = 0; i < items.size(); i++)
		items[i] = _items[i].c_str();
	parent->add_child(create_standard_combobox(120.f, font_atlas, font_size_scale, app.root, items.size(), items.data()));
}

void create_enum_checkboxs(EnumInfo* info, FontAtlas* font_atlas, float font_size_scale, Entity* parent)
{
	for (auto i = 0; i < info->item_count(); i++)
		parent->add_child(wrap_standard_text(create_standard_checkbox(), false, font_atlas, font_size_scale, s2w(info->item(i)->name()).c_str()));
}

Entity* popup_dialog()
{
	auto t = get_topmost(app.root);
	if (!t)
	{
		t = create_topmost(app.root, false, false, true, Vec4c(0, 0, 0, 127), true);
		{
			t->add_component(cLayout::create(LayoutFree));
		}
	}

	auto e_dialog = Entity::create();
	t->add_child(e_dialog);
	{
		auto c_element = cElement::create();
		c_element->inner_padding_ = Vec4f(8.f);
		c_element->color_ = Vec4c(255);
		e_dialog->add_component(c_element);

		auto c_aligner = cAligner::create();
		c_aligner->x_align_ = AlignxMiddle;
		c_aligner->y_align_ = AlignyMiddle;
		e_dialog->add_component(c_aligner);

		auto c_layout = cLayout::create(LayoutVertical);
		c_layout->item_padding = 4.f;
		e_dialog->add_component(c_layout);
	}

	return e_dialog;
}

void popup_message_dialog(const std::wstring& text)
{
	auto e_dialog = popup_dialog();

	auto e_text = Entity::create();
	e_dialog->add_child(e_text);
	{
		e_text->add_component(cElement::create());

		auto c_text = cText::create(app.font_atlas_pixel);
		c_text->set_text(text.c_str());
		e_text->add_component(c_text);
	}

	auto e_ok = create_standard_button(app.font_atlas_pixel, 1.f, L"OK");
	e_dialog->add_child(e_ok);
	{
		e_ok->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			if (is_mouse_clicked(action, key))
				destroy_topmost(app.root, false);
		}, Mail());
	}
}

void popup_confirm_dialog(const std::wstring& title, void (*callback)(void* c, bool yes), const Mail<>& _capture)
{
	auto e_dialog = popup_dialog();

	auto e_text = Entity::create();
	e_dialog->add_child(e_text);
	{
		e_text->add_component(cElement::create());

		auto c_text = cText::create(app.font_atlas_pixel);
		c_text->set_text(title.c_str());
		e_text->add_component(c_text);
	}

	auto e_buttons = Entity::create();
	e_dialog->add_child(e_buttons);
	{
		e_buttons->add_component(cElement::create());

		auto c_layout = cLayout::create(LayoutHorizontal);
		c_layout->item_padding = 4.f;
		e_buttons->add_component(c_layout);
	}

	struct Capture
	{
		void (*c)(void* c, bool yes);
		Mail<> m;
	}capture;
	capture.c = callback;
	capture.m = _capture;

	auto e_yes = create_standard_button(app.font_atlas_pixel, 1.f, L"Yes");
	e_buttons->add_child(e_yes);
	{
		e_yes->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto& capture = *(Capture*)c;

			if (is_mouse_clicked(action, key))
			{
				destroy_topmost(app.root, false);

				capture.c(capture.m.p, true);
				delete_mail(capture.m);
			}
		}, new_mail(&capture));
	}

	auto e_no = create_standard_button(app.font_atlas_pixel, 1.f, L"No");
	e_buttons->add_child(e_no);
	{
		e_no->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto& capture = *(Capture*)c;

			if (is_mouse_clicked(action, key))
			{
				destroy_topmost(app.root, false);

				capture.c(capture.m.p, false);
				delete_mail(capture.m);
			}
		}, new_mail(&capture));
	}
}

void popup_input_dialog(const std::wstring& title, void (*callback)(void* c, bool ok, const std::wstring& text), const Mail<>& _capture)
{
	auto e_dialog = popup_dialog();

	auto e_input = create_standard_edit(100.f, app.font_atlas_pixel, 1.f);
	e_dialog->add_child(wrap_standard_text(e_input, false, app.font_atlas_pixel, 1.f, title.c_str()));

	auto e_buttons = Entity::create();
	e_dialog->add_child(e_buttons);
	{
		e_buttons->add_component(cElement::create());

		auto c_layout = cLayout::create(LayoutHorizontal);
		c_layout->item_padding = 4.f;
		e_buttons->add_component(c_layout);
	}

	struct Capture
	{
		void (*c)(void* c, bool ok, const std::wstring& text);
		Mail<> m;
		cText* t;
	}capture;
	capture.c = callback;
	capture.m = _capture;
	capture.t = e_input->get_component(cText);

	auto e_ok = create_standard_button(app.font_atlas_pixel, 1.f, L"Ok");
	e_buttons->add_child(e_ok);
	{
		e_ok->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto& capture = *(Capture*)c;

			if (is_mouse_clicked(action, key))
			{
				auto text = capture.t->text();
				destroy_topmost(app.root, false);

				capture.c(capture.m.p, true, text);
				delete_mail(capture.m);
			}
		}, new_mail(&capture));
	}

	auto e_cancel = create_standard_button(app.font_atlas_pixel, 1.f, L"Cancel");
	e_buttons->add_child(e_cancel);
	{
		e_cancel->get_component(cEventReceiver)->mouse_listeners.add([](void* c, KeyState action, MouseKey key, const Vec2i& pos) {
			auto& capture = *(Capture*)c;

			if (is_mouse_clicked(action, key))
			{
				destroy_topmost(app.root, false);

				capture.c(capture.m.p, false, L"");
				delete_mail(capture.m);
			}
		}, new_mail(&capture));
	}
}

extern "C" void mainCRTStartup();
static void* init_crt_ev = nullptr;
extern "C" __declspec(dllexport) void init_crt(void* ev)
{
	init_crt_ev = ev;
	mainCRTStartup();
}

int main(int argc, char **args)
{
	if (init_crt_ev)
	{
		set_event(init_crt_ev);
		for (;;)
			sleep(60000);
	}

	app.create();

	looper().loop([](void* c) {
		auto app = (*(App**)c);
		app->run();
	}, new_mail_p(&app));

	return 0;
}

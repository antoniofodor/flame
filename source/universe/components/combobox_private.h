#pragma once

#include "../entity_private.h"
#include <flame/universe/components/combobox.h>

namespace flame
{
	struct cTextPrivate;

	struct cComboboxPrivate : cCombobox
	{
		int index = -1;

		cTextPrivate* text = nullptr;
		//cMenuPrivate* menu = nullptr;

		int get_index() const override { return index; }
		void set_index(int index) override;
	};

	struct cComboboxItemPrivate : cComboboxItem
	{
		cComboboxPrivate* combobox = nullptr;
		cComboboxPrivate* staging_combobox = nullptr;

		cReceiverPrivate* receiver = nullptr;

		void* mouse_listener = nullptr;

		void on_gain_receiver();
		void on_lost_receiver();
		void on_lost_combobox();
	};
}

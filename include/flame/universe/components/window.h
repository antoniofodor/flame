#pragma once

#include <flame/universe/component.h>

namespace flame
{
	struct cElement;
	struct cEventReceiver;
	struct cLayout;
	struct cListItem;

	struct cWindow : Component
	{
		cElement* element;
		cEventReceiver* event_receiver;

		cWindow() :
			Component("Window")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cWindow() override;

		FLAME_UNIVERSE_EXPORTS virtual void start() override;
		FLAME_UNIVERSE_EXPORTS virtual void update() override;
		FLAME_UNIVERSE_EXPORTS virtual Component* copy() override;

		FLAME_UNIVERSE_EXPORTS void* add_pos_listener(void (*listener)(void* c), const Mail<>& capture);

		FLAME_UNIVERSE_EXPORTS void remove_pos_listener(void* ret_by_add);

		FLAME_UNIVERSE_EXPORTS static cWindow* create();
	};

	struct cSizeDragger : Component 
	{
		cEventReceiver* event_receiver;

		cSizeDragger() :
			Component("SizeDragger")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cSizeDragger() override;

		FLAME_UNIVERSE_EXPORTS virtual void start() override;
		FLAME_UNIVERSE_EXPORTS virtual void update() override;
		FLAME_UNIVERSE_EXPORTS virtual Component* copy() override;

		FLAME_UNIVERSE_EXPORTS static cSizeDragger* create();
	};

	struct cDockerTab : Component
	{
		cElement* element;
		cEventReceiver* event_receiver;
		cListItem* list_item;

		Entity* root;

		cDockerTab() :
			Component("DockerTab")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cDockerTab() override;

		FLAME_UNIVERSE_EXPORTS virtual void start() override;
		FLAME_UNIVERSE_EXPORTS virtual void update() override;
		FLAME_UNIVERSE_EXPORTS virtual Component* copy() override;

		FLAME_UNIVERSE_EXPORTS static cDockerTab* create();
	};

	struct cDockerTabbar : Component
	{
		cElement* element;
		cEventReceiver* event_receiver;

		cDockerTabbar() :
			Component("DockerTabbar")
		{
		}

		FLAME_UNIVERSE_EXPORTS virtual ~cDockerTabbar() override;

		FLAME_UNIVERSE_EXPORTS virtual void start() override;
		FLAME_UNIVERSE_EXPORTS virtual void update() override;
		FLAME_UNIVERSE_EXPORTS virtual Component* copy() override;

		FLAME_UNIVERSE_EXPORTS static cDockerTabbar* create();
	};

	FLAME_UNIVERSE_EXPORTS Entity* get_docker_tab_model();
}

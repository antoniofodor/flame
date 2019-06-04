// MIT License
// 
// Copyright (c) 2019 wjs
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <flame/foundation/serialize.h>
#include <flame/foundation/blueprint.h>
#include <flame/network/network.h>

using namespace flame;

void generate_graph_and_layout(BP *bp)
{
	if (GRAPHVIZ_PATH == std::string(""))
		assert(0);
	auto dot_path = s2w(GRAPHVIZ_PATH) + L"/bin/dot.exe";

	std::string gv = "digraph bp {\nrankdir=LR\nnode [shape = Mrecord];\n";
	for (auto i = 0; i < bp->node_count(); i++)
	{
		auto src = bp->node(i);
		auto name = std::string(src->id());

		auto n = "\t" + name + " [label = \"" + name + "|{{";
		for (auto j = 0; j < src->input_count(); j++)
		{
			auto input = src->input(j);
			auto name = std::string(input->variable_info()->name());
			n += "<" + name + ">" + name;
			if (j != src->input_count() - 1)
				n += "|";
		}
		n += "}|{";
		for (auto j = 0; j < src->output_count(); j++)
		{
			auto output = src->output(j);
			auto name = std::string(output->variable_info()->name());
			n += "<" + name + ">" + name;
			if (j != src->output_count() - 1)
				n += "|";
		}
		n += "}}\"];\n";

		gv += n;
	}
	for (auto i = 0; i < bp->node_count(); i++)
	{
		auto src = bp->node(i);

		for (auto j = 0; j < src->input_count(); j++)
		{
			auto input = src->input(j);
			if (input->link())
			{
				auto in_sp = string_split(std::string(input->get_address().v), '.');
				auto out_sp = string_split(std::string(input->link()->get_address().v), '.');

				gv += "\t" + out_sp[0] + ":" + out_sp[1] + " -> " + in_sp[0] + ":" + in_sp[1] + ";\n";
			}
		}
	}
	gv += "}\n";

	std::ofstream file("bp.gv");
	file << gv;
	file.close();

	exec(dot_path.c_str(), "-Tpng bp.gv -o bp.png", true);
	exec(dot_path.c_str(), "-Tplain bp.gv -y -o bp.graph.txt", true);
}

struct App
{
	std::wstring filename;
	BP* bp;
}app;
auto papp = &app;

int main(int argc, char **args)
{
	auto typeinfo_lv = typeinfo_free_level();
	typeinfo_load(L"flame_foundation.typeinfo", typeinfo_lv);
	//typeinfo_load(L"flame_network.typeinfo", typeinfo_lv);
	//typeinfo_load(L"flame_graphics.typeinfo", typeinfo_lv);
	//typeinfo_load(L"flame_sound.typeinfo", typeinfo_lv);
	//typeinfo_load(L"flame_universe.typeinfo", typeinfo_lv);

	app.bp = nullptr;
	if (argc > 1)
	{
		app.filename = s2w(args[1]);
		app.bp = BP::create_from_file(app.filename.c_str());
		if (!app.bp)
			app.filename = L"";
	}

	if (!app.bp)
		app.bp = BP::create();
	if (!app.filename.empty())
		printf("\"%s\":\n", w2s(app.filename).c_str());
	else
		printf("\"unnamed\":\n");

	network_init();

	std::vector<UdtInfo*> available_udts;
	{
		auto udts = get_udts();
		for (auto i = 0; i < udts.size; i++)
		{
			auto u = udts[i];
			auto name = std::string(u->name());
			if (name.find("BP_") == 0)
				available_udts.push_back(u);
		}
		std::sort(available_udts.begin(), available_udts.end(), [](UdtInfo* a, UdtInfo* b) {
			return std::string(a->name()) < std::string(b->name());
		});
	}

	while (true)
	{
		char command_line[260];
		scanf("%s", command_line);
		auto s_command_line = std::string(command_line);
		if (s_command_line == "help")
		{
			printf(
				"  help - show this help\n"
				"  show udts - show all available udts (see blueprint.h for more details)\n"
				"  show udt [udt_name] - show an udt\n"
				"  show nodes - show all nodes\n"
				"  show node [id] - show a node\n"
				"  show graph - use Graphviz to show graph\n"
				"  add node [type_name] [id] - add a node (id of '-' means don't care)\n"
				"  add link [out_adress] [in_adress] - add a link\n"
				"  remove node [id] - remove a node\n"
				"  remove link [in_adress] - remove a link\n"
				"  set [in_adress] [value] - set value for input\n"
				"  update - update this blueprint\n"
				"  refresh - reload the bp\n"
				"  save [filename] - save this blueprint (you don't need filename while this blueprint already having one)\n"
				"  reload - reload the bp\n"
				"  set-layout - set nodes' positions using 'bp.png' and 'bp.graph.txt', need do show graph first\n"
				"  gui-browser - use the power of browser to show and edit\n"
			);
		}
		else if (s_command_line == "show")
		{
			scanf("%s", command_line);
			auto s_what = std::string(command_line);

			if (s_what == "udts")
			{
				for (auto u : available_udts)
					printf("%s\n", u->name());
			}
			else if (s_what == "udt")
			{
				scanf("%s", command_line);
				auto s_name = std::string(command_line);

				auto udt = find_udt(H(s_name.c_str()), s_name.c_str());
				if (udt)
				{
					printf("%s:\n", udt->name());
					std::vector<VariableInfo*> inputs;
					std::vector<VariableInfo*> outputs;
					for (auto i_i = 0; i_i < udt->item_count(); i_i++)
					{
						auto item = udt->item(i_i);
						auto attribute = std::string(item->attribute());
						if (attribute.find('i') != std::string::npos)
							inputs.push_back(item);
						if (attribute.find('o') != std::string::npos)
							outputs.push_back(item);
					}
					printf("[In]\n");
					for (auto &i : inputs)
						printf(" name:%s attribute:%s tag:%s type:%s\n", i->name(), i->attribute(), get_type_tag_name(i->type()->tag()), i->type()->name());
					printf("[Out]\n");
					for (auto &i : outputs)
						printf(" name:%s attribute:%s tag:%s type:%s\n", i->name(), i->attribute(), get_type_tag_name(i->type()->tag()), i->type()->name());
				}
				else
					printf("udt not found\n");
			}
			else if (s_what == "nodes")
			{
				for (auto i = 0; i < app.bp->node_count(); i++)
				{
					auto n = app.bp->node(i);
					printf("id:%s type:%s\n", n->id(), n->udt()->name());
				}
			}
			else if (s_what == "node")
			{
				scanf("%s", command_line);
				auto s_id = std::string(command_line);

				auto n = app.bp->find_node(s_id.c_str());
				if (n)
				{
					printf("[In]\n");
					for (auto i = 0; i < n->input_count(); i++)
					{
						auto input = n->input(i);
						auto v = input->variable_info();
						auto type = v->type();
						printf(" %s\n", v->name());
						std::string link_address;
						if (input->link())
							link_address = input->link()->get_address().v;
						printf("   [%s]->\n", link_address.c_str());
						auto str = serialize_value(type->tag(), type->hash(), type->name(), input->data(), 2);
						if (str.size == 0)
							str = "-";
						printf("   %s\n", str.v);
					}
					printf("[Out]\n");
					for (auto i = 0; i < n->output_count(); i++)
					{
						auto output = n->output(i);
						auto v = output->variable_info();
						auto type = v->type();
						printf(" %s\n", output->variable_info()->name());
						auto str = serialize_value(type->tag(), type->hash(), type->name(), output->data(), 2);
						if (str.size == 0)
							str = "-";
						printf("   %s\n", str.v);
					}
				}
				else
					printf("node not found\n");
			}
			else if (s_what == "graph")
			{
				if (!std::fs::exists(L"bp.png") || std::fs::last_write_time(L"bp.png") < std::fs::last_write_time(app.filename))
					generate_graph_and_layout(app.bp);
				if (std::fs::exists(L"bp.png"))
				{
					exec(L"bp.png", "", false);
					printf("ok\n");
				}
				else
					printf("bp.png not found, perhaps Graphviz is not available\n");
			}
			else
				printf("unknow object to show\n");
		}
		else if (s_command_line == "add")
		{
			scanf("%s", command_line);
			auto s_what = std::string(command_line);

			if (s_what == "node")
			{
				scanf("%s", command_line);
				auto s_tn = std::string(command_line);

				scanf("%s", command_line);
				auto s_id = std::string(command_line);

				auto n = app.bp->add_node(s_tn.c_str(), s_id == "-" ? nullptr : s_id.c_str());
				if (!n)
					printf("bad udt name or id already exist\n");
				else
					printf("node added: %s\n", n->id());
			}
			else if (s_what == "link")
			{
				scanf("%s", command_line);
				auto s_out_address = std::string(command_line);

				scanf("%s", command_line);
				auto s_in_address = std::string(command_line);

				auto out = app.bp->find_output(s_out_address.c_str());
				auto in = app.bp->find_input(s_in_address.c_str());
				if (out && in)
				{
					in->link_to(out);
					printf("link added: %s - %s\n", in->link()->get_address().v, in->get_address().v);
				}
				else
					printf("wrong address\n");
			}
			else
				printf("unknow object to add\n");
		}
		else if (s_command_line == "remove")
		{
			scanf("%s", command_line);
			auto s_what = std::string(command_line);

			if (s_what == "node")
			{
				scanf("%s", command_line);
				auto s_id = std::string(command_line);

				auto n = app.bp->find_node(s_id.c_str());
				if (n)
				{
					app.bp->remove_node(n);
					printf("node removed: %s\n", s_id.c_str());
				}
				else
					printf("node not found\n");
			}
			else if (s_what == "link")
			{
				scanf("%s", command_line);
				auto s_in_address = std::string(command_line);

				auto i = app.bp->find_input(s_in_address.c_str());
				if (i)
				{
					i->link_to(nullptr);
					printf("link removed: %s\n", s_in_address.c_str());
				}
				else
					printf("input not found\n");
			}
			else
				printf("unknow object to remove\n");
		}
		else if (s_command_line == "set")
		{
			scanf("%s", command_line);
			auto s_address = std::string(command_line);

			scanf("%s", command_line);
			auto s_value = std::string(command_line);

			auto i = app.bp->find_input(s_address.c_str());
			if (i)
			{
				auto v = i->variable_info();
				auto type = v->type();
				auto value_before = serialize_value(type->tag(), type->hash(), type->name(), i->data(), 2);
				unserialize_value(type->tag(), type->hash(), type->name(), s_value, i->data());
				auto value_after = serialize_value(type->tag(), type->hash(), type->name(), i->data(), 2);
				printf("set value: %s, %s -> %s\n", s_address.c_str(), value_before.v, value_after.v);
			}
			else
				printf("input not found\n");
		}
		else if (s_command_line == "update")
		{
			app.bp->initialize();
			app.bp->update();
			app.bp->finish();
			printf("BP updated\n");
		}
		else if (s_command_line == "refresh")
		{
			if (app.filename == L"")
				printf("you need to save first\n");
			else
			{

			}
		}
		else if (s_command_line == "save")
		{
			if (!app.filename.empty())
			{
				app.bp->save(app.filename.c_str());
				printf("file saved\n");
			}
			else
			{
				scanf("%s", command_line);
				auto s_filename = std::string(command_line);

				if (!std::fs::exists(s_filename))
				{
					app.filename = s2w(s_filename);
					app.bp->save(app.filename.c_str());
					printf("file saved\n");
					printf("%s:\n", s_filename.c_str());
				}
				else
					printf("app.filename taken\n");
			}
		}
		else if (s_command_line == "reload")
		{
			if (!app.filename.empty())
			{
				BP::destroy(app.bp);
				app.bp = BP::create_from_file(app.filename.c_str());
				printf("reloaded\n");
			}
			else
				printf("you need to save the bp first\n");
		}
		else if (s_command_line == "set-layout")
		{
			if (!std::fs::exists(L"bp.graph.txt") || std::fs::last_write_time(L"bp.graph.txt") < std::fs::last_write_time(app.filename))
				generate_graph_and_layout(app.bp);
			if (std::fs::exists(L"bp.graph.txt"))
			{
				auto str = get_file_string(L"bp.graph.txt");
				std::regex reg_node(R"(node ([\w]+) ([\d\.]+) ([\d\.]+))");
				std::smatch match;
				while (std::regex_search(str, match, reg_node))
				{
					auto n = app.bp->find_node(match[1].str().c_str());
					if (n)
						n->set_position(Vec2f(std::stof(match[2].str().c_str()), std::stof(match[3].str().c_str())) * 100.f);

					str = match.suffix();
				}
				printf("ok\n");
			}
			else
				printf("bp.graph.txt not found\n");
		}
		else if (s_command_line == "gui-browser")
		{
			//exec((std::wstring(L"file:///") + get_curr_path() + L"/bp.html").c_str(), "", false);
			printf("waiting for browser on port 5566 ...");

			auto s = OneClientServer::create(SocketWeb, 5566, 100, Function<void(void*, int, void*)>(
				[](void* c, int len, void* data) {
					auto app = *(App**)c;
					auto wtf = (char*)data;
					std::ofstream file(app->filename);
					file << (char*)data;
					file.close();

					BP::destroy(app->bp);
					app->bp = BP::create_from_file(app->filename.c_str());

					printf("browser: bp updated\n");
				}, sizeof(void*), &papp));
			if (!s)
				printf("  timeout\n");
			else
			{
				printf("  ok\nbrowser: working\n");

				auto json = SerializableNode::create_from_json_file(app.filename);
				json->new_attr("file", w2s(app.filename));
				auto str = json->to_string_json();
				s->send(str.size, str.v);
				SerializableNode::destroy(json);

				wait_for(s->ev_closed);
				printf("browser: closed\n");
			}
		}
		else
			printf("unknow command\n");
	}

	return 0;
}

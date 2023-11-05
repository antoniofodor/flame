#include "../../foundation/blueprint.h"
#include "../../graphics/model_ext.h"
#include "node_private.h"
#include "mesh_private.h"
#include "procedure_mesh_private.h"
#include "../systems/renderer_private.h"

namespace flame
{
	cProcedureMeshPrivate::~cProcedureMeshPrivate()
	{
		if (mesh->mesh_res_id != -1)
		{
			sRenderer::instance()->release_mesh_res(mesh->mesh_res_id);
			mesh->mesh = nullptr;
			mesh->mesh_res_id = -1;
		}
	}

	void cProcedureMeshPrivate::set_blueprint_name(const std::filesystem::path& name)
	{
		if (blueprint_name == name)
			return;
		blueprint_name = name;

		if (!blueprint_name.empty())
		{
			if (auto bp = Blueprint::get(blueprint_name); bp)
			{
				auto ins = BlueprintInstance::create(bp);
				std::vector<voidptr> outputs;

				graphics::ControlMesh* p_control_mesh = nullptr;
				outputs.push_back(&p_control_mesh);
				ins->call("main"_h, nullptr, outputs.data());

				if (p_control_mesh)
				{
					p_control_mesh->convert_to_mesh(converted_mesh);
					assert(mesh->mesh_res_id == -1);
					mesh->mesh = &converted_mesh;
					mesh->mesh_res_id = sRenderer::instance()->get_mesh_res(&converted_mesh, -1);
					mesh->node->mark_transform_dirty();

					mesh->set_material_name(L"default");
				}

				delete ins;
				Blueprint::release(bp);
			}
		}
	}

	struct cProcedureMeshCreate : cProcedureMesh::Create
	{
		cProcedureMeshPtr operator()(EntityPtr e) override
		{
			return new cProcedureMeshPrivate();
		}
	}cProcedureMesh_create;
	cProcedureMesh::Create& cProcedureMesh::create = cProcedureMesh_create;
}

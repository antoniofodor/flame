#pragma once

#include <flame/graphics/buffer.h>

#include <flame/type.h>
#include <flame/model/model.h>

#include <vector>

namespace flame
{
	namespace graphics
	{
		struct ModelManager
		{
			ModelDescription md;
			Buffer *vb;
			int vb_size;
			Buffer *ib;
			int vertex_count;
			int indice_count;
			std::vector<std::pair<std::string, Model*>> models;

			Device *d;

			inline ModelManager(Device *_d)
			{
				d = _d;

				static VertexSemantic vs[] = {
					VertexPosition,
					VertexUV0,
					VertexNormal
				};
				vb_size = 0;
				for (auto i = 0; i < FLAME_ARRAYSIZE(vs); i++)
					vb_size += vertex_semantic_size(vs[i]);
				static VertexBufferDescription vbd;
				vbd.active_if_has_bone = false;
				vbd.semantic_count = FLAME_ARRAYSIZE(vs);
				vbd.semantics = vs;
				md.set_to_default();
				md.desired_vertex_buffer_count = 1;
				md.desired_vertex_buffers = &vbd;

				vertex_count = 0;
				indice_count = 0;
				create_vb();
			}

			inline ~ModelManager()
			{
				destroy_vb();
				create_vb();
			}

			inline void add_model(const char *filename)
			{
				auto m = load_model(&md, filename);
				if (m)
				{
					models.emplace_back(filename, m);
					vertex_count += m->vertex_count;
					indice_count += m->indice_count;
				}
			}

			inline Model *get_model(const char *filename)
			{
				for (auto &m : models)
				{
					if (m.first == filename)
						return m.second;
				}
				return nullptr;
			}

			inline void build()
			{
				destroy_vb();
				create_vb();
			}

			inline void destroy_vb()
			{
				destroy_buffer(vb);
				destroy_buffer(ib);
			}

			inline void create_vb()
			{
				vb = create_buffer(d, vertex_count == 0 ? 16 : (vertex_count * vb_size * sizeof(float)), BufferUsageVertex |
					BufferUsageTransferDst, MemPropDevice);
				ib = create_buffer(d, indice_count == 0 ? 16 : (indice_count * (md.indice_type == IndiceTypeUint ? sizeof(uint) : sizeof(ushort))), BufferUsageIndex |
					BufferUsageTransferDst, MemPropDevice);
			}
		};
	}
}

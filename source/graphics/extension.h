#pragma once

#include "../foundation/typeinfo.h"
#include "command.h"
#include "buffer.h"
#include "shader.h"

namespace flame
{
	namespace graphics
	{
		struct InstanceCommandBuffer : std::unique_ptr<CommandBufferT>
		{
			FencePtr fence;

			InstanceCommandBuffer(FencePtr fence = nullptr) :
				fence(fence)
			{
				reset(CommandBuffer::create(CommandPool::get()));
				get()->begin(true);
			}

			void excute()
			{
				get()->end();
				auto q = Queue::get();
				q->submit1(get(), nullptr, nullptr, fence);
				if (!fence)
					q->wait_idle();
				else
					fence->wait();
			}
		};

		struct StagingBuffer : std::unique_ptr<BufferT>
		{
			StagingBuffer(uint size, void* data = nullptr, BufferUsageFlags extra_usage = BufferUsageNone)
			{
				reset(Buffer::create(size, BufferUsageTransferSrc | BufferUsageTransferDst | extra_usage, MemoryPropertyHost | MemoryPropertyCoherent));
				get()->map();
				if (data)
					memcpy(get()->mapped, data, size);
			}
		};

		struct SparseArray
		{
			uint capacity;
			std::deque<uint> free_slots;

			void init(uint _capacity)
			{
				capacity = _capacity;
				free_slots.resize(capacity);
				std::iota(free_slots.begin(), free_slots.end(), 0);
			}

			inline int get_free_item()
			{
				if (free_slots.empty())
					return -1;
				auto ret = free_slots.front();
				free_slots.pop_front();
				return ret;
			}

			inline void release_item(uint id)
			{
				free_slots.push_back(id);
			}
		};

		constexpr inline AccessFlags u2a(BufferUsageFlags u)
		{
			switch (u)
			{
			case BufferUsageVertex:
				return AccessVertexAttributeRead;
			case BufferUsageIndex:
				return AccessIndexRead;
			case BufferUsageIndirect:
				return AccessIndirectCommandRead;
			case BufferUsageUniform:
				return AccessShaderRead;
			case BufferUsageStorage:
				return AccessShaderRead | AccessShaderWrite;
			}
			return AccessNone;
		}

		constexpr inline PipelineStageFlags u2s(BufferUsageFlags u)
		{
			switch (u)
			{
			case BufferUsageVertex:
			case BufferUsageIndex:
				return PipelineStageVertexInput;
			case BufferUsageIndirect:
				return PipelineStageDrawIndirect;
			}
			return PipelineStageAllCommand;
		}

		struct StorageBuffer2 : VirtualStruct
		{
			BufferUsageFlags			usage;
			std::unique_ptr<BufferT>	buf;
			std::unique_ptr<BufferT>	stag;

			void create(BufferUsageFlags _usage, UdtInfo* ui, BufferUsageFlags _stag_usage = BufferUsageNone)
			{
				usage = _usage;
				buf.reset(Buffer::create(ui->size, BufferUsageTransferDst | usage, MemoryPropertyDevice));
				stag.reset(Buffer::create(buf->size, BufferUsageTransferSrc | _stag_usage, MemoryPropertyHost | MemoryPropertyCoherent));
				stag->map();
				init(ui, stag->mapped);
			}

			void upload(CommandBufferPtr cb)
			{
				if (dirty_regions.empty())
					return;
				std::vector<BufferCopy> copies;
				for (auto& r : dirty_regions)
				{
					BufferCopy cpy;
					cpy.src_off = cpy.dst_off = r.first;
					cpy.size = r.second;
					copies.push_back(cpy);
				}
				dirty_regions.clear();
				cb->copy_buffer(stag.get(), buf.get(), copies);
				cb->buffer_barrier(buf.get(), AccessTransferWrite, u2a(usage), PipelineStageTransfer, u2s(usage));
			}
		};

		struct VertexBuffer : VirtualStruct
		{
			UdtInfo						array_type;
			uint						capacity;
			std::unique_ptr<BufferT>	buf;
			std::unique_ptr<BufferT>	stag;
			uint						buf_top;
			uint						stag_top;

			void create(UdtInfo* ui, uint _capacity)
			{
				capacity = _capacity;
				buf.reset(Buffer::create(capacity * ui->size, BufferUsageTransferDst | BufferUsageVertex, MemoryPropertyDevice));
				stag.reset(Buffer::create(buf->size, BufferUsageTransferSrc, MemoryPropertyHost | MemoryPropertyCoherent));
				stag->map();
				auto& vi = array_type.variables.emplace_back();
				vi.type = TypeInfo::get(TagU, ui->name, *ui->db);
				vi.array_size = capacity;
				vi.array_stride = ui->size;
				array_type.size = ui->size;
				init(&array_type, stag->mapped);
			}

			VirtualData add()
			{
				return item_i(0, stag_top++);
			}

			void upload(CommandBufferPtr cb)
			{
				if (buf_top < stag_top)
				{
					BufferCopy cpy;
					cpy.src_off = cpy.dst_off = buf_top;
					cpy.size = (stag_top - buf_top) * ui->size;
					cb->copy_buffer(stag.get(), buf.get(), { &cpy, 1 });
					cb->buffer_barrier(buf.get(), AccessTransferWrite, u2a(BufferUsageVertex), PipelineStageTransfer, u2s(BufferUsageVertex));
					buf_top = stag_top;
				}
			}
		};

		struct IndexBuffer
		{
			uint						capacity;
			std::unique_ptr<BufferT>	buf;
			std::unique_ptr<BufferT>	stag;
			uint						buf_top;
			uint						stag_top;

			void create(uint _capacity)
			{
				capacity = _capacity;
				buf.reset(Buffer::create(capacity * sizeof(uint), BufferUsageTransferDst | BufferUsageIndex, MemoryPropertyDevice));
				stag.reset(Buffer::create(buf->size, BufferUsageTransferSrc, MemoryPropertyHost | MemoryPropertyCoherent));
				stag->map();
			}

			void add(const uint* src, uint size)
			{
				memcpy((char*)stag->mapped + stag_top * sizeof(uint), src, size * sizeof(uint));
				stag_top += size;
			}

			void upload(CommandBufferPtr cb)
			{
				if (buf_top < stag_top)
				{
					BufferCopy cpy;
					cpy.src_off = cpy.dst_off = buf_top;
					cpy.size = (stag_top - buf_top) * sizeof(uint);
					cb->copy_buffer(stag.get(), buf.get(), { &cpy, 1 });
					cb->buffer_barrier(buf.get(), AccessTransferWrite, u2a(BufferUsageIndex), PipelineStageTransfer, u2s(BufferUsageIndex));
					buf_top = stag_top;
				}
			}
		};

		template<uint id, BufferUsageFlags usage, bool rewind = true, bool sparse = false>
		struct StorageBuffer : VirtualUdt<id>
		{
			using VirtualUdt<id>::ui;
			using VirtualUdt<id>::var_off;
			using VirtualUdt<id>::set_var;

			uint size = 0;
			uint array_capacity = 0;

			std::unique_ptr<BufferT> buf;
			std::unique_ptr<BufferT> stagbuf;
			std::vector<BufferCopy> copies;
			std::deque<uint> free_slots;
			char* pbeg;
			char* pend;

			void create(uint _size, uint _array_capacity = 1)
			{
				size = usage == BufferUsageIndirect ? sizeof(graphics::DrawIndexedIndirectCommand) : _size;
				array_capacity = _array_capacity;
				buf.reset(Buffer::create(array_capacity * size, BufferUsageTransferDst | usage, MemoryPropertyDevice));
				stagbuf.reset(Buffer::create(buf->size, BufferUsageTransferSrc, MemoryPropertyHost | MemoryPropertyCoherent));
				stagbuf->map();
				pbeg = pend = (char*)stagbuf->mapped;

				if (sparse)
				{
					free_slots.resize(_array_capacity);
					std::iota(free_slots.begin(), free_slots.end(), 0);
				}
			}

			void create(UdtInfo* _ui, uint _array_capacity = 1, uint _size = 0)
			{
				ui = _ui;
				create(_size > 0 ? _size : ui->size, _array_capacity);
			}

			void create_with_array_type(UdtInfo* _ui)
			{
				auto& vi = _ui->variables[0];
				auto vui = vi.type->retrive_ui();
				if (vui)
					create(vi.type->retrive_ui(), vi.array_size, vi.array_stride);
				else
					create(vi.array_stride, vi.array_size);
			}

			inline uint item_offset()
			{
				assert(!sparse);
				return (pend - (char*)stagbuf->mapped) / size;
			}

			inline void next_item()
			{
				assert(!sparse);
				pend += size;
			}

			inline void push(uint n, void* d)
			{
				auto s = n * size;
				memcpy(pend, d, s);
				pend += s;
			}

			inline int get_free_item()
			{
				assert(sparse);
				if (free_slots.empty())
					return -1;
				auto ret = free_slots.front();
				free_slots.pop_front();
				return ret;
			}

			inline void release_item(uint id)
			{
				assert(sparse);
				free_slots.push_back(id);
			}

			inline void select_item(uint off, bool mark_dirty = true)
			{
				assert(sparse);
				pend = (char*)stagbuf->mapped + off * size;
				if (mark_dirty)
				{
					BufferCopy cpy;
					cpy.src_off = cpy.dst_off = pend - (char*)stagbuf->mapped;
					cpy.size = size;
					copies.push_back(cpy);
				}
			}

			template<typename T>
			inline void set_item(const T& v)
			{
				*(T*)pend = v;
			}

			template<uint nh, typename T>
			inline void set_var(const T& v)
			{
				VirtualUdt<id>::set_var<nh>(pend, v);
			}

			template<uint nh>
			inline char* var_addr()
			{
				return pend + var_off<nh>();
			}

			inline void add_draw_indirect(uint vertex_count, uint first_vertex = 0, uint instance_count = 1, uint first_instance = 0)
			{
				assert(usage == BufferUsageIndirect);
				DrawIndirectCommand c;
				c.vertex_count = vertex_count;
				c.instance_count = instance_count;
				c.first_vertex = first_vertex;
				c.first_instance = first_instance;
				set_item(c);
				next_item();
			}

			inline void add_draw_indexed_indirect(uint index_count, uint first_index = 0, int vertex_offset = 0, uint instance_count = 1, uint first_instance = 0)
			{
				assert(usage == BufferUsageIndirect);
				DrawIndexedIndirectCommand c;
				c.index_count = index_count;
				c.instance_count = instance_count;
				c.first_index = first_index;
				c.vertex_offset = vertex_offset;
				c.first_instance = first_instance;
				set_item(c);
				next_item();
			}

			void upload(CommandBufferPtr cb)
			{
				if (sparse)
				{
					if (copies.empty())
						return;
					cb->copy_buffer(stagbuf.get(), buf.get(), copies);
					copies.clear();
				}
				else
				{
					BufferCopy cpy;
					if (array_capacity > 1)
					{
						cpy.size = pend - pbeg;
						if (rewind)
							pend = pbeg;
						else
						{
							cpy.src_off = cpy.dst_off = pbeg - (char*)stagbuf->mapped;
							pbeg = pend;
						}
					}
					else
						cpy.size = size;
					if (cpy.size == 0)
						return;
					cb->copy_buffer(stagbuf.get(), buf.get(), { &cpy, 1 });
				}
				cb->buffer_barrier(buf.get(), AccessTransferWrite, u2a(usage), PipelineStageTransfer, u2s(usage));
			}

			void upload_whole(CommandBufferPtr cb)
			{
				BufferCopy cpy;
				cpy.size = size * array_capacity;
				cb->copy_buffer(stagbuf.get(), buf.get(), { &cpy, 1 });
				cb->buffer_barrier(buf.get(), AccessTransferWrite, u2a(usage), PipelineStageTransfer, u2s(usage));
			}
		};

		struct PipelineResourceManager
		{
			PipelineLayoutPtr pll = nullptr;
			PipelineType plt = PipelineGraphics;
			std::unordered_map<uint, int> dsl_map;

			DescriptorSetPtr temp_dss[8];
			VirtualStruct pc;

			void init(PipelineLayoutPtr _pll, PipelineType _plt = PipelineGraphics)
			{
				pll = _pll;
				plt = _plt;
				for (auto i = 0; i < pll->dsls.size(); i++)
				{
					auto dsl = pll->dsls[i];

					std::string name;
					if (dsl->filename != pll->filename)
						name = dsl->filename.stem().string();
					else
						name = "";
					dsl_map.emplace(sh(name.c_str()), i);
				}

				pc.init(pll->pc_ui);

				memset(temp_dss, 0, sizeof(temp_dss));
			}

			inline int dsl_idx(uint nh)
			{
				auto it = dsl_map.find(nh);
				if (it == dsl_map.end())
				{
					assert(0);
					return -1;
				}
				return it->second;
			}

			inline DescriptorSetLayoutPtr get_dsl(uint nh)
			{
				auto idx = dsl_idx(nh);
				return idx == -1 ? nullptr : pll->dsls[idx];
			}

			inline void set_ds(uint nh, DescriptorSetPtr ds)
			{
				auto idx = dsl_idx(nh);
				if (idx != -1)
					temp_dss[idx] = ds;
			}

			inline void bind_dss(CommandBufferPtr cb, uint off = 0, uint count = 0xffffffff)
			{
				for (auto i = 0U; i < count; i++)
				{
					auto ii = off + i;
					if (ii >= dsl_map.size() || ii >= _countof(temp_dss) || !temp_dss[ii])
					{
						count = i;
						break;
					}
				}
				if (count > 0)
				{
					cb->bind_pipeline_layout(pll, plt);
					cb->bind_descriptor_sets(off, { temp_dss + off, count });
				}
			}

			inline void push_constant(CommandBufferPtr cb)
			{
				cb->bind_pipeline_layout(pll);
				if (!pc.dirty_regions.empty())
				{
					for (auto& r : pc.dirty_regions)
						cb->push_constant(r.first, r.second, pc.data.get() + r.first);
					pc.dirty_regions.clear();
				}
				else
					cb->push_constant(0, pll->pc_sz, pc.data.get());
			}
		};
	}
}

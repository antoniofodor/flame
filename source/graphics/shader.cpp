#include "../xml.h"
#include "../foundation/typeinfo.h"
#include "../foundation/typeinfo_serialize.h"
#include "../foundation/system.h"
#include "device_private.h"
#include "command_private.h"
#include "renderpass_private.h"
#include "buffer_private.h"
#include "image_private.h"
#include "shader_private.h"

#include <spirv_glsl.hpp>

namespace flame
{
	namespace graphics
	{
		TypeInfo* get_shader_type(const spirv_cross::CompilerGLSL& glsl, const spirv_cross::SPIRType& src, TypeInfoDataBase& db)
		{
			TypeInfo* ret = nullptr;

			if (src.basetype == spirv_cross::SPIRType::Struct)
			{
				auto name = glsl.get_name(src.self);
				auto size = glsl.get_declared_struct_size(src);
				{
					auto m = size % 16;
					if (m != 0)
						size += (16 - m);
				}

				auto& ui = db.udts.emplace(name, UdtInfo()).first->second;
				ui.name = name;
				ui.size = size;

				ret = TypeInfo::get(TagData, name, db);

				for (auto i = 0; i < src.member_types.size(); i++)
				{
					auto id = src.member_types[i];

					auto type = get_shader_type(glsl, glsl.get_type(id), db);
					auto name = glsl.get_member_name(src.self, i);
					auto offset = glsl.type_struct_member_offset(src, i);
					auto arr_size = glsl.get_type(id).array[0];
					auto arr_stride = glsl.get_decoration(id, spv::DecorationArrayStride);
					if (arr_stride == 0)
						arr_size = 1;
					auto& vi = ui.variables.emplace_back();
					vi.type = type;
					vi.name = name;
					vi.offset = offset;
					vi.array_size = arr_size;
					vi.array_stride = arr_stride;
				}
			}
			else if (src.basetype == spirv_cross::SPIRType::Image || src.basetype == spirv_cross::SPIRType::SampledImage)
				ret = TypeInfo::get(TagPointer, "ShaderImage", db);
			else
			{
				switch (src.basetype)
				{
				case spirv_cross::SPIRType::Int:
					switch (src.columns)
					{
					case 1:
						switch (src.vecsize)
						{
						case 1: ret = TypeInfo::get<int>();		break;
						case 2: ret = TypeInfo::get<ivec2>();	break;
						case 3: ret = TypeInfo::get<ivec3>();	break;
						case 4: ret = TypeInfo::get<ivec4>();	break;
						default: assert(0);
						}
						break;
					default:
						assert(0);
					}
					break;
				case spirv_cross::SPIRType::UInt:
					switch (src.columns)
					{
					case 1:
						switch (src.vecsize)
						{
						case 1: ret = TypeInfo::get<uint>();	break;
						case 2: ret = TypeInfo::get<uvec2>();	break;
						case 3: ret = TypeInfo::get<uvec3>();	break;
						case 4: ret = TypeInfo::get<uvec4>();	break;
						default: assert(0);
						}
						break;
					default:
						assert(0);
					}
					break;
				case spirv_cross::SPIRType::Float:
					switch (src.columns)
					{
					case 1:
						switch (src.vecsize)
						{
						case 1: ret = TypeInfo::get<float>();	break;
						case 2: ret = TypeInfo::get<vec2>();	break;
						case 3: ret = TypeInfo::get<vec3>();	break;
						case 4: ret = TypeInfo::get<vec4>();	break;
						default: assert(0);
						}
						break;
					case 2:
						switch (src.vecsize)
						{
						case 2: ret = TypeInfo::get<mat2>(); break;
						default: assert(0);
						}
						break;
					case 3:
						switch (src.vecsize)
						{
						case 3: ret = TypeInfo::get<mat3>(); break;
						default: assert(0);
						}
						break;
					case 4:
						switch (src.vecsize)
						{
						case 4: ret = TypeInfo::get<mat4>(); break;
						default: assert(0);
						}
						break;
					default:
						assert(0);
					}
					break;
				}
			}

			return ret;
		}

		static void write_udts_to_header(std::string& header, TypeInfoDataBase& db)
		{
			for (auto& ui : db.udts)
			{
				header += "\tstruct " + ui.second.name + "\n\t{\n";
				auto dummy_id = 0;
				auto push_dummy = [&](int d) {
					switch (d)
					{
					case 4:
						header += "\t\tfloat dummy_" + std::to_string(dummy_id) + ";\n";
						break;
					case 8:
						header += "\t\tvec2 dummy_" + std::to_string(dummy_id) + ";\n";
						break;
					case 12:
						header += "\t\tvec3 dummy_" + std::to_string(dummy_id) + ";\n";
						break;
					default:
						assert(0);
					}
				};
				auto off = 0;
				for (auto& vi : ui.second.variables)
				{
					auto off2 = (int)vi.offset;
					if (off != off2)
					{
						push_dummy(off2 - off);
						off = off2;
						dummy_id++;
					}

					auto type_name = vi.type->name;
					SUS::cut_head_if(type_name, "glm::");

					header += "\t\t" + type_name + " " + vi.name;
					auto size = vi.type->size;
					auto array_size = vi.array_size;
					if (array_size > 1)
					{
						assert(size == vi.array_stride);
						header += "[" + std::to_string(array_size) + "]";
					}
					header += ";\n";
					off += size * array_size;
				}
				auto size = (int)ui.second.size;
				if (off != size)
					push_dummy(size - off);
				header += "\t};\n\n";
			}
		}

		static std::string basic_glsl_prefix()
		{
			std::string ret;
			ret += "#version 450 core\n";
			ret += "#extension GL_ARB_shading_language_420pack : enable\n";
			ret += "#extension GL_ARB_separate_shader_objects : enable\n\n";
			return ret;
		}

		static std::string add_lineno_to_code(const std::string& temp)
		{
			auto lines = SUS::split(temp, '\n');
			auto ret = std::string();
			auto ndigits = (int)log10(lines.size()) + 1;
			for (auto i = 0; i < lines.size(); i++)
			{
				char buf[32];
				sprintf(buf, "%*d", ndigits, i + 1);
				ret += std::string(buf) + "    " + lines[i] + "\n";
			}
			return ret;
		}

		bool compile_shader(const std::filesystem::path& src_path, const std::filesystem::path& dst_path)
		{
			auto up_to_date = true;
			if (!std::filesystem::exists(dst_path))
				up_to_date = false;
			else
			{
				auto dst_date = std::filesystem::last_write_time(dst_path);
				if (std::filesystem::last_write_time(src_path) > dst_date)
					up_to_date = false;
				else
				{
					std::vector<std::filesystem::path> dependencies;

					std::ifstream dst(dst_path);
					while (!dst.eof())
					{
						std::string line;
						std::getline(dst, line);
						if (line.empty())
							break;
						if (line == "Dependencies:")
						{
							while (!dst.eof())
							{
								std::getline(dst, line);
								if (line.empty())
									break;
								dependencies.push_back(line);
							}
							break;
						}
					}
					dst.close();

					for (auto& d : dependencies)
					{
						if (std::filesystem::last_write_time(d) > dst_date)
						{
							up_to_date = false;
							break;
						}
					}
				}
			}

			if (up_to_date)
				return false;

			auto dst_ppath = dst_path.parent_path();
			if (!std::filesystem::exists(dst_ppath))
				std::filesystem::create_directories(dst_ppath);

			std::ofstream dst(dst_path);

			{
				std::vector<std::filesystem::path> dependencies;
				std::list<std::filesystem::path> headers;
				headers.push_back(src_path);
				while (!headers.empty())
				{
					auto fn = headers.front();
					headers.pop_front();

					if (!std::filesystem::exists(fn))
						continue;

					if (dependencies.end() == std::find(dependencies.begin(), dependencies.end(), fn))
						dependencies.push_back(fn);

					std::ifstream file(fn);
					auto ppath = fn.parent_path();
					while (!file.eof())
					{
						std::string line;
						std::getline(file, line);
						if (!line.empty() && line[0] != '#')
							break;
						if (SUS::cut_head_if(line, "#include "))
							headers.push_back(ppath / line.substr(1, line.size() - 2));
					}
					file.close();
				}

				dst << "Dependencies:" << std::endl;
				for (auto& d : dependencies)
					dst << d.string() << std::endl;
				dst << std::endl;
			}

			auto vk_sdk_path = getenv("VK_SDK_PATH");
			if (!vk_sdk_path)
			{
				printf("cannot find vk sdk\n");
				return false;
			}

			std::ofstream code("temp.glsl");
			code << "#version 450 core" << std::endl;
			code << "#extension GL_ARB_shading_language_420pack : enable" << std::endl;
			code << "#extension GL_ARB_separate_shader_objects : enable" << std::endl;
			code << std::endl;

			std::ifstream src(src_path);
			auto src_ext = src_path.extension();
			std::wstring stage;
			if (src_ext == L".dsl")
			{
				code << "#ifndef SET" << std::endl;
				code << "#define SET 0" << std::endl;
				code << "#endif" << std::endl;
				while (!src.eof())
				{
					std::string line;
					std::getline(src, line);
					code << line << std::endl;
				}
				code << std::endl;
				code << "void main() {}" << std::endl;
				code << std::endl;

				stage = L"frag";
			}
			else if (src_ext == L".pll")
			{
				while (!src.eof())
				{
					std::string line;
					std::getline(src, line);
					static std::regex reg("#include\\s+.([\\w\\/\\.]+)");
					if (std::regex_search(line, reg))
						continue;
					code << line << std::endl;
				}
				code << std::endl;
				code << "void main() {}" << std::endl;
				code << std::endl;

				stage = L"frag";
			}
			else
			{
				while (!src.eof())
				{
					std::string line;
					std::getline(src, line);
					std::smatch res;
					static std::regex reg("#include\\s+.([\\w\\/\\.]+\\.pll)");
					if (std::regex_search(line, res, reg))
					{
						code << std::endl;

						auto set = 0;
						std::ifstream pll(src_path.parent_path() / res[1].str());
						while (!pll.eof())
						{
							std::getline(pll, line);
							static std::regex reg("#include\\s+.([\\w\\/\\.]+\\.dsl)");
							if (std::regex_search(line, reg))
							{
								code << "#undef SET" << std::endl;
								code << "#define SET " << std::to_string(set++) << std::endl;
							}
							code << line << std::endl;
						}
						pll.close();
						continue;
					}
					code << line << std::endl;
				}

				stage = src_ext.wstring().substr(1);
			}
			src.close();

			code.close();

			wprintf(L"compiling: %s\n", src_path.c_str());
			wprintf(L"   with defines: \n");
			wprintf(L"   with substitutes: \n");
			std::filesystem::remove(L"temp.spv");
			std::string errors;
			exec(std::filesystem::path(vk_sdk_path) / L"Bin/glslc.exe", L" -fshader-stage=" + stage + L" temp.glsl -o temp.spv", &errors);
			if (!std::filesystem::exists(L"temp.spv"))
			{
				printf("%s\n", errors.c_str());
				shell_exec(L"temp.glsl", L"", false, true);
				assert(0);
				return false;
			}
			printf(" - done\n");
			std::filesystem::remove(L"temp.glsl");

			auto spv = get_file_content(L"temp.spv");
			auto spv_array = std::vector<uint>(spv.size() / 4);
			memcpy(spv_array.data(), spv.data(), spv_array.size() * sizeof(uint));

			TypeInfoDataBase db;
			auto spvcross_compiler = spirv_cross::CompilerGLSL(spv_array.data(), spv_array.size());
			auto spvcross_resources = spvcross_compiler.get_shader_resources();

			if (src_ext == L".dsl")
			{
				std::vector<DescriptorBinding> bindings;

				auto get_binding = [&](spirv_cross::Resource& r, DescriptorType type) {
					get_shader_type(spvcross_compiler, spvcross_compiler.get_type(r.base_type_id), db);

					auto binding = spvcross_compiler.get_decoration(r.id, spv::DecorationBinding);
					if (bindings.size() <= binding)
						bindings.resize(binding + 1);

					auto& b = bindings[binding];
					b.type = type;
					b.count = spvcross_compiler.get_type(r.type_id).array[0];
					b.name = r.name;
					if (type == DescriptorUniformBuffer || type == DescriptorStorageBuffer)
						b.ti = find_udt(spvcross_compiler.get_name(r.base_type_id), db);
				};

				for (auto& r : spvcross_resources.uniform_buffers)
					get_binding(r, DescriptorUniformBuffer);
				for (auto& r : spvcross_resources.storage_buffers)
					get_binding(r, DescriptorStorageBuffer);
				for (auto& r : spvcross_resources.sampled_images)
					get_binding(r, DescriptorSampledImage);
				for (auto& r : spvcross_resources.storage_images)
					get_binding(r, DescriptorStorageImage);

				dst << "DSL:" << std::endl;
				serialize_text(&bindings, dst);
				dst << std::endl;
			}

			if (src_ext == L".pll")
			{

				//		for (auto& r : spvcross_resources.push_constant_buffers)
				//		{
				//			get_shader_type(spvcross_compiler, spvcross_compiler.get_type(r.base_type_id), db);
				//			pc_ti = find_udt(spvcross_compiler.get_name(r.base_type_id).c_str(), db);
				//			break;
				//		}

				dst << "PLL:" << std::endl;
				dst << std::endl;
			}

			if (src_ext != L".dsl" && src_ext != L".pll")
			{
				dst << "Spv:" << std::endl;

				for (auto i = 0; i < spv_array.size(); i++)
				{
					dst << spv_array[i] << " ";
					if (i == 99)
						dst << std::endl;
				}

				dst << std::endl;
			}

			dst.close();
			return true;
		}

		DescriptorPoolPrivate::~DescriptorPoolPrivate()
		{
			vkDestroyDescriptorPool(device->vk_device, vk_descriptor_pool, nullptr);
		}

		struct DescriptorPoolCurrent : DescriptorPool::Current
		{
			DescriptorPoolPtr operator()(DevicePtr device) override
			{
				if (!device)
					device = current_device;

				return device->dsp.get();
			}
		}DescriptorPool_current;
		DescriptorPool::Current& DescriptorPool::current = DescriptorPool_current;


		struct DescriptorPoolCreate : DescriptorPool::Create
		{
			DescriptorPoolPtr operator()(DevicePtr device) override
			{
				if (!device)
					device = current_device;

				auto ret = new DescriptorPoolPrivate;
				ret->device = device;

				VkDescriptorPoolSize descriptorPoolSizes[] = {
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8 },
				};

				VkDescriptorPoolCreateInfo descriptorPoolInfo;
				descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				descriptorPoolInfo.pNext = nullptr;
				descriptorPoolInfo.poolSizeCount = _countof(descriptorPoolSizes);
				descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;
				descriptorPoolInfo.maxSets = 128;
				chk_res(vkCreateDescriptorPool(device->vk_device, &descriptorPoolInfo, nullptr, &ret->vk_descriptor_pool));

				return ret;
			}
		}DescriptorPool_create;
		DescriptorPool::Create& DescriptorPool::create = DescriptorPool_create;

		DescriptorSetLayoutPrivate::~DescriptorSetLayoutPrivate()
		{
			vkDestroyDescriptorSetLayout(device->vk_device, vk_descriptor_set_layout, nullptr);
		}

		struct DescriptorSetLayoutGet : DescriptorSetLayout::Get
		{
			DescriptorSetLayoutPtr operator()(DevicePtr device, const std::filesystem::path& _filename) override
			{
				if (!device)
					device = current_device;

				auto filename = Path::get(_filename);
				filename.make_preferred();

				if (device)
				{
					for (auto& d : device->dsls)
					{
						if (d->filename.filename() == filename)
							return d.get();
					}
				}

				if (!std::filesystem::exists(filename))
				{
					wprintf(L"cannot find dsl: %s\n", _filename.c_str());
					return nullptr;
				}

				auto dst_path = filename;
				dst_path += L".res";
				compile_shader(filename, dst_path);

				//auto res_path = filename.parent_path() / L"build";
				//if (!std::filesystem::exists(res_path))
				//	std::filesystem::create_directories(res_path);
				//res_path /= filename.filename();
				//res_path += L".res";

				//auto ti_path = res_path;
				//ti_path.replace_extension(L".typeinfo");

				std::vector<DescriptorBinding> bindings;
				TypeInfoDataBase db;

				//if (!std::filesystem::exists(res_path) || std::filesystem::last_write_time(res_path) < std::filesystem::last_write_time(filename) ||
				//	!std::filesystem::exists(ti_path) || std::filesystem::last_write_time(ti_path) < std::filesystem::last_write_time(filename))
				//{
				//	auto vk_sdk_path = getenv("VK_SDK_PATH");
				//	if (vk_sdk_path)
				//	{
				//		auto temp = basic_glsl_prefix();
				//		temp += "#define MAKE_DSL\n";
				//		std::ifstream dsl(filename);
				//		while (!dsl.eof())
				//		{
				//			std::string line;
				//			std::getline(dsl, line);
				//			temp += line + "\n";
				//		}
				//		temp += "void main()\n{\n}\n";
				//		dsl.close();

				//		auto temp_fn = filename;
				//		temp_fn.replace_filename(L"temp.frag");
				//		std::ofstream temp_file(temp_fn);
				//		temp_file << temp << std::endl;
				//		temp_file.close();

				//		std::filesystem::remove(L"temp.spv");

				//		auto glslc_path = std::filesystem::path(vk_sdk_path);
				//		glslc_path /= L"Bin/glslc.exe";

				//		auto command_line = L" " + temp_fn.wstring();

				//		printf("compiling dsl: %s", filename.string().c_str());

				//		std::string output;
				//		exec(glslc_path, command_line, &output);
				//		if (!std::filesystem::exists(L"temp.spv"))
				//		{
				//			temp = add_lineno_to_code(temp);
				//			printf("\n========\n%s\n========\n%s\n", temp.c_str(), output.c_str());
				//			return nullptr;
				//		}

				//		printf(" done\n");

				//		std::filesystem::remove(temp_fn);

				//		auto spv = get_file_content(L"temp.spv");
				//		std::filesystem::remove(L"temp.spv");
				//		auto glsl = spirv_cross::CompilerGLSL((uint*)spv.c_str(), spv.size() / sizeof(uint));
				//		auto resources = glsl.get_shader_resources();

				//		auto get_binding = [&](spirv_cross::Resource& r, DescriptorType type) {
				//			get_shader_type(glsl, glsl.get_type(r.base_type_id), db);

				//			auto binding = glsl.get_decoration(r.id, spv::DecorationBinding);
				//			if (bindings.size() <= binding)
				//				bindings.resize(binding + 1);

				//			auto& b = bindings[binding];
				//			b.type = type;
				//			b.count = glsl.get_type(r.type_id).array[0];
				//			b.name = r.name;
				//			if (type == DescriptorUniformBuffer || type == DescriptorStorageBuffer)
				//				b.ti = find_udt(glsl.get_name(r.base_type_id), db);
				//		};

				//		for (auto& r : resources.uniform_buffers)
				//			get_binding(r, DescriptorUniformBuffer);
				//		for (auto& r : resources.storage_buffers)
				//			get_binding(r, DescriptorStorageBuffer);
				//		for (auto& r : resources.sampled_images)
				//			get_binding(r, DescriptorSampledImage);
				//		for (auto& r : resources.storage_images)
				//			get_binding(r, DescriptorStorageImage);

				//		db.save_typeinfo(ti_path);

				//		pugi::xml_document res;
				//		auto root = res.append_child("res");

				//		auto n_bindings = root.append_child("bindings");
				//		for (auto i = 0; i < bindings.size(); i++)
				//		{
				//			auto& b = bindings[i];
				//			if (b.type != Descriptor_Max)
				//			{
				//				auto n_binding = n_bindings.append_child("binding");
				//				n_binding.append_attribute("type").set_value(TypeInfo::serialize_t(&b.type).c_str());
				//				n_binding.append_attribute("binding").set_value(i);
				//				n_binding.append_attribute("count").set_value(b.count);
				//				n_binding.append_attribute("name").set_value(b.name.c_str());
				//				if (b.type == DescriptorUniformBuffer || b.type == DescriptorStorageBuffer)
				//					n_binding.append_attribute("type_name").set_value(b.ti->name.c_str());
				//			}
				//		}

				//		res.save_file(res_path.c_str());
				//	}
				//	else
				//	{
				//		printf("cannot find vk sdk\n");
				//		return nullptr;
				//	}
				//}
				//else
				//{
				//	auto ti_path = res_path;
				//	ti_path.replace_extension(L".typeinfo");
				//	db.load_typeinfo(ti_path);

				//	pugi::xml_document res;
				//	pugi::xml_node root;
				//	if (!res.load_file(res_path.c_str()) || (root = res.first_child()).name() != std::string("res"))
				//	{
				//		printf("res file wrong format\n");
				//		return nullptr;
				//	}

				//	for (auto n_binding : root.child("bindings"))
				//	{
				//		auto binding = n_binding.attribute("binding").as_int();
				//		if (binding != -1)
				//		{
				//			if (binding >= bindings.size())
				//				bindings.resize(binding + 1);
				//			auto& b = bindings[binding];
				//			TypeInfo::unserialize_t(n_binding.attribute("type").value(), &b.type);
				//			b.count = n_binding.attribute("count").as_uint();
				//			b.name = n_binding.attribute("name").value();
				//			if (b.type == DescriptorUniformBuffer || b.type == DescriptorStorageBuffer)
				//				b.ti = find_udt(n_binding.attribute("type_name").value(), db);
				//		}
				//	}
				//}

				//auto header_path = filename;
				//header_path += L".h";
				//if (!std::filesystem::exists(header_path) || std::filesystem::last_write_time(header_path) < std::filesystem::last_write_time(ti_path))
				//{
				//	std::string header;
				//	header += "namespace DSL_" + filename.filename().stem().string() + "\n{\n";
				//	write_udts_to_header(header, db);
				//	auto idx = 0;
				//	for (auto& b : bindings)
				//	{
				//		header += "\tinline uint " + b.name + "_binding = " + std::to_string(idx) + ";\n";
				//		header += "\tinline uint " + b.name + "_count = " + std::to_string(b.count) + ";\n";
				//		idx++;
				//	}
				//	header += "}\n";
				//	std::ofstream file(header_path);
				//	file << header;
				//	file.close();
				//}

				if (device)
				{
					auto ret = DescriptorSetLayout::create(device, bindings);
					ret->db = std::move(db);
					ret->filename = filename;
					device->dsls.emplace_back(ret);
					return ret;
				}
				return nullptr;
			}
		}DescriptorSetLayout_get;
		DescriptorSetLayout::Get& DescriptorSetLayout::get = DescriptorSetLayout_get;

		struct DescriptorSetLayoutCreate : DescriptorSetLayout::Create
		{
			DescriptorSetLayoutPtr operator()(DevicePtr device, std::span<DescriptorBinding> bindings) override
			{
				if (!device)
					device = current_device;

				auto ret = new DescriptorSetLayoutPrivate;
				ret->device = device;
				ret->bindings.assign(bindings.begin(), bindings.end());

				std::vector<VkDescriptorSetLayoutBinding> vk_bindings(bindings.size());
				for (auto i = 0; i < bindings.size(); i++)
				{
					auto& src = bindings[i];
					auto& dst = vk_bindings[i];

					dst.binding = i;
					dst.descriptorType = to_backend(src.type);
					dst.descriptorCount = src.count;
					dst.stageFlags = to_backend_flags<ShaderStageFlags>(ShaderStageAll);
					dst.pImmutableSamplers = nullptr;
				}

				VkDescriptorSetLayoutCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				info.flags = 0;
				info.pNext = nullptr;
				info.bindingCount = vk_bindings.size();
				info.pBindings = vk_bindings.data();

				chk_res(vkCreateDescriptorSetLayout(device->vk_device, &info, nullptr, &ret->vk_descriptor_set_layout));

				return ret;
			}
		}DescriptorSetLayout_create;
		DescriptorSetLayout::Create& DescriptorSetLayout::create = DescriptorSetLayout_create;

		DescriptorSetPrivate::~DescriptorSetPrivate()
		{
			chk_res(vkFreeDescriptorSets(device->vk_device, pool->vk_descriptor_pool, 1, &vk_descriptor_set));
		}

		void DescriptorSetPrivate::set_buffer(uint binding, uint index, BufferPtr buf, uint offset, uint range)
		{
			if (binding >= reses.size() || index >= reses[binding].size())
				return;

			auto& res = reses[binding][index].b;
			if (res.p == buf && res.offset == offset && res.range == range)
				return;

			res.p = buf;
			res.offset = offset;
			res.range = range;

			buf_updates.emplace_back(binding, index);
		}

		void DescriptorSetPrivate::set_image(uint binding, uint index, ImageViewPtr iv, SamplerPtr sp)
		{
			if (binding >= reses.size() || index >= reses[binding].size())
				return;

			auto& res = reses[binding][index].i;
			if (res.p == iv && res.sp == sp)
				return;

			res.p = iv;
			res.sp = sp;

			img_updates.emplace_back(binding, index);
		}

		void DescriptorSetPrivate::update()
		{
			if (buf_updates.empty() && img_updates.empty())
				return;

			Queue::get(device)->wait_idle();
			std::vector<VkDescriptorBufferInfo> vk_buf_infos;
			std::vector<VkDescriptorImageInfo> vk_img_infos;
			std::vector<VkWriteDescriptorSet> vk_writes;
			vk_buf_infos.resize(buf_updates.size());
			vk_img_infos.resize(img_updates.size());
			vk_writes.resize(buf_updates.size() + img_updates.size());
			auto idx = 0;
			for (auto i = 0; i < vk_buf_infos.size(); i++)
			{
				auto& u = buf_updates[i];
				auto& res = reses[u.first][u.second];

				auto& info = vk_buf_infos[i];
				info.buffer = res.b.p->vk_buffer;
				info.offset = res.b.offset;
				info.range = res.b.range == 0 ? res.b.p->size : res.b.range;

				auto& wrt = vk_writes[idx];
				wrt.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wrt.pNext = nullptr;
				wrt.dstSet = vk_descriptor_set;
				wrt.dstBinding = u.first;
				wrt.dstArrayElement = u.second;
				wrt.descriptorType = to_backend(layout->bindings[u.first].type);
				wrt.descriptorCount = 1;
				wrt.pBufferInfo = &info;
				wrt.pImageInfo = nullptr;
				wrt.pTexelBufferView = nullptr;

				idx++;
			}
			for (auto i = 0; i < vk_img_infos.size(); i++)
			{
				auto& u = img_updates[i];
				auto& res = reses[u.first][u.second];

				auto& info = vk_img_infos[i];
				info.imageView = res.i.p->vk_image_view;
				info.imageLayout = layout->bindings[u.first].type == DescriptorSampledImage ?
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
				info.sampler = res.i.sp ? res.i.sp->vk_sampler : nullptr;

				auto& wrt = vk_writes[idx];
				wrt.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wrt.pNext = nullptr;
				wrt.dstSet = vk_descriptor_set;
				wrt.dstBinding = u.first;
				wrt.dstArrayElement = u.second;
				wrt.descriptorType = to_backend(layout->bindings[u.first].type);
				wrt.descriptorCount = 1;
				wrt.pBufferInfo = nullptr;
				wrt.pImageInfo = &info;
				wrt.pTexelBufferView = nullptr;

				idx++;
			}

			buf_updates.clear();
			img_updates.clear();

			vkUpdateDescriptorSets(device->vk_device, vk_writes.size(), vk_writes.data(), 0, nullptr);
		}

		struct DescriptorSetCreate : DescriptorSet::Create
		{
			DescriptorSetPtr operator()(DescriptorPoolPtr pool, DescriptorSetLayoutPtr layout) override
			{
				if (!pool)
					pool = DescriptorPool::current();

				auto ret = new DescriptorSetPrivate;
				ret->pool = pool;
				ret->layout = layout;

				ret->reses.resize(layout->bindings.size());
				for (auto i = 0; i < ret->reses.size(); i++)
					ret->reses[i].resize(max(1U, layout->bindings[i].count));

				VkDescriptorSetAllocateInfo info;
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				info.pNext = nullptr;
				info.descriptorPool = pool->vk_descriptor_pool;
				info.descriptorSetCount = 1;
				info.pSetLayouts = &layout->vk_descriptor_set_layout;

				chk_res(vkAllocateDescriptorSets(pool->device->vk_device, &info, &ret->vk_descriptor_set));

				return ret;
			}
		}DescriptorSet_create;
		DescriptorSet::Create& DescriptorSet::create = DescriptorSet_create;

		PipelineLayoutPrivate::~PipelineLayoutPrivate()
		{
			vkDestroyPipelineLayout(device->vk_device, vk_pipeline_layout, nullptr);
		}

		struct PipelineLayoutGet : PipelineLayout::Get
		{
			PipelineLayoutPtr operator()(DevicePtr device, const std::filesystem::path& _filename) override
			{
				if (!device)
					device = current_device;

				auto filename = Path::get(_filename);
				filename.make_preferred();

				if (device)
				{
					for (auto& p : device->plls)
					{
						if (p->filename == filename)
							return p.get();
					}
				}

				if (!std::filesystem::exists(filename))
				{
					wprintf(L"cannot find pll: %s\n", _filename.c_str());
					return nullptr;
				}

				auto dst_path = filename;
				dst_path += L".res";
				compile_shader(filename, dst_path);

				//auto res_path = filename.parent_path() / L"build";
				//if (!std::filesystem::exists(res_path))
				//	std::filesystem::create_directories(res_path);
				//res_path /= filename.filename();
				//res_path += L".res";

				//auto ti_path = res_path;
				//ti_path.replace_extension(L".typeinfo");

				std::vector<DescriptorSetLayoutPrivate*> dsls;

				//auto ppath = filename.parent_path();
				//auto dependencies = get_make_dependencies(filename);
				//for (auto& d : dependencies)
				//{
				//	if (d.extension() == L".dsl")
				//		dsls.push_back(DescriptorSetLayoutPrivate::get(device, d));
				//	else
				//		d.clear();
				//}

				TypeInfoDataBase db;
				UdtInfo* pc_ti = nullptr;

				//if (!std::filesystem::exists(res_path) || std::filesystem::last_write_time(res_path) < std::filesystem::last_write_time(filename) ||
				//	!std::filesystem::exists(ti_path) || std::filesystem::last_write_time(ti_path) < std::filesystem::last_write_time(filename))
				//{
				//	auto vk_sdk_path = getenv("VK_SDK_PATH");
				//	if (vk_sdk_path)
				//	{
				//		auto temp = basic_glsl_prefix();
				//		temp += "#define MAKE_PLL\n";
				//		std::ifstream pll(filename);
				//		while (!pll.eof())
				//		{
				//			std::string line;
				//			std::getline(pll, line);
				//			temp += line + "\n";
				//		}
				//		temp += "void main()\n{\n}\n";
				//		pll.close();

				//		auto temp_fn = filename;
				//		temp_fn.replace_filename(L"temp.frag");
				//		std::ofstream temp_file(temp_fn);
				//		temp_file << temp << std::endl;
				//		temp_file.close();

				//		std::filesystem::remove(L"temp.spv");

				//		auto command_line = L" " + temp_fn.wstring();

				//		printf("compiling pll: %s", filename.string().c_str());

				//		std::string output;
				//		exec((std::filesystem::path(vk_sdk_path) / L"Bin/glslc.exe").c_str(), command_line, &output);
				//		std::filesystem::remove(temp_fn);
				//		if (!std::filesystem::exists(L"temp.spv"))
				//		{
				//			temp = add_lineno_to_code(temp);
				//			printf("\n========\n%s\n========\n%s\n", temp.c_str(), output.c_str());
				//			return nullptr;
				//		}

				//		printf(" done\n");

				//		auto spv = get_file_content(L"temp.spv");
				//		std::filesystem::remove(L"temp.spv");
				//		auto glsl = spirv_cross::CompilerGLSL((uint*)spv.c_str(), spv.size() / sizeof(uint));
				//		auto resources = glsl.get_shader_resources();

				//		for (auto& r : resources.push_constant_buffers)
				//		{
				//			get_shader_type(glsl, glsl.get_type(r.base_type_id), db);
				//			pc_ti = find_udt(glsl.get_name(r.base_type_id).c_str(), db);
				//			break;
				//		}
				//	}
				//	else
				//	{
				//		printf("cannot find vk sdk\n");
				//		return nullptr;
				//	}

				//	db.save_typeinfo(ti_path);

				//	pugi::xml_document res;
				//	auto root = res.append_child("res");

				//	auto n_push_constant = root.append_child("push_constant");
				//	n_push_constant.append_attribute("type_name").set_value(pc_ti ? pc_ti->name.c_str() : "");

				//	res.save_file(res_path.c_str());
				//}
				//else
				//{
				//	auto ti_path = res_path;
				//	ti_path.replace_extension(L".typeinfo");
				//	db.load_typeinfo(ti_path);

				//	pugi::xml_document res;
				//	pugi::xml_node root;
				//	if (!res.load_file(res_path.c_str()) || (root = res.first_child()).name() != std::string("res"))
				//	{
				//		printf("res file wrong format\n");
				//		return nullptr;
				//	}

				//	auto n_push_constant = root.child("push_constant");
				//	pc_ti = find_udt(n_push_constant.attribute("type_name").value(), db);
				//}

				//auto header_path = filename;
				//header_path += L".h";
				//if (!std::filesystem::exists(header_path) || std::filesystem::last_write_time(header_path) < std::filesystem::last_write_time(ti_path))
				//{
				//	std::string header;
				//	header += "namespace PLL_" + filename.filename().stem().string() + "\n{\n";
				//	header += "\tenum Binding\n\t{\n";
				//	for (auto& d : dependencies)
				//	{
				//		if (!d.empty())
				//			header += "\t\tBinding_" + d.filename().stem().string() + ",\n";
				//	}
				//	header += "\t\tBinding_Max\n";
				//	header += "\t};\n\n";
				//	write_udts_to_header(header, db);
				//	header += "}\n";
				//	std::ofstream file(header_path);
				//	file << header;
				//	file.close();
				//}

				if (device)
				{
					auto ret = PipelineLayout::create(device, dsls, pc_ti ? pc_ti->size : 0);
					ret->device = device;
					ret->db = std::move(db);
					ret->pc_ti = pc_ti;
					ret->filename = filename;

					{
						std::vector<VkDescriptorSetLayout> vk_descriptor_set_layouts;
						vk_descriptor_set_layouts.resize(dsls.size());
						for (auto i = 0; i < dsls.size(); i++)
							vk_descriptor_set_layouts[i] = dsls[i]->vk_descriptor_set_layout;

						VkPushConstantRange vk_pushconstant_range;
						vk_pushconstant_range.offset = 0;
						vk_pushconstant_range.size = ret->pc_sz;
						vk_pushconstant_range.stageFlags = to_backend_flags<ShaderStageFlags>(ShaderStageAll);

						VkPipelineLayoutCreateInfo info;
						info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
						info.flags = 0;
						info.pNext = nullptr;
						info.setLayoutCount = vk_descriptor_set_layouts.size();
						info.pSetLayouts = vk_descriptor_set_layouts.data();
						info.pushConstantRangeCount = ret->pc_sz > 0 ? 1 : 0;
						info.pPushConstantRanges = ret->pc_sz > 0 ? &vk_pushconstant_range : nullptr;

						chk_res(vkCreatePipelineLayout(device->vk_device, &info, nullptr, &ret->vk_pipeline_layout));
					}

					device->plls.emplace_back(ret);
					return ret;
				}
				return nullptr;
			}
		}PipelineLayout_get;
		PipelineLayout::Get& PipelineLayout::get = PipelineLayout_get;


		struct PipelineLayoutCreate : PipelineLayout::Create
		{
			PipelineLayoutPtr operator()(DevicePtr device, std::span<DescriptorSetLayoutPtr> descriptor_set_layouts, uint push_constant_size) override
			{
				if (!device)
					device = current_device;

				auto ret = new PipelineLayoutPrivate;
				ret->device = device;
				ret->descriptor_set_layouts.assign(descriptor_set_layouts.begin(), descriptor_set_layouts.end());
				ret->pc_sz = push_constant_size;

				std::vector<VkDescriptorSetLayout> vk_descriptor_set_layouts;
				vk_descriptor_set_layouts.resize(descriptor_set_layouts.size());
				for (auto i = 0; i < descriptor_set_layouts.size(); i++)
					vk_descriptor_set_layouts[i] = descriptor_set_layouts[i]->vk_descriptor_set_layout;

				VkPushConstantRange vk_pushconstant_range;
				vk_pushconstant_range.offset = 0;
				vk_pushconstant_range.size = push_constant_size;
				vk_pushconstant_range.stageFlags = to_backend_flags<ShaderStageFlags>(ShaderStageAll);

				VkPipelineLayoutCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				info.flags = 0;
				info.pNext = nullptr;
				info.setLayoutCount = vk_descriptor_set_layouts.size();
				info.pSetLayouts = vk_descriptor_set_layouts.data();
				info.pushConstantRangeCount = push_constant_size > 0 ? 1 : 0;
				info.pPushConstantRanges = push_constant_size > 0 ? &vk_pushconstant_range : nullptr;

				chk_res(vkCreatePipelineLayout(device->vk_device, &info, nullptr, &ret->vk_pipeline_layout));

				return ret;
			}
		}PipelineLayout_create;
		PipelineLayout::Create& PipelineLayout::create = PipelineLayout_create;

		ShaderPrivate::~ShaderPrivate()
		{
			if (vk_module)
				vkDestroyShaderModule(device->vk_device, vk_module, nullptr);
		}

		struct ShaderGet : Shader::Get
		{
			ShaderPtr operator()(DevicePtr device, const std::filesystem::path& _filename, const std::vector<std::string>& defines, const std::vector<std::pair<std::string, std::string>>& substitutes) override
			{
				if (!device)
					device = current_device;

				auto filename = Path::get(_filename);
				filename.make_preferred();

				if (device)
				{
					for (auto& s : device->sds)
					{
						if (s->filename == filename && s->defines == defines && s->substitutes == substitutes)
							return s.get();
					}
				}

				if (!std::filesystem::exists(filename))
				{
					wprintf(L"cannot find shader: %s\n", _filename.c_str());
					return nullptr;
				}

				auto ppath = filename.parent_path();

				auto hash = 0U;
				for (auto& d : defines)
					hash = hash ^ std::hash<std::string>()(d);
				for (auto& s : substitutes)
					hash = hash ^ std::hash<std::string>()(s.first) ^ std::hash<std::string>()(s.second);
				auto str_hash = to_hex_wstring(hash);

				auto spv_path = ppath / L"build";
				if (!std::filesystem::exists(spv_path))
					std::filesystem::create_directories(spv_path);
				spv_path /= filename.filename();
				spv_path += L"." + str_hash + L".spv";

				auto dependencies = get_make_dependencies(filename);

				auto unfolded_substitutes = substitutes;
				for (auto& s : unfolded_substitutes)
				{
					if (s.first.ends_with("_FILE"))
					{
						auto fn = std::filesystem::path(s.second);
						if (!fn.is_absolute())
							fn = ppath / fn;
						s.second = get_file_content(fn);
						assert(!s.second.empty());
						SUS::remove_char(s.second, '\r');
						dependencies.push_back(fn);
					}
				}

				if (should_remake(dependencies, spv_path))
				{
					auto vk_sdk_path = getenv("VK_SDK_PATH");
					if (vk_sdk_path)
					{
						auto temp = basic_glsl_prefix();
						std::ifstream glsl(filename);
						while (!glsl.eof())
						{
							std::string line;
							std::getline(glsl, line);
							for (auto& s : unfolded_substitutes)
								SUS::replace_all(line, s.first, s.second);
							temp += line + "\n";
						}
						glsl.close();

						auto temp_fn = filename;
						temp_fn.replace_filename(L"temp");
						temp_fn.replace_extension(filename.extension());
						std::ofstream temp_file(temp_fn);
						temp_file << temp << std::endl;
						temp_file.close();

						std::filesystem::remove(spv_path);

						auto glslc_path = std::filesystem::path(vk_sdk_path);
						glslc_path /= L"Bin/glslc.exe";

						auto command_line = L" " + temp_fn.wstring() + L" -o" + spv_path.wstring();
						for (auto& d : defines)
							command_line += L" -D" + s2w(d);

						printf("compiling shader: %s (%s) (%s)", filename.string().c_str(), [&]() {
							std::string ret;
							for (auto i = 0; i < defines.size(); i++)
							{
								ret += defines[i];
								if (i < defines.size() - 1)
									ret += " ";
							}
							return ret;
						}().c_str(), [&]() {
							std::string ret;
							for (auto i = 0; i < substitutes.size(); i++)
							{
								ret += substitutes[i].first + " " + substitutes[i].second;
								if (i < substitutes.size() - 1)
									ret += " ";
							}
							return ret;
						}().c_str());

						std::string output;
						exec(glslc_path.c_str(), (wchar_t*)command_line.c_str(), &output);
						if (!std::filesystem::exists(spv_path))
						{
							temp = add_lineno_to_code(temp);
							printf("\n========\n%s\n========\n%s\n", temp.c_str(), output.c_str());
							return nullptr;
						}

						printf(" done\n");

						std::filesystem::remove(temp_fn);
					}
					else
					{
						printf("cannot find vk sdk\n");
						return nullptr;
					}
				}

				if (device)
				{
					auto ret = new ShaderPrivate;
					ret->device = device;
					auto ext = filename.extension();
					if (ext == L".vert")
						ret->type = ShaderStageVert;
					else if (ext == L".tesc")
						ret->type = ShaderStageTesc;
					else if (ext == L".tese")
						ret->type = ShaderStageTese;
					else if (ext == L".geom")
						ret->type = ShaderStageGeom;
					else if (ext == L".frag")
						ret->type = ShaderStageFrag;
					else if (ext == L".comp")
						ret->type = ShaderStageComp;
					ret->filename = filename;
					ret->defines = defines;
					ret->substitutes = substitutes;

					auto spv_content = get_file_content(spv_path);
					VkShaderModuleCreateInfo shader_info;
					shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					shader_info.flags = 0;
					shader_info.pNext = nullptr;
					shader_info.codeSize = spv_content.size();
					shader_info.pCode = (uint*)spv_content.data();
					chk_res(vkCreateShaderModule(device->vk_device, &shader_info, nullptr, &ret->vk_module));

					device->sds.emplace_back(ret);
					return ret;
				}

				return nullptr;
			}
		}Shader_get;
		Shader::Get& Shader::get = Shader_get;

		GraphicsPipelinePrivate::~GraphicsPipelinePrivate()
		{
			vkDestroyPipeline(device->vk_device, vk_pipeline, nullptr);
		}

		struct GraphicsPipelineCreate : GraphicsPipeline::Create
		{
			GraphicsPipelinePtr operator()(DevicePtr device, const GraphicsPipelineInfo& info) override
			{
				if (!device)
					device = current_device;

				auto ret = new GraphicsPipelinePrivate;
				ret->device = device;
				ret->info = info;

				std::vector<VkPipelineShaderStageCreateInfo> vk_stage_infos;
				std::vector<VkVertexInputAttributeDescription> vk_vi_attributes;
				std::vector<VkVertexInputBindingDescription> vk_vi_bindings;
				std::vector<VkPipelineColorBlendAttachmentState> vk_blend_attachment_states;
				std::vector<VkDynamicState> vk_dynamic_states;

				vk_stage_infos.resize(info.shaders.size());
				for (auto i = 0; i < info.shaders.size(); i++)
				{
					auto shader = info.shaders[i];

					auto& dst = vk_stage_infos[i];
					dst.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					dst.flags = 0;
					dst.pNext = nullptr;
					dst.pSpecializationInfo = nullptr;
					dst.pName = "main";
					dst.stage = to_backend(shader->type);
					dst.module = shader->vk_module;
				}

				vk_vi_bindings.resize(info.vertex_buffers.size());
				for (auto i = 0; i < vk_vi_bindings.size(); i++)
				{
					auto& src_buf = info.vertex_buffers[i];
					auto& dst_buf = vk_vi_bindings[i];
					dst_buf.binding = i;
					auto offset = 0;
					for (auto j = 0; j < src_buf.attributes.size(); j++)
					{
						auto& src_att = src_buf.attributes[j];
						VkVertexInputAttributeDescription dst_att;
						dst_att.location = src_att.location;
						dst_att.binding = i;
						if (src_att.offset != -1)
							offset = src_att.offset;
						dst_att.offset = offset;
						offset += format_size(src_att.format);
						dst_att.format = to_backend(src_att.format);
						vk_vi_attributes.push_back(dst_att);
					}
					dst_buf.inputRate = to_backend(src_buf.rate);
					dst_buf.stride = src_buf.stride ? src_buf.stride : offset;
				}

				VkPipelineVertexInputStateCreateInfo vertex_input_state;
				vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertex_input_state.pNext = nullptr;
				vertex_input_state.flags = 0;
				vertex_input_state.vertexBindingDescriptionCount = vk_vi_bindings.size();
				vertex_input_state.pVertexBindingDescriptions = vk_vi_bindings.empty() ? nullptr : vk_vi_bindings.data();
				vertex_input_state.vertexAttributeDescriptionCount = vk_vi_attributes.size();
				vertex_input_state.pVertexAttributeDescriptions = vk_vi_attributes.empty() ? nullptr : vk_vi_attributes.data();

				VkPipelineInputAssemblyStateCreateInfo assembly_state;
				assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				assembly_state.flags = 0;
				assembly_state.pNext = nullptr;
				assembly_state.topology = to_backend(info.primitive_topology);
				assembly_state.primitiveRestartEnable = VK_FALSE;

				VkPipelineTessellationStateCreateInfo tess_state;
				tess_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tess_state.pNext = nullptr;
				tess_state.flags = 0;
				tess_state.patchControlPoints = info.patch_control_points;

				VkViewport viewport;
				viewport.width = 1.f;
				viewport.height = 1.f;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				viewport.x = 0;
				viewport.y = 0;

				VkRect2D scissor;
				scissor.extent.width = 1;
				scissor.extent.height = 1;
				scissor.offset.x = 0;
				scissor.offset.y = 0;

				VkPipelineViewportStateCreateInfo viewport_state;
				viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewport_state.pNext = nullptr;
				viewport_state.flags = 0;
				viewport_state.viewportCount = 1;
				viewport_state.scissorCount = 1;
				viewport_state.pScissors = &scissor;
				viewport_state.pViewports = &viewport;

				VkPipelineRasterizationStateCreateInfo raster_state;
				raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				raster_state.pNext = nullptr;
				raster_state.flags = 0;
				raster_state.depthClampEnable = VK_FALSE;
				raster_state.rasterizerDiscardEnable = VK_FALSE;
				raster_state.polygonMode = to_backend(info.polygon_mode);
				raster_state.cullMode = to_backend(info.cull_mode);
				raster_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				raster_state.depthBiasEnable = VK_FALSE;
				raster_state.depthBiasConstantFactor = 0.f;
				raster_state.depthBiasClamp = 0.f;
				raster_state.depthBiasSlopeFactor = 0.f;
				raster_state.lineWidth = 1.f;

				VkPipelineMultisampleStateCreateInfo multisample_state;
				multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisample_state.flags = 0;
				multisample_state.pNext = nullptr;
				if (info.sample_count == SampleCount_1)
				{
					auto& res_atts = info.renderpass->subpasses[info.subpass_index].resolve_attachments;
					multisample_state.rasterizationSamples = to_backend(!res_atts.empty() ? info.renderpass->attachments[res_atts[0]].sample_count : SampleCount_1);
				}
				else
					multisample_state.rasterizationSamples = to_backend(info.sample_count);
				multisample_state.sampleShadingEnable = VK_FALSE;
				multisample_state.minSampleShading = 0.f;
				multisample_state.pSampleMask = nullptr;
				multisample_state.alphaToCoverageEnable = info.alpha_to_coverage;
				multisample_state.alphaToOneEnable = VK_FALSE;

				VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
				depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depth_stencil_state.flags = 0;
				depth_stencil_state.pNext = nullptr;
				depth_stencil_state.depthTestEnable = info.depth_test;
				depth_stencil_state.depthWriteEnable = info.depth_write;
				depth_stencil_state.depthCompareOp = to_backend(info.compare_op);
				depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
				depth_stencil_state.minDepthBounds = 0;
				depth_stencil_state.maxDepthBounds = 0;
				depth_stencil_state.stencilTestEnable = VK_FALSE;
				depth_stencil_state.front = {};
				depth_stencil_state.back = {};

				vk_blend_attachment_states.resize(info.renderpass->subpasses[info.subpass_index].color_attachments.size());
				for (auto& a : vk_blend_attachment_states)
				{
					a.blendEnable = VK_FALSE;
					a.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
					a.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
					a.colorBlendOp = VK_BLEND_OP_ADD;
					a.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					a.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					a.alphaBlendOp = VK_BLEND_OP_ADD;
					a.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				}
				for (auto i = 0; i < info.blend_options.size(); i++)
				{
					auto& src = info.blend_options[i];
					auto& dst = vk_blend_attachment_states[i];
					dst.blendEnable = src.enable;
					dst.srcColorBlendFactor = to_backend(src.src_color);
					dst.dstColorBlendFactor = to_backend(src.dst_color);
					dst.colorBlendOp = VK_BLEND_OP_ADD;
					dst.srcAlphaBlendFactor = to_backend(src.src_alpha);
					dst.dstAlphaBlendFactor = to_backend(src.dst_alpha);
					dst.alphaBlendOp = VK_BLEND_OP_ADD;
					dst.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				}

				VkPipelineColorBlendStateCreateInfo blend_state;
				blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				blend_state.flags = 0;
				blend_state.pNext = nullptr;
				blend_state.blendConstants[0] = 0.f;
				blend_state.blendConstants[1] = 0.f;
				blend_state.blendConstants[2] = 0.f;
				blend_state.blendConstants[3] = 0.f;
				blend_state.logicOpEnable = VK_FALSE;
				blend_state.logicOp = VK_LOGIC_OP_COPY;
				blend_state.attachmentCount = vk_blend_attachment_states.size();
				blend_state.pAttachments = vk_blend_attachment_states.data();

				for (auto i = 0; i < info.dynamic_states.size(); i++)
					vk_dynamic_states.push_back(to_backend((DynamicState)info.dynamic_states[i]));
				if (std::find(vk_dynamic_states.begin(), vk_dynamic_states.end(), VK_DYNAMIC_STATE_VIEWPORT) == vk_dynamic_states.end())
					vk_dynamic_states.push_back(VK_DYNAMIC_STATE_VIEWPORT);
				if (std::find(vk_dynamic_states.begin(), vk_dynamic_states.end(), VK_DYNAMIC_STATE_SCISSOR) == vk_dynamic_states.end())
					vk_dynamic_states.push_back(VK_DYNAMIC_STATE_SCISSOR);

				VkPipelineDynamicStateCreateInfo dynamic_state;
				dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamic_state.pNext = nullptr;
				dynamic_state.flags = 0;
				dynamic_state.dynamicStateCount = vk_dynamic_states.size();
				dynamic_state.pDynamicStates = vk_dynamic_states.data();

				VkGraphicsPipelineCreateInfo pipeline_info;
				pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipeline_info.pNext = nullptr;
				pipeline_info.flags = 0;
				pipeline_info.stageCount = vk_stage_infos.size();
				pipeline_info.pStages = vk_stage_infos.data();
				pipeline_info.pVertexInputState = &vertex_input_state;
				pipeline_info.pInputAssemblyState = &assembly_state;
				pipeline_info.pTessellationState = tess_state.patchControlPoints > 0 ? &tess_state : nullptr;
				pipeline_info.pViewportState = &viewport_state;
				pipeline_info.pRasterizationState = &raster_state;
				pipeline_info.pMultisampleState = &multisample_state;
				pipeline_info.pDepthStencilState = &depth_stencil_state;
				pipeline_info.pColorBlendState = &blend_state;
				pipeline_info.pDynamicState = vk_dynamic_states.size() ? &dynamic_state : nullptr;
				pipeline_info.layout = info.layout->vk_pipeline_layout;
				pipeline_info.renderPass = info.renderpass->vk_renderpass;
				pipeline_info.subpass = info.subpass_index;
				pipeline_info.basePipelineHandle = 0;
				pipeline_info.basePipelineIndex = 0;

				chk_res(vkCreateGraphicsPipelines(device->vk_device, 0, 1, &pipeline_info, nullptr, &ret->vk_pipeline));

				return ret;
			}
		}GraphicsPipeline_create;
		GraphicsPipeline::Create& GraphicsPipeline::create = GraphicsPipeline_create;

		struct GraphicsPipelineGet : GraphicsPipeline::Get
		{
			GraphicsPipelinePtr operator()(DevicePtr device, const std::filesystem::path& _filename) override
			{
				if (!device)
					device = current_device;

				auto filename = Path::get(_filename);
				filename.make_preferred();

				if (device)
				{
					for (auto& pl : device->pls)
					{
						if (pl->filename == filename)
							return pl.get();
					}
				}

				if (!std::filesystem::exists(filename))
				{
					wprintf(L"cannot find pipeline: %s\n", _filename.c_str());
					return nullptr;
				}

				pugi::xml_document doc;
				pugi::xml_node doc_root;

				if (!doc.load_file(filename.c_str()) || (doc_root = doc.first_child()).name() != std::string("pipeline"))
				{
					printf("pipeline wrong format: %s\n", _filename.string().c_str());
					return nullptr;
				}

				GraphicsPipelineInfo info;
				auto ppath = filename.parent_path();

				for (auto n_shdr : doc_root.child("shaders"))
				{
					std::filesystem::path fn = n_shdr.attribute("filename").value();
					Path::cat_if_in(ppath, fn);
					auto shader = Shader::get(device, fn, Shader::format_defines(n_shdr.attribute("defines").value()), {});
					assert(shader);
					info.shaders.push_back(shader);
				}

				std::filesystem::path pll_fn = doc_root.child("layout").attribute("filename").value();
				Path::cat_if_in(ppath, pll_fn);
				info.layout = PipelineLayout::get(device, pll_fn);
				assert(info.layout);

				auto n_rp = doc_root.child("renderpass");
				std::filesystem::path rp_fn = n_rp.attribute("filename").value();
				Path::cat_if_in(ppath, rp_fn);
				info.renderpass = Renderpass::get(device, rp_fn);
				assert(info.renderpass);
				info.subpass_index = n_rp.attribute("index").as_uint();

				for (auto n_buf : doc_root.child("vertex_buffers"))
				{
					auto& vbuf = info.vertex_buffers.emplace_back();
					for (auto n_att : n_buf)
					{
						auto& att = vbuf.attributes.emplace_back();
						att.location = n_att.attribute("location").as_uint();
						TypeInfo::unserialize_t(n_att.attribute("format").value(), &att.format);
					}
				}

				if (auto n = doc_root.child("primitive_topology"); n)
					TypeInfo::unserialize_t(n.attribute("v").value(), &info.primitive_topology);
				if (auto n = doc_root.child("cull_mode"); n)
					TypeInfo::unserialize_t(n.attribute("v").value(), &info.cull_mode);
				if (auto n = doc_root.child("sample_count"); n)
					TypeInfo::unserialize_t(n.attribute("v").value(), &info.sample_count);
				if (auto n = doc_root.child("alpha_to_coverage"); n)
					info.alpha_to_coverage = n.attribute("v").as_bool();
				if (auto n = doc_root.child("depth_test"); n)
					info.depth_test = n.attribute("v").as_bool();
				if (auto n = doc_root.child("depth_write"); n)
					info.depth_write = n.attribute("v").as_bool();
				if (auto n = doc_root.child("compare_op"); n)
					TypeInfo::unserialize_t(n.attribute("v").value(), &info.compare_op);

				std::vector<BlendOption> blend_options;
				for (auto n_bo : doc_root.child("blend_options"))
				{
					auto& bo = info.blend_options.emplace_back();
					bo.enable = n_bo.attribute("enable").as_bool();
					if (auto a = n_bo.attribute("src_color"); a)
						TypeInfo::unserialize_t(a.value(), &bo.src_color);
					if (auto a = n_bo.attribute("dst_color"); a)
						TypeInfo::unserialize_t(a.value(), &bo.dst_color);
					if (auto a = n_bo.attribute("src_alpha"); a)
						TypeInfo::unserialize_t(a.value(), &bo.src_alpha);
					if (auto a = n_bo.attribute("dst_alpha"); a)
						TypeInfo::unserialize_t(a.value(), &bo.dst_alpha);
				}

				if (device)
				{
					auto ret = GraphicsPipeline::create(device, info);
					ret->filename = filename;
					device->pls.emplace_back(ret);
					return ret;
				}

				return nullptr;
			}
		}GraphicsPipeline_get;
		GraphicsPipeline::Get& GraphicsPipeline::get = GraphicsPipeline_get;

		ComputePipelinePrivate::~ComputePipelinePrivate()
		{
			vkDestroyPipeline(device->vk_device, vk_pipeline, nullptr);
		}

		struct ComputePipelineCreate : ComputePipeline::Create
		{
			ComputePipelinePtr operator()(DevicePtr device, const ComputePipelineInfo& info) override
			{
				if (!device)
					device = current_device;

				auto ret = new ComputePipelinePrivate;
				ret->device = device;
				ret->info = info;

				VkComputePipelineCreateInfo pipeline_info;
				pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
				pipeline_info.pNext = nullptr;
				pipeline_info.flags = 0;

				pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				pipeline_info.stage.flags = 0;
				pipeline_info.stage.pNext = nullptr;
				pipeline_info.stage.pSpecializationInfo = nullptr;
				pipeline_info.stage.pName = "main";
				pipeline_info.stage.stage = to_backend(ShaderStageComp);
				pipeline_info.stage.module = info.shader->vk_module;

				pipeline_info.basePipelineHandle = 0;
				pipeline_info.basePipelineIndex = 0;
				pipeline_info.layout = info.layout->vk_pipeline_layout;

				chk_res(vkCreateComputePipelines(device->vk_device, 0, 1, &pipeline_info, nullptr, &ret->vk_pipeline));

				return ret;
			}
		}ComputePipeline_create;
		ComputePipeline::Create& ComputePipeline::create = ComputePipeline_create;
	}
}

#include "../base64.h"
#include "../xml.h"
#include "../foundation/typeinfo.h"
#include "material_private.h"
#include "model_private.h"
#include "model_ext.h"
#include "animation_private.h"

#ifdef USE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

#ifdef USE_FBXSDK
#include <fbxsdk.h>
#endif

namespace flame
{
	namespace graphics
	{
		ModelPrivate::~ModelPrivate()
		{
			for (auto& m : meshes)
			{
				for (auto mat : m.materials)
					Material::release(mat);
			}
		}

		void ModelPrivate::save(const std::filesystem::path& filename)
		{
			pugi::xml_document doc;
			auto n_model = doc.append_child("model");

			auto data_filename = filename;
			data_filename += L".dat";
			std::ofstream data_file(data_filename, std::ios::binary);

			auto append_data = [&](void* data, uint size, pugi::xml_node n) {
				n.append_attribute("offset").set_value(data_file.tellp());
				n.append_attribute("size").set_value(size);
				data_file.write((char*)data, size);
			};

			auto n_meshes = n_model.append_child("meshes");
			for (auto& m : meshes)
			{
				auto n_mesh = n_meshes.append_child("mesh");
				std::string material_names;
				for (auto& mat : m.materials)
				{
					if (!material_names.empty())
						material_names += ';';
					material_names += mat->filename.string();
				}
				n_mesh.append_attribute("materials").set_value(material_names.c_str());

				if (!m.positions.empty())
					append_data(m.positions.data(), m.positions.size() * sizeof(vec3), n_mesh.append_child("positions"));
				if (!m.uvs.empty())
					append_data(m.uvs.data(), m.uvs.size() * sizeof(vec2), n_mesh.append_child("uvs"));
				if (!m.normals.empty())
					append_data(m.normals.data(), m.normals.size() * sizeof(vec3), n_mesh.append_child("normals"));
				if (!m.bone_ids.empty())
					append_data(m.bone_ids.data(), m.bone_ids.size() * sizeof(ivec4), n_mesh.append_child("bone_ids"));
				if (!m.bone_weights.empty())
					append_data(m.bone_weights.data(), m.bone_weights.size() * sizeof(vec4), n_mesh.append_child("bone_weights"));
				if (!m.indices.empty())
					append_data(m.indices.data(), m.indices.size() * sizeof(uint), n_mesh.append_child("indices"));

				n_mesh.append_attribute("bounds").set_value(str((mat2x3&)m.bounds).c_str());
			}

			auto n_bones = n_model.append_child("bones");
			for (auto& b : bones)
			{
				auto n_bone = n_bones.append_child("bone");
				n_bone.append_attribute("name").set_value(b.name.c_str());
				append_data(&b.offset_matrix, sizeof(mat4), n_bone.append_child("offset_matrix"));
			}

			data_file.close();
			doc.save_file(filename.c_str());
		}

		void Model::convert(const std::filesystem::path& _filename)
		{
			auto filename = Path::reverse(_filename);
			auto ppath = filename.parent_path();
			auto model_name = filename.filename().stem().string();
			auto ext = filename.extension();

			auto format_mat_name = [](const std::string& name, int i) {
				auto ret = name;
				if (ret.empty())
					ret = str(i);
				else
				{
					for (auto& ch : ret)
						if (ch == ' ' || ch == ':') ch = '_';
				}
				return ret;
			};

			if (ext == L".fbx")
			{
#ifdef USE_FBXSDK
				using namespace fbxsdk;

				static FbxManager* sdk_manager = nullptr;
				if (!sdk_manager)
				{
					sdk_manager = FbxManager::Create();
					if (!sdk_manager)
					{
						printf("FBX SDK: Unable to create FBX Manager!\n");
						return;
					}

					auto ios = FbxIOSettings::Create(sdk_manager, IOSROOT);
					sdk_manager->SetIOSettings(ios);
				}

				auto scene = FbxScene::Create(sdk_manager, "Scene");
				if (!scene)
				{
					printf("FBX SDK: Unable to create FBX scene!\n");
					return;
				}

				auto importer = FbxImporter::Create(sdk_manager, "");
				if (!importer->Initialize(_filename.string().c_str(), -1, sdk_manager->GetIOSettings()))
				{
					printf("cannot import model %s: %s\n", _filename.string().c_str(), importer->GetStatus().GetErrorString());
					return;
				}

				if (!importer->Import(scene))
				{
					printf("cannot import model %s: %s\n", _filename.string().c_str(), importer->GetStatus().GetErrorString());
					return;
				}

				importer->Destroy();

				FbxAxisSystem our_axis_system(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
				if (scene->GetGlobalSettings().GetAxisSystem() != our_axis_system)
					our_axis_system.ConvertScene(scene);

				FbxGeometryConverter geom_converter(sdk_manager);
				try 
				{
					geom_converter.Triangulate(scene, /*replace*/true);
				}
				catch (std::runtime_error) 
				{
					printf("FBX SDK: cannot triangulate %s\n", _filename.string().c_str());
					return;
				}

				std::vector<std::unique_ptr<MaterialT>> materials;
				std::vector<Mesh> meshes;
				pugi::xml_document doc_prefab;

				std::function<void(pugi::xml_node, FbxNode*)> process_node;
				process_node = [&](pugi::xml_node dst, FbxNode* src) {
					dst.append_attribute("name").set_value(src->GetName());

					auto n_components = dst.append_child("components");

					{
						auto n_node = n_components.append_child("item");
						n_node.append_attribute("type_name").set_value("flame::cNode"_h);

						auto p = src->LclTranslation.Get();
						auto r = src->LclRotation.Get();
						auto ro = src->RotationOrder.Get();
						auto s = src->LclScaling.Get();

						auto pos = vec3(p[0], p[1], p[2]);
						if (pos != vec3(0.f))
							n_node.append_attribute("pos").set_value(str(pos).c_str());
						auto eul = vec3(r[0], r[1], r[2]);
						switch (ro)
						{
						case FbxEuler::eOrderXYZ: eul = vec3(eul.y, eul.x, eul.z);	break;
						case FbxEuler::eOrderXZY: eul = vec3(eul.y, eul.z, eul.x);	break;
						case FbxEuler::eOrderYZX: eul = vec3(eul.x, eul.z, eul.y);	break;
						case FbxEuler::eOrderYXZ:									break;
						case FbxEuler::eOrderZXY: eul = vec3(eul.z, eul.y, eul.x);	break;
						case FbxEuler::eOrderZYX: eul = vec3(eul.z, eul.x, eul.y);	break;
						}
						if (eul != vec3(0.f))
							n_node.append_attribute("eul").set_value(str(eul).c_str());
						auto scl = vec3(s[0], s[1], s[2]);
						if (scl != vec3(1.f))
							n_node.append_attribute("scl").set_value(str(scl).c_str());
					}

					auto material_count = src->GetMaterialCount();
					for (auto i = 0; i < material_count; i++)
					{
						auto fbx_mat = src->GetMaterial(i);
						if (!fbx_mat->GetUserDataPtr())
						{
							auto material = Material::create();
							auto map_id = 0;
							{
								auto prop = fbx_mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
								if (prop.IsValid())
								{
									auto res = prop.Get<FbxDouble3>();
									material->color[0] = res[0];
									material->color[1] = res[1];
									material->color[2] = res[2];
									if (prop.GetSrcObjectCount<FbxFileTexture>()) 
									{
										if (auto tex = prop.GetSrcObject<FbxFileTexture>(); tex)
										{
											 material->textures[map_id].filename = Path::get(ppath / tex->GetFileName());
											 material->color_map = map_id++;
										}
									}
								}
							}
							fbx_mat->SetUserDataPtr(material);
						}
					}

					if (auto node_attr = src->GetNodeAttribute(); node_attr)
					{
						if (node_attr->GetAttributeType() == FbxNodeAttribute::eMesh)
						{
							auto fbx_mesh = src->GetMesh();
							if (!fbx_mesh->GetUserDataPtr())
							{
								auto polygon_count = fbx_mesh->GetPolygonCount();
								if (auto element_mat = fbx_mesh->GetElementMaterial(); element_mat)
								{
									if (element_mat->GetMappingMode() == FbxGeometryElement::eByPolygon)
									{
										auto& mat_indices = element_mat->GetIndexArray();
										for (auto i = 0; i < polygon_count; i++)
										{
											auto mat_idx = mat_indices.GetAt(i);
										}
									}
								}

								fbx_mesh->SetUserDataPtr((void*)meshes.size());
							}
						}
					}

					auto child_count = src->GetChildCount();
					if (child_count > 0)
					{
						auto n_children = dst.append_child("children");
						for (auto i = 0; i < child_count; i++)
							process_node(n_children.append_child("item"), src->GetChild(i));
					}
				};
				process_node(doc_prefab.append_child("prefab"), scene->GetRootNode());
#endif
			}
			else
			{
#ifdef USE_ASSIMP
				Assimp::Importer importer;
				importer.SetPropertyString(AI_CONFIG_PP_OG_EXCLUDE_LIST, "trigger");
				auto load_flags =
					aiProcess_RemoveRedundantMaterials |
					aiProcess_Triangulate |
					aiProcess_JoinIdenticalVertices |
					aiProcess_SortByPType |
					aiProcess_FlipUVs |
					aiProcess_LimitBoneWeights |
					aiProcess_OptimizeMeshes |
					aiProcess_OptimizeGraph;
				auto scene = importer.ReadFile(_filename.string(), load_flags);
				if (!scene)
				{
					printf("cannot import model %s: %s\n", _filename.string().c_str(), importer.GetErrorString());
					return;
				}

				std::vector<std::unique_ptr<MaterialT>> materials;

				for (auto i = 0; i < scene->mNumMaterials; i++)
				{
					aiString ai_name;
					auto ai_mat = scene->mMaterials[i];
					auto material = Material::create();
					auto map_id = 0;

					{
						ai_name.Clear();
						ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &ai_name);
						auto name = std::string(ai_name.C_Str());
						if (!name.empty())
						{
							material->textures[map_id].filename = Path::get(ppath / name);
							material->color_map = map_id++;
						}
					}

					{
						ai_name.Clear();
						ai_mat->GetTexture(aiTextureType_OPACITY, 0, &ai_name);
						auto name = std::string(ai_name.C_Str());
						if (!name.empty())
						{
							material->textures[map_id].filename = Path::get(ppath / name);
							material->alpha_map = map_id++;
						}
					}

					auto material_name = format_mat_name(ai_mat->GetName().C_Str(), i);
					material_name = std::format("{}_{}.fmat", model_name, material_name);
					material->save(Path::get(ppath / material_name));

					materials.emplace_back(material);
				}

				auto model = Model::create();

				for (auto i = 0; i < scene->mNumMeshes; i++)
				{
					auto ai_mesh = scene->mMeshes[i];
					auto& mesh = model->meshes.emplace_back();
					mesh.model = model;

					mesh.materials.push_back(materials[ai_mesh->mMaterialIndex].get());

					auto vertex_count = ai_mesh->mNumVertices;

					if (ai_mesh->mVertices)
					{
						mesh.positions.resize(vertex_count);
						memcpy(mesh.positions.data(), ai_mesh->mVertices, sizeof(vec3) * vertex_count);
					}

					if (auto puv = ai_mesh->mTextureCoords[0]; puv)
					{
						mesh.uvs.resize(vertex_count);
						for (auto j = 0; j < vertex_count; j++)
						{
							auto& uv = puv[j];
							mesh.uvs[j] = vec2(uv.x, uv.y);
						}
					}

					if (ai_mesh->mNormals)
					{
						mesh.normals.resize(vertex_count);
						memcpy(mesh.normals.data(), ai_mesh->mNormals, sizeof(vec3) * vertex_count);
					}

					if (ai_mesh->mNumBones > 0)
					{
						mesh.bone_ids.resize(vertex_count);
						mesh.bone_weights.resize(vertex_count);
						for (auto j = 0; j < vertex_count; j++)
						{
							mesh.bone_ids[j] = ivec4(-1);
							mesh.bone_weights[j] = vec4(0.f);
						}

						for (auto j = 0; j < ai_mesh->mNumBones; j++)
						{
							auto ai_bone = ai_mesh->mBones[j];

							std::string name = ai_bone->mName.C_Str();
							auto find_bone = [&](std::string_view name) {
								for (auto i = 0; i < model->bones.size(); i++)
								{
									if (model->bones[i].name == name)
										return i;
								}
								return -1;
							};
							auto bid = find_bone(name);
							if (bid == -1)
							{
								bid = model->bones.size();
								auto& m = ai_bone->mOffsetMatrix;
								auto& b = model->bones.emplace_back();
								b.name = name;
								b.offset_matrix = mat4(
									vec4(m.a1, m.b1, m.c1, m.d1),
									vec4(m.a2, m.b2, m.c2, m.d2),
									vec4(m.a3, m.b3, m.c3, m.d3),
									vec4(m.a4, m.b4, m.c4, m.d4)
								);
							}

							auto weights_count = ai_bone->mNumWeights;
							if (weights_count > 0)
							{
								auto get_idx = [&](uint vi) {
									auto& ids = mesh.bone_ids[vi];
									for (auto i = 0; i < 4; i++)
									{
										if (ids[i] == -1)
											return i;
									}
									return -1;
								};
								for (auto j = 0; j < weights_count; j++)
								{
									auto w = ai_bone->mWeights[j];
									auto idx = get_idx(w.mVertexId);
									if (idx == -1)
										continue;
									mesh.bone_ids[w.mVertexId][idx] = bid;
									mesh.bone_weights[w.mVertexId][idx] = w.mWeight;
								}
							}
						}
					}

					mesh.indices.resize(ai_mesh->mNumFaces * 3);
					for (auto j = 0; j < ai_mesh->mNumFaces; j++)
					{
						mesh.indices[j * 3 + 0] = ai_mesh->mFaces[j].mIndices[0];
						mesh.indices[j * 3 + 1] = ai_mesh->mFaces[j].mIndices[1];
						mesh.indices[j * 3 + 2] = ai_mesh->mFaces[j].mIndices[2];
					}

					mesh.calc_bounds();
				}

				model->save(replace_ext(_filename, L".fmod"));

				pugi::xml_document doc_prefab;

				std::function<void(pugi::xml_node, aiNode*)> print_node;
				print_node = [&](pugi::xml_node dst, aiNode* src) {
					auto name = std::string(src->mName.C_Str());
					dst.append_attribute("name").set_value(name.c_str());

					auto n_components = dst.append_child("components");

					{
						auto n_node = n_components.append_child("item");
						n_node.append_attribute("type_name").set_value("flame::cNode"_h);

						aiVector3D s;
						aiVector3D r;
						ai_real a;
						aiVector3D p;
						src->mTransformation.Decompose(s, r, a, p);
						a *= 0.5f;
						auto q = normalize(vec4(sin(a) * vec3(r.x, r.y, r.z), cos(a)));

						auto pos = vec3(p.x, p.y, p.z);
						if (pos != vec3(0.f))
							n_node.append_attribute("pos").set_value(str(pos).c_str());
						auto qut = vec4(q.w, q.x, q.y, q.z);
						if (qut != vec4(1.f, 0.f, 0.f, 0.f))
							n_node.append_attribute("qut").set_value(str(qut).c_str());
						auto scl = vec3(s.x, s.y, s.z);
						if (scl != vec3(1.f))
							n_node.append_attribute("scl").set_value(str(scl).c_str());
					}

					if (src->mNumMeshes > 0)
					{
						if (name == "trigger")
						{
							auto n_rigid = n_components.append_child("item");
							n_rigid.append_attribute("type_name").set_value("flame::cRigid");
							n_rigid.append_attribute("dynamic").set_value(false);
							auto n_shape = n_components.append_child("item");
							n_shape.append_attribute("type_name").set_value("flame::cShape");
							n_shape.append_attribute("type").set_value("Cube");
							n_shape.append_attribute("size").set_value("2,2,0.01");
							n_shape.append_attribute("trigger").set_value(true);
						}
						else
						{
							auto n_mesh = n_components.append_child("item");
							n_mesh.append_attribute("type_name").set_value("flame::cMesh");
							n_mesh.append_attribute("model_name").set_value(filename.string().c_str());
							n_mesh.append_attribute("mesh_index").set_value(src->mMeshes[0]);
							if (name == "mesh_collider")
							{
								auto n_rigid = n_components.append_child("item");
								n_rigid.append_attribute("type_name").set_value("flame::cRigid");
								n_rigid.append_attribute("dynamic").set_value(false);
								auto n_shape = n_components.append_child("item");
								n_shape.append_attribute("type_name").set_value("flame::cShape");
								n_shape.append_attribute("type").set_value("Mesh");
							}
						}
					}

					if (src->mNumChildren > 0)
					{
						auto n_children = dst.append_child("children");
						for (auto i = 0; i < src->mNumChildren; i++)
							print_node(n_children.append_child("item"), src->mChildren[i]);
					}
				};
				print_node(doc_prefab.append_child("prefab"), scene->mRootNode);
				if (!model->bones.empty())
				{
					auto n_armature = doc_prefab.first_child().child("components").append_child("item");
					n_armature.append_attribute("type_name").set_value("flame::cArmature");
					n_armature.append_attribute("model_name").set_value(filename.string().c_str());
				}

				auto prefab_path = filename;
				prefab_path.replace_extension(L".prefab");
				doc_prefab.save_file(Path::get(prefab_path).c_str());

				delete model;

				for (auto i = 0; i < scene->mNumAnimations; i++)
				{
					auto ai_ani = scene->mAnimations[i];

					auto animation_name = std::format("{}_{}.fani", model_name, ai_ani->mName.C_Str());
					for (auto& ch : animation_name)
						if (ch == '|') ch = '_';
					auto animation_filename = Path::get(ppath / animation_name);

					pugi::xml_document doc_animation;
					auto n_animation = doc_animation.append_child("animation");
					n_animation.append_attribute("duration").set_value((float)ai_ani->mDuration);
					auto n_channels = n_animation.append_child("channels");

					auto data_filename = animation_filename;
					data_filename += L".dat";
					std::ofstream data_file(data_filename, std::ios::binary);

					for (auto j = 0; j < ai_ani->mNumChannels; j++)
					{
						auto ai_ch = ai_ani->mChannels[j];
						auto n_channel = n_channels.append_child("channel");
						n_channel.append_attribute("node_name").set_value(ai_ch->mNodeName.C_Str());
						assert(ai_ch->mNumPositionKeys > 0 && ai_ch->mNumRotationKeys > 0 &&
							ai_ch->mNumPositionKeys == ai_ch->mNumRotationKeys);

						{
							std::vector<Channel::PositionKey> keys;
							keys.resize(ai_ch->mNumPositionKeys);
							for (auto k = 0; k < keys.size(); k++)
							{
								auto& p = ai_ch->mPositionKeys[k].mValue;
								keys[k].p = vec3(p.x, p.y, p.z);
							}
							auto n_keys = n_channel.append_child("position_keys");
							n_keys.append_attribute("offset").set_value(data_file.tellp());
							auto size = sizeof(Channel::PositionKey) * keys.size();
							n_keys.append_attribute("size").set_value(size);
							data_file.write((char*)keys.data(), size);
						}

						{
							std::vector<Channel::RotationKey> keys;
							keys.resize(ai_ch->mNumRotationKeys);
							for (auto k = 0; k < keys.size(); k++)
							{
								auto& q = ai_ch->mRotationKeys[k].mValue;
								keys[k].q = quat(q.w, q.x, q.y, q.z);
							}
							auto n_keys = n_channel.append_child("rotation_keys");
							n_keys.append_attribute("offset").set_value(data_file.tellp());
							auto size = sizeof(Channel::RotationKey) * keys.size();
							n_keys.append_attribute("size").set_value(size);
							data_file.write((char*)keys.data(), size);
						}
					}

					data_file.close();

					doc_animation.save_file(animation_filename.c_str());
				}
#endif
			}
		}

		struct ModelCreate : Model::Create
		{
			ModelPtr operator()() override
			{
				return new ModelPrivate;
			}
		}Model_create;
		Model::Create& Model::create = Model_create;

		static ModelPtr standard_cube = nullptr;
		static ModelPtr standard_sphere = nullptr;

		static std::vector<std::unique_ptr<ModelT>> models;

		struct ModelGet : Model::Get
		{
			ModelPtr operator()(const std::filesystem::path& _filename) override
			{
				if (_filename.wstring().starts_with(L"standard:"))
				{
					auto name = _filename.wstring().substr(9);
					if (name == L"cube")
					{
						if (!standard_cube)
						{
							auto m = new ModelPrivate;
							m->filename = L"standard:cube";
							auto& mesh = m->meshes.emplace_back();
							mesh.model = m;
							mesh.materials.push_back(default_material);
							mesh_add_cube(mesh, vec3(1.f), vec3(0.f), mat3(1.f));
							mesh.calc_bounds();

							standard_cube = m;
						}
						return standard_cube;
					}
					else if (name == L"sphere")
					{
						if (!standard_sphere)
						{
							auto m = new ModelPrivate;
							m->filename = L"standard:sphere";
							auto& mesh = m->meshes.emplace_back();
							mesh.model = m;
							mesh.materials.push_back(default_material);
							mesh_add_sphere(mesh, 0.5f, 12, 12, vec3(0.f), mat3(1.f));
							mesh.calc_bounds();

							standard_sphere = m;
						}
						return standard_sphere;
					}
					return nullptr;
				}

				auto filename = Path::get(_filename);

				for (auto& m : models)
				{
					if (m->filename == filename)
						return m.get();
				}

				if (!std::filesystem::exists(filename))
				{
					wprintf(L"cannot find model: %s\n", _filename.c_str());
					return nullptr;
				}

				if (filename.extension() != L".fmod")
					return nullptr;

				pugi::xml_document doc;
				pugi::xml_node doc_root;
				if (!doc.load_file(filename.c_str()) || (doc_root = doc.first_child()).name() != std::string("model"))
				{
					printf("model does not exist: %s\n", filename.string().c_str());
					return nullptr;
				}

				auto model_data_filename = filename;
				model_data_filename += L".dat";
				std::ifstream model_data_file(model_data_filename, std::ios::binary);
				if (!model_data_file.good())
				{
					printf("missing .dat file for: %s\n", filename.string().c_str());
					return nullptr;
				}

				auto ret = new ModelPrivate();
				ret->filename = filename;
				auto ppath = filename.parent_path();

				for (auto& n_mesh : doc_root.child("meshes"))
				{
					auto& m = ret->meshes.emplace_back();
					m.model = ret;
					for (auto& sp : SUS::split(n_mesh.attribute("materials").value(), ';'))
					{
						auto material_filename = std::filesystem::path(sp);
						auto fn = ppath / material_filename;
						if (!std::filesystem::exists(fn))
							fn = material_filename;
						m.materials.push_back(Material::get(fn.c_str()));
					}

					auto n_positions = n_mesh.child("positions");
					{
						auto offset = n_positions.attribute("offset").as_uint();
						auto size = n_positions.attribute("size").as_uint();
						m.positions.resize(size / sizeof(vec3));
						model_data_file.read((char*)m.positions.data(), size);
					}

					auto n_uvs = n_mesh.child("uvs");
					if (n_uvs)
					{
						auto offset = n_uvs.attribute("offset").as_uint();
						auto size = n_uvs.attribute("size").as_uint();
						m.uvs.resize(size / sizeof(vec2));
						model_data_file.read((char*)m.uvs.data(), size);
					}

					auto n_normals = n_mesh.child("normals");
					if (n_normals)
					{
						auto offset = n_normals.attribute("offset").as_uint();
						auto size = n_normals.attribute("size").as_uint();
						m.normals.resize(size / sizeof(vec3));
						model_data_file.read((char*)m.normals.data(), size);
					}

					auto n_bids = n_mesh.child("bone_ids");
					if (n_bids)
					{
						auto offset = n_bids.attribute("offset").as_uint();
						auto size = n_bids.attribute("size").as_uint();
						m.bone_ids.resize(size / sizeof(ivec4));
						model_data_file.read((char*)m.bone_ids.data(), size);
					}

					auto n_wgts = n_mesh.child("bone_weights");
					if (n_wgts)
					{
						auto offset = n_wgts.attribute("offset").as_uint();
						auto size = n_wgts.attribute("size").as_uint();
						m.bone_weights.resize(size / sizeof(vec4));
						model_data_file.read((char*)m.bone_weights.data(), size);
					}

					auto n_indices = n_mesh.child("indices");
					{
						auto offset = n_indices.attribute("offset").as_uint();
						auto size = n_indices.attribute("size").as_uint();
						m.indices.resize(size / sizeof(uint));
						model_data_file.read((char*)m.indices.data(), size);
					}

					m.bounds = (AABB&)s2t<2, 3, float>(n_mesh.attribute("bounds").value());
				}

				for (auto n_bone : doc_root.child("bones"))
				{
					auto& b = ret->bones.emplace_back();
					b.name = n_bone.attribute("name").value();
					{
						auto n_matrix = n_bone.child("offset_matrix");
						auto offset = n_matrix.attribute("offset").as_uint();
						auto size = n_matrix.attribute("size").as_uint();
						model_data_file.read((char*)&b.offset_matrix, size);
					}
				}

				for (auto& m : ret->meshes)
					ret->bounds.expand(m.bounds);

				model_data_file.close();

				ret->ref = 1;
				models.emplace_back(ret);
				return ret;
			}
		}Model_get;
		Model::Get& Model::get = Model_get;

		struct ModelRelease : Model::Release
		{
			void operator()(ModelPtr model) override
			{
				if (model->filename.wstring().starts_with(L"standard:"))
					return;
				if (model->ref == 1)
				{
					std::erase_if(models, [&](const auto& i) {
						return i.get() == model;
					});
				}
				else
					model->ref--;
			}
		}Model_release;
		Model::Release& Model::release = Model_release;
	}
}

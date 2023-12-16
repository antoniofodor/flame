﻿import pathlib
import os
import glob

current_directory = pathlib.Path.cwd()
thirdparty_directory = current_directory / "thirdparty"
print("Current Directory: " + str(current_directory))

os.system("chcp 65001")

os.system("vswhere.exe -latest -property installationPath > temp.txt")
f = open("temp.txt")
vs_path = pathlib.Path(f.read().strip("\n"))
f.close()
if not vs_path.exists():
	print("Cannot find visual studio, abort")
	exit(0)

if os.system("cmake --version") != 0:
	print("Cannot find CMake, abort")
	exit(0)

print("Download or build libraries?\n 1 - All\n 2 - Manually\n 3 - All Required\n 4 - Skip")
op = int(input())

if op != 4:
	print("== library glm ==")
	ok = True
	address = "https://github.com/tkgamegroup/glm.git"
	lib_dir = thirdparty_directory / "glm"
	if not lib_dir.exists():
		if op == 2:
			print("Download glm from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	print("== library PugiXML ==")
	ok = True
	address = "https://github.com/tkgamegroup/pugixml.git"
	lib_dir = thirdparty_directory / "pugixml"
	if not lib_dir.exists():
		if op == 2:
			print("Download PugiXML from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")

	ok = True
	bud_dir = lib_dir / "build"
	if not bud_dir.exists():
		if op == 2:
			print("Build PugiXML into %s ? y/n" % str(bud_dir))
			ok = input() == "y"
		if ok:
			bud_dir.mkdir()
			os.system("cmake -S \"%s\" -B \"%s\"" % (lib_dir, bud_dir))
			os.chdir("%s/MSBuild/Current/Bin" % str(vs_path));
			files = glob.glob("%s/*.sln" % str(bud_dir))
			if len(files) > 0:
				os.system("msbuild \"%s\"" % files[0])
			else:
				print("CMake Failed")
			os.chdir(current_directory)	
	else:
		print("%s exists, skip build" % str(bud_dir))
	print("====\n")
			
	print("== library nlohmann json ==")
	ok = True
	address = "https://github.com/tkgamegroup/json.git"
	lib_dir = thirdparty_directory / "njson"
	if not lib_dir.exists():
		if op == 2:
			print("Download nlohmann json from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	print("== library cppcodec ==")
	ok = True
	address = "https://github.com/tkgamegroup/cppcodec.git"
	lib_dir = thirdparty_directory / "cppcodec"
	if not lib_dir.exists():
		if op == 2:
			print("Download cppcodec from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	print("== library stb ==")
	ok = True
	address = "https://github.com/tkgamegroup/stb.git"
	lib_dir = thirdparty_directory / "stb"
	if not lib_dir.exists():
		if op == 2:
			print("Download STB from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	# (Optional)
	if op != 3:
		print("== library msdfgen ==")
		ok = True
		address = "https://github.com/tkgamegroup/msdfgen.git"
		lib_dir = thirdparty_directory / "msdfgen"
		if not lib_dir.exists():
			if op == 2:
				print("Download msdfgen from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))

		ok = True
		bud_dir = lib_dir / "build"
		if not bud_dir.exists():
			if op == 2:
				print("Build msdfgen into %s ? y/n" % str(bud_dir))
				ok = input() == "y"
			if ok:
				bud_dir.mkdir()
				os.system("cmake -S \"%s\" -B \"%s\" -DMSDFGEN_CORE_ONLY:BOOL=ON -DMSDFGEN_DYNAMIC_RUNTIME:BOOL=ON -DMSDFGEN_BUILD_STANDALONE:BOOL=OFF -DMSDFGEN_USE_SKIA:BOOL=OFF -DMSDFGEN_USE_VCPKG:BOOL=OFF" % (lib_dir, bud_dir))
				os.chdir("%s/MSBuild/Current/Bin" % str(vs_path));
				files = glob.glob("%s/*.sln" % str(bud_dir))
				if len(files) > 0:
					os.system("msbuild \"%s\"" % files[0])
				else:
					print("CMake Failed")
				os.chdir(current_directory)
		else:
			print("%s exists, skip build" % str(bud_dir))
		print("====\n")
			
	print("== library sha1 ==")
	ok = True
	address = "https://github.com/tkgamegroup/sha1.git"
	lib_dir = thirdparty_directory / "sha1"
	if not lib_dir.exists():
		if op == 2:
			print("Download SHA1 from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	print("== library gli ==")
	ok = True
	address = "https://github.com/tkgamegroup/gli"
	lib_dir = thirdparty_directory / "gli"
	if not lib_dir.exists():
		if op == 2:
			print("Download GLI from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	print("== library SPIRV-Cross ==")
	ok = True
	address = "https://github.com/tkgamegroup/SPIRV-Cross.git"
	lib_dir = thirdparty_directory / "SPIRV-Cross"
	if not lib_dir.exists():
		if op == 2:
			print("Download SPIRV-Cross from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
			
	ok = True
	bud_dir = lib_dir / "build"
	if not bud_dir.exists():
		if op == 2:
			print("Build SPIRV-Cross into %s ? y/n" % str(bud_dir))
			ok = input() == "y"
		if ok:
			bud_dir.mkdir()
			os.system("cmake -S \"%s\" -B \"%s\"" % (lib_dir, bud_dir))
			os.chdir("%s/MSBuild/Current/Bin" % str(vs_path));
			files = glob.glob("%s/*.sln" % str(bud_dir))
			if len(files) > 0:
				os.system("msbuild \"%s\"" % files[0])
			else:
				print("CMake Failed")
			os.chdir(current_directory)
	else:
		print("%s exists, skip build" % str(bud_dir))
	print("====\n")
			
	print("== library exprtk ==")
	ok = True
	address = "https://github.com/tkgamegroup/exprtk.git"
	lib_dir = thirdparty_directory / "exprtk"
	if not lib_dir.exists():
		if op == 2:
			print("Download exprtk from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
	else:
		print("%s exists, skip download" % str(lib_dir))
	print("====\n")
		
	# (Optional)
	if op != 3:
		print("== library assimp ==")
		ok = True
		address = "https://github.com/tkgamegroup/assimp.git"
		lib_dir = thirdparty_directory / "assimp"
		if not lib_dir.exists():
			if op == 2:
				print("Download Assimp from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))

		ok = True
		bud_dir = lib_dir / "build"
		if not bud_dir.exists():
			if op == 2:
				print("Build Assimp into %s ? y/n" % str(bud_dir))
				ok = input() == "y"
			if ok:
				bud_dir.mkdir()
				os.system("cmake -S \"%s\" -B \"%s\" -DASSIMP_WARNINGS_AS_ERRORS=OFF" % (lib_dir, bud_dir))
				os.system("xcopy %s %s" % (lib_dir / "build/include/assimp/config.h", lib_dir / "include/assimp/config.h"))
				os.chdir("%s/MSBuild/Current/Bin" % str(vs_path));
				files = glob.glob("%s/*.sln" % str(bud_dir))
				if len(files) > 0:
					os.system("msbuild \"%s\"" % files[0])
				else:
					print("CMake Failed")
				os.chdir(current_directory)
		else:
			print("%s exists, skip build" % str(bud_dir))
		print("====\n")
			
	# (Optional)
	'''
	if op != 3:
		print("== library PhysX ==")
		ok = True
		address = "https://github.com/NVIDIAGameWorks/PhysX.git"
		lib_dir = parent_directory / "PhysX"
		if op == 2:
			print("Download PhysX from %s into %s ? y/n" % (address, str(lib_dir)))
			ok = input() == "y"
		if ok:
			if not lib_dir.exists():
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
			else:
				print("%s exists, skip download" % str(lib_dir))
		print("====\n")
	'''
			
	# (Optional)
	if op != 3:
		print("== library recastnavigation ==")
		ok = True
		address = "https://github.com/tkgamegroup/recastnavigation.git"
		lib_dir = thirdparty_directory / "recastnavigation"
		if not lib_dir.exists():
			if op == 2:
				print("Download recastnavigation from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))

		ok = True
		bud_dir = lib_dir / "build"
		if not bud_dir.exists():
			if op == 2:
				print("Build recastnavigation into %s ? y/n" % str(bud_dir))
				ok = input() == "y"
			if ok:
				bud_dir.mkdir()
				os.system("cmake -S \"%s\" -B \"%s\" -D RECASTNAVIGATION_DEMO=OFF -D RECASTNAVIGATION_EXAMPLES=OFF -D RECASTNAVIGATION_TESTS=OFF" % (lib_dir, bud_dir))
				os.chdir("%s/MSBuild/Current/Bin" % str(vs_path));
				files = glob.glob("%s/*.sln" % str(bud_dir))
				if len(files) > 0:
					os.system("msbuild \"%s\"" % files[0])
				else:
					print("CMake Failed")
				os.chdir(current_directory)
		else:
			print("%s exists, skip build" % str(bud_dir))
		print("====\n")
	
	# (Optional)
	if op != 3:
		print("== library Font-Awesome ==")
		ok = True
		address = "https://github.com/tkgamegroup/Font-Awesome.git"
		lib_dir = thirdparty_directory / "Font-Awesome"
		if not lib_dir.exists():
			if op == 2:
				print("Download Font-Awesome from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))
		print("====\n")
			
	# (Optional)
	if op != 3:
		print("== library imgui ==")
		ok = True
		address = "https://github.com/tkgamegroup/imgui.git"
		lib_dir = thirdparty_directory / "imgui"
		if not lib_dir.exists():
			if op == 2:
				print("Download imgui from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 --branch docking %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))
		print("====\n")
			
	# (Optional)
	if op != 3:
		print("== library ImGuizmo ==")
		ok = True
		address = "https://github.com/tkgamegroup/ImGuizmo.git"
		lib_dir = thirdparty_directory / "ImGuizmo"
		if not lib_dir.exists():
			if op == 2:
				print("Download ImGuizmo from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))
		print("====\n")
		
	# (Optional)
	if op != 3:
		print("== library ImGui-Node-Editor ==")
		ok = True
		address = "https://github.com/tkgamegroup/imgui-node-editor.git"
		lib_dir = thirdparty_directory / "imgui-node-editor"
		if not lib_dir.exists():
			if op == 2:
				print("Download ImGui-Node-Editor from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))
		print("====\n")
			
	# (Optional)
	if op != 3:
		print("== library FortuneAlgorithm ==")
		ok = True
		address = "https://github.com/tkgamegroup/FortuneAlgorithm.git"
		lib_dir = thirdparty_directory / "FortuneAlgorithm"
		if not lib_dir.exists():
			if op == 2:
				print("Download FortuneAlgorithm from %s into %s ? y/n" % (address, str(lib_dir)))
				ok = input() == "y"
			if ok:
				os.system("git clone --depth 1 %s %s && echo ok" % (address, str(lib_dir)))
		else:
			print("%s exists, skip download" % str(lib_dir))

		ok = True
		bud_dir = lib_dir / "build"
		if not bud_dir.exists():
			if op == 2:
				print("Build FortuneAlgorithm into %s ? y/n" % str(bud_dir))
				ok = input() == "y"
			if ok:
				bud_dir.mkdir()
				os.system("cmake -S \"%s\" -B \"%s\"" % (lib_dir, bud_dir))
				os.chdir("%s/MSBuild/Current/Bin" % str(vs_path));
				files = glob.glob("%s/*.sln" % str(bud_dir))
				if len(files) > 0:
					os.system("msbuild \"%s\"" % files[0])
				else:
					print("CMake Failed")
				os.chdir(current_directory)
		else:
			print("%s exists, skip build" % str(bud_dir))
		print("====\n")
	
print("\nSet environment variable of Flame Engine:")
os.system("setx FLAME_PATH \"%s\"" % str(current_directory))

print("\nRegister DIA dll ? y/n")
if input() == "y":
	os.system("regsvr32 \"%s/DIA SDK/bin/amd64/msdia140.dll\"" % str(vs_path))
	
print("\nDone.")
os.system("pause")

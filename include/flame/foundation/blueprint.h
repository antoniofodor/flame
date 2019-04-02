// MIT License
// 
// Copyright (c) 2018 wjs
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

#pragma once

#include <flame/foundation/foundation.h>

namespace flame
{
	/*
		- A blueprint(BP) is a scene that represents relations between objects.
		- An object is called node in a BP.
		- A node is bound to an udt which its name started with 'BP_'.
		- The reflected members of the udt will be separated into inputs and outpus.
		- An input has an attribute 'i', and an output has an attribute 'o'.
		- Both inputs and outputs must be compatible with CommonData.
		- Only for inputs, if it is an Array<>, there will be multiple items of it,
		  else, there will be only one item of it
		- Address in BP: [node_id].[varible_name].[item_index]
		  you can use address to find an object in BP, e.g.
		  'a'     for node
		  'a.b'   for node input or output
		  'a.b.3' for item, index is default to 0
		- An available udt must:
			inherites from BP_Object and implements the update function
			all datas work fine with zero memory (basically, BP nodes will use udts to create an all-zeros
			 memory, (Array<>, String, StringW are fined with zero memory))
			have an nonparametric no-return-value function called 'update'
		- A BP file is basically a XML file, you can use both .xml or .bp.
	*/

	struct EnumInfo;
	struct VariableInfo;
	struct UdtInfo;

	struct BP
	{
		struct Node;
		struct Input;
		struct Output;

		struct Item
		{
			FLAME_FOUNDATION_EXPORTS Input *parent_i() const; // null or its parent is input
			FLAME_FOUNDATION_EXPORTS Output *parent_o() const; // null or its parent is output
			FLAME_FOUNDATION_EXPORTS CommonData &data();
			FLAME_FOUNDATION_EXPORTS void set_data(const CommonData &d); // setting datas for output's item is ok, but the data will be rushed when the node update

			FLAME_FOUNDATION_EXPORTS Item *link() const; // link is only storaged in input's item
			FLAME_FOUNDATION_EXPORTS bool set_link(Item *target); // it is vaild for input's item only

			FLAME_FOUNDATION_EXPORTS String get_address() const;
		};

		struct Input
		{
			FLAME_FOUNDATION_EXPORTS Node *node() const;
			FLAME_FOUNDATION_EXPORTS VariableInfo*variable_info() const;

			FLAME_FOUNDATION_EXPORTS int array_item_count() const;
			FLAME_FOUNDATION_EXPORTS Item *array_item(int idx) const;
			FLAME_FOUNDATION_EXPORTS Item *array_insert_item(int idx);
			FLAME_FOUNDATION_EXPORTS void array_remove_item(int idx);
			FLAME_FOUNDATION_EXPORTS void array_clear() const;
		};

		struct Output
		{
			FLAME_FOUNDATION_EXPORTS Node *node() const;
			FLAME_FOUNDATION_EXPORTS VariableInfo*variable_info() const;
			FLAME_FOUNDATION_EXPORTS Item *item() const;
		};

		struct Node
		{
			FLAME_FOUNDATION_EXPORTS BP *bp() const;
			FLAME_FOUNDATION_EXPORTS const char *id() const;
			FLAME_FOUNDATION_EXPORTS UdtInfo*udt() const;

			FLAME_FOUNDATION_EXPORTS int input_count() const;
			FLAME_FOUNDATION_EXPORTS Input *input(int idx) const;
			FLAME_FOUNDATION_EXPORTS int output_count() const;
			FLAME_FOUNDATION_EXPORTS Output *output(int idx) const;

			FLAME_FOUNDATION_EXPORTS Input *find_input(const char *name) const;
			FLAME_FOUNDATION_EXPORTS Output *find_output(const char *name) const;

			FLAME_FOUNDATION_EXPORTS bool enable() const;
			FLAME_FOUNDATION_EXPORTS void set_enable(bool enable) const;

			FLAME_FOUNDATION_EXPORTS void create() const;
			FLAME_FOUNDATION_EXPORTS void destroy() const;
			FLAME_FOUNDATION_EXPORTS void update() const;
		};

		FLAME_FOUNDATION_EXPORTS int node_count() const;
		FLAME_FOUNDATION_EXPORTS Node *node(int idx) const;
		FLAME_FOUNDATION_EXPORTS Node *add_node(const char *id, UdtInfo*udt);
		FLAME_FOUNDATION_EXPORTS void remove_node(Node *n);

		FLAME_FOUNDATION_EXPORTS Node *find_node(const char *id) const;
		FLAME_FOUNDATION_EXPORTS Input *find_input(const char *address) const;
		FLAME_FOUNDATION_EXPORTS Output *find_output(const char *address) const;
		FLAME_FOUNDATION_EXPORTS Item *find_item(const char *address) const;

		FLAME_FOUNDATION_EXPORTS void clear();

		// build data for 'update' or 'tobin'
		// let all notes create a piece of memory to represent the 'true' object and
		// determines update order
		FLAME_FOUNDATION_EXPORTS void prepare();
		// release the data that built by 'prepare'
		FLAME_FOUNDATION_EXPORTS void unprepare();

		// update the 'bp' using nodes
		FLAME_FOUNDATION_EXPORTS void update();

		FLAME_FOUNDATION_EXPORTS void save(const wchar_t *filename);
		// generate cpp codes that do the 'update' job and save to a file
		// use that file to compile a dll
		// note: need filename first and all nodes need to have 'code' function
		FLAME_FOUNDATION_EXPORTS void tobin();

		FLAME_FOUNDATION_EXPORTS static BP *create();
		FLAME_FOUNDATION_EXPORTS static BP *create_from_file(const wchar_t *filename);
		FLAME_FOUNDATION_EXPORTS static void destroy(BP *bp);
	};

	struct BP_Object // pod
	{
		virtual void update(float delta_time) = 0;
	};

	// here, we define some basic udt for blueprint nodes

	struct BP_Int$
	{
		int v$i;

		int v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Float$
	{
		float v$i;

		float v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Bool$
	{
		bool v$i;

		bool v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Vec2$
	{
		float x$i;
		float y$i;

		Vec2 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Vec3$
	{
		float x$i;
		float y$i;
		float z$i;

		Vec3 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Vec4$
	{
		float x$i;
		float y$i;
		float z$i;
		float w$i;

		Vec4 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Ivec2$
	{
		int x$i;
		int y$i;

		Ivec2 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Ivec3$
	{
		int x$i;
		int y$i;
		int z$i;

		Ivec3 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Ivec4$
	{
		int x$i;
		int y$i;
		int z$i;
		int w$i;

		Ivec4 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Bvec2$
	{
		uchar x$i;
		uchar y$i;

		Bvec2 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Bvec3$
	{
		uchar x$i;
		uchar y$i;
		uchar z$i;

		Bvec3 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};

	struct BP_Bvec4$
	{
		uchar x$i;
		uchar y$i;
		uchar z$i;
		uchar w$i;

		Bvec4 v$o;

		FLAME_FOUNDATION_EXPORTS void update();
	};
}


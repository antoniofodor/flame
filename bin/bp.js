window.onload = function(){
    var svg = document.getElementById("svg");
    svg.ns = svg.namespaceURI;

    var toolbar = document.getElementById("toolbar");

    function CreatePath() {
        var path = document.createElementNS(svg.ns, "path");
        path.setAttributeNS(null, "stroke", "#8e8e8e");
        path.setAttributeNS(null, "stroke-width", "2");
        path.setAttributeNS(null, "fill", "none");
        svg.appendChild(path);
        return path;
    }

    var enums = [];
    window.enums = enums;

    var udts = [];
    window.udts = udts;
    var find_udt = function(name){
        for (var i in udts)
        {
            var u = udts[i];
            if (u.name == name)
                return u;
        }
        return null;
    };

    var functions = [];
    window.functions = functions;

    var nodes = [];
    window.nodes = nodes;

    var staging_links = [];
    window.staging_links = staging_links;

    var load_typeinfo = function(json){
        for (var i in json.enums)
            enums.push(json.enums[i]);
        
        for (var i in json.udts)
            udts.push(json.udts[i]);
    
        for (var i in json.functions)
            functions.push(json.functions[i]);
    };

    var mouse = {
        curr_slot: null,
        path: CreatePath()
    };

    function FindNode(name) {
        for (var i in nodes)
        {
            var n = nodes[i];
            if (n.name == name)
                return n;
        }
        return null;
    }

    function GetGlobalOffset(element) {
        var offset = {
            top: element.offsetTop,
            left: element.offsetLeft
        };
    
        if (element.offsetParent) {
            var po = GetGlobalOffset(element.offsetParent);
            offset.top += po.top;
            offset.left += po.left;
            return offset;
        } 
        else 
            return offset;
    }

    function Node(udt_name, id, x, y) {
        this.name = id;

        this.eMain = document.createElement("div");
        this.eMain.classList.add("node");
        this.eMain.setAttribute("title", this.name);

        this.eMain.style.left = x + "px";
        this.eMain.style.top = y + "px";

        var thiz = this;
        $(this.eMain).draggable({
            containment: "window",
            cancel: ".slot",
            drag: function (event, ui) {
                thiz.UpdatePosition();
            }
        });

        this.eLeft = document.createElement("div");
        this.eLeft.style.display = "inline-block";
        this.eLeft.style.marginRight = "30px";
        this.eMain.appendChild(this.eLeft);

        this.eRight = document.createElement("div");
        this.eRight.style.display = "inline-block";
        this.eRight.style.float = "right";
        this.eMain.appendChild(this.eRight);

        this.inputs = [];
        this.outputs = [];

        var thiz = this;

        var sp = udt_name.split(":");
        var load = function(u_name){
            var udt = find_udt(u_name);
            if (!udt)
                return false;
            for (var i in udt.items)
            {
                var item = udt.items[i];
                if (item.attribute.indexOf("i") >= 0)
                    thiz.AddInput(item);
                else if (item.attribute.indexOf("o") >= 0)
                    thiz.AddOutput(item);
            }
            for (var i = 0; i < window.staging_links.length; i++)
            {
                var sl = window.staging_links[i];
    
                var addr_in = sl.in.split(".");
                var addr_out = sl.out.split(".");
    
                var n1 = FindNode(addr_in[0]);
                var n2 = FindNode(addr_out[0]);
                var input = n1.FindInput(addr_in[1]);
                var output = n2.FindOutput(addr_out[1]);
    
                if (input && output)
                {
                    input.links[0] = output;
                    output.links.push(input);

                    n1.UpdatePosition();
                    n2.UpdatePosition();

                    window.staging_links.splice(i, 1);
                    i--;
                }
            }
            return true;
        };
        if (sp.length == 1)
            console.assert(load(sp[0]));
        else
        {
            if (!load(sp[1]))
            {
                var url = sp[0].replace(/.dll/, ".typeinfo");
                $.getJSON(url, function(res, status){
                    if (status == "success")
                    {
                        load_typeinfo(res);
                        console.assert(load(sp[1]));
                    }
                    else
                        console.log("load failed: " + url);
                });
            }
        }
    }

    function Slot(vi, io) {
        this.vi = vi;
        this.node = null;
        this.links = [];
    
        this.eMain = document.createElement("div");

        this.eName = document.createElement("div");
        this.eName.innerHTML = vi.name;
        this.eName.style.display = "inline-block";
        
        this.eSlot = document.createElement("div");
        this.eSlot.innerHTML = '*';
        this.eSlot.classList.add("slot");

        var thiz = this;

        this.io = io;
        if (io == 0)
        {
            this.links.push(null);

            this.path = CreatePath();

            this.eMain.appendChild(this.eSlot);
            this.eMain.appendChild(this.eName);
    
            this.eSlot.onclick = function (e) {
                if (!mouse.curr_slot) {
                    if (thiz.links[0])
                    {
                        thiz.path.setAttributeNS(null, "d", "");

                        var o = thiz.links[0];
                        for (var i = 0; i < o.links; i++)
                        {
                            if (o.links[i] == thiz)
                            {
                                o.links.splice(i, 1);
                                break;
                            }
                        }
                        thiz.links[0] = null;
                    }

                    mouse.curr_slot = thiz;
                    mouse.path.setAttributeNS(null, "d", "");
                }
                e.stopPropagation();
            };
        }
        else
        {
            this.eMain.style.textAlign = "right";

            this.eMain.appendChild(this.eName);
            this.eMain.appendChild(this.eSlot);
    
            this.eSlot.onclick = function (e) {
                if (mouse.curr_slot && mouse.curr_slot.io == 0) {
                    mouse.curr_slot.links[0] = thiz;
                    thiz.links.push(mouse.curr_slot);

                    var a = mouse.curr_slot.GetPos();
                    var b = thiz.GetPos();
                    mouse.curr_slot.SetPath(a, b);

                    mouse.curr_slot = null;
                    mouse.path.setAttributeNS(null, "d", "");
                }
                e.stopPropagation();
            };
        }
    }

    Node.prototype.AddInput = function (item) {
        var s = new Slot(item, 0);
        s.node = this;
        this.inputs.push(s);
        this.eLeft.appendChild(s.eMain);
    };

    Node.prototype.AddOutput = function (item) {
        var s = new Slot(item, 1);
        s.node = this;
        this.outputs.push(s);
        this.eRight.appendChild(s.eMain);
    };

    Node.prototype.FindInput = function (name) {
        for (var i in this.inputs)
        {
            var s = this.inputs[i];
            if (s.vi.name == name)
                return s;
        }
        return null;
    };

    Node.prototype.FindOutput = function (name) {
        for (var i in this.outputs)
        {
            var s = this.outputs[i];
            if (s.vi.name == name)
                return s;
        }
        return null;
    };

    Node.prototype.UpdatePosition = function () {
        this.x = parseInt(this.eMain.style.left);
        this.y = parseInt(this.eMain.style.top);

        for (var i in this.inputs)
        {
            var s = this.inputs[i];
            if (s.links[0])
            {
                var a = s.GetPos();
                var b = s.links[0].GetPos();
        
                s.SetPath(a, b);
            }
        }
        for (var i in this.outputs)
        {
            var s = this.outputs[i];
            for (var j in s.links)
            {
                var t = s.links[j];

                var a = t.GetPos();
                var b = s.GetPos();
        
                t.SetPath(a, b);
            }

        }
    };
    
    Slot.prototype.GetPos = function () {
        var offset = GetGlobalOffset(this.eSlot);
        return {
            x: offset.left + this.eSlot.offsetWidth / 2,
            y: offset.top + this.eSlot.offsetHeight / 2
        };
    };

    Slot.prototype.SetPath = function (a, b) {
        this.path.setAttributeNS(null, "d", "M" + a.x + " " + a.y + " C" + (a.x - 50) + " " + a.y + " " + (b.x + 50) + " " + b.y + " " + b.x + " " + b.y);
    };

    window.onmousemove = function (e) {
        if (mouse.curr_slot) {
            var a = mouse.curr_slot.GetPos();
            var b = { x: e.pageX, y: e.pageY };
            mouse.path.setAttributeNS(null, "d", "M" + a.x + " " + a.y + " C" + (a.x - 50) + " " + a.y + " " + b.x + " " + b.y + " " + b.x + " " + b.y);
        }
    };
    
    window.onclick = function (e) {
        if (mouse.curr_slot) {
            mouse.path.setAttributeNS(null, "d", "");
            mouse.curr_slot = null;
        }
    };
    
    var wait_typeinfos = 5;
    var do_connect = function(){
        var sock_s = new WebSocket("ws://localhost:5566/");
        window.sock_s = sock_s;
        sock_s.onmessage = function(res){
            for (var i in nodes)
            {
                var n = nodes[i];
    
                document.body.removeChild(n.eMain);
    
                for (var j in n.inputs)
                    svg.removeChild(n.inputs[j].path);
            }
    
            nodes = [];
    
            var src = eval('(' + res.data + ')');
    
            var src_nodes = src.nodes;
            for (var i in src_nodes)
            {
                var sn = src_nodes[i];
                var n = new Node(sn.udt_name, sn.id, sn.x, sn.y);
                for (var item in sn)
                {
                    if (item == "input")
                    {
    
                    }
                }
                nodes.push(n);
            
                n.eMain.style.position = "absolute";
                document.body.appendChild(n.eMain);
            }
    
            window.staging_links = src.links;
    
            for (var i in nodes)
                nodes[i].UpdatePosition();
            
        };
        sock_s.onclose = function(){
            setTimeout(function(){
                var s = new WebSocket("ws://localhost:5566/");
                s.onmessage = window.sock_s.onmessage;
                s.onclose = window.sock_s.onclose;
                window.sock_s = s;
            }, 2000);
        };
    };
	$.getJSON("flame_foundation.typeinfo", function(res){
        load_typeinfo(res);
        wait_typeinfos--;
        if (wait_typeinfos == 0)
            do_connect();
	});
	$.getJSON("flame_network.typeinfo", function(res){
        load_typeinfo(res);
        wait_typeinfos--;
        if (wait_typeinfos == 0)
            do_connect();
	});
	$.getJSON("flame_graphics.typeinfo", function(res){
        load_typeinfo(res);
        wait_typeinfos--;
        if (wait_typeinfos == 0)
            do_connect();
	});
	$.getJSON("flame_sound.typeinfo", function(res){
        load_typeinfo(res);
        wait_typeinfos--;
        if (wait_typeinfos == 0)
            do_connect();
	});
	$.getJSON("flame_universe.typeinfo", function(res){
        load_typeinfo(res);
        wait_typeinfos--;
        if (wait_typeinfos == 0)
            do_connect();
	});

    /*
    var n = new Node('test', 0, 0);
    nodes.push(n);
    n.eMain.style.position = "absolute";
    document.body.appendChild(n.eMain);
    */
};

function on_save_clicked()
{
    var sock_s = window.sock_s;
    if (!sock_s)
        return;

    var dst = {};
    dst.nodes = [];
    for (var i in window.nodes)
    {
        var sn = window.nodes[i];
        var n = {};
        n.name = sn.name;
        n.x = sn.x;
        n.y = sn.y;
        dst.nodes.push(n);
    }
    for (var i in window.nodes)
    {
        
    }
    dst.links = [];

    var json = JSON.stringify(dst);
    sock_s.send(json);
}

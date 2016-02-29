ONE_EDGE_WIDTH = 30;
INITIAL_EDGE_OFFSET = 10;
ARROW_IMAGE_SIZE = 7;
DEFAULT_COLOR = "#E0E0E0";
TEXT_COLOR = "#101010";
ERROR_COLOR = "#FF8787";
SELECTED_COLOR = "#d7e7f7";
// Edge

function Edge(params) {
	this.params = params;

	if (this.params.graph) {
		this.init_elements();
	}
}

Edge.get_id = function(edge) {
	return edge.params.from + ':' + edge.params.to + ':' + edge.params.label;
}

Edge.prototype.init_elements = function () {
	var edge_id = Edge.get_id(this);

	this.params.graph.params.container.append(
		'<div'
			+ ' id="first_edge_' + edge_id + '"'
			+ ' class="connector edge_' + edge_id + '"'
			+ ' style="position: absolute; overflow: hidden; z-index: 1;"'
		+'>'
		+ '</div>'
		+ '<div'
			+ ' id="second_edge_' + edge_id + '"'
			+ ' class="connector edge_' + edge_id + '"'
			+ ' style="position: absolute; overflow: hidden; z-index: 1;"'
		+'>'
		+ '</div>'
		+ '<div'
			+ ' id="third_edge_' + edge_id + '"'
			+ ' class="connector edge_' + edge_id + '"'
			+ ' style="position: absolute; overflow: hidden; z-index: 1;"'
		+'>'
		+ '</div>'
		+ '<label'
			+ ' id="label_' + edge_id + '"'
			+ ' class="destination-label edge_' + edge_id + '"'
			+ ' style="position: absolute; overflow: hidden; z-index: 2; background-color:white"'
		+ '>'
			+ (this.params.label || '')
		+ '</label>'
		+ '<img'
			+ ' class="edge_' + edge_id + '"'
			+ ' src="arrow_r.gif"'
			+ ' id="arrow_' + edge_id + '"'
			+ ' style="position: absolute; overflow: hidden;"'
		+ '>'
	);
}

Edge.change_segment = function(segment, start_x, start_y, stop_x, stop_y) {
	if (start_x == stop_x) {
		segment.css('width', '2px');
		segment.css('height', Math.abs(start_y - stop_y) + 'px');
		segment.css('left', start_x + 'px');
		segment.css('top', Math.min(start_y, stop_y) + 'px');
	} else if (start_y == stop_y) {
		segment.css('width',  Math.abs(start_x - stop_x) + 'px');
		segment.css('height', '2px');
		segment.css('left', Math.min(start_x, stop_x) + 'px');
		segment.css('top', start_y + 'px');
	} else {
		throw new Error("cannot draw segment: not vertical or horizontal");
	}
}

Edge.place_label_near_segment = function(label, segment) {
	var center_x = segment.position().left + segment.outerWidth() / 2;
	var center_y = segment.position().top + segment.outerHeight() / 2;

	label.css('left', center_x - label.outerWidth() / 2);
	label.css('top', center_y - label.outerHeight() / 2);
}

Edge.prototype.render = function () {
	var from = this.params.vertex_from.get_vertex_div();
	var to = this.params.vertex_to.get_vertex_div();

	var edge_id = Edge.get_id(this);
	var first_edge = this.params.graph.params.container.find('div[id="first_edge_' + edge_id + '"]');
	var second_edge = this.params.graph.params.container.find('div[id="second_edge_' + edge_id + '"]');
	var third_edge = this.params.graph.params.container.find('div[id="third_edge_' + edge_id + '"]');
	var image = this.params.graph.params.container.find('img[id="arrow_' + edge_id + '"]');
	var label = this.params.graph.params.container.find('label[id="label_' + edge_id + '"]');

	first_edge.show();
	second_edge.show();
	third_edge.show();
	image.show();
	label.show();

	this.params.vertex_from.remove_bad_edge(this);
	this.params.vertex_to.remove_bad_edge(this);

	var shift = -0.5;

	if (
		from.position().left + from.outerWidth() < to.position().left
	) {

		var from_x = from.position().left + from.outerWidth();
		var from_y = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.from_index + from.position().top;
		var to_x = to.position().left;
		var to_y = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.to_index + to.position().top;
		var middle_x = (
			(to.position().left + from.position().left + from.outerWidth()) / 2
			//- (to_x - from_x) / ((this.to_index + 1) * 4)
		);

		Edge.change_segment(first_edge, from_x, from_y, middle_x, from_y);
		Edge.change_segment(second_edge, middle_x, from_y, middle_x, to_y);
		Edge.change_segment(third_edge, middle_x, to_y, to_x, to_y);

		image.attr('src', 'arrow_r.gif');
		image.css('left', (to_x - ARROW_IMAGE_SIZE) + 'px');
		image.css('top', (to_y - ARROW_IMAGE_SIZE / 2) - shift + 'px');
	} else if (
		to.position().left + to.outerWidth() < from.position().left
	) {
		var from_x = from.position().left;
		var from_y = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.from_index + from.position().top;
		var to_x = to.position().left + to.outerWidth()
		var to_y = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.to_index + to.position().top;
		var middle_x = (
			(to.position().left + to.outerWidth() + from.position().left) / 2
			//+ (from_x - to_x) / ((this.to_index + 1) * 4)
		);


		Edge.change_segment(first_edge, from_x, from_y, middle_x, from_y);
		Edge.change_segment(second_edge, middle_x, from_y, middle_x, to_y);
		Edge.change_segment(third_edge, middle_x, to_y, to_x, to_y);

		image.attr('src', 'arrow_l.gif');
		image.css('left', to_x + 'px');
		image.css('top', (to_y - ARROW_IMAGE_SIZE / 2) - shift + 'px');
	} else if (
		to.position().top + to.outerHeight() < from.position().top
	) {
		var from_x = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.from_index + from.position().left;
		var from_y = from.position().top;
		var to_x = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.to_index + to.position().left;
		var to_y = to.position().top + to.outerHeight();
		var middle_y = (
			(to.position().top + to.outerHeight() + from.position().top) / 2
			//- (to_y - to_x) / ((this.to_index + 1) * 4)
		);

		Edge.change_segment(first_edge, from_x, from_y, from_x, middle_y);
		Edge.change_segment(second_edge, from_x, middle_y, to_x, middle_y);
		Edge.change_segment(third_edge, to_x, middle_y, to_x, to_y);

		image.attr('src', 'arrow_u.gif');
		image.css('left', (to_x - ARROW_IMAGE_SIZE / 2) - shift + 'px');
		image.css('top', to_y + 'px');
	} else if (
		from.position().top + from.outerHeight() < to.position().top
	) {
		var from_x = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.from_index + from.position().left;
		var from_y = from.position().top + from.outerHeight();
		var to_x = INITIAL_EDGE_OFFSET + ONE_EDGE_WIDTH * this.to_index + to.position().left;
		var to_y = to.position().top;
		var middle_y = (
			(to.position().top + from.position().top + from.outerHeight()) / 2
			//+ (to_y - to_x) / ((this.to_index + 1) * 4)
		);

		Edge.change_segment(first_edge, from_x, from_y, from_x, middle_y);
		Edge.change_segment(second_edge, from_x, middle_y, to_x, middle_y);
		Edge.change_segment(third_edge, to_x, middle_y, to_x, to_y);

		image.attr('src', 'arrow_d.gif');
		image.css('left', (to_x - ARROW_IMAGE_SIZE / 2) - shift + 'px');
		image.css('top', (to_y - ARROW_IMAGE_SIZE) + 'px');
	} else {
		first_edge.hide();
		second_edge.hide();
		third_edge.hide();
		image.hide();
		label.hide();
		this.params.vertex_from.add_bad_edge(this);
		this.params.vertex_to.add_bad_edge(this);
	}

	Edge.place_label_near_segment(label, third_edge);
}

Edge.prototype.remove = function () {
	var edge_id = Edge.get_id(this);
	this.params.graph.params.container.find('[class*="edge_' + edge_id + '"]').remove();
}


// Vertex
function Vertex(params) {
	this.params = params;
	this.edges = new Set([], {"id_func" : Edge.get_id});
	this.incoming_edges = new Set([], {"id_func" : Edge.get_id});
	this.outgoing_edges =  new Set([], {"id_func" : Edge.get_id});
	this.bad_edges = new Set([], {"id_func" : Edge.get_id});

	if (this.params.graph) {
		this.init_elements();
		this.render();
		this.init_mouse_move();
	}
}

Vertex.prototype.add_bad_edge = function (edge) {
	this.bad_edges.add(edge);
	this.get_vertex_div().css('background-color', ERROR_COLOR);
}

Vertex.prototype.remove_bad_edge = function (edge) {
	this.bad_edges.remove(edge);
	if (this.bad_edges.size() == 0) {
		this.get_vertex_div().css('color', TEXT_COLOR);
		this.get_vertex_div().css('background-color', DEFAULT_COLOR);

	}
}

Vertex.prototype.stop_dragging = function () {
	var vertex_div = this.get_vertex_div();
	vertex_div.removeClass('draggable');
	vertex_div.unbind('mousedown', this.mouse_down_handler);
}

Vertex.prototype.start_dragging = function () {
	var vertex_div = this.get_vertex_div();
	vertex_div.addClass('draggable');
	vertex_div.bind('mousedown', this.mouse_down_handler);
}

Vertex.prototype.enable_select_mode = function () {
	var vertex_div = this.get_vertex_div();

	vertex_div.bind('mouseover', this.selecting_mouse_over_handler);
	vertex_div.bind('mouseout', this.selecting_mouse_out_handler);
	vertex_div.bind('click', this.selecting_click_handler);
}

Vertex.prototype.disable_select_mode = function () {
	var vertex_div = this.get_vertex_div();

	this.deselect();

	vertex_div.unbind('mouseover', this.selecting_mouse_over_handler);
	vertex_div.unbind('mouseout', this.selecting_mouse_out_handler);
	vertex_div.unbind('click', this.selecting_click_handler);
}

Vertex.prototype.deselect = function () {
	var vertex_div = this.get_vertex_div();

	vertex_div.css('border-width', '2px');
	vertex_div.css('color', TEXT_COLOR);
	vertex_div.css('background-color', DEFAULT_COLOR);
}

Vertex.prototype.init_mouse_move = function () {
	var _this = this;
	var vertex_div = this.get_vertex_div();

	var mouse_move_handler = function (e) {
		var x = e.pageX;
		var y = e.pageY;
		var container = _this.params.graph.params.container;
		if (
			x >= container.position().left
			&& x <= container.position().left + container.width()
			&& y >= container.position().top
			&& y <= container.position().top + container.height()
		) {
			vertex_div.css('left', (x - vertex_div.outerWidth() / 2) + 'px');
			vertex_div.css('top', (y - vertex_div.outerHeight() / 2) + 'px');
			_this.render_edges();
			_this.save_coordinates();
		}
	}

	this.mouse_down_handler = function (e) {
		_this.params.graph.params.container.addClass('unselectable');
		_this.params.graph.params.container.bind('mousemove', mouse_move_handler);
	}

	this.selecting_mouse_over_handler = function (e) {
		vertex_div.css('border-width', '5px');
		vertex_div.css('top', (vertex_div.position().top - 4) + 'px');
		vertex_div.css('left', (vertex_div.position().left - 4) + 'px');
	}

	this.selecting_mouse_out_handler = function (e) {
		vertex_div.css('border-width', '2px');
		vertex_div.css('top', (vertex_div.position().top + 4) + 'px');
		vertex_div.css('left', (vertex_div.position().left + 4) + 'px');
	}

	this.selecting_click_handler = function (e) {
		vertex_div.css('background-color', SELECTED_COLOR);
	}

	_this.start_dragging();

	_this.params.graph.params.container.bind('mouseup', function (e) {
		_this.params.graph.params.container.removeClass('unselectable');
		_this.params.graph.params.container.unbind('mousemove', mouse_move_handler);
	});
}

Vertex.prototype.get_vertex_div = function () {
	return this.params.graph.params.container.find('div[id="' + this.params.id + '"]');
}

Vertex.prototype.init_elements = function () {
	this.params.graph.params.container.append(
			'<div'
			+ ' id=' + this.params.id
			+ ' class="block draggable"'
			+ '>'
			+ (this.params.content || '')
			+ '</div>'
			);

	var vertex_div = this.get_vertex_div();

	vertex_div.css('border-bottom-left-radius', '5px');
	vertex_div.css('border-bottom-right-radius', '5px');
	vertex_div.css('border-top-left-radius', '5px');
	vertex_div.css('border-top-right-radius', '5px');

	vertex_div.css('position', 'absolute');
	vertex_div.css('overflow', 'hidden');
	vertex_div.css('display', 'block');
	var saved_coordinates = this.get_saved_coordinates();
	vertex_div.css('left', saved_coordinates.x + 'px');
	vertex_div.css('top', saved_coordinates.y + 'px');
	vertex_div.css('z-index', 2);
	this.save_coordinates();
}

Vertex.prototype.get_saved_coordinates = function () {
	var x = (
		read_cookie(this.params.id + '_x') || (
			this.params.graph.params.container.position().left
			+ Math.floor(Math.random() * this.params.graph.params.container.outerWidth() / 20)
			* 10
		)
	);
	var y = (
		read_cookie(this.params.id + '_y') || (
			this.params.graph.params.container.position().top
			+ Math.floor(Math.random() * this.params.graph.params.container.outerHeight() / 20)
			* 10
		)
	);

	return {x: x, y: y};
}

Vertex.prototype.save_coordinates = function () {
	var vertex_div = this.get_vertex_div();

	create_cookie(this.params.id + '_x', vertex_div.position().left, 1000);
	create_cookie(this.params.id + '_y', vertex_div.position().top, 1000);
}

Vertex.prototype.render_edges = function () {
	this.render();
	this.edges.each(function (edge) {
		edge.render();
	});
}

Vertex.prototype.render = function () {
	var vertex_div = this.get_vertex_div();

	var min_size = ONE_EDGE_WIDTH * this.edges.size();

	if (vertex_div.outerWidth() < min_size) {
		vertex_div.width(min_size);
	}

	if (vertex_div.outerHeight() < min_size) {
		vertex_div.height(min_size);
	}
}

Vertex.prototype.remove = function () {
	var vertex_div = this.get_vertex_div();

	vertex_div.remove();
}

Vertex.get_id = function (vertex) {
	return vertex.params.id;
}

Vertex.prototype.add_incoming_edge = function (edge) {
	edge.to_index = this.edges.size();
	this.edges.add(edge);
	this.incoming_edges.add(edge);
}

Vertex.prototype.add_outgoing_edge = function (edge) {
	edge.from_index = this.edges.size();
	this.edges.add(edge);
	this.outgoing_edges.add(edge);
}

Vertex.prototype.resend_indexes = function (edge) {
	var _this = this;
	var index = 0;
	this.edges.each(function (edge) {
		if (_this.incoming_edges.count(edge)) {
			edge.to_index = index++;
		} else {
			edge.from_index = index++;
		}
	});
}

Vertex.prototype.remove_incoming_edge = function (edge) {
	this.edges.remove(edge);
	this.incoming_edges.remove(edge);
	this.resend_indexes();
}

Vertex.prototype.remove_outgoing_edge = function (edge) {
	this.edges.remove(edge);
	this.outgoing_edges.remove(edge);
	this.resend_indexes();
}

// Graph
function Graph(params) {
	this.params = params;
	this.vertices = new Set([], {"id_func" : Vertex.get_id});
	this.edges = new Set([], {"id_func" : Edge.get_id});
}

Graph.prototype.get_vertex_by_id = function (id) {
	return this.vertices.get(new Vertex({'id':id}));
}

Graph.prototype.has_vertex = function (params) {
	return this.vertices.count(new Vertex(params));
}


Graph.prototype.add_vertex = function (params) {
	params.graph = this;

	this.vertices.add(new Vertex(params));
}

Graph.prototype.remove_vertex = function (params) {
	var _this = this;

	var vertex = this.get_vertex_by_id(params.id);
	vertex.edges.each(function (edge) {
		_this.remove_edge(edge.params);
	});

	vertex.remove();
	this.vertices.remove(vertex);
}

Graph.prototype.enhance_edge_params = function (params) {
	params.graph = this;

	params.vertex_from = this.get_vertex_by_id(params.from);
	params.vertex_to = this.get_vertex_by_id(params.to);
}

Graph.prototype.add_edge = function (params) {
	if (this.edges.count(new Edge(params)) == 0) {
		this.enhance_edge_params(params);

		var edge = new Edge(params);

		params.vertex_from.add_outgoing_edge(edge);
		params.vertex_to.add_incoming_edge(edge);
		params.vertex_from.render_edges();
		params.vertex_to.render_edges();

		edge.render();

		this.edges.add(edge);
	}
}

Graph.prototype.launch_blocks_func = function (func_name) {
	this.vertices.each(function (vertex) {
		vertex[func_name]();
	});

}

Graph.prototype.start_dragging = function () {
	this.launch_blocks_func('start_dragging');
}

Graph.prototype.stop_dragging = function () {
	this.launch_blocks_func('stop_dragging');
}

Graph.prototype.enable_select_mode = function () {
	this.launch_blocks_func('enable_select_mode');
}

Graph.prototype.disable_select_mode = function () {
	this.launch_blocks_func('disable_select_mode');
}

Graph.prototype.deselect_all = function () {
	this.launch_blocks_func('deselect');
}

Graph.prototype.remove_edge = function (params) {
	var edge = this.edges.get(new Edge(params));

	this.enhance_edge_params(params);

	params.vertex_from.remove_outgoing_edge(edge);
	params.vertex_to.remove_incoming_edge(edge);
	params.vertex_from.render_edges();
	params.vertex_to.render_edges();

	edge.remove();
	this.edges.remove(edge);
}

Graph.prototype.to_json = function () {
	var json_graph = {
		"vertices" : [],
		"edges" : [],
	};

	this.vertices.each(function (vertex) {
		json_graph["vertices"].push(vertex.params.id);
	});

	this.edges.each(function (edge) {
		json_graph["edges"].push({
			"from" : edge.params.from,
			"to" : edge.params.to,
		});
	});

	return json_graph;
}

Graph.prototype.remove = function() {
	this.vertices.each(function (vertex) {
		vertex.remove();
	});
	this.edges.each(function (edge) {
		edge.remove();
	});
}



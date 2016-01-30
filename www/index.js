function call_or_get(object, context) {
	if (jQuery.isFunction(object)) {
		return object(context);
	} else {
		return object;
	}
}

$(function() {
	function sec() {
		alert("Are you stupid? I told you! Don\'t touch it!");
	}

	function first() {
		$('#gan').unbind('click', first);
		$('#gan').bind('click', sec);
		alert('Don\'t touch it!');
	}

	$('#gan').bind('click', first);

	new State({
		'start' : new Combine([
			new Builder({
				'container' : $('#get_graph_button'),
				'func' : function (context, container) {
					container.append(
						'<div class="text-center">'
							+ '<p>'
									+ '<button'
									+ ' id=create_new_graph'
									+ ' class="btn btn-lg"'
									+ ' type="button"'
									+ ' style="background-color: #101010; color:#9d9d9d;"'
								+'>'
									+ 'Создать новый граф'
								+'</button>'
								+ '&nbsp&nbsp&nbsp'
								+ '<button'
									+ ' id=download_graph'
									+ ' class="btn btn-lg"'
									+ ' type="button"'
									+ ' style="background-color: #101010; color:#9d9d9d;"'
								+'>'
									+'Загрузить граф'
								+'</button>'
								+ '&nbsp&nbsp&nbsp'
								+ '<button'
									+ ' id=delete_graph'
									+ ' class="btn btn-lg"'
									+ ' type="button"'
									+ ' style="background-color: #101010; color:#9d9d9d;"'
								+'>'
									+'Удалить граф'
								+'</button>'

							+ '</p>'
						+ '</div>'
					);
				},
			}),
			new Binder({
				'target' : function() {
					return $('#download_graph');
				},
			    	'action' : 'click',
			    	'type' : 'next',
			    	'new_state' : 'set_edit_graph',
			}),
			new Binder({
				'target' : function() {
					return $('#create_new_graph');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'set_create_graph',
			}),
			new Binder({
				'target' : function() {
					return $('#delete_graph');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'set_delete_graph',
			}),
		]),
		'set_delete_graph' : new Combine([
			new Executer(function(context) {
				context.remove_graph = true;
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'set_edit_graph',
			}),
		]),
		'set_edit_graph' : new Combine([
			new Executer(function (context) {
				context.create = false;
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph',
			}),
		]),
		'set_create_graph' : new Combine([
			new Executer(function (context) {
				context.create = true;
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph',
			}),
		]),
		'edit_graph' : new Combine([
			new Builder({
				'container' : $('body'),
				'func' : function(context, container) {
					container.append(
						'<div id="mainCanvas" class=canvas style="border-color:black; border-width: 3px;  border-style: double;  width: 100%; height: 1000px;"></div>'
					);
					context.graph = new Graph({
						'container' : $('#mainCanvas'),
					});
				},

			}),
			new GoTo({
				'new_state' : 'edit_graph::get_graph_list',
				'type' : 'substate',
			}),
		]),
		'edit_graph::get_graph_list' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'graphs',
				}
			},
	      		'write_to' : 'graphs_names',
			'type' : 'next',
			'new_state' : function (context) {
				if (context.parent.create) {
					return 'edit_graph::set_new_graph';
				} else {
					return 'edit_graph::check_graphs_count';
				}
			},
		}),
		'edit_graph::check_graphs_count' : new GoTo({
			'type' : 'next',
			'new_state' : function(context) {
				if (context.graphs_names.length > 0) {
					return 'edit_graph::set_graph';
				} else {
					alert("No graphs.");
					return 'edit_graph::cancel_edit';
				}
			}
		}),
		'edit_graph::set_new_graph' : new Combine([
			new BDialog({
				'id' : 'new_create_graph_dialog',
				'title' : 'Create graph.',
				'data' : function(context) {
					return 	(
						'<div class="input-group">'
							+'<span class="input-group-addon" id="basic-addon1">'
								+ 'Graph name'
							+ '</span>'
							+'<input'

								+ ' type="text"'
								+ ' class="form-control"'
								+ ' placeholder="new_graph_name"'
								+ ' aria-describedby="basic-addon1"'
								+ ' id=new_graph_name'
							+ '>'
						+ '</div><br>'
					);
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new GoTo({
				'new_state' : 'edit_graph::set_new_graph::listen',
				'type' : 'substate',
			}),
		]),
		'edit_graph::set_new_graph::listen' : new Combine([
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_new_graph::check_graph_name',
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::cancel_edit',
			}),
		]),
		'edit_graph::set_new_graph::check_graph_name' : new Combine([
			new Executer(function(context) {
				context.is_graph_exist = false;
				for (var i = 0; i < context.parent.graphs_names.length; ++i) {
					if ($('#new_graph_name').val() == context.parent.graphs_names[i].GraphName) {
						context.is_graph_exist = true;
						return;
					}
				}
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : function(context) {
					if ($('#new_graph_name').val() === "") {
						alert("Input of graph name is emplty.");
						return 'edit_graph::set_new_graph::listen';
					} else if (context.is_graph_exist) {
						alert(
							'Graph with name '
							+ $('#new_graph_name').val()
							+ ' already exist'
						);
						return 'edit_graph::set_new_graph::listen';
					} else {
						return 'edit_graph::set_new_graph::create_graph';
					}
				},
			}),
		]),
		'edit_graph::set_new_graph::create_graph' : new SendQuery({
			'ajax_data' : function(context) {
				context.parent.parent.graph_name = $('#new_graph_name').val();
				return {
					'type' : 'create',
		 			'object' : 'graph',
		 			'graph' : context.parent.parent.graph_name,
				};
			},
			'type' : 'exit_state',
			'new_state' : 'edit_graph::listen',
		}),
		'edit_graph::set_graph' : new Combine([
			new BDialog({
				'id' : 'new_graph_dialog',
				'title' : 'Choose graph.',
				'data' : function(context) {
					var select_html = '';
					for (var i = 0; i < context.graphs_names.length; ++i) {
						select_html += (
							'<option '
								+ 'value=' +  context.graphs_names[i].GraphName
							+ '>'
								+ context.graphs_names[i].GraphName
							+ '</option>'
						);
					}
					return 	(
						'<div class="input-group">'
							+'<span class="input-group-addon" id="basic-addon1">'
								+ 'Graph name'
							+ '</span>'
							+ '<select'
								+ ' id=graph_names'
								+ ' class="form-control input-sm"'
								+ ' style="width:50%"'
							+ ' >'
								+ select_html
							+ '</select>'
						+ '</div><br>'
					);
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new GoTo({
				'new_state' : 'edit_graph::set_graph::listen',
		       		'type' : 'substate'
			}),
		]),
		'edit_graph::set_graph::listen' : new Combine ([
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : function(context) {
					if (context.parent.parent.remove_graph){
						return 'edit_graph::set_graph::delete_graph';
					} else {
						return 'edit_graph::set_graph::save_graph_name';
					}
				},
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::cancel_edit',
			}),
		]),
		'edit_graph::set_graph::save_graph_name' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph_name  = $('#graph_names').val();
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::get_vertices',
			}),
		]),
		'edit_graph::cancel_edit' : new GoTo({
			'type' : 'exit_state',
			'new_state' : 'start',
		}),
		'edit_graph::set_graph::delete_graph' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'delete',
				 	'object' :  'graph',
				 	'graph' : $('#graph_names').val(),
				};
			},
			'type' : 'exit_state',
			'new_state' : 'edit_graph::cancel_edit'
		}),
		'edit_graph::get_vertices' : new Combine([
			new Executer(function(context) {
				$('.modal-backdrop').remove();
			}),
			new SendQuery({
				'ajax_data' : function(context) {
					return {
						'type' : 'show',
						'object' : 'blocks',
						'graph' : context.parent.graph_name,
					};
				},
				'write_to' : 'blocks',
				'type' : 'next',
				'new_state' : 'edit_graph::get_edges',
			}),
		]),
		'edit_graph::get_edges' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'edges',
					'graph' : context.parent.graph_name,
				};
			},
			'write_to' : 'edges',
			'type' : 'next',
			'new_state' : 'edit_graph::draw',
		}),
		'edit_graph::draw' : new Combine([
			new Executer(function(context) {
				for (var i = 0; i < context.blocks.length; ++i) {
					context.parent.graph.add_vertex({
						'id' : context.blocks[i].Name,
						'content' : (
							context.blocks[i].Name
							+ ' : '
							+ context.blocks[i].Type
						),
					});
				}
				for (var i = 0; i < context.edges.length; ++i) {
					context.parent.graph.add_edge({
						'from' : context.edges[i].From,
						'to' : context.edges[i].To,
						'label' : context.edges[i].EdgeName,
					});
				}
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::listen',
			})
		]),
		'edit_graph::listen': new Combine([
			new Enabler({
				'target' : function() {
					return $('#new_vertex_link');
				}
			}),
			new Enabler({
				'target' : function() {
					return $('#delete_vertex_link');
				}
			}),
			new Enabler({
				'target' : function() {
					return $('#finish_editing_link');
				}
			}),
			new Enabler({
				'target' : function() {
					return $('#new_edge_link');
				},
			}),
			new Enabler({
				'target' : function() {
					return $('#delete_edge_link');
				},
			}),
			new Enabler({
				'target' : function() {
					return $('#show_params_link');
				},
			}),
			new Enabler({
				'target' : function() {
					return $('#send_points_link');
				},
			}),
			new Enabler({
				'target' : function() {
					return $('#fit_graph_link');
				},
			}),
			new Binder({
				'target' : function () {
					return $('#new_vertex');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::get_vertex_types',
			}),
			new Binder({
				'target' : function() {
					return $('#finish_editing');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'start',
			}),
			new Binder({
				'target' : function() {
					return $('#delete_vertex');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::delete_vertex',
			}),
			new Binder({
				'target' : function() {
					return $('#new_edge');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::create_edges',
			}),
			new Binder({
				'target' : function() {
					return $('#delete_edge');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::delete_edges',
			}),
			new Binder({
				'target' : function() {
					return $('#show_params');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::change_params',
			}),
			new Binder({
				'target' : function() {
					return $('#send_points');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::deploy_graph',
			}),
			new Binder({
				'target' : function() {
					return $('#fit_graph');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_points_for_fit'
			}),
		]),
		'edit_graph::set_points_for_fit' : new Combine([
			new BDialog({
				'id' : 'new_points_dialog',
				'title' : 'Set points for fit graph',
				'data' : function(context) {
					return 	(
						'<div id=new_points_dialog title="Set points for fit graph">'
							+ '<textarea class="form-control" rows="5" id=points placeholder="series_name 1452291138 -3.4 <1 if point is bad or 0> [target_block]">'
							+ '</textarea>'
						+ '</div>'
					);
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::set_points_for_fit::listen',
			}),
		]),
		'edit_graph::set_points_for_fit::listen' : new Combine([
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_points_for_fit::fit_graph',
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
		]),
		'edit_graph::set_points_for_fit::fit_graph' : new SendQuery({
			'ajax_data' : function(context) {
				var points_info = $('#points').val().split('\n');
				var points = [];
				for (var i = 0; i < points_info.length; ++i) {
					var point = points_info[i].split(/\s+/);
					if (point.length >= 3) {
						var point_hash = {
							'series' : point[0],
							'time' : parseFloat(point[1]),
							'value' : parseFloat(point[2]),
							'is_bad' : parseFloat(point[3]),
						};
						if (point.length >= 5 && point[4] !== '') {
							point_hash['block'] = point[4];
						}
						points.push(point_hash);
					}
				}
				return {
					'type' : 'fit_graph',
					'graph' : context.parent.parent.graph_name,
					'train_data' : points,
					'build_graph_method' : 'take_initial',
					'ml_method' : 'SVM',
					'iteration_count' : 1,
				}
			},
			'type' : 'next',
			'new_state' : 'edit_graph::set_points_for_fit::redraw_graph'

		}),
		'edit_graph::set_points_for_fit::redraw_graph' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph.remove();
				context.parent.is_graph_chosen = true;
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::get_vertices',
			}),
		]),
		'edit_graph::deploy_graph' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'check_graph',
					'object' : 'graph',
					'graph' : context.parent.graph_name,
				};
			},
			'write_to' : 'graph_info',
			'type' : 'next',
			'new_state' : 'edit_graph::check_graph',
		}),
		'edit_graph::check_graph' : new Combine([
			new Executer(function(context) {
				if (context.graph_info.Status == '0') {
					alert('Invalid graph: ' + context.graph_info.Reason);
				}
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : function (context) {
					if (context.graph_info.Status == '0') {
						return 'edit_graph::listen';
					} else {
						return 'edit_graph::set_points';
					}
				},
			}),
		]),
		'edit_graph::set_points' : new Combine([
			new BDialog({
				'id' : 'new_points_dialog',
				'title' : 'Set points',
				'data' : function(context) {
					return 	(
						'<div id=new_points_dialog title="Set points">'
							+ '<textarea class="form-control" rows="5" id=points placeholder="series_name 1452291138 -3.4 [target_block]">'
							+ '</textarea>'
						+ '</div>'
					);
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::set_points::listen',
			}),
		]),
		'edit_graph::set_points::listen' : new Combine([
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_points::enable_debug_mode',
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
		]),
		'edit_graph::set_points::enable_debug_mode' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'debug',
					'object' : 'mode',
					'graph' : context.parent.parent.graph_name,
					'enable' : true,
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::set_points::send_points',
		}),
		'edit_graph::set_points::send_points' : new SendQuery({
			'ajax_data' : function(context) {
				var points_info = $('#points').val().split('\n');
				var points = [];
				for (var i = 0; i < points_info.length; ++i) {
					var point = points_info[i].split(/\s+/);
					if (point.length >= 3) {
						var point_hash = {
							'series' : point[0],
							'time' : parseFloat(point[1]),
							'value' : parseFloat(point[2]),
						};
						if (point.length >= 4 && point[3] !== '') {
							point_hash['block'] = point[3];
						}
						points.push(point_hash);
					}
				}
				return {
					'type' : 'insert',
					'graph' : context.parent.parent.graph_name,
					'points' : points,
					'ignore' : 0,
				}
			},
			'type' : 'next',
			'new_state' : 'edit_graph::set_points::get_debug_info',
		}),
		'edit_graph::set_points::get_debug_info' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'debug',
					'object' : 'info',
					'graph' : context.parent.parent.graph_name,
				};
			},
			'write_to' : 'debug_info',
			'type' : 'next',
			'new_state' : 'edit_graph::set_points::disable_debug_mode',
		}),
		'edit_graph::set_points::disable_debug_mode' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'debug',
					'object' : 'mode',
					'graph' : context.parent.parent.graph_name,
					'enable' : false,
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::set_points::parse_debug_info',
		}),
		'edit_graph::set_points::parse_debug_info' : new Combine([
			new Executer(function(context) {
				context.parent.block_series_name = {};
				context.parent.block_points = {};
				for (var i = 0; i < context.debug_info.length; ++i) {
					var event = JSON.parse(context.debug_info[i].Event);
					if (event.type == 'point_emmition') {
						context.parent.block_series_name[event.block] = event.point.series_name;
						if (!context.parent.block_points[event.block]) {
							context.parent.block_points[event.block] = [];
						}
						context.parent.block_points[event.block].push([event.point.time * 1000, event.point.value]);
					}
				}
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::series',
			}),
		]),
		'edit_graph::series' : new Combine([
			new HTMLReplacer({
				'target' : function() {
					return $('#send_points');
				},
				'new_html' : 'Завершить прогон точек',
			}),
			new Builder({
				'container' : $('body'),
				'func' : function(context, container) {
					container.append('<div id=series style="height:400px"></div>');
				},
			}),
			new SwitchToSelectMode({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::series::choose_block_to_show_series'
			}),
		]),
		'edit_graph::series::choose_block_to_show_series' : new Combine([
			new Enabler({
				'target' : function() {
					return $('#send_points_link');
				},
			}),
			new Binder({
				'target' : function() {
					return $('#send_points');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'write_to' : 'block',
				'type' : 'next',
				'new_state' : 'edit_graph::series::draw_series',
			}),
		]),
		'edit_graph::series::draw_series' : new Combine([
			new Executer(function(context) {
				if (context.prev_block) {
					context.parent.parent.graph.get_vertex_by_id(context.prev_block.attr('id')).deselect();
				}
				context.prev_block = context.block;
				var block = context.block.attr('id');
				$('#series').html('');
				if (context.parent.block_series_name[block]) {
					var chart  = new Highcharts.Chart({
						chart: {
							type: 'line',
							renderTo: "progresschart",
							renderTo: 'series',
				        	},
						'height' : 300,
						'title' : {
							text: 'Points from block ' + block,
						},
						'xAxis' : {
							'type' : 'datetime',
							'title' : {
								'text' : 'Time',
							},
						},
						'yAxis' : {
							'title' : {
								'text' : 'Value',
							},
						},
						'series' : [{
							'name': context.parent.block_series_name[block],
							'data' : context.parent.block_points[block].sort(function (x,y) {
								if (x[0] < y[0]) {
									return -1;
								} else if (x[0] > y[0]) {
									return 1;
								} else {
									return 0;
								}
							}),
						}],
					});
				} else {
					alert('No points emmited from vertex ' + block);
				}
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::series::choose_block_to_show_series'
			}),
		]),
		'edit_graph::change_params' : new Combine([
			new HTMLReplacer({
				'target' : function() {
					return $('#show_params');
				},
				'new_html' : 'Завершить изменение параметров',
			}),
			new Enabler({
				'target' : function() {
					return $('#show_params_link');
				},
			}),
			new SwitchToSelectMode({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::change_params::listen',
			}),
		]),
		'edit_graph::change_params::listen' : new Combine([
			new Executer(function (context) {
				context.parent.parent.graph.deselect_all();
			}),
			new Binder({
				'target' : function() {
					return $('#show_params');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'write_to' : 'block',
				'type' : 'next',
				'new_state' : 'edit_graph::change_params::get_params',
			}),
		]),
		'edit_graph::change_params::get_params' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'params',
					'graph' : context.parent.parent.graph_name,
					'block' : context.block.attr('id'),
				};
			},
			'write_to' : 'blocks_params',
			'type' : 'next',
			'new_state' : 'edit_graph::change_params::set_params',
		}),
		'edit_graph::change_params::set_params' : new Combine([
			new BDialog({
				'id' : 'new_params_dialog',
				'title' : 'Change params of block',
				'data' : function(context) {
					if (context.blocks_params.length) {
						var input_html = '';
						for (var i = 0; i < context.blocks_params.length; ++i) {
							input_html += (
								'<div class="input-group">'
									+'<span class="input-group-addon" id="basic-addon1">'
										+ context.blocks_params[i].Name
									+ '</span>'
									+'<input'

										+ ' type="text"'
										+ ' class="form-control"'
										+ ' placeholder=""'
										+ ' aria-describedby="basic-addon1"'
										+ ' value="' + context.blocks_params[i].Value + '"'
										+ ' id=' + context.blocks_params[i].Name
									+ '>'
								+ '</div><br>'

							);
						}

						return input_html;
					} else {
						alert("No params in block " + context.block.attr('id'));
					}
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new GoTo({
				'type' : function(context) {
					if (context.blocks_params.length) {
						return 'substate';
					} else {
						return 'next';
					}
				},
				'new_state' : function (context) {
					if (context.blocks_params.length) {
						return 'edit_graph::change_params::set_params::listen';
					} else {
						return 'edit_graph::change_params::listen';
					}
				},
			}),
		]),
		'edit_graph::change_params::set_params::listen' : new Combine([
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::change_params::set_params::save_params',
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::change_params::listen',
			}),
		]),
		'edit_graph::change_params::set_params::save_params' : new SendQuery({
			'ajax_data' : function(context) {
				var new_params_values = [];

				for (var i = 0; i < context.parent.blocks_params.length; ++i) {
					var value = $('#' + context.parent.blocks_params[i].Name).val();
					if (value != '') {
						new_params_values.push({
							'name' : context.parent.blocks_params[i].Name,
							'value' : value,
						});
					}
				}

				return {
					'type' : 'modify',
					'object' : 'params',
					"values" : new_params_values,
					"block" : context.parent.block.attr('id'),
					"graph" : context.parent.parent.parent.graph_name,
				};
			},
			'type' : 'exit_state',
			'new_state' : 'edit_graph::change_params::listen',
		}),
		'edit_graph::delete_edges': new Combine([
			new Enabler({
				'target' : function() {
					return $('#delete_edge_link');
				},
			}),
			new HTMLReplacer({
				'target' : function() {
					return $('#delete_edge');
				},
				'new_html' : 'Завершить удаление ребер',
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::delete_edges::listen',
			}),
		]),
		'edit_graph::delete_edges::listen' : new Combine([
			new Binder({
				'target' : function() {
					return $('.destination-label');
				},
				'action' : 'click',
				'write_to' : 'edge',
				'type' : 'next',
				'new_state' : 'edit_graph::delete_edges::delete_edge',
			}),
			new Binder({
				'target' : function() {
					return $('#delete_edge');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
		]),
		'edit_graph::delete_edges::delete_edge' : new SendQuery({
			'ajax_data' : function(context) {
				var edge_params = context.edge.attr('id').replace('label_', '').split(':');
				context.from = edge_params[0];
				context.to = edge_params[1];
				context.label = edge_params[2];

				return {
					'type' : 'delete',
					'object' : 'edge',
					'edge' : context.label,
					'graph' : context.parent.parent.graph_name,
					'from' : context.from,
					'to': context.to,
					'ignore' : 0,
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::delete_edges::clear_edge',
		}),
		'edit_graph::delete_edges::clear_edge' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph.remove_edge({
					'from' : context.from,
					'to': context.to,
					'label' : context.label,
				});
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::delete_edges::listen',
			}),
		]),
		'edit_graph::create_edges' : new Combine([
			new Enabler({
				'target' : function() {
					return $('#new_edge_link');
				},
			}),
			new HTMLReplacer({
				'target' : function() {
					return $('#new_edge');
				},
				'new_html' : 'Завершить создания ребер',
			}),
			new SwitchToSelectMode({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::create_edges::listen',
			}),
		]),
		'edit_graph::create_edges::listen' : new Combine([
			new Executer(function (context) {
				context.parent.parent.graph.deselect_all();
			}),
			new Binder({
				'target' : function() {
					return $('#new_edge');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'write_to' : 'from',
				'type' : 'next',
				'new_state' : 'edit_graph::create_edges::choose_to_vertex',
			}),
		]),
		'edit_graph::create_edges::choose_to_vertex' : new Combine([
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'write_to' : 'to',
				'type' : 'next',
				'new_state' : 'edit_graph::create_edges::download_missing_edges',
			}),
		]),
		'edit_graph::create_edges::download_missing_edges' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'missing_edges',
					'block' : context.to.attr('id'),
					'graph' : context.parent.parent.graph_name,
				};
			},
			'write_to' : 'missing_edges',
			'type' : 'next',
			'new_state' : 'edit_graph::create_edges::set_edge',
		}),
		'edit_graph::create_edges::set_edge' : new Combine([
			new BDialog({
				'id' : 'new_edge_dialog',
				'title' : 'Choose edge name.',
				'data' : function(context) {
					if (context.missing_edges.length) {
						context.missing_edges.sort(function (x,y) {
							if (x.EdgeName < y.EdgeName) {
								return -1;
							} else if (x.EdgeName > y.EdgeName) {
								return 1;
							} else {
								return 0;
							}
						});
						select_html = '';
						for (var i = 0; i < context.missing_edges.length; ++i) {
							select_html += (
								'<option'
									+' value="' + context.missing_edges[i].EdgeName + '"'
								+ '>'
									+ context.missing_edges[i].EdgeName
								+ '</option>'
							);
						}
					return 	(

						'<div class="input-group">'
							+'<span class="input-group-addon" id="basic-addon1">'
								+ 'Edge name'
							+ '</span>'
							+ '<select'
								+ ' id=edge_name'
								+ ' class="form-control"'
								+ ' style="width:50%"'
							+ ' >'
								+ select_html
							+ '</select>'
						+ '</div><br>'
					);
					} else {
						alert('Vertex ' + context.to.attr('id') + ' can\'t have more incoming edges.');
					}
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new GoTo({
				'type' : function(context) {
					if (context.missing_edges.length) {
						return 'substate';
					} else {
						return 'next';
					}
				},
				'new_state' : function(context) {
					if (context.missing_edges.length) {
						return 'edit_graph::create_edges::set_edge::listen';
					} else {
						return 'edit_graph::create_edges::listen';
					}
				}
			}),
		]),
		'edit_graph::create_edges::set_edge::listen' : new Combine([
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::create_edges::set_edge::save_edge',
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::create_edges::listen',
			}),
		]),
		'edit_graph::create_edges::set_edge::save_edge' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'create',
					'object' : 'edge',
					'edge' : $('#edge_name').val(),
					'graph' : context.parent.parent.parent.graph_name,
					'from' : context.parent.from.attr('id'),
					'to' : context.parent.to.attr('id'),
					'ignore' : 0
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::create_edges::set_edge::draw_edge',
		}),
		'edit_graph::create_edges::set_edge::draw_edge' : new Combine([
			new Executer(function(context) {
				context.parent.parent.parent.graph.add_edge({
					'from' : context.parent.from.attr('id'),
					'to' : context.parent.to.attr('id'),
					'label' : $('#edge_name').val(),
				});
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::create_edges::listen',
			}),
		]),
		'edit_graph::delete_vertex' : new Combine([
			new Enabler({
				'target' : function() {
					return $('#delete_vertex_link');
				},
			}),
			new HTMLReplacer({
				'target' : function() {
					return $('#delete_vertex');
				},
				'new_html' : 'Завершить удаление вершин',
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::delete_vertex::listen',
			}),
			new SwitchToSelectMode({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
		]),
		'edit_graph::delete_vertex::listen' : new Combine([
			new Binder({
				'target' : function() {
					return $('#delete_vertex');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen'
			}),
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::delete_vertex::delete_vertex',
			}),
		]),
		'edit_graph::delete_vertex::delete_vertex' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'delete',
					'object' : 'block',
					'block' : context.actor.attr('id'),
					'graph' : context.parent.parent.graph_name,
					'ignore' : 0
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::delete_vertex::clear_vertex',
		}),
		'edit_graph::delete_vertex::clear_vertex' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph.remove_vertex({'id' : context.actor.attr('id')});
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::delete_vertex::listen',
			}),
		]),
		'edit_graph::get_vertex_types' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'types',
				};
			},
			'write_to' : 'types',
			'type' : 'next',
			'new_state' : 'edit_graph::set_vertex',
		}),
		'edit_graph::set_vertex' : new Combine ([
			new BDialog({
				'id' : 'new_vertex_dialog',
				'title' : 'Chose type of vertex',
				'data' : function(context) {
					context.descriptions_by_types = {};
					var select_html = '';
					for(var i = 0; i < context.types.length; ++i) {
						context.descriptions_by_types[context.types[i].Type] =
							context.types[i].Description;

						select_html += (
							'<option'
								+ ' value=' + context.types[i].Type
							+ '>'
								+  context.types[i].Type
							+ '</option>'
						);
					}
					return (
						'<div class="input-group">'
							+'<span class="input-group-addon" id="basic-addon1">'
								+ 'Vertex type'
							+ '</span>'
							+ '<select'
								+ ' id=vertex_types'
								+ ' class="form-control"'
								+ ' style="width:50%"'
							+ ' >'
								+ select_html
							+ '</select>'
						+ '</div><br>'

						+'<div class="input-group" id=edges_count_div>'
							+'<span class="input-group-addon" id="basic-addon1">'
								+ 'Edges count'
							+ '</span>'
							+'<input'
								+ ' type="number"'
								+ ' class="form-control"'
								+ ' placeholder=""'
								+ ' aria-describedby="basic-addon1"'
								+ ' id=edges_count'
							+ '>'
						+ '</div><br>'

						+'<div class="input-group">'
							+'<span class="input-group-addon" id="basic-addon1">'
								+ 'Vertex name'
							+ '</span>'
							+'<input'
								+ ' type="text"'
								+ ' class="form-control"'
								+ ' placeholder=""'
								+ ' aria-describedby="basic-addon1"'
								+ ' id=vertex_name'
							+ '>'
						+ '</div><br>'
						+ '<hr color="#101010">'
						+ '<div id=vertex_description>'
						+ '</div><br>'
					);
				},
				'buttons' :  '<button id=Ok type="button" class="btn btn-default">Ok</button>',
			}),
			new Executer(function(context) {
				context.change_description = function () {
						var type = $('#vertex_types').val();
						$('#vertex_description').html(context.descriptions_by_types[type]);
						if (type.indexOf('%d') == -1) {
							$('#edges_count_div').hide();
						} else {
							$('#edges_count_div').show();
						}
					};
					context.change_description();
					context.get_type = function () {
						var type = $('#vertex_types').val();

						return type.replace('%d', $('#edges_count').val() || "0");
					}
			}),
			new GoTo({
				'type' : 'substate',
				'new_state' : 'edit_graph::set_vertex::listen',
			}),
		]),
		'edit_graph::set_vertex::listen' : new Combine([
			new Binder({
				'target' : function() {
					return $('#vertex_types');
				},
				'action' : 'change',
				'type' : 'next',
				'new_state' : 'edit_graph::set_vertex::change_description',
			}),
			new Binder({
				'target' : function () {
					return $('#Ok');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_vertex::check_vertex_name',
			}),
			new Binder({
				'target' : function() {
					return $('#close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
		]),
		'edit_graph::set_vertex::check_vertex_name' : new Combine([
			new Executer(function(context) {
				var vertex_name = $('#vertex_name').val();
				context.has_vertex = context.parent.parent.graph.has_vertex({"id" : vertex_name});

				if (context.has_vertex) {
					alert('Vertex with name ' + vertex_name + ' already exists.');
				}
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : function (context) {
					if ($('#vertex_name').val() === "") {
						alert("Input of vertex name is empty.");
					} else if (context.has_vertex) {
						return 'edit_graph::set_vertex::listen';
					} else {
						return 'edit_graph::set_vertex::save_vertex';
					}
				},
			}),
		]),
		'edit_graph::set_vertex::save_vertex' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type': 'create',
					'object' : 'block',
					'block_type' : context.parent.get_type(),
					'block' : $('#vertex_name').val(),
					'graph' : context.parent.parent.graph_name,
					'ignore' : 0,
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::set_vertex::draw_vertex',
		}),
		'edit_graph::set_vertex::change_description' : new Combine([
			new Executer(function(context) {
				context.parent.change_description();
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::set_vertex::listen',
			}),
		]),
		'edit_graph::set_vertex::draw_vertex' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph.add_vertex({
					'id' : $('#vertex_name').val(),
					'content' : (
						$('#vertex_name').val()
						+ ' : '
						+ context.parent.get_type()
					),
				});
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			})
		]),

	});
});


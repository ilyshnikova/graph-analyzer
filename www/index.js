function call_or_get(object, context) {
	if (jQuery.isFunction(object)) {
		return object(context);
	} else {
		return object;
	}
}

$(function() {
	new State({
		'start' : new Combine([
			new Builder({
				'container' : $('#button_panel'),
				'func' : function (context, container) {
					container.append(
						'<button type=button id=download_graph>'
							+ 'Загрузить граф'
						+ '</button>'
					);
				},
			}),
			new Binder({
				'target' : function() {
					return $('#download_graph');
				},
			    	'action' : 'click',
			    	'type' : 'next',
			    	'new_state' : 'edit_graph',
			})
		]),
		'edit_graph' : new Combine([
			new Builder({
				'container' : $('body'),
				'func' : function(context, container) {
					container.append(
						'<div id="mainCanvas" class=canvas style="border-color:black; border-width: 3px;  border-style: double;  width: 100%; height: 400px;"></div>'
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
			'new_state' : 'edit_graph::set_graph',
		}),
		'edit_graph::set_graph' : new Combine([
			new Builder({
				'container' : $('body'),
				'func' : function(context, container) {
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
					container.append(
						'<div id=download_graph_dialog title="Choose graph.">'
							+ '<select id=graph_names>'
								+ select_html
							+ '</select>'
						+ '</div>'
					);

				},
			}),
			new Dialog({
				'target' : function() {
					return $('#download_graph_dialog')
				}
			}),
			new GoTo({
				'new_state' : 'edit_graph::set_graph::listen',
		       		'type' : 'substate'
			}),
		]),
		'edit_graph::set_graph::listen' : new Combine ([
			new Binder({
				'target' : function () {
					return $('.ui-button-text-only');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_graph::get_vertices',
			}),
			new Binder({
				'target' : function() {
					return $('.ui-dialog-titlebar-close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::cancel_edit',
			}),
		]),
		'edit_graph::cancel_edit' : new GoTo({
			'type' : 'exit_state',
			'new_state' : 'start',
		}),
		'edit_graph::set_graph::get_vertices' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph_name = $('#graph_names').val();
			}),
			new SendQuery({
				'ajax_data' : function(context) {
					return {
						'type' : 'show',
						'object' : 'blocks',
						'graph' : $('#graph_names').val(),
					};
				},
				'write_to' : 'blocks',
				'type' : 'next',
				'new_state' : 'edit_graph::set_graph::get_edges',
			}),
		]),
		'edit_graph::set_graph::get_edges' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'edges',
					'graph' : $('#graph_names').val(),
				};
			},
			'write_to' : 'edges',
			'type' : 'next',
			'new_state' : 'edit_graph::set_graph::draw',
		}),
		'edit_graph::set_graph::draw' : new Combine([
			new Executer(function(context) {
				for (var i = 0; i < context.blocks.length; ++i) {
					context.parent.parent.graph.add_vertex({
						'id' : context.blocks[i].Name,
						'content' : (
							context.blocks[i].Name
							+ ' : '
							+ context.blocks[i].Type
						),
					});
				}
				for (var i = 0; i < context.edges.length; ++i) {
					context.parent.parent.graph.add_edge({
						'from' : context.edges[i].From,
						'to' : context.edges[i].To,
						'label' : context.edges[i].EdgeName,
					});
				}
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			})
		]),
		'edit_graph::listen': new Combine([
			new Enabler({
				'target' : function() {
					return $('#new_vertex');
				}
			}),
			new Enabler({
				'target' : function() {
					return $('#delete_vertex');
				}
			}),
			new Enabler({
				'target' : function() {
					return $('#finish_editing');
				}
			}),
			new Enabler({
				'target' : function() {
					return $('#new_edge');
				},
			}),
			new Enabler({
				'target' : function() {
					return $('#delete_edge');
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
				'new_state' : 'edit_graph::choose_vertex_to_delete',
			}),
			new Binder({
				'target' : function() {
					return $('#new_edge');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::choose_from_vertex',
			}),
			new Binder({
				'target' : function() {
					return $('#delete_edge');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::choose_edge_to_delete',
			}),
		]),
		'edit_graph::choose_edge_to_delete' : new Binder({
			'target' : function() {
				return $('.destination-label');
			},
			'action' : 'click',
			'write_to' : 'edge',
			'type' : 'next',
			'new_state' : 'edit_graph::delete_edge',
		}),
		'edit_graph::delete_edge' : new SendQuery({
			'ajax_data' : function(context) {
				var edge_params = context.edge.attr('id').replace('label_', '').split(':');
				context.from = edge_params[0];
				context.to = edge_params[1];
				context.label = edge_params[2];

				return {
					'type' : 'delete',
					'object' : 'edge',
					'edge' : context.label,
					'graph' : context.parent.graph_name,
					'from' : context.from,
					'to': context.to,
					'ignore' : 0,
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::clear_edge',
		}),
		'edit_graph::clear_edge' : new Combine([
			new Executer(function(context) {
				context.parent.graph.remove_edge({
					'from' : context.from,
					'to': context.to,
					'label' : context.label,
				});
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::listen',
			}),
		]),
		'edit_graph::choose_from_vertex' : new Combine([
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'write_to' : 'from',
				'type' : 'next',
				'new_state' : 'edit_graph::choose_to_vertex',
			}),
			new StopGraphDraggable({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
		]),
		'edit_graph::choose_to_vertex' : new Combine([
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'write_to' : 'to',
				'type' : 'next',
				'new_state' : 'edit_graph::download_missing_edges',
			}),
			new StopGraphDraggable({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
		]),
		'edit_graph::download_missing_edges' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'missing_edges',
					'block' : context.to.attr('id'),
					'graph' : context.parent.graph_name,
				};
			},
			'write_to' : 'missing_edges',
			'type' : 'next',
			'new_state' : 'edit_graph::set_edge',
		}),
		'edit_graph::set_edge' : new Combine([
			new Builder({
				'container' : $('body'),
				'func' : function(context, container) {
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
							 	+' value=' + context.missing_edges[i].EdgeName
							+ '>'
								+ context.missing_edges[i].EdgeName
							+ '</option>'
						);
					}
					container.append(
						'<div id=new_edge_dialog title="Choose edge name">'
							+ 'Edge name:'
							+ '<select id=edge_name>'
								+ select_html
							+ '</select>'
						+ '</div>'
					);
				}
			}),
			new Dialog({
				'target' : function() {
					return $('#new_edge_dialog');
				},
			}),
			new Executer(function(context) {
				if (context.missing_edges.length == 0) {
					alert('Vertex ' + context.to.attr('id') + ' can\'t have more incoming edges.');
				}
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
						return 'edit_graph::set_edge::listen';
					} else {
						return 'edit_graph::listen';
					}
				}
			}),
		]),
		'edit_graph::set_edge::listen' : new Combine([
			new Binder({
				'target' : function () {
					return $('.ui-button-text-only');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_edge::save_edge',
			}),
			new Binder({
				'target' : function() {
					return $('.ui-dialog-titlebar-close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			})

		]),
		'edit_graph::set_edge::save_edge' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'create',
					'object' : 'edge',
					'edge' : $('#edge_name').val(),
					'graph' : context.parent.parent.graph_name,
					'from' : context.parent.from.attr('id'),
					'to' : context.parent.to.attr('id'),
					'ignore' : 0
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::set_edge::draw_edge',
		}),
		'edit_graph::set_edge::draw_edge' : new Combine([
			new Executer(function(context) {
				context.parent.parent.graph.add_edge({
					'from' : context.parent.from.attr('id'),
					'to' : context.parent.to.attr('id'),
					'label' : $('#edge_name').val(),
				});
			}),
			new GoTo({
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			}),
		]),
		'edit_graph::choose_vertex_to_delete' : new Combine([
			new Binder({
				'target' : function() {
					return $('.block');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::delete_vertex',
			}),
			new StopGraphDraggable({
				'graph' : function(context) {
					return context.parent.graph;
				},
			}),
		]),
		'edit_graph::delete_vertex' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'delete',
					'object' : 'block',
					'block' : context.actor.attr('id'),
					'graph' : context.parent.graph_name,
					'ignore' : 0
				};
			},
			'type' : 'next',
			'new_state' : 'edit_graph::clear_vertex',
		}),
		'edit_graph::clear_vertex' : new Combine([
			new Executer(function(context) {
				context.parent.graph.remove_vertex({'id' : context.actor.attr('id')});
			}),
			new GoTo({
				'type' : 'next',
				'new_state' : 'edit_graph::listen'
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
			new Builder({
				'container' : $('body'),
				'func' : function(context, container) {
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
					container.append(
						'<div id=new_vertex_dialog title="Chose type of vertex">'
							+ '<div>'
								+ '<select id=vertex_types>'
									+ select_html
								+ '</select>'
								+ '&nbsp; <input type=number value=1 id=edges_count min=1 max=99>'
							+ '</div><br>'
							+ 'Vertex name:'
							+ '<input type="text" id="vertex_name" value=""><br>'
							+ '<div id=vertex_description>'
							+ '</div><br>'
						+ '</div>'
					);
					$('#edges_count').hide();
					context.change_description = function () {
						var type = $('#vertex_types').val();
						$('#vertex_description').html(context.descriptions_by_types[type]);
						if (type.indexOf('%d') == -1) {
							$('#edges_count').hide();
						} else {
							$('#edges_count').show();
						}
					};
					context.change_description();
					context.get_type = function () {
						var type = $('#vertex_types').val();

						return type.replace('%d', $('#edges_count').val() || "0");
					}
				},
			}),
			new Dialog({
				'target' : function () {
					return $('#new_vertex_dialog');
				},
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
					return $('.ui-button-text-only');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'edit_graph::set_vertex::check_vertex_name',
			}),
			new Binder({
				'target' : function() {
					return $('.ui-dialog-titlebar-close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'edit_graph::listen',
			})
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
					if (context.has_vertex) {
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
				'new_state' : 'edit_graph::listen',
				'type' : 'exit_state',
			})
		]),

	});
});


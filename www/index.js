function call_or_get(object) {
	if (jQuery.isFunction(object)) {
		return object();
	} else {
		return object;
	}
}

$(function() {
	var graph = new Graph({
		'container' : $('#mainCanvas'),
	});

	new State({
		'start': new Binder({
			'target' : function () {
				return $('#new_vertex');
			},
			'action' : 'click',
			'type' : 'next',
			'new_state' : 'get_vertex_types',
		}),
		'get_vertex_types' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type' : 'show',
					'object' : 'types',
				};

			},
			'write_to' : 'types',
			'type' : 'next',
			'new_state' : 'set_vertex',
		}),
		'set_vertex' : new Combine ([
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
					container.hide();
					container.append(
						'<div id=new_vertex_dialog title="Chose type of vertex">'
							+ '<select id=vertex_types>'
								+ select_html
							+ '</select><br>'
							+ 'Vertex name:'
							+ '<input type="text" id="vertex_name" value=""><br>'
							+ '<div id=vertex_description>'
							+ '</div><br>'
						+ '</div>'
					);
				},
			}),
			new Dialog({
				'target' : function () {
					return $('#new_vertex_dialog');
				},
			}),
			new GoTo({
				'new_state' : 'set_vertex::listen',
				'type' : 'substate',
			}),
		]),
		'set_vertex::listen' : new Combine([
			new Binder({
				'target' : function() {
					return $('#vertex_types');
				},
				'action' : 'change',
				'type' : 'next',
				'new_state' : 'set_vertex::change_description',
			}),
			new Executer(function (context) {
				var type = $('#vertex_types').val();
				$('#vertex_description').html(context.parent.descriptions_by_types[type]);
			}),
			new Binder({
				'target' : function () {
					return $('.ui-button-text-only');
				},
				'action' : 'click',
				'type' : 'next',
				'new_state' : 'set_vertex::save_vertex',
			}),
			new Binder({
				'target' : function() {
					return $('.ui-dialog-titlebar-close');
				},
				'action' : 'click',
				'type' : 'exit_state',
				'new_state' : 'start',
			})
		]),
		'set_vertex::save_vertex' : new SendQuery({
			'ajax_data' : function(context) {
				return {
					'type': 'create',
					'object' : 'block',
					'block_type' : $('#vertex_types').val(),
					'block' : $('#vertex_name').val(),
					'graph' : 'A',
					'ignore' : 0,
				};
			},
			'type' : 'next',
			'new_state' : 'set_vertex::draw_vertex',

		}),
		'set_vertex::change_description' : new Combine([
			new Executer(function(context) {
				var type = $('#vertex_types').val();
				$('#vertex_description').html(context.parent.descriptions_by_types[type]);
			}),
			new GoTo({
				'new_state' : "set_vertex::listen",
				'type' : 'next',
			}),
		]),
		'set_vertex::draw_vertex' : new Combine([
			new Executer(function(context) {

				graph.add_vertex({
					'id' : $('#vertex_name').val(),
					'content' : (
						$('#vertex_name').val()
						+ ' : '
						+ $('#vertex_types').val()
					),
				});
			}),
			new GoTo({
				'new_state' : 'start',
				'type' : 'exit_state',
			})
		]),
	});
});


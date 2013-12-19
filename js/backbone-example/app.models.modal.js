var app = app || {};
app.models = app.models || {};
app.models.modal = app.models.modal || {};

$( function() {

	app.models.modal = Backbone.Model.extend({

		ajaxUrl: '/apps/modal/ajax/get-modal-content',

		dataLayerCloseParams: {},
		dataLayerOpenParams: {},

		dataLayerPush: function ( params ) {
			"use strict";
			window.log( params );
			if ( window.dataLayer && ! $.isEmptyObject(params) ) {
				window.dataLayer.push(params);
			}
		},

		dataLayerOpen: function () {
			"use strict";
			this.dataLayerPush( this.dataLayerOpenParams );
		},

		dataLayerClose: function () {
			"use strict";
			this.dataLayerPush( this.dataLayerCloseParams );
		},

		dataLayerError: function ( msg ) {
			"use strict";
			var params = this.dataLayerErrorParams;
			params['data-action'] = msg || 'Not Specified';
			this.dataLayerPush( params );
		},

		// Simple default validator.
		// Only handles simple inputs.
		validator: function ( msgs, field ) {

			"use strict";

			var name = $(field).attr('name');
			var pattern = $(field).attr('pattern');
			var val = $(field).val();

			if ( val.trim() == '' ) {
				msgs[name] = 'This field is required';
			}
			else if ( typeof pattern !== 'undefined' && ! val.match(pattern) ) {

				if ( name == 'email' ) {
					msgs[name] = 'Please enter a valid email address';
				}
				else {
					msgs[name] = 'Please match the specified format';
				}
			}

			return msgs;
		},

		validate: function () {

			"use strict";

			var required = this.get('required');
			if (required) {
				var msgs = _.reduce( required, this.validator, {} );
				if ( ! $.isEmptyObject(msgs) ) {
					return msgs;
				}
			}
		}
	});
});
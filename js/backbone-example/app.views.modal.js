var app = app || {};
app.views = app.views || {};
app.debug = true;

$( function() {

	app.views.ModalView = Backbone.View.extend({

		model: new app.models.modal(),
		className: 'mw-modal hide',

		events: {
			'click .close-modal': 'closeModal'
		},

		initialize: function () {
			"use strict";
			this.render();
			this.listenTo( this.model, 'change:isBusy', this.updateBusyUI );
			this.model.dataLayerOpen();
		},

		renderContent: function ( html ) {
			"use strict";
			this.$el.html( html );
			this.$el.modal();
		},

		render: function () {
			"use strict";
			var that = this;
			var cachedContent = this.model.get('cachedContent');

			if ( cachedContent ) {
				this.renderContent( cachedContent );
				return this;
			}

			$.ajax({
				url: this.model.ajaxUrl,
				type: 'post',
				async: false,
				data: { 'id' : this.id },
				success: function (response) {
					var json = $.parseJSON(response);
					var html = json.data.html;
					that.renderContent( html );
					that.model.set( { cachedContent : html } );
					that.model.set( { required : that.$('[required]') } );
				}
			});

			return this;
		},

		closeModal: function () {
			"use strict";
			this.$el.modal('hide');
			this.remove();
			this.unbind();
			this.model.dataLayerClose();
		},

		updateBusyUI: function() { return true; },

		redirectOrReload: function (loc) {
			"use strict";
			if ((typeof loc !== 'undefined') && (loc !== '') && loc) {
				location.href = loc;
				return;
			}
			location.reload();
		},

		inheritEvents: function (parent) {
			"use strict";
			var parentEvents = parent.prototype.events;
			if( _.isFunction(parentEvents) ) {
				parentEvents = parentEvents();
			}
			this.events = _.extend( {}, parentEvents, this.events );
		}
	});
});

var app = app || {};
app.views = app.views || {};

$( function() {

	app.views.JoinNowModalView = app.views.ModalView.extend({

		model: new app.models.JoinNowModalModel(),

		id: 'join-now-modal',

		submitButton: null,
		emailInput: null,
		passwordInput: null,
		formMsg: null,

		events: {
			'click button[type="submit"]' : 'submit'
		},

		submit: function () {

			"use strict";

			this.setBusy(true);
			$('.error-msg').html('').removeClass('icon-exclamation-sign');
			this.formMsg.hide();

			if ( this.model.isValid() ) {

				app.pubsub.trigger( 'login:createUser',
					{
						'context' : this,
						'data' : {
							'email': this.emailField.val(),
							'password': this.passwordField.val()
						},
						'successcb' : this.createUserSuccessCb,
						'errorcb' : this.createUserFailCb
					}
				);
			}
			else {

				this.model.dataLayerError( 'Form validation error' );
				var that = this;

				$.each( that.model.validationError, function (name, msg) {
					that
						.$('input[name="' + name + '"]')
						.next('.error-msg')
						.html(msg)
						.addClass('icon-exclamation-sign');
				});
				this.setBusy(false);
			}
		},

		initialize: function () {
			"use strict";
			if ( MW.util.getTimerInitVal('signupTimer') == 1 ) {
				this.model.setModalB();
			}
			app.views.ModalView.prototype.initialize.apply( this, arguments );
			this.inheritEvents( app.views.ModalView );
		},

		render: function () {
			"use strict";
			app.views.ModalView.prototype.render.apply( this, arguments );

			this.submitButton = this.$('button[type="submit"]');
			this.emailField = this.$('input[name="email"]');
			this.passwordField = this.$('input[name="password"]');
			this.formMsg = this.$('.form-msg');

			this.model.set( { email: this.emailField.val() } );
			this.model.set( { password: this.passwordField.val() } );
		},

		createUserSuccessCb: function ( data, context ) {
			"use strict";
			context.model.dataLayerPush( context.model.dataLayerSaveParams );
			context.redirectOrReload();
		},

		createUserFailCb: function (context) {
			"use strict";
			context.formMsg.html( 'you already have an account - <a class="close-modal loginTrigger">log in</a>' );
			context.formMsg.show();
			context.setBusy( false, context );
			context.model.dataLayerError( 'User already exists' );
		},

		updateBusyUI: function ( model, value, options ) {
			"use strict";
			this.$('button[type="submit"]').prop( 'disabled', model.get('isBusy') );
		},

		setBusy: function ( busy, context ) {
			"use strict";
			context = context ? context : this;
			context.model.set( { isBusy : busy } );
		}
	});
});
var app = app || {};
app.models = app.models || {};

$( function() {

	app.models.JoinNowModalModel = app.models.modal.extend({

		username: null,
		password: null,

		dataLayerOpenParams:  {
			"event"        : "click",
			"data-category": "AB Testing",
			"data-action": "Join Now Modal Open",
			"data-label": "[Modal A]"
		},

		dataLayerCloseParams: {
			"event"        : "click",
			"data-category": "AB Testing",
			"data-action": "Join Now Modal Exit",
			"data-label": "[Modal A]"
		},

		dataLayerSaveParams: {
			"event"        : "click",
			"data-category": "Join Now Modal",
			"data-action": "User Successfully Created",
			"data-label": "[Modal A]"
		},

		dataLayerErrorParams: {
			"event": "click",
			"data-category": "Join Now Modal",
			"data-action": "Error",
			"data-label": "[Modal A]"
		},

		setModalB: function () {
			var modalB = '[Modal B]';
			var allParams = [
				this.dataLayerOpenParams,
				this.dataLayerCloseParams,
				this.dataLayerSaveParams,
				this.dataLayerErrorParams
			]
			$.each( allParams, function () {
				this['data-label'] = modalB;
			});
		}
	});
});
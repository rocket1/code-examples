if (typeof MW == "undefined" || !MW) {
	var MW = {};
}

if ( ! window.log ) {
	window.log = function() {
		var log = {};
		log.history = log.history || [];   // store logs to an array for reference
		log.history.push(arguments);
		if (window.console) {
			console.log( Array.prototype.slice.call(arguments) );
		}
	};
}

/**
 * Returns the namespace specified and creates it if it doesn't exist
 * <pre>
 * MW.namespace("property.package");
 * MW.namespace("MW.property.package");
 * </pre>
 * Either of the above would create MW.property, then
 * MW.property.package
 *
 * This method is copied from YUI's global YAHOO object.
 *
 * @method namespace
 * @static
 * @param  {String*} arguments 1-n namespaces to create
 * @return {Object}  A reference to the last namespace object created
 */
MW.namespace = function() {
	var a=arguments, o=null, i, j, d;
	for (i=0; i<a.length; i=i+1) {
		d=(""+a[i]).split(".");
		o=MW;

		// MW is implied, so it is ignored if it is included
		for (j=(d[0] == "MW") ? 1 : 0; j<d.length; j=j+1) {
			o[d[j]]=o[d[j]] || {};
			o=o[d[j]];
		}
	}

	return o;
};

MW.namespace("util");

MW.util.cookie = {

	'get' : function (name) {
		var nameMatch = name+"=";
		var cookies = document.cookie.split(/;\s?/);
		for (var i = 0; i < cookies.length; i++) {
			if (cookies[i].indexOf(nameMatch) == 0) {
				return unescape( cookies[i].substring(nameMatch.length, cookies[i].length) );
			}
		}
		return null;
	},

	'set' : function ( name, value, expire, path ) {
		var cookieValue = name + "=" + escape(value);
		var expireDate = this.parseExpire(expire);
		if (expireDate != null) {
			cookieValue += "; expires=" + expireDate.toGMTString();
		}
		cookieValue += "; path=/";// + (path == null ? "/" : path);
		document.cookie = cookieValue;
	},

	'remove' : function (name, path) {
		this.set( name, "", -1, path );
	},

	// Parses the expire time sent to the sent method. Acceptable values
	// are whole numbers, optionally followed by a letter specifying unit.
	// The letter is case-insensitive, and can be one of D, M, or H,
	// meaning days, hours and minutes, respectively. Default is seconds.
	// Returns Date object.
	'parseExpire' : function (expire) {

		// Any negative value is interpreted as an expiration command.
		if (parseInt(expire) < 0) {
			return new Date(new Date().getTime() - 86400000);
		}
		var re = /^(\d+)([a-z]?)/i;
		var m = re.exec(expire);

		if ( m != null ) {

			var addTime;
			var unit = m[2] == null ? "" : m[2].toUpperCase();

			if (unit == "D") {
				addTime = m[1] * 86400000;
			}
			else if (unit == "H") {
				addTime = m[1] * 3600000;
			}
			else if (unit == "M") {
				addTime = m[1] * 60000;
			}
			else {
				addTime = m[1] * 1000;
			}

			var expireDate = new Date();
			expireDate.setTime(new Date().getTime() + addTime);
			return expireDate;
		}
		// If the expire time is unparseable, return null to default to
		// a session cookie.
		else {
			return null;
		}
	}
}

MW.util.queryString = {

	'getVals' : function() {
		var vars = [], hash;
		var hashes = window.location.href.slice(window.location.href.indexOf('?') + 1).split('&');
		for(var i = 0; i < hashes.length; i++) {
			hash = hashes[i].split('=');
			vars.push(hash[0]);
			vars[hash[0]] = hash[1];
		}
		return vars;
	},

	'get' : function( key ) {
		var vals = this.getVals();
		return (key in vals) ? vals[key] : null;
	}
}

// Pageview Timers
$( function () {

	"use strict";

	var _disabledVal = -1;

	MW.util.pageviewTimersPausedFlag = false;

	// Extract timers from a param like ?mwrt=signupTimer,ddTimer,fooTimer...
	var timersFromQuery = function( paramName ) {
		var timerNames = MW.util.queryString.get( paramName );
		if ( timerNames !== null ) {
			var timers = timerNames.split(',');
			if ( timers.length > 0 ) {
				return timers;
			}
		}
		return null;
	}

	MW.util.registerPageviewTimer = function( timerName, initVal ) {

		// registerPageviewTimer() can be called non-destructively.
		// i.e. multiple calls will not update/mess up already registered timer
		// with the same name.
		if ( MW.util.cookie.get( timerName ) === null ) {
			MW.util.cookie.set( timerName, initVal );
			MW.util.cookie.set( timerName + '_init', initVal );
		}
	}

	MW.util.unregisterPageviewTimer = function( timerName ) {

		// You must explicitly call unregisterPageviewTimer() if you want to
		// call registerPageviewTimer() with a previous active name.
		MW.util.cookie.remove( timerName );
		MW.util.cookie.remove( timerName + '_init' );
	}

	MW.util.disablePageviewTimer = function( timerName ) {

		// You must explicitly call unregisterPageviewTimer() if you want to
		// call registerPageviewTimer() with a previous active name.
		MW.util.cookie.set( timerName, _disabledVal, 10 * 365 );
	}

	// mwdt
	// add ?mwdt=timer1,timer2,etc to disable these timers (set to -1).
	MW.util.disableTimersByQueryString = function() {
		var timers = timersFromQuery('mwdt');
		if ( ! timers ) {
			return false;
		}
		$.map( timers, function(timer) {
			MW.util.disablePageviewTimer( timer );
		});
		return true;
	}

	// mwrt
	// add ?mwrt=timer1,timer2,etc to remove these timers (delete cookie).
	MW.util.unregisterTimersByQueryString = function() {
		var timers = timersFromQuery('mwrt');
		if ( ! timers ) {
			return false;
		}
		$.map( timers, function(timer) {
			MW.util.unregisterPageviewTimer( timer );
		});
		return true;
	}

	// mwst
	// add ?mwst=myTimer,3 to set myTimer to 3
	MW.util.setTimerByQueryString = function() {
		var timer = MW.util.queryString.get( 'mwst' );

		if ( ! timer ) {
			return false;
		}
		var timerTokens = timer.split(',')
		var timerName = timerTokens[0];
		var timerVal = timerTokens[1];
		MW.util.unregisterPageviewTimer( timerName );
		MW.util.registerPageviewTimer( timerName, timerVal );
		return true;
	}

	MW.util.decrementPageviewTimer = function( timerName, callback ) {

		var timerVal = MW.util.cookie.get( timerName );

		// We do not have a timer with this name, so return.
		if ( timerVal === null ) {
			return;
		}

		// The timers should only run once for a given name.
		// -1 indicates the timer has already ran, so return.
		if ( timerVal <= _disabledVal ) {
			return;
		}

		// If we have reached zero, then fire the callback
		// and set the value to -1 to indicate that the timer
		// should not run again.
		if ( timerVal == 0 ) {
			callback();
			MW.util.disablePageviewTimer( timerName );
			return;
		}

		// Substract 1 from the value.
		// You <3 this syntax :)
		timerVal--;

		MW.util.cookie.set( timerName, timerVal );
	}

	MW.util.pausePageviewTimers = function() {
		MW.util.pageviewTimersPausedFlag = true;
	}

	MW.util.pageviewTimersPaused = function() {
		return MW.util.pageviewTimersPausedFlag;
	}

	MW.util.getTimerInitVal = function ( timerName ) {
		return MW.util.cookie.get( timerName + '_init' );
	}
});

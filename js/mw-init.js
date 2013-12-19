// Homepage Tabs.
// Depends on lib/assets/js/plugins/jquery-observehashchange/jquery.observehashchange.js
$( function() {

	// just for handy.
	var homeTabs = $('#myTab');

	// Are we on the right page?
	if ( ! homeTabs.length ) {
		return;
	}

	// Couple o' globals.
	var FADE_SPEED = 500;
	var targetIdAtr = 'target-id';

	// Returns the url hash (or null) _without_ the # character.
	var getHash = function( hash ) {
		hash = window.location.hash.replace('#', '');
		return hash !== '' ? hash : null;
	}

	var showTabById = function( tabId, callback ) {

		var target = cleanTarget( tabId );
		var tabElem = $('a[href="/' + target + '"]');

		// When first loading the page, a hash like #inspiration
		// will cause the browser to scroll to the anchor with href as this.
		// I append -tab to the ids differentiate and break that behavior.
		var contentElem = $('#' + tabId + '-tab');

		var fadeInElem = function( callback ) {

			// Hide *all* the panes for a moment.
			$('.tab-pane').hide();

			// Remove active from all tabs briefly.
			homeTabs.find('li').removeClass('active');
			homeTabs.find('span').removeClass('active');

			// Make the correct tab active.
			tabElem.parent('li').addClass('active');
			tabElem.find('span').addClass('active');

			// Show our tab as selected (no content loaded just yet).
			tabElem.show();

			// Fade our content in.
			$(contentElem).fadeIn( FADE_SPEED, callback );
		}

		if ( tabElem.length && contentElem.length )
		{
			// Poor man cache. Don't load twice.
			if ( $(contentElem).is(':parent') ) {
				loadAds( tabId );
				// The tab is already loaded, so just show it.
				fadeInElem( callback );
			}
			else {
				// Load the content via ajax, although since we
				// are preloading, it is unlikely to even reach this.
				$(contentElem).load( '/' + target, function() {
					loadAds( tabId );
					fadeInElem( callback );
				});
			}
		}
	}

	var preloadTabs = function() {
		$('.tab-pane').each( function() {
			var targetTabId = $(this).data(targetIdAtr);
			if (targetTabId === 'registries') {
				// Let's not preload registries because the Macy's slider
				// is not lazy loading ads.
				return;
			}
			var currentTabId = getTabId();
			// Only load the hidden tabs.
			if ( targetTabId !== currentTabId ) {
				var target = cleanTarget( $(this).data(targetIdAtr) );
				var url = '/' + target;
				$(this).load( url );
			}
		});
	}

	var cleanTarget = function( target ) {
		if (target === 'find-vendor') {
			target = 'home/find-vendor';
		}
		if (target === 'ideas') {
			target = '';
		}
		return target;
	}

	var getTabId = function() {
		var tabId = getHash();
		return tabId ? tabId : $('.default-active').data(targetIdAtr);
	}

	var showTabFromHash = function( callback ) {
		var tabId = getTabId();
		showTabById( tabId, callback );
		fireTracking( cleanTarget(tabId) );
	}

	var loadAds = function( tabId ) {
		//	window.log('Fetching ads for: ' + tabId);
		var prefix = 'lazy-ad-';
		if ( typeof _lazyAds === 'undefined' ) {
			return;
		}
		var ads = _lazyAds[tabId];
		if ( ! $(ads).length ) {
			return;
		}
		$.each( ads, function(key, val) {
			var ids = {
				'script' : prefix + 'script-' + key,
				'iframe' : prefix + 'iframe-' + key,
				'a' : prefix + 'a-' + key,
				'img' : prefix + 'img-' + key
			}
			var that = this;
			$.each( ids, function(key, val) {
				var id = '#' + val;
				if ( $(id).length ) {
					if ( $(id).attr('src') !== 'undefined' ) {
						if ( $(id).attr('src') === '') {
							$(id).attr('src', that[key]);
						}
					}
					else if ( $(id).attr('href') !== 'undefined' ) {
						if ( $(id).attr('href') === '' ) {
							$(id).attr('href', that[key]);
						}
					}
				}
			});
		});
	}

	var fireTracking = function( target ) {
	    return true;
	}

	var postSetup = function() {

		preloadTabs();

		// Watch for changes in the hash value.
		$(window).hashchange( function() {
			showTabFromHash( function() {
				// Change the page title.
				if ( $(MW_homepage_titles).length ) {
					var tabId = getHash();
					if ( tabId in MW_homepage_titles ) {
						document.title = MW_homepage_titles[tabId].title;
					}
				}
			});
		});

		// Change tabs on clicking.
		homeTabs.bind( 'click', function(e) {
			e.preventDefault();
			MW.util.decrementPageviewTimer( 'signupTimer', function() {
				var modal = new app.views.JoinNowModalView();
			});
			var tabId = $(e.target).parent('li').data(targetIdAtr);
			if (tabId) {
				window.location.hash = tabId;
			}
		});
	}

	// Setup the nav links to trigger tabs instead of redirecting.
	var setupNavLinks = function() {

		var navLinks = [
			'free-wedding-websites',
			'registries',
			'planning',
			'travel'
		]

		// Little closure to set the hash.
		var getLinkClick = function(link) {
			return function(e) {
				e.preventDefault();
				window.location.hash = link;
			}
		}

		for ( var i = 0; i < navLinks.length; ++i ) {
			var link = navLinks[i];
			var selector = '#mainNavBottom a[href="/' + link + '"]';
			var linkElem = $(selector);
			if ( linkElem.length ) {
				linkElem.click( getLinkClick(link) );
			}
		}
	}

	var main = function() {

		setupNavLinks();

		if ( getHash() ) {
			showTabFromHash( postSetup );
		}
		else {
			var activeTab = getTabId();
			loadAds(activeTab);
			postSetup();
		}
	}

	main();
});

// Flag menu in new design.
$( function($) {

	var flagMenu = $('#flagMenu');
	var flagPopover = $('#countries');
	var flags    = $('#countries li, #countries li>a');
	var saveUrl  = '/apps/gateway/context/save-home-country';

	// Are we on the right page?
	if ( ! flagMenu.length ) {
		return;
	}

	var saveCountry = function( placeId ) {
		$.getJSON(
			saveUrl,
			{ placeId: placeId },
			function (json) {
				// We've change locations, so we must refresh
				// to see (e.g.) any locale language translations.
				document.location.reload(true);
			}
		);
	}

	var main = function() {
		flags.click( function(e) {
			e.preventDefault();
			flagPopover.hide();
			saveCountry( $(this).data('place-id') );
		});
	}

	main();
});

// Handle timers for Sign Up modal or Delightful Deals modal after sign up.
$( function () {

	if ( ! MW.userContext || MW.userContext.disableTimers() || window.pausePageviewTimers || MW.util.pageviewTimersPaused() ) {
		return;
	}

	if ( ! (MW.util.disableTimersByQueryString() || MW.util.unregisterTimersByQueryString() || MW.util.setTimerByQueryString()) ) {

		if ( MW.userContext.isLoggedIn() ) {

			// If the person gets a successful login at least once,
			// disable the sign up modal forever.
			MW.util.disablePageviewTimer( 'signupTimer' );

			// If the person just signed up then a timer has been set
			// to pop up the delightful deals modal.
			MW.util.decrementPageviewTimer( 'ddTimer', function() {
				$('#onboarding-deals').modal();
				MW.util.scrollToTop();
			});
		}
		else {

			// If we are logged out, kill the deals modal timer.
			MW.util.unregisterPageviewTimer( 'ddTimer' );

			// Here we FIRST decrement THEN we register the signup timer.
			// Seems weird, but we actually want the first decrement to fail,
			// then register the timer, then second pageview on, decrement
			// will do its thing like normal.

			MW.util.decrementPageviewTimer( 'signupTimer', function() {
				var modal = new app.views.JoinNowModalView();
			});

			// Launch a signup modal after the 2nd OR 3rd pageview,
			// if you are a new user not logged in.
			// This won't happen again if the user has already seen the popup,
			// and hasn't cleared cookies.
			var secondOrThirdPageview = (new Date().getTime() % 2 === 0) ? 0 : 1;
			MW.util.registerPageviewTimer( 'signupTimer', secondOrThirdPageview );
		}
	}
});

(function(window) {
    'use strict';
    const Cero = window.Cero = window.Cero || {};
    Cero.bootstrap = function() {
        window.loadAccount();
        window.loadVersions();
        window.fetchFriendsOnce();
        window.subscribeFriendsWS();
    };
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', Cero.bootstrap);
    else Cero.bootstrap();
})(window);

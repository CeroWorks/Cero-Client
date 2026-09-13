(function (window) {
    'use strict';

    function friendsMaintenanceMarkup() {
        const title = window.t ? window.t('maintenance.title') : 'Maintenance';
        const desc = window.t ? window.t('maintenance.desc') : 'Serveur indisponible, nouvelle tentative automatique...';
        return '<div class="friends-maintenance">' +
            '<svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">' +
            '<path d="M14.7 6.3a1 1 0 0 0 1.4 0l1.6-1.6a1 1 0 0 0 0-1.4 5 5 0 0 0-6.6 6.6L2.3 18.7a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l8.9-8.9a5 5 0 0 0 6.6-6.6l-1.6 1.6a1 1 0 0 1-1.4 0l-1.4-1.4a1 1 0 0 1 0-1.4z" stroke-linecap="round" stroke-linejoin="round"/>' +
            '</svg>' +
            '<p class="title">' + title + '</p>' +
            '<p class="sub">' + desc + '</p>' +
            '</div>';
    }

    function applyMaintenanceUI(active) {
        const navBtn = document.querySelector('.app-tab[data-tab="message"]');
        if (navBtn) navBtn.classList.toggle('nav-disabled', active);

        const card = document.getElementById('friendsCard');
        if (card) card.classList.toggle('maintenance', active);

        const list = document.getElementById('friendsList');
        if (list) {
            if (active) {
                list.dataset.savedBeforeMaintenance = list.innerHTML;
                list.innerHTML = friendsMaintenanceMarkup();
            } else if (list.dataset.savedBeforeMaintenance !== undefined) {
                delete list.dataset.savedBeforeMaintenance;
                if (window.fetchFriendsOnce) window.fetchFriendsOnce();
            }
        }

        if (active) {
            const onChat = document.getElementById('appTab-message') &&
                document.getElementById('appTab-message').style.display !== 'none';
            if (onChat && window.switchAppTab) window.switchAppTab('jouer');
        }
    }

    document.addEventListener('cero:maintenance', function (ev) {
        applyMaintenanceUI(!!(ev.detail && ev.detail.active));
    });

    document.addEventListener('cero:i18n-applied', function () {
        if (window.ceroWS && window.ceroWS.isMaintenance && window.ceroWS.isMaintenance()) {
            applyMaintenanceUI(true);
        }
    });

    document.addEventListener('DOMContentLoaded', function () {
        if (window.ceroWS && window.ceroWS.isMaintenance && window.ceroWS.isMaintenance()) {
            applyMaintenanceUI(true);
        }
    });
})(window);
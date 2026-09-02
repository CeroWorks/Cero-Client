(function(window) {
    "use strict";
    const Cero = window.Cero = window.Cero || {};
(function() {
    const NO_DRAG = 'a,button,input,select,textarea,canvas,' +
        '.card,.version-dropdown,.dd-item,.dd-tab,' +
        '.play-arrow,.play-main,.kill-btn,.wc-btn,' +
        '#closeMenu,#skinViewer,.friends-card,' +
        '[onclick],.nodrag';

    document.addEventListener('mousedown', (e) => {
        if (e.button !== 0) return;
        if (e.target.closest(NO_DRAG)) return;
        if (!e.target.closest('.app-drag')) return;
        if (window.drag_start) {
            e.preventDefault();
            window.drag_start();
        }
    }, true);
})();

    Cero.modules = Cero.modules || {};
})(window);

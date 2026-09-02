(function(window) {
    "use strict";
    const Cero = window.Cero = window.Cero || {};
window.addEventListener('keydown', (e) => {
    const k    = e.key.toLowerCase();
    const ctrl = e.ctrlKey || e.metaKey;
    if (ctrl && ['f','p','g','s','u','j','h','d','a'].includes(k)) e.preventDefault();
    if (['F3','F5','F7'].includes(e.key)) e.preventDefault();
    if (ctrl && e.shiftKey && k === 'r') e.preventDefault();
}, true);


    Cero.modules = Cero.modules || {};
})(window);

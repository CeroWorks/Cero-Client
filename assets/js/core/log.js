(function(window) {
    "use strict";
    const Cero = window.Cero = window.Cero || {};
function logErr(context, e) {
    const msg = e instanceof Error 
        ? e.message 
        : (typeof e === 'string' ? e : JSON.stringify(e));
    console.error(context + ': ' + msg);
    if (e instanceof Error && e.stack) console.error(e.stack);
    return msg;
}

    Cero.modules = Cero.modules || {};
    Cero.modules.logErr = logErr; window.logErr = logErr;
    Cero.logErr = logErr; window.logErr = logErr;
})(window);

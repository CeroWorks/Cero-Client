(function() {
    const WS_URL = 'ws://www.arcadiafr.fr:3134/ws';

    let ws = null;
    let reconnectTimer = null;
    let reconnectDelay = 1000;
    let lastStatus = 'online';
    let lastHello = null;
    const listeners = new Set();

    function log(...a)  { try { console.log('[WS]', ...a); } catch(_){} }
    function warn(...a) { try { console.warn('[WS]', ...a); } catch(_){} }
    function err(...a)  { try { console.error('[WS]', ...a); } catch(_){} }

    function scheduleReconnect(delay) {
        if (reconnectTimer) return;
        reconnectTimer = setTimeout(() => {
            reconnectTimer = null;
            connectWS();
        }, delay);
    }

    function connectWS() {
        log('connectWS() called');

        if (typeof WebSocket === 'undefined') {
            err('WebSocket API not available in this webview');
            return;
        }
        if (typeof window.getMcToken !== 'function') {
            warn('getMcToken not ready yet, retry in 1s');
            scheduleReconnect(1000);
            return;
        }

        let p;
        try {
            p = window.getMcToken();
        } catch (e) {
            err('getMcToken threw:', e && e.message ? e.message : String(e));
            scheduleReconnect(5000);
            return;
        }

        Promise.resolve(p).then(function(token) {
            log('token received, len=', token ? String(token).length : 0);
            if (!token || typeof token !== 'string') {
                warn('no token, retry in 5s');
                scheduleReconnect(5000);
                return;
            }

            try {
                ws = new WebSocket(WS_URL + '?token=' + encodeURIComponent(token));
            } catch (e) {
                err('WebSocket ctor failed:', e && e.message ? e.message : String(e));
                scheduleReconnect(5000);
                return;
            }

            ws.onopen = function() {
                log('connected');
                reconnectDelay = 1000;
                rawSend({ type: 'status', value: lastStatus });
            };
            ws.onmessage = function(ev) {
                try {
                    const msg = JSON.parse(ev.data);
                    log('<-', msg);
                    if (msg && msg.type === 'hello') lastHello = msg;
                    listeners.forEach(function(fn){ try { fn(msg); } catch(e){ err('listener:', e); } });
                } catch (e) {
                    err('bad message:', e);
                }
            };
            ws.onclose = function(ev) {
                warn('closed code=' + ev.code + ' reason=' + ev.reason);
                ws = null;
                if (ev.code === 4001) {
                    scheduleReconnect(10000);
                } else {
                    scheduleReconnect(reconnectDelay);
                    reconnectDelay = Math.min(reconnectDelay * 2, 30000);
                }
            };
            ws.onerror = function(e) {
                err('socket error', e && e.message ? e.message : '');
            };
        }).catch(function(e) {
            err('getMcToken rejected:', e && e.message ? e.message : String(e));
            scheduleReconnect(5000);
        });
    }

    function rawSend(obj) {
        if (ws && ws.readyState === 1) {
            try { ws.send(JSON.stringify(obj)); return true; } catch(_){}
        }
        return false;
    }

    function sendStatus(value) {
        if (typeof value !== 'string') return;
        lastStatus = value;
        rawSend({ type: 'status', value: value });
    }

    function onMessage(fn) {
        listeners.add(fn);
        if (lastHello) { try { fn(lastHello); } catch(e){ err('listener:', e); } }
        return function(){ listeners.delete(fn); };
    }

    window.ceroWS = { sendStatus: sendStatus, onMessage: onMessage };

    function start() {
        setTimeout(connectWS, 800);
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', start);
    } else {
        start();
    }
})();

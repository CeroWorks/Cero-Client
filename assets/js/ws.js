(function() {
    const wsUrl = window.Cero.config.wsUrl;
    const apiBase = window.Cero.config.apiBase;

    const HEALTH_RETRY_DELAY = 30000;
    const HEALTH_TIMEOUT = 5000;

    let ws = null;
    let reconnectTimer = null;
    let healthTimer = null;
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

    function scheduleHealthCheck(delay) {
        if (healthTimer) return;
        healthTimer = setTimeout(() => {
            healthTimer = null;
            checkHealthThenConnect();
        }, delay);
    }

    function checkHealthThenConnect() {
        if (typeof fetch !== 'function' || !apiBase) {
            connectWS();
            return;
        }

        const controller = (typeof AbortController !== 'undefined') ? new AbortController() : null;
        const timeoutId = controller ? setTimeout(() => controller.abort(), HEALTH_TIMEOUT) : null;

        fetch(apiBase + '/health', controller ? { signal: controller.signal } : {})
            .then(function(res) {
                if (timeoutId) clearTimeout(timeoutId);
                if (res && res.ok) {
                    log('health check OK, connecting...');
                    connectWS();
                } else {
                    warn('health check failed (status ' + (res ? res.status : '?') + '), retry in 30s');
                    scheduleHealthCheck(HEALTH_RETRY_DELAY);
                }
            })
            .catch(function() {
                if (timeoutId) clearTimeout(timeoutId);
                warn('server unreachable, retry in 30s');
                scheduleHealthCheck(HEALTH_RETRY_DELAY);
            });
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
            scheduleHealthCheck(HEALTH_RETRY_DELAY);
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
                ws = new WebSocket(wsUrl + '?token=' + encodeURIComponent(token));
            } catch (e) {
                err('WebSocket ctor failed:', e && e.message ? e.message : String(e));
                scheduleHealthCheck(HEALTH_RETRY_DELAY);
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
                    scheduleHealthCheck(HEALTH_RETRY_DELAY);
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
        setTimeout(checkHealthThenConnect, 800);
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', start);
    } else {
        start();
    }
})();
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

document.addEventListener('dragstart', e => { if (e.target.tagName === 'IMG') e.preventDefault(); }, true);

window.addEventListener('keydown', (e) => {
    const k    = e.key.toLowerCase();
    const ctrl = e.ctrlKey || e.metaKey;
    if (ctrl && ['f','p','g','s','u','j','h','d','a'].includes(k)) e.preventDefault();
    if (['F3','F5','F7'].includes(e.key)) e.preventDefault();
    if (ctrl && e.shiftKey && k === 'r') e.preventDefault();
}, true);

window.addEventListener('contextmenu', e => e.preventDefault());

function logErr(context, e) {
    const msg = e instanceof Error 
        ? e.message 
        : (typeof e === 'string' ? e : JSON.stringify(e));
    console.error(context + ': ' + msg);
    if (e instanceof Error && e.stack) console.error(e.stack);
    return msg;
}

const canvas = document.getElementById('grainCanvas');
const ctx = canvas.getContext('2d');
let W, H;

function resize() { W = canvas.width = innerWidth; H = canvas.height = innerHeight; }
addEventListener('resize', resize); 
resize();

(function grain() {
    const img = ctx.createImageData(W, H);
    for (let i = 0; i < img.data.length; i += 4) {
        const v = Math.random() * 255;
        img.data[i] = img.data[i+1] = img.data[i+2] = v;
        img.data[i+3] = 255;
    }
    ctx.putImageData(img, 0, 0);
    requestAnimationFrame(grain);
})();

let selectedVersion = null;
let skinViewer = null;
let allVersions = [];
let currentFilter = 'release';

function setFilter(f) {
    currentFilter = f;
    document.querySelectorAll('.dd-tab').forEach(b => b.classList.remove('active'));
    const btn = document.getElementById('tab-' + f);
    if (btn) btn.classList.add('active');
    renderDropdown();
}

function updatePlayButton() {
    const btn = document.getElementById('launchBtn');
    if (!selectedVersion) return;
    btn.innerHTML = `▶ Jouer ${selectedVersion.id}`;
    btn.disabled = false;
    btn.style.opacity = '';
}

function persistSelection() {
    if (!selectedVersion || !window.setVersion) return;
    window.setVersion(selectedVersion.id);
}

async function loadVersions() {
    try {
        const r = await fetch('https://launchermeta.mojang.com/mc/game/version_manifest.json');
        const data = await r.json();
        allVersions = data.versions;

        const saved = await window.getVersion();

        selectedVersion =
            (saved && allVersions.find(v => v.id === saved)) ||
            allVersions.find(v => v.id === data.latest.release) ||
            allVersions[0];

        document.getElementById('selectedVersionDisplay').textContent = selectedVersion.id;
        renderDropdown();
        updatePlayButton();
    } catch (e) { console.error('loadVersions:', e); }
}

function selectVersion(id) {
    const v = allVersions.find(x => x.id === id);
    if (!v) return;
    selectedVersion = v;
    document.getElementById('selectedVersionDisplay').textContent = v.id;
    persistSelection();
    updatePlayButton();
    closeVersionDropdown();
}

function renderDropdown() {
    const q = (document.getElementById('ddSearch').value || '').toLowerCase();
    const list = allVersions.filter(v => {
        if (currentFilter !== 'all' && v.type !== currentFilter) return false;
        if (q && !v.id.toLowerCase().includes(q)) return false;
        return true;
    }).slice(0, 200);

    const ddList = document.getElementById('ddList');
    ddList.innerHTML = list.map(v => {
        return `
            <div class="dd-item" onclick="selectVersion('${v.id}')">
                <span>${v.id}</span>
                <span class="dd-type">${v.type}</span>
            </div>
        `;
    }).join('');
}

function toggleVersionDropdown(e) {
    e.stopPropagation();
    const dd = document.getElementById('versionDropdown');
    if (dd.classList.contains('open')) { closeVersionDropdown(); return; }

    dd.style.visibility = 'hidden';
    dd.classList.add('open');
    renderDropdown();

    requestAnimationFrame(() => {
        const grp = document.getElementById('playGroup');
        const dd = document.getElementById('versionDropdown');
        const rGrp = grp.getBoundingClientRect();
        
        if (dd.parentElement !== document.body) {
            document.body.appendChild(dd);
        }
        
        dd.style.position = 'fixed'; 
        const left = Math.max(8, rGrp.right - dd.offsetWidth);
        dd.style.left = left + 'px';
        
        const maxH = Math.min(300, rGrp.top - 16); 
        dd.style.maxHeight = maxH + 'px';
        dd.style.bottom = (window.innerHeight - rGrp.top + 4) + 'px';
        
        dd.style.top = 'auto';
        dd.style.right = 'auto';
        dd.style.visibility = 'visible';

        document.getElementById('ddSearch').focus();
    });
}

function closeVersionDropdown() {
    const dd = document.getElementById('versionDropdown');
    if (dd) dd.classList.remove('open');
}

document.addEventListener('click', (e) => {
    const dd = document.getElementById('versionDropdown');
    if (!dd || !dd.classList.contains('open')) return;
    if (e.target.closest('#versionDropdown') || e.target.closest('#playArrowBtn')) return;
    closeVersionDropdown();
});

function initSkinViewer(skinUrl) {
    const canvasEl = document.getElementById('skinViewer');
    const parent   = document.getElementById('skinContainer');

    function getSize() {
        const r = parent.getBoundingClientRect();
        return { w: Math.max(200, Math.floor(r.width)), h: Math.max(200, Math.floor(r.height)) };
    }

    if (skinViewer) { skinViewer.loadSkin(skinUrl).catch(() => {}); return; }
    if (typeof skinview3d === 'undefined') return;

    requestAnimationFrame(() => {
        const { w, h } = getSize();
        try {
            skinViewer = new skinview3d.SkinViewer({ canvas: canvasEl, width: w, height: h });
            skinViewer.loadSkin(skinUrl).catch(() => skinViewer.loadSkin('https://mc-heads.net/skin/MHF_Steve'));
            skinViewer.fov = 35;
            skinViewer.zoom = 0.85;
            skinViewer.controls.enableZoom = false;
            skinViewer.controls.enablePan  = false;
            skinViewer.animation = new skinview3d.WalkingAnimation();
            skinViewer.animation.speed = 0.5;
            new ResizeObserver(() => {
                const s = getSize();
                skinViewer.setSize(s.w, s.h);
            }).observe(parent);
        } catch(e) { console.error('SkinViewer:', e); }
    });
}

async function loadAccount() {
    try {
        const raw = await window.getAccount();
        const acc  = typeof raw === 'string' ? JSON.parse(raw) : raw;
        const name = acc.name || acc.username || 'Joueur';
        const uuid = acc.uuid || acc.id || '';

        document.getElementById('playerName').textContent = name;
    
        if (uuid) {
            window.myUuid = uuid;
            initSkinViewer(`https://mc-heads.net/skin/${uuid}`);
            const av = document.getElementById('overlayAvatar');
            av.src = `https://mc-heads.net/avatar/${uuid}/64`;
            av.classList.remove('hidden');
            document.getElementById('overlayIcon').classList.add('hidden');
        }
    } catch(e) { console.error('loadAccount:', e); }
}

const SERVER_URL = 'http://www.arcadiafr.fr:3134';
const STATUS_ORDER = { ingame: 0, online: 1, offline: 2 };

let friendsFetchId = 0;
let renderFriendsTimeout = null;

window.friendsCache = [];

function normalizeFriends(arr) {
    return (arr || []).map(f => ({
        name: f.name,
        uuid: f.uuid,
        status: f.status || 'offline'
    }));
}

async function fetchFriendsOnce() {
    const currentFetchId = ++friendsFetchId; // On marque cette requête
    try {
        const token = await window.getMcToken();
        if (!token) throw new Error('No token');
        const res = await fetch(`${SERVER_URL}/api/friends`, {
            headers: { 'Authorization': `Bearer ${token}` }
        });
        if (!res.ok) {
            const errData = await res.json().catch(() => ({}));
            throw new Error(errData.error || `HTTP ${res.status}`);
        }
        const data = await res.json();
        
        if (currentFetchId !== friendsFetchId) return; 
        
        window.friendsCache = normalizeFriends(data.friends);
        debouncedRenderFriends(window.friendsCache);
        if (typeof window.onFriendsCacheReady === 'function') window.onFriendsCacheReady();
    } catch (e) {
        if (currentFetchId === friendsFetchId) {
            logErr('fetchFriendsOnce', e);
            debouncedRenderFriends([]);
        }
    }
}

function debouncedRenderFriends(friends) {
    if (renderFriendsTimeout) clearTimeout(renderFriendsTimeout);
    renderFriendsTimeout = setTimeout(() => renderFriends(friends), 50);
}

function renderFriends(friends) {
    const list = document.getElementById('friendsList');
    if (!list) return;

    const scrollTop = list.scrollTop;

    if (!friends || friends.length === 0) {
        list.innerHTML = '<div class="friends-empty">Aucun ami pour le moment</div>';
        document.getElementById('friendsOnlineCount').textContent = '0';
        return;
    }

    friends.sort((a,b) => (STATUS_ORDER[a.status] ?? 9) - (STATUS_ORDER[b.status] ?? 9));

    const onlineCount = friends.filter(f => f.status !== 'offline').length;
    document.getElementById('friendsOnlineCount').textContent = onlineCount;

    list.innerHTML = friends.map(f => `
        <div class="friend-item" title="${f.name}${f.status === 'ingame' ? ' • En jeu' : f.status === 'online' ? ' • En ligne' : ' • Hors ligne'}">
            <img class="friend-head" src="https://mc-heads.net/avatar/${encodeURIComponent(f.uuid || f.name)}/28" alt="">
            <span class="friend-name" style="${f.status === 'offline' ? 'opacity:0.4' : ''}">${f.name}</span>
            <button class="friend-remove nodrag" onclick="event.stopPropagation(); openRemoveFriendModal('${f.uuid}')">✕</button>
            <div class="friend-status-dot ${f.status}"></div>
        </div>
    `).join('');

    list.scrollTop = scrollTop;
}

function applyFriendStatus(uuid, status) {
    const f = window.friendsCache.find(x => x.uuid === uuid);
    if (f) {
        f.status = status || 'offline';
    } else {
        fetchFriendsOnce(); return;
    }
    debouncedRenderFriends(window.friendsCache); // Changé ici
}

let pendingRemoveUuid = null;

function openRemoveFriendModal(uuid) {
    const friend = window.friendsCache.find(f => f.uuid === uuid);
    if (!friend) return;
    
    pendingRemoveUuid = uuid;
    
    document.getElementById('removeFriendAvatar').src = `https://mc-heads.net/avatar/${encodeURIComponent(uuid)}/48`;
    document.getElementById('removeFriendText').textContent = `Retirer ${friend.name} de tes amis ?`;
    
    document.getElementById('removeFriendConfirmBtn').disabled = false;
    document.getElementById('removeFriendConfirmBtn').textContent = 'Retirer';
    
    document.getElementById('removeFriendModal').classList.add('active');
}

function closeRemoveFriendModal() {
    document.getElementById('removeFriendModal').classList.remove('active');
    pendingRemoveUuid = null;
}

async function confirmRemoveFriend() {
    if (!pendingRemoveUuid) return;

    const btn = document.getElementById('removeFriendConfirmBtn');
    btn.disabled = true;
    btn.textContent = '...';

    try {
        const token = await window.getMcToken();
        if (!token) throw new Error("Non authentifié");

        const res = await fetch(`${SERVER_URL}/api/friends/${pendingRemoveUuid}`, {
            method: 'DELETE',
            headers: { 'Authorization': `Bearer ${token}` }
        });

        if (!res.ok) {
            const errData = await res.json().catch(() => ({}));
            throw new Error(errData.error || `Erreur serveur (${res.status})`);
        }

        window.friendsCache = window.friendsCache.filter(f => f.uuid !== pendingRemoveUuid);
        renderFriends(window.friendsCache);
        closeRemoveFriendModal();
    } catch (e) {
        console.error('confirmRemoveFriend:', e);
        btn.disabled = false;
        btn.textContent = 'Retirer';
        alert("Impossible de retirer cet ami : " + e.message);
    }
}

function openAddFriendModal() {
    document.getElementById('addFriendInput').value = '';
    document.getElementById('addFriendError').textContent = '';
    document.getElementById('addFriendModal').classList.add('active');
    setTimeout(() => document.getElementById('addFriendInput').focus(), 100);
}

function closeAddFriendModal() {
    document.getElementById('addFriendModal').classList.remove('active');
}

async function submitAddFriend() {
    const input = document.getElementById('addFriendInput');
    const errEl = document.getElementById('addFriendError');
    const btn = document.getElementById('addFriendSubmit');
    
    const username = input.value.trim();
    if (!username) { errEl.textContent = 'Pseudo vide'; return; }

    btn.disabled = true;
    btn.textContent = '...';
    errEl.textContent = '';

    try {
        const token = await window.getMcToken();
        if (!token) throw new Error("Non authentifié");

        const res = await fetch(`${SERVER_URL}/api/friends/add`, {
            method: 'POST',
            headers: { 'Authorization': `Bearer ${token}`, 'Content-Type': 'application/json' },
            body: JSON.stringify({ username })
        });

        const data = await res.json();
        if (!res.ok) throw new Error(data.hint || data.error || `Erreur ${res.status}`);

        if (data.status === 'accepted') await fetchFriendsOnce();
        
        closeAddFriendModal();
    } catch (e) {
        console.error('submitAddFriend:', e);
        errEl.textContent = e.message;
    } finally {
        btn.disabled = false;
        btn.textContent = 'Envoyer';
    }
}

function openRequestsModal() {
    document.getElementById('requestsModal').classList.add('active');
    loadRequests();
}

function closeRequestsModal() {
    document.getElementById('requestsModal').classList.remove('active');
}

async function loadRequests() {
    const body = document.getElementById('requestsBody');
    body.innerHTML = '<div class="modal-empty">Chargement...</div>';

    try {
        const token = await window.getMcToken();
        if (!token) throw new Error('Non authentifié');

        const res = await fetch(`${SERVER_URL}/api/friends/requests`, {
            headers: { 'Authorization': `Bearer ${token}` }
        });

        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const data = await res.json();
        const { incoming, outgoing } = data;

        if ((!incoming || incoming.length === 0) && (!outgoing || outgoing.length === 0)) {
            body.innerHTML = '<div class="modal-empty">Aucune demande</div>';
            document.getElementById('reqBadge').classList.add('hidden');
            return;
        }

        let html = '';
        if (incoming && incoming.length > 0) {
            html += '<p class="req-section-title">Reçues</p>';
            incoming.forEach(r => {
                html += `
                    <div class="req-item">
                        <img class="req-head" src="https://mc-heads.net/avatar/${encodeURIComponent(r.uuid)}/32" alt="">
                        <div class="req-info"><div class="req-name">${r.username}</div></div>
                        <div class="req-actions">
                            <button class="req-btn accept" onclick="acceptRequest('${r.uuid}')">✓</button>
                            <button class="req-btn decline" onclick="declineRequest('${r.uuid}')">✕</button>
                        </div>
                    </div>`;
            });
        }

        if (outgoing && outgoing.length > 0) {
            html += '<p class="req-section-title">Envoyées</p>';
            outgoing.forEach(r => {
                html += `
                    <div class="req-item" style="opacity:0.5">
                        <img class="req-head" src="https://mc-heads.net/avatar/${encodeURIComponent(r.uuid)}/32" alt="">
                        <div class="req-info">
                            <div class="req-name">${r.username}</div>
                            <div class="req-sub">En attente</div>
                        </div>
                    </div>`;
            });
        }

        body.innerHTML = html;

        const count = incoming ? incoming.length : 0;
        const badge = document.getElementById('reqBadge');
        if (count > 0) { badge.textContent = count; badge.classList.remove('hidden'); }
        else { badge.classList.add('hidden'); }

    } catch (e) {
        console.error('loadRequests:', e);
        body.innerHTML = '<div class="modal-empty">Erreur de chargement</div>';
    }
}

async function acceptRequest(uuid) {
    try {
        const token = await window.getMcToken();
        if (!token) throw new Error("Non authentifié");

        const res = await fetch(`${SERVER_URL}/api/friends/accept`, {
            method: 'POST',
            headers: { 'Authorization': `Bearer ${token}`, 'Content-Type': 'application/json' },
            body: JSON.stringify({ uuid })
        });

        if (!res.ok) {
            const errData = await res.json().catch(() => ({}));
            console.error('[acceptRequest] Server response:', res.status, errData);
            throw new Error(errData.error || errData.hint || `Erreur serveur ${res.status}`);
        }

        await fetchFriendsOnce();
        loadRequests();
    } catch (e) {
        const msg = logErr('acceptRequest', e);
        alert("Impossible d'accepter : " + msg);
    }
}

async function declineRequest(uuid) {
    try {
        const token = await window.getMcToken();
        if (!token) throw new Error("Non authentifié");

        const res = await fetch(`${SERVER_URL}/api/friends/${uuid}`, {
            method: 'DELETE',
            headers: { 'Authorization': `Bearer ${token}` }
        });

        if (!res.ok) {
            const errData = await res.json().catch(() => ({}));
            console.error('[declineRequest] Server response:', res.status, errData);
            throw new Error(errData.error || errData.hint || `Erreur serveur ${res.status}`);
        }
        loadRequests();
    } catch (e) {
        const msg = logErr('declineRequest', e);
        alert("Impossible de refuser : " + msg);
    }
}

function subscribeFriendsWS() {
    if (!window.ceroWS) { setTimeout(subscribeFriendsWS, 500); return; }
    window.ceroWS.onMessage((msg) => {
        if (!msg || !msg.type) return;
        switch (msg.type) {
            case 'friends':
                window.friendsCache = normalizeFriends(msg.friends);
                debouncedRenderFriends(window.friendsCache); // Changé ici
                break;
            case 'friend_status':
                applyFriendStatus(msg.uuid, msg.status);
                break;
            case 'friend_request':
                const badge = document.getElementById('reqBadge');
                let count = parseInt(badge.textContent) || 0;
                badge.textContent = count + 1;
                badge.classList.remove('hidden');
                break;
            case 'friend_accepted':
                fetchFriendsOnce();
                break;
            case 'friend_removed':
                window.friendsCache = window.friendsCache.filter(f => f.uuid !== msg.by);
                debouncedRenderFriends(window.friendsCache); // Changé ici
                break;
            case 'friend_request_declined':
                if (document.getElementById('requestsModal').classList.contains('active')) {
                    loadRequests();
                }
                break;
        }
    });
}

window.loadFriends = fetchFriendsOnce;
window.getFriendsCache = function() { return window.friendsCache; };

const STEPS = [
    { key: 'account',   label: 'Compte' },
    { key: 'manifest',  label: 'Manifest' },
    { key: 'client',    label: 'Client JAR' },
    { key: 'libraries', label: 'Librairies' },
    { key: 'natives',   label: 'Natives' },
    { key: 'assets',    label: 'Assets' },
    { key: 'java',      label: 'Java' },
    { key: 'launch',    label: 'Lancement' },
];

function buildStepsList() {
    document.getElementById('stepsList').innerHTML = STEPS.map(s =>
        `<div class="step-item pending" id="step-${s.key}"><div class="step-dot"></div><span class="text-xs font-medium">${s.label}</span></div>`
    ).join('');
}

function overlayShow(version) {
    buildStepsList();
    document.getElementById('overlayVersion').textContent = 'Minecraft ' + version;
    setOverlayProgress('Initialisation...', 0);
    document.getElementById('launchOverlay').classList.add('active');
}

function overlayHide() { document.getElementById('launchOverlay').classList.remove('active'); }

function setOverlayProgress(step, pct) {
    document.getElementById('currentStepLabel').textContent = step;
    document.getElementById('progressBar').style.width = pct + '%';
    document.getElementById('progressPct').textContent  = pct + '%';
}

function overlayError(msg) {
    document.getElementById('currentStepLabel').textContent = 'Error : ' + msg;
    document.getElementById('progressBar').style.background = '#f87171';
    setTimeout(() => overlayHide(), 3000);
}

window.onLaunchProgress = function(step, pct) {
    setOverlayProgress(step, pct);
    if (pct >= 100) {
        setTimeout(() => {
            overlayHide();
            const btn = document.getElementById('launchBtn');
            if(btn) btn.disabled = false;
            updatePlayButton();
        }, 800);
    }
};

window.onLaunchError = function(msg) {
    overlayError(msg);
    const btn = document.getElementById('launchBtn');
    if(btn) btn.disabled = false;
    updatePlayButton();
};

window._onGameStart = function() {
    document.getElementById('playGroup').style.display = 'none';
    document.getElementById('killBtn').style.display   = 'block';

    if (window.get_settings) {
        window.get_settings().then(function(s) {
            if (s && s.hideOnLaunch && window.close_to_tray) {
                window.close_to_tray();
            }
        }).catch(() => {});
    }
};

window._onGameStop = function() {
    document.getElementById('playGroup').style.display = 'flex';
    document.getElementById('killBtn').style.display   = 'none';

    if (window.get_settings) {
        window.get_settings().then(function(s) {
            if (s && s.reopenOnStop && window.show_window) {
                window.show_window();
            }
        }).catch(() => {});
    }
};

async function launchGame() {
    if (!selectedVersion) { document.getElementById('launchStatus').textContent = 'Pas de version'; return; }
    const btn = document.getElementById('launchBtn');
    btn.disabled = true;
    btn.innerHTML = '⏳ Lancement...';
    overlayShow(selectedVersion.id);
    try {
        await window.launch_mc(selectedVersion.id);
    }   
    catch(e) { overlayError(e?.message || String(e)); }
}

async function killGame() { if (window.kill_game) await window.kill_game(); }

function showCloseMenu(e) {
    e.stopPropagation();
    const m = document.getElementById('closeMenu');
    m.style.display = 'block';
    requestAnimationFrame(() => {
        const r = m.getBoundingClientRect();
        m.style.left = Math.min(e.clientX, innerWidth  - r.width  - 8) + 'px';
        m.style.top  = Math.min(e.clientY, innerHeight - r.height - 8) + 'px';
    });
}

function hideCloseMenu() { document.getElementById('closeMenu').style.display = 'none'; }

document.addEventListener('click', (e) => {
    if (!e.target.closest('#closeMenu') && !e.target.closest('.wc-btn.close'))
        hideCloseMenu();
});

loadAccount();
loadVersions();
fetchFriendsOnce();
subscribeFriendsWS();
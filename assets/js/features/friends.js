(function(window) {
    "use strict";
    const Cero = window.Cero = window.Cero || {};
//const SERVER_URL = 'http://www.arcadiafr.fr:3134';
const SERVER_URL = 'http://localhost:3134';
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
    const currentFetchId = ++friendsFetchId;
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
            (Cero.logErr || console.error)('fetchFriendsOnce', e);
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
        const msg = (Cero.logErr || console.error)('acceptRequest', e);
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
        const msg = (Cero.logErr || console.error)('declineRequest', e);
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
                debouncedRenderFriends(window.friendsCache);
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
                debouncedRenderFriends(window.friendsCache);
                break;
            case 'friend_request_declined':
                if (document.getElementById('requestsModal').classList.contains('active')) {
                    loadRequests();
                }
                break;
        }
    });
}

window.loadFriends = fetchFriendsOnce; window.fetchFriendsOnce = fetchFriendsOnce; window.subscribeFriendsWS = subscribeFriendsWS; window.openRemoveFriendModal = openRemoveFriendModal; window.acceptRequest = acceptRequest; window.declineRequest = declineRequest;
window.getFriendsCache = function() { return window.friendsCache; };


    Cero.modules = Cero.modules || {};
    Cero.modules.openRemoveFriendModal = openRemoveFriendModal; window.openRemoveFriendModal = openRemoveFriendModal;
    Cero.modules.closeRemoveFriendModal = closeRemoveFriendModal; window.closeRemoveFriendModal = closeRemoveFriendModal;
    Cero.modules.confirmRemoveFriend = confirmRemoveFriend; window.confirmRemoveFriend = confirmRemoveFriend;
    Cero.modules.openAddFriendModal = openAddFriendModal; window.openAddFriendModal = openAddFriendModal;
    Cero.modules.closeAddFriendModal = closeAddFriendModal; window.closeAddFriendModal = closeAddFriendModal;
    Cero.modules.submitAddFriend = submitAddFriend; window.submitAddFriend = submitAddFriend;
    Cero.modules.openRequestsModal = openRequestsModal; window.openRequestsModal = openRequestsModal;
    Cero.modules.closeRequestsModal = closeRequestsModal; window.closeRequestsModal = closeRequestsModal;
    Cero.modules.loadRequests = loadRequests; window.loadRequests = loadRequests;
    Cero.modules.acceptRequest = acceptRequest; window.acceptRequest = acceptRequest;
    Cero.modules.declineRequest = declineRequest; window.declineRequest = declineRequest;
    window.loadFriends = fetchFriendsOnce; window.fetchFriendsOnce = fetchFriendsOnce; window.subscribeFriendsWS = subscribeFriendsWS; window.openRemoveFriendModal = openRemoveFriendModal; window.acceptRequest = acceptRequest; window.declineRequest = declineRequest; Cero.friends = Cero.friends || {};
})(window);

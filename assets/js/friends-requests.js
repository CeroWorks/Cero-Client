const FR_API = 'http://www.arcadiafr.fr:3134';
let pendingIncoming = [];
let pendingOutgoing = [];

async function frHeaders() {
    const token = await window.getMcToken();
    return {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ' + token
    };
}

function updateBadge() {
    const badge = document.getElementById('reqBadge');
    const n = pendingIncoming.length;
    if (n === 0) { badge.classList.add('hidden'); }
    else { badge.classList.remove('hidden'); badge.textContent = n > 9 ? '9+' : n; }
}

async function loadRequests() {
    try {
        const res = await fetch(FR_API + '/api/friends/requests', { headers: await frHeaders() });
        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();
        pendingIncoming = data.incoming || [];
        pendingOutgoing = data.outgoing || [];
        updateBadge();
        if (document.getElementById('requestsModal').classList.contains('active'))
            renderRequests();
    } catch (e) {
        console.error('[friends] loadRequests:', e);
    }
}

function renderRequests() {
    const body = document.getElementById('requestsBody');
    if (pendingIncoming.length === 0 && pendingOutgoing.length === 0) {
        body.innerHTML = '<div class="modal-empty">Aucune demande en attente</div>';
        return;
    }
    let html = '';
    if (pendingIncoming.length) {
        html += '<p class="text-[10px] font-bold uppercase tracking-[0.15em] text-[var(--text-muted)] mb-2">Reçues</p>';
        html += pendingIncoming.map(r => `
            <div class="req-item">
                <img class="req-head" src="https://mc-heads.net/avatar/${encodeURIComponent(r.username)}/32" alt="">
                <div class="req-info">
                    <div class="req-name">${escapeHtml(r.username)}</div>
                    <div class="req-sub">veut être ton ami</div>
                </div>
                <div class="req-actions">
                    <button class="req-btn accept" onclick="acceptReq('${r.requested_by}')" title="Accepter">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3">
                            <path d="M5 12l5 5L20 7" stroke-linecap="round" stroke-linejoin="round"/>
                        </svg>
                    </button>
                    <button class="req-btn decline" onclick="declineReq('${r.requested_by}')" title="Refuser">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3">
                            <line x1="6" y1="6" x2="18" y2="18" stroke-linecap="round"/>
                            <line x1="18" y1="6" x2="6" y2="18" stroke-linecap="round"/>
                        </svg>
                    </button>
                </div>
            </div>
        `).join('');
    }
    if (pendingOutgoing.length) {
        html += '<p class="text-[10px] font-bold uppercase tracking-[0.15em] text-[var(--text-muted)] mt-4 mb-2">Envoyées</p>';
        html += pendingOutgoing.map(r => {
            const otherUuid = r.user_a === r.requested_by ? r.user_b : r.user_a;
            return `
            <div class="req-item">
                <img class="req-head" src="https://mc-heads.net/avatar/${encodeURIComponent(r.username)}/32" alt="">
                <div class="req-info">
                    <div class="req-name">${escapeHtml(r.username)}</div>
                    <div class="req-sub">en attente...</div>
                </div>
                <div class="req-actions">
                    <button class="req-btn decline" onclick="cancelReq('${otherUuid}')" title="Annuler">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3">
                            <line x1="6" y1="6" x2="18" y2="18" stroke-linecap="round"/>
                            <line x1="18" y1="6" x2="6" y2="18" stroke-linecap="round"/>
                        </svg>
                    </button>
                </div>
            </div>`;
        }).join('');
    }
    body.innerHTML = html;
}

function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}

async function acceptReq(uuid) {
    const res = await fetch(FR_API + '/api/friends/accept', {
        method: 'POST', headers: await frHeaders(),
        body: JSON.stringify({ uuid })
    });
    if (res.ok) {
        pendingIncoming = pendingIncoming.filter(r => r.requested_by !== uuid);
        updateBadge(); renderRequests();
        if (window.loadFriends) window.loadFriends();
    }
}
async function declineReq(uuid) {
    const res = await fetch(FR_API + '/api/friends/' + uuid, {
        method: 'DELETE', headers: await frHeaders()
    });
    if (res.ok) {
        pendingIncoming = pendingIncoming.filter(r => r.requested_by !== uuid);
        updateBadge(); renderRequests();
    }
}
async function cancelReq(uuid) {
    const res = await fetch(FR_API + '/api/friends/' + uuid, {
        method: 'DELETE', headers: await frHeaders()
    });
    if (res.ok) { loadRequests(); }
}

function openAddFriendModal() {
    document.getElementById('addFriendModal').classList.add('active');
    const input = document.getElementById('addFriendInput');
    input.value = ''; document.getElementById('addFriendError').textContent = '';
    setTimeout(() => input.focus(), 50);
}
function closeAddFriendModal() {
    document.getElementById('addFriendModal').classList.remove('active');
}

async function submitAddFriend() {
    const input = document.getElementById('addFriendInput');
    const errEl = document.getElementById('addFriendError');
    const btn = document.getElementById('addFriendSubmit');
    const username = input.value.trim();
    errEl.textContent = '';
    if (!username) { errEl.textContent = 'Entre un pseudo'; return; }

    btn.disabled = true;
    try {
        const res = await fetch(FR_API + '/api/friends/add', {
            method: 'POST', headers: await frHeaders(),
            body: JSON.stringify({ username })
        });
        const data = await res.json().catch(() => ({}));
        if (!res.ok) {
            const msgs = {
                user_not_found: "Ce joueur ne s'est jamais connecté à CeroClient",
                cannot_add_self: "Tu ne peux pas t'ajouter toi-même",
                already_friends: "Vous êtes déjà amis",
                already_requested: "Demande déjà envoyée",
                blocked: "Action impossible",
                invalid_username: "Pseudo invalide"
            };
            errEl.textContent = msgs[data.error] || ('Erreur: ' + (data.error || res.status));
            return;
        }
        closeAddFriendModal();
        if (data.status === 'accepted' && window.loadFriends) window.loadFriends();
        else loadRequests();
    } catch (e) {
        errEl.textContent = 'Erreur réseau';
    } finally {
        btn.disabled = false;
    }
}

function openRequestsModal() {
    document.getElementById('requestsModal').classList.add('active');
    renderRequests();
    loadRequests();
}
function closeRequestsModal() {
    document.getElementById('requestsModal').classList.remove('active');
}

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        closeAddFriendModal();
        closeRequestsModal();
    }
});
document.getElementById('addFriendInput')?.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') submitAddFriend();
});
['addFriendModal','requestsModal'].forEach(id => {
    document.getElementById(id)?.addEventListener('click', (e) => {
        if (e.target.id === id) e.target.classList.remove('active');
    });
});

function hookWS() {
    if (!window.ceroWS) return setTimeout(hookWS, 300);
    window.ceroWS.onMessage((msg) => {
        if (msg.type === 'friend_request') {
            loadRequests();
        } else if (msg.type === 'friend_accepted' || msg.type === 'friend_removed') {
            loadRequests();
            if (window.loadFriends) window.loadFriends();
        }
    });
}
hookWS();

loadRequests();

window.frReload = loadRequests;
